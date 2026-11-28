// VR-1B M2 adoption verification: shoulder proximal-10mm offset adopted into the
// PAE RigDefinition (pae-rig-v2.2). Verifies 9-shape joint sync + full-pose
// regression (legacy v2.1 vs adopted v2.2). Host-only; no Android, no DQS.

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "wp4_poses.h"
#include "wp4_quality.h"
#include "wp4_rig.h"
#include "wp4_skin.h"
#include "wp4_weights.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace pae;
using ht::D3;
using ht::fmt;

namespace {
std::string arg(int argc, char** argv, const std::string& k, const std::string& d) {
    for (int i = 1; i + 1 < argc; ++i) if (k == argv[i]) return argv[i + 1];
    return d;
}
void fitOne(ht::Camera& cam, const std::vector<D3>& pos) {
    std::vector<ht::Camera> v{cam}; std::vector<const std::vector<D3>*> s{&pos};
    ht::fitSharedFrames(v, s); cam = v[0];
}
D3 scalePivot(const ht::ProofContext& c) {
    double xMin = 1e30, xMax = -1e30, yMin = 1e30;
    for (const D3& p : c.body.pos) { xMin = std::min(xMin, p.x); xMax = std::max(xMax, p.x); yMin = std::min(yMin, p.y); }
    const int ri = c.rigDeform.find("root");
    return D3{0.5 * (xMin + xMax), yMin, ri >= 0 ? c.rigDeform.bones[static_cast<size_t>(ri)].head.z : 0.0};
}
double baseStature(const ht::ProofContext& c) {
    double lo = 1e30, hi = -1e30;
    for (const D3& p : c.body.pos) { lo = std::min(lo, p.y); hi = std::max(hi, p.y); }
    return hi - lo;
}
struct Shape { std::string name; std::vector<std::pair<std::string, double>> kv; double stature; };
std::vector<D3> applyShape(const ht::ProofContext& c, const Shape& s) {
    std::vector<std::pair<const ht::ShapeTarget*, double>> w;
    for (const auto& kv : s.kv) {
        const ht::ShapeParam* sp = nullptr;
        for (const ht::ShapeParam& pp : c.params) if (pp.key == kv.first) sp = &pp;
        if (!sp || std::fabs(kv.second) < 1e-9) continue;
        w.push_back({kv.second >= 0 ? &sp->incr : &sp->decr, std::fabs(kv.second)});
    }
    std::vector<D3> full; ht::applyToFull(c, w, full);
    if (s.stature > 0) { const D3 piv = scalePivot(c); const double k = s.stature / baseStature(c); for (D3& p : full) p = piv + (p - piv) * k; }
    return full;
}
std::vector<D3> bodyFromFull(const ht::ProofContext& c, const std::vector<D3>& full) {
    std::vector<D3> b(c.bodyStableIds.size());
    for (size_t i = 0; i < c.bodyStableIds.size(); ++i) b[i] = full[static_cast<size_t>(c.bodyStableIds[i])];
    return b;
}
double shoulderOffsetMag(const wp4::RigV2& legacy, const wp4::RigV2& adopted) {
    const int a = legacy.find("upper_arm.L"), b = adopted.find("upper_arm.L");
    if (a < 0 || b < 0) return -1;
    return (adopted.bones[static_cast<size_t>(b)].head - legacy.bones[static_cast<size_t>(a)].head).length();
}
} // namespace

int main(int argc, char** argv) {
    const std::string dataDir = arg(argc, argv, "--data", "");
    const std::string repoRoot = arg(argc, argv, "--repo", "");
    const std::string outDir = arg(argc, argv, "--out", "wp4adopt-out");
    if (dataDir.empty() || repoRoot.empty()) { std::printf("usage: --data --repo --out\n"); return 2; }
    ht::makeDirs(outDir); ht::makeDirs(outDir + "/images");
    std::string log;
    auto say = [&](const std::string& s) { log += s + "\n"; std::printf("%s\n", s.c_str()); };

    ht::ProofContext ctx; std::string err;
    if (!ctx.load(dataDir, repoRoot, err)) { std::printf("load failed: %s\n", err.c_str()); return 1; }
    const int rootV = wp4::resolveRootAnchor(ctx.obj, ctx.full);
    wp4::JointSet js0; wp4::resolveJoints(ctx.obj, ctx.full, rootV, js0);
    wp4::RigV2 legacy, adopted;
    wp4::buildRig(js0, wp4::Profile::Full92, legacy, err, false);
    wp4::buildRig(js0, wp4::Profile::Full92, adopted, err, true);
    const double offMag = shoulderOffsetMag(legacy, adopted);
    say(fmt("[adopt] legacy=%s adopted=%s shoulder offset magnitude=%.5f m (target %.3f)", legacy.version.c_str(),
            adopted.version.c_str(), offMag, wp4::kShoulderProximalOffsetM));
    std::string verMd = "# M2 shoulder offset adoption verification (generated)\n\n";
    verMd += fmt("- legacy rig version: %s\n- adopted rig version: %s\n", legacy.version.c_str(), adopted.version.c_str());
    verMd += fmt("- authored proximal offset: %.4f m along humerus axis\n", wp4::kShoulderProximalOffsetM);
    verMd += fmt("- measured upper_arm.L head shift (legacy->adopted): %.5f m\n", offMag);
    const int ua = adopted.find("upper_arm.L");
    verMd += fmt("- adopted upper_arm.L head: (%.4f, %.4f, %.4f)\n", adopted.bones[ua].head.x, adopted.bones[ua].head.y, adopted.bones[ua].head.z);
    verMd += fmt("- offset magnitude correct (within 0.1mm): %d\n\n", std::fabs(offMag - wp4::kShoulderProximalOffsetM) < 1e-4);

    const int W = 360, H = 720;
    ht::Camera front = ht::makeCamera("front", 0, 0, W, H); fitOne(front, ctx.body.pos);
    wp4::WeightOptions opt;
    auto render = [&](const std::string& tag, const std::vector<D3>& posed, const ht::SkinWeights& sw,
                      const wp4::RigV2& rig, bool closeup, const D3& focus) {
        const ht::Rig hr = rig.asHtRig();
        const auto regs = ht::labelRegions(ctx.body, hr);
        ht::RenderSurface s{&ctx.body, &posed, &regs, &sw, &hr};
        ht::writePngFile(outDir + "/images/" + tag + "-clay.png", ht::renderSurface(s, front, ht::ShadeMode::Clay, false));
        if (closeup) {
            ht::Camera cu = ht::makeCamera("cu", 25, 8, 320, 320);
            ht::fitCameraToBox(cu, focus - D3(0.10, 0.10, 0.10), focus + D3(0.10, 0.10, 0.10));
            ht::writePngFile(outDir + "/images/" + tag + "-closeup.png", ht::renderSurface(s, cu, ht::ShadeMode::Clay, true));
        }
    };

    // ---- 9-shape joint sync ------------------------------------------------
    const std::vector<Shape> shapes = {
        {"baseline", {}, 0.0},
        {"broader-shoulder", {{"shoulderWidth", 1.0}}, 0.0},
        {"broader-ribcage", {{"ribcageWidth", 1.0}}, 0.0},
        {"narrower-waist", {{"waistWidth", -1.0}}, 0.0},
        {"wider-hip", {{"hipWidth", 1.0}}, 0.0},
        {"taller", {}, 1.80},
        {"shorter", {}, 1.60},
        {"combo-athletic", {{"shoulderWidth", 0.75}, {"waistWidth", -0.5}, {"armShape", 0.5}}, 0.0},
        {"combo-pear", {{"hipWidth", 0.75}, {"waistWidth", -0.25}, {"shoulderWidth", -0.5}}, 0.0},
    };
    std::string syncCsv = "shape,jointsResolved,rigValid,version,bindErr,unbound,lrAsym,shoulderOffsetMag,offsetCorrect\n";
    verMd += "## 9-shape joint sync (adopted v2.2)\n\n| shape | joints | rig | bindErr | unbound | L/R asym | shoulder offset(m) | offset ok |\n|---|---|---|---|---|---|---|---|\n";
    for (const Shape& s : shapes) {
        const auto full = applyShape(ctx, s);
        const auto body = bodyFromFull(ctx, full);
        ht::PolyMesh defBody = ctx.body; defBody.pos = body;
        wp4::JointSet js; const bool jok = wp4::resolveJoints(ctx.obj, full, rootV, js);
        wp4::RigV2 rigL, rigA;
        wp4::buildRig(js, wp4::Profile::Full92, rigL, err, false);
        const bool rokA = wp4::buildRig(js, wp4::Profile::Full92, rigA, err, true);
        const auto sw = wp4::generateWeights(defBody, ctx.topo, rigA, wp4::WeightMethod::Region, opt);
        const double be = wp4::bindRecoveryError(body, sw, rigA);
        const auto wd = wp4::diagnoseWeights(defBody, ctx.topo, rigA, sw);
        const double mag = shoulderOffsetMag(rigL, rigA);
        const bool ok = std::fabs(mag - wp4::kShoulderProximalOffsetM) < 1e-4;
        syncCsv += fmt("%s,%d,%d,%s,%.2e,%ld,%.6f,%.5f,%d\n", s.name.c_str(), jok, rokA, rigA.version.c_str(), be, wd.unbound, js.maxLrAsymmetry, mag, ok);
        verMd += fmt("| %s | %d | %d | %.2e | %ld | %.6f | %.5f | %d |\n", s.name.c_str(), jok, rokA, be, wd.unbound, js.maxLrAsymmetry, mag, ok);
        render("shape-" + s.name, body, sw, rigA, false, D3{});
        say(fmt("[shape] %-18s joints=%d rig=%d bindErr=%.2e offset=%.5f ok=%d", s.name.c_str(), jok, rokA, be, mag, ok));
    }
    ht::writeFile(outDir + "/adoption-sync.csv", syncCsv);

    // ---- full-pose regression: legacy vs adopted ---------------------------
    const auto suite = wp4::poseSuite();
    const ht::SkinWeights wL = wp4::generateWeights(ctx.body, ctx.topo, legacy, wp4::WeightMethod::Region, opt);
    const ht::SkinWeights wA = wp4::generateWeights(ctx.body, ctx.topo, adopted, wp4::WeightMethod::Region, opt);
    const auto regsL = ht::labelRegions(ctx.body, legacy.asHtRig());
    const auto regsA = ht::labelRegions(ctx.body, adopted.asHtRig());
    const auto jrL = wp4::jointRegions(legacy, js0);
    const auto jrA = wp4::jointRegions(adopted, js0);
    auto axilla = [&](const std::vector<ht::Region>& regs, const std::vector<wp4::JointRegion>& jr, const std::vector<D3>& posed) {
        wp4::RegionMetrics m;
        for (const auto& r : jr) if (r.name == "shoulder_axilla.L") {
            const auto mask = wp4::regionMask(ctx.body, ctx.body.pos, regs, r);
            m = wp4::evaluateRegion(ctx.body, ctx.topo, ctx.body.pos, posed, mask, r.name);
        }
        return m;
    };
    std::string regCsv = "pose,legacy_flips,adopted_flips,legacy_edgeP99,adopted_edgeP99,legacy_axillaEdgeMax,adopted_axillaEdgeMax,legacy_axillaFlips,adopted_axillaFlips,adopted_volRatio\n";
    verMd += "\n## Full-pose regression (legacy v2.1 vs adopted v2.2)\n\n| pose | flips L->A | edgeP99 L->A | axilla edgeMax L->A | axilla flips L->A |\n|---|---|---|---|---|\n";
    for (const wp4::Pose& pose : suite) {
        const auto gL = wp4::poseGlobals(legacy, pose), gA = wp4::poseGlobals(adopted, pose);
        const auto posedL = wp4::skinPositions(ctx.body.pos, wL, wp4::skinMatrices(legacy, gL));
        const auto posedA = wp4::skinPositions(ctx.body.pos, wA, wp4::skinMatrices(adopted, gA));
        const auto dqL = ht::compareSurfaces(ctx.body, posedL, ctx.topo);
        const auto dqA = ht::compareSurfaces(ctx.body, posedA, ctx.topo);
        const auto axL = axilla(regsL, jrL, posedL);
        const auto axA = axilla(regsA, jrA, posedA);
        regCsv += fmt("%s,%ld,%ld,%.4f,%.4f,%.4f,%.4f,%ld,%ld,%.4f\n", pose.name.c_str(), dqL.flippedFaces, dqA.flippedFaces,
                      dqL.edgeRatioP99, dqA.edgeRatioP99, axL.edgeRatioMax, axA.edgeRatioMax, axL.flippedFaces, axA.flippedFaces, dqA.volumeRatio);
        verMd += fmt("| %s | %ld->%ld | %.3f->%.3f | %.3f->%.3f | %ld->%ld |\n", pose.name.c_str(), dqL.flippedFaces, dqA.flippedFaces,
                     dqL.edgeRatioP99, dqA.edgeRatioP99, axL.edgeRatioMax, axA.edgeRatioMax, axL.flippedFaces, axA.flippedFaces);
        const bool cu = pose.name.rfind("shoulder", 0) == 0;
        render("adopted-" + pose.name, posedA, wA, adopted, cu, js0.group.at("joint-l-shoulder"));
        if (pose.name == "shoulder_abduction_90") render("legacy-" + pose.name, posedL, wL, legacy, true, js0.group.at("joint-l-shoulder"));
        say(fmt("[pose] %-26s flips %ld->%ld axillaEdgeMax %.3f->%.3f", pose.name.c_str(), dqL.flippedFaces, dqA.flippedFaces, axL.edgeRatioMax, axA.edgeRatioMax));
    }
    ht::writeFile(outDir + "/pose-regression.csv", regCsv);
    ht::writeFile(outDir + "/adoption-verification.md", verMd);
    ht::writeFile(outDir + "/run.log", log);
    say("[done] M2 adoption verification complete");
    ht::writeFile(outDir + "/run.log", log);
    return 0;
}