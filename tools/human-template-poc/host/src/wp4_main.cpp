// WP4-HOST-RIG-01 experiment runner. Host-only; no Android, no APK, no commit.
// Pipeline per shape: apply targets -> resolve joints (CC0) -> build PAE rig ->
// generate weights -> CPU LBS -> pose validation + per-joint quality + images.

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "ht_topology.h"
#include "wp3_quality.h"
#include "wp4_poses.h"
#include "wp4_quality.h"
#include "wp4_rig.h"
#include "wp4_skin.h"
#include "wp4_weights.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace pae;
using ht::D3;
using ht::fmt;

namespace {

struct Variant {
    std::string name;
    std::vector<std::pair<std::string, double>> weighted;
    double uniformScaleTo = 0.0;
};

D3 scalePivot(const ht::ProofContext& c) {
    double xMin = 1e30, xMax = -1e30, yMin = 1e30;
    for (const D3& p : c.body.pos) { xMin = std::min(xMin, p.x); xMax = std::max(xMax, p.x); yMin = std::min(yMin, p.y); }
    const int ri = c.rigDeform.find("root");
    const double rootZ = ri >= 0 ? c.rigDeform.bones[static_cast<size_t>(ri)].head.z : 0.0;
    return D3{0.5 * (xMin + xMax), yMin, rootZ};
}

std::vector<D3> applyVariant(const ht::ProofContext& c, const Variant& v) {
    std::vector<std::pair<const ht::ShapeTarget*, double>> weighted;
    for (const auto& w : v.weighted) {
        const ht::ShapeParam* p = nullptr;
        for (const ht::ShapeParam& pp : c.params) if (pp.key == w.first) p = &pp;
        if (!p || std::fabs(w.second) < 1e-9) continue;
        weighted.push_back({w.second >= 0 ? &p->incr : &p->decr, std::fabs(w.second)});
    }
    std::vector<D3> full;
    ht::applyToFull(c, weighted, full);
    if (v.uniformScaleTo > 0.0) {
        const D3 pivot = scalePivot(c);
        double lo = 1e30, hi = -1e30;
        for (const D3& p : c.body.pos) { lo = std::min(lo, p.y); hi = std::max(hi, p.y); }
        const double k = v.uniformScaleTo / (hi - lo);
        for (D3& p : full) p = pivot + (p - pivot) * k;
    }
    return full;
}

std::vector<D3> bodyFromFull(const ht::ProofContext& c, const std::vector<D3>& full) {
    std::vector<D3> body(c.bodyStableIds.size());
    for (size_t i = 0; i < c.bodyStableIds.size(); ++i) body[i] = full[static_cast<size_t>(c.bodyStableIds[i])];
    return body;
}

std::vector<D3> restNormals(const ht::PolyMesh& body) {
    std::vector<D3> n(body.pos.size());
    for (const ht::PolyFace& f : body.faces) {
        D3 fn;
        const size_t k = f.v.size();
        for (size_t i = 0; i < k; ++i) {
            const D3& a = body.pos[static_cast<size_t>(f.v[i])];
            const D3& b = body.pos[static_cast<size_t>(f.v[(i + 1) % k])];
            fn.x += (a.y - b.y) * (a.z + b.z); fn.y += (a.z - b.z) * (a.x + b.x); fn.z += (a.x - b.x) * (a.y + b.y);
        }
        for (int v : f.v) n[static_cast<size_t>(v)] += fn;
    }
    for (D3& v : n) v = v.normalized();
    return n;
}

// --- skeleton overlay (draw bone segments on a rendered image) --------------
void drawLine(ht::Rgb& img, const ht::Camera& c, const D3& a, const D3& b, uint8_t r, uint8_t g, uint8_t bl) {
    const double sx = c.width / (c.uMax - c.uMin), sy = c.height / (c.vMax - c.vMin);
    auto proj = [&](const D3& p) {
        const double u = p.x * c.right[0] + p.y * c.right[1] + p.z * c.right[2];
        const double v = p.x * c.up[0] + p.y * c.up[1] + p.z * c.up[2];
        return std::pair<double, double>{(u - c.uMin) * sx, (c.vMax - v) * sy};
    };
    auto p0 = proj(a), p1 = proj(b);
    const int steps = static_cast<int>(std::max(std::abs(p1.first - p0.first), std::abs(p1.second - p0.second))) + 1;
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        const int x = static_cast<int>(p0.first + (p1.first - p0.first) * t);
        const int y = static_cast<int>(p0.second + (p1.second - p0.second) * t);
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
            const int xx = x + dx, yy = y + dy;
            if (xx < 0 || yy < 0 || xx >= img.width || yy >= img.height) continue;
            const size_t idx = (static_cast<size_t>(yy) * img.width + xx) * 3;
            img.pixels[idx] = r; img.pixels[idx + 1] = g; img.pixels[idx + 2] = bl;
        }
    }
}

ht::Rgb skeletonOverlay(const ht::Rgb& base, const ht::Camera& cam, const wp4::RigV2& rig,
                        const std::vector<ht::Affine>& globals) {
    ht::Rgb img = base;
    for (size_t b = 0; b < rig.bones.size(); ++b) {
        const D3 head = globals[b].t;
        const D3 tail = globals[b].apply(rig.bones[b].tail - rig.bones[b].head);
        drawLine(img, cam, head, tail, 255, 80, 80);
    }
    return img;
}

// --- direction assertions ---------------------------------------------------
struct Assertion { std::string pose, check; bool pass; std::string detail; };

D3 posedHead(const std::vector<ht::Affine>& g, const wp4::RigV2& rig, const std::string& id) {
    const int b = rig.find(id);
    return b >= 0 ? g[static_cast<size_t>(b)].t : D3{};
}

std::vector<Assertion> assertions(const std::string& pose, const wp4::RigV2& rig,
                                  const std::vector<ht::Affine>& rest, const std::vector<ht::Affine>& posed) {
    std::vector<Assertion> out;
    auto add = [&](const std::string& check, bool pass, const std::string& detail) { out.push_back({pose, check, pass, detail}); };
    const D3 shL = posedHead(rest, rig, "shoulder.L"), wrL0 = posedHead(rest, rig, "hand.L");
    const D3 wrL = posedHead(posed, rig, "hand.L"), elL = posedHead(posed, rig, "forearm.L");
    if (pose.rfind("elbow_", 0) == 0 && pose != "elbow_0") {
        const double d0 = (wrL0 - shL).length(), d1 = (wrL - posedHead(posed, rig, "shoulder.L")).length();
        add("hand_toward_shoulder", d1 < d0 - 0.02, fmt("dist %.3f->%.3f m", d0, d1));
        add("hand_anterior_to_elbow", wrL.z > elL.z, fmt("hand.z %.3f elbow.z %.3f", wrL.z, elL.z));
    }
    if (pose.rfind("shoulder_abduction", 0) == 0) {
        add("wrist_rises", wrL.y > wrL0.y + 0.02, fmt("wrist.y %.3f->%.3f m", wrL0.y, wrL.y));
    }
    if (pose == "shoulder_forward_flexion") {
        add("wrist_forward", wrL.z > wrL0.z + 0.02, fmt("wrist.z %.3f->%.3f m", wrL0.z, wrL.z));
    }
    if (pose.rfind("knee_", 0) == 0) {
        const D3 an0 = posedHead(rest, rig, "foot.L"), an = posedHead(posed, rig, "foot.L");
        add("ankle_posterior", an.z < an0.z - 0.01, fmt("ankle.z %.3f->%.3f m", an0.z, an.z));
    }
    if (pose == "hip_flexion") {
        const D3 kn0 = posedHead(rest, rig, "shin.L"), kn = posedHead(posed, rig, "shin.L");
        add("knee_forward_or_up", kn.z > kn0.z + 0.01 || kn.y > kn0.y + 0.01, fmt("knee z %.3f->%.3f y %.3f->%.3f", kn0.z, kn.z, kn0.y, kn.y));
    }
    if (pose == "hip_abduction") {
        const D3 kn0 = posedHead(rest, rig, "shin.L"), kn = posedHead(posed, rig, "shin.L");
        add("knee_lateral", kn.x > kn0.x + 0.01, fmt("knee.x %.3f->%.3f m", kn0.x, kn.x));
    }
    if (pose == "one_arm_raised") {
        const D3 wrR0 = posedHead(rest, rig, "hand.R"), wrR = posedHead(posed, rig, "hand.R");
        const double dev = (wrR - wrR0).length();
        add("contralateral_still", dev < 1e-5, fmt("R wrist dev %.6f m", dev));
    }
    return out;
}

std::string arg(int argc, char** argv, const std::string& key, const std::string& def) {
    for (int i = 1; i + 1 < argc; ++i) if (key == argv[i]) return argv[i + 1];
    return def;
}

} // namespace

int main(int argc, char** argv) {
    const std::string dataDir = arg(argc, argv, "--data", "");
    const std::string repoRoot = arg(argc, argv, "--repo", "");
    const std::string outDir = arg(argc, argv, "--out", "wp4-out");
    if (dataDir.empty() || repoRoot.empty()) { std::printf("usage: --data <dir> --repo <dir> --out <dir>\n"); return 2; }
    ht::makeDirs(outDir);
    ht::makeDirs(outDir + "/images");
    std::string log;
    auto say = [&](const std::string& s) { log += s + "\n"; std::printf("%s\n", s.c_str()); };

    ht::ProofContext ctx;
    std::string err;
    if (!ctx.load(dataDir, repoRoot, err)) { std::printf("load failed: %s\n", err.c_str()); return 1; }
    say(fmt("[load] body=%zu full=%zu params=%zu", ctx.body.pos.size(), ctx.full.size(), ctx.params.size()));

    const int rootVertex = wp4::resolveRootAnchor(ctx.obj, ctx.full);
    say(fmt("[anchor] root head vertex id=%d pos=(%.5f,%.5f,%.5f)", rootVertex,
            ctx.full[static_cast<size_t>(rootVertex)].x, ctx.full[static_cast<size_t>(rootVertex)].y, ctx.full[static_cast<size_t>(rootVertex)].z));

    // ---- baseline rig + deviation vs research reference --------------------
    wp4::JointSet js0;
    if (!wp4::resolveJoints(ctx.obj, ctx.full, rootVertex, js0)) { say("[FAIL] unresolved joints"); return 1; }
    wp4::RigV2 full0, core0;
    if (!wp4::buildRig(js0, wp4::Profile::Full92, full0, err, false) || !wp4::buildRig(js0, wp4::Profile::Core20, core0, err, false)) {
        say("[FAIL] build rig: " + err); return 1;
    }
    const auto vf = wp4::validateRig(full0), vc = wp4::validateRig(core0);
    say(fmt("[rig] full92 bones=%d valid=%d minLen=%.5f | core20 bones=%d valid=%d", vf.boneCount, vf.ok, vf.minBoneLength, vc.boneCount, vc.ok));
    say(fmt("[symmetry] L/R pairs=%d maxAsymmetry=%.6f m", js0.lrPairs, js0.maxLrAsymmetry));

    const auto dev = wp4::compareToReference(full0, ctx.rigDeform);
    double maxDev = 0.0; for (const auto& d : dev) maxDev = std::max(maxDev, std::max(d.headDev, d.tailDev));
    say(fmt("[deviation] vs PENDING research reference: %zu shared bones, max joint dev=%.5f m", dev.size(), maxDev));

    // ---- weights on baseline (3 methods) -----------------------------------
    const ht::Rig hr0 = full0.asHtRig();
    wp4::WeightOptions opt;
    const ht::SkinWeights wDist = wp4::generateWeights(ctx.body, ctx.topo, full0, wp4::WeightMethod::Distance, opt);
    const ht::SkinWeights wHeat = wp4::generateWeights(ctx.body, ctx.topo, full0, wp4::WeightMethod::Heat, opt);
    const ht::SkinWeights wReg = wp4::generateWeights(ctx.body, ctx.topo, full0, wp4::WeightMethod::Region, opt);
    const auto dDist = wp4::diagnoseWeights(ctx.body, ctx.topo, full0, wDist);
    const auto dHeat = wp4::diagnoseWeights(ctx.body, ctx.topo, full0, wHeat);
    const auto dReg = wp4::diagnoseWeights(ctx.body, ctx.topo, full0, wReg);
    say(fmt("[weights] distance unbound=%ld crossSide=%.4f farBound=%ld smooth=%.4f genMs=%.0f", dDist.unbound, dDist.crossSideMass, dDist.farBoundVertices, dDist.smoothnessL1, dDist.generationMs));
    say(fmt("[weights] heat     unbound=%ld crossSide=%.4f farBound=%ld smooth=%.4f genMs=%.0f", dHeat.unbound, dHeat.crossSideMass, dHeat.farBoundVertices, dHeat.smoothnessL1, dHeat.generationMs));
    say(fmt("[weights] region   unbound=%ld crossSide=%.4f farBound=%ld smooth=%.4f genMs=%.0f", dReg.unbound, dReg.crossSideMass, dReg.farBoundVertices, dReg.smoothnessL1, dReg.generationMs));

    const double bindErr = wp4::bindRecoveryError(ctx.body.pos, wDist, full0);
    say(fmt("[bind] recovery max error=%.3e m (must ~0)", bindErr));

    // ---- cameras -----------------------------------------------------------
    const int W = 360, H = 720;
    auto fitOne = [&](ht::Camera& cam, const std::vector<D3>& pos) {
        std::vector<ht::Camera> v{cam};
        std::vector<const std::vector<D3>*> sets{&pos};
        ht::fitSharedFrames(v, sets);
        cam = v[0];
    };
    ht::Camera front = ht::makeCamera("front", 0, 0, W, H);
    ht::Camera side = ht::makeCamera("side-left", 90, 0, W, H);
    fitOne(front, ctx.body.pos); fitOne(side, ctx.body.pos);
    const std::vector<ht::Region> regions0 = ht::labelRegions(ctx.body, hr0);
    const auto jregions = wp4::jointRegions(full0, js0);
    const std::vector<D3> rn = restNormals(ctx.body);

    auto renderPose = [&](const std::string& tag, const std::vector<D3>& posed, const ht::SkinWeights& sw,
                          const wp4::RigV2& rig, const std::vector<ht::Affine>& globals, bool closeup, const D3& focus) {
        const ht::Rig hr = rig.asHtRig();
        ht::RenderSurface s2{&ctx.body, &posed, &regions0, &sw, &hr};
        ht::writePngFile(outDir + "/images/" + tag + "-clay.png", ht::renderSurface(s2, front, ht::ShadeMode::Clay, false));
        ht::writePngFile(outDir + "/images/" + tag + "-wire.png", ht::renderSurface(s2, front, ht::ShadeMode::Wireframe, false));
        ht::writePngFile(outDir + "/images/" + tag + "-normal.png", ht::renderSurface(s2, front, ht::ShadeMode::ViewNormal, false));
        ht::writePngFile(outDir + "/images/" + tag + "-heat.png", ht::renderSurface(s2, front, ht::ShadeMode::WeightHeat, false));
        ht::Rgb clay = ht::renderSurface(s2, front, ht::ShadeMode::Clay, false);
        ht::writePngFile(outDir + "/images/" + tag + "-overlay.png", skeletonOverlay(clay, front, rig, globals));
        if (closeup) {
            ht::Camera cu = ht::makeCamera("closeup", 20, 5, 320, 320);
            ht::fitCameraToBox(cu, focus - D3(0.09, 0.09, 0.09), focus + D3(0.09, 0.09, 0.09));
            ht::writePngFile(outDir + "/images/" + tag + "-closeup.png", ht::renderSurface(s2, cu, ht::ShadeMode::Clay, true));
        }
    };

    // ---- baseline full pose suite ------------------------------------------
    const auto suite = wp4::poseSuite();
    const auto restG = wp4::poseGlobals(full0, wp4::Pose{"bind", {}});
    std::string defCsv = "pose,flipped,selfIntPairs,edgeP99,edgeMax,volumeRatio,minNormalDot,cpuMs,bindErr\n";
    std::string regCsv = "pose,region,vertices,faces,flipped,minNormalDot,edgeP99,edgeMax,areaRatio\n";
    std::string assertMd = "# WP4 direction assertions (generated)\n\n| pose | check | pass | detail |\n|---|---|---|---|\n";
    long assertFail = 0;
    const D3 shoulderFocus = js0.group["joint-l-shoulder"];
    for (const wp4::Pose& pose : suite) {
        const double t0 = ht::nowMs();
        const auto g = wp4::poseGlobals(full0, pose);
        const auto skin = wp4::skinMatrices(full0, g);
        const auto posed = wp4::skinPositions(ctx.body.pos, wReg, skin);
        const auto posedN = wp4::skinNormals(rn, wReg, skin);
        const double ms = ht::nowMs() - t0;
        const auto dq = ht::compareSurfaces(ctx.body, posed, ctx.topo);
        const auto q = wp3::evaluateQuality(ctx.body, posed, ctx.topo, &ctx.body.pos);
        defCsv += fmt("%s,%ld,%zu,%.4f,%.4f,%.4f,%.4f,%.2f,%.2e\n", pose.name.c_str(), dq.flippedFaces,
                      q.selfIntersectingPairs, dq.edgeRatioP99, dq.maxEdgeRatio, dq.volumeRatio, dq.minNormalDot, ms, bindErr);
        for (const auto& jr : jregions) {
            const auto mask = wp4::regionMask(ctx.body, ctx.body.pos, regions0, jr);
            const auto rm = wp4::evaluateRegion(ctx.body, ctx.topo, ctx.body.pos, posed, mask, jr.name);
            regCsv += fmt("%s,%s,%ld,%ld,%ld,%.4f,%.4f,%.4f,%.4f\n", pose.name.c_str(), jr.name.c_str(), rm.vertices,
                          rm.faces, rm.flippedFaces, rm.minNormalDot, rm.edgeRatioP99, rm.edgeRatioMax, rm.areaRatio);
        }
        for (const auto& a : assertions(pose.name, full0, restG, g)) {
            assertMd += fmt("| %s | %s | %s | %s |\n", a.pose.c_str(), a.check.c_str(), a.pass ? "PASS" : "FAIL", a.detail.c_str());
            if (!a.pass) ++assertFail;
        }
        const bool cu = pose.name.rfind("shoulder", 0) == 0 || pose.name.rfind("elbow", 0) == 0 || pose.name == "fist";
        renderPose("base-" + pose.name, posed, wReg, full0, g, cu, shoulderFocus);
        say(fmt("[pose] %-26s flips=%ld selfInt=%zu edgeP99=%.3f vol=%.3f cpu=%.1fms", pose.name.c_str(),
                dq.flippedFaces, q.selfIntersectingPairs, dq.edgeRatioP99, dq.volumeRatio, ms));
        (void)posedN;
    }

    // ---- 9-shape sync sweep ------------------------------------------------
    const std::vector<Variant> shapes = {
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
    std::string syncCsv = "shape,jointsResolved,maxLrAsym,rigValid,bindErr,weightUnbound,pose,flipped,edgeP99,volumeRatio\n";
    const auto subset = wp4::syncSubset();
    for (const Variant& v : shapes) {
        const auto full = applyVariant(ctx, v);
        const auto body = bodyFromFull(ctx, full);
        ht::PolyMesh defBody = ctx.body; defBody.pos = body;  // current-shape rest
        wp4::JointSet js;
        const bool jok = wp4::resolveJoints(ctx.obj, full, rootVertex, js);
        wp4::RigV2 rig;
        const bool rok = wp4::buildRig(js, wp4::Profile::Full92, rig, err, false);
        const auto sw = wp4::generateWeights(defBody, ctx.topo, rig, wp4::WeightMethod::Region, opt);
        const double be = wp4::bindRecoveryError(body, sw, rig);
        const auto wd = wp4::diagnoseWeights(defBody, ctx.topo, rig, sw);
        const auto regs = ht::labelRegions(defBody, rig.asHtRig());
        for (const wp4::Pose& pose : subset) {
            const auto g = wp4::poseGlobals(rig, pose);
            const auto skin = wp4::skinMatrices(rig, g);
            const auto posed = wp4::skinPositions(body, sw, skin);
            const auto dq = ht::compareSurfaces(defBody, posed, ctx.topo);
            syncCsv += fmt("%s,%d,%.6f,%d,%.2e,%ld,%s,%ld,%.4f,%.4f\n", v.name.c_str(), jok, js.maxLrAsymmetry, rok,
                           be, wd.unbound, pose.name.c_str(), dq.flippedFaces, dq.edgeRatioP99, dq.volumeRatio);
            if (pose.name == "bind") {
                ht::Camera cam = ht::makeCamera("front", 0, 0, W, H);
                fitOne(cam, body);
                const ht::Rig hr = rig.asHtRig();
                ht::RenderSurface s2{&ctx.body, &body, &regs, &sw, &hr};
                ht::writePngFile(outDir + "/images/shape-" + v.name + "-bind-clay.png", ht::renderSurface(s2, cam, ht::ShadeMode::Clay, false));
            }
        }
        say(fmt("[shape] %-18s joints=%d rig=%d bindErr=%.2e unbound=%ld", v.name.c_str(), jok, rok, be, wd.unbound));
    }

    // ---- 2 extra shapes full suite (baseline already done) -----------------
    for (const std::string& nm : {std::string("combo-athletic"), std::string("wider-hip")}) {
        Variant v; for (const Variant& s : shapes) if (s.name == nm) v = s;
        const auto full = applyVariant(ctx, v);
        const auto body = bodyFromFull(ctx, full);
        ht::PolyMesh defBody = ctx.body; defBody.pos = body;
        wp4::JointSet js; wp4::resolveJoints(ctx.obj, full, rootVertex, js);
        wp4::RigV2 rig; wp4::buildRig(js, wp4::Profile::Full92, rig, err, false);
        const auto sw = wp4::generateWeights(defBody, ctx.topo, rig, wp4::WeightMethod::Region, opt);
        const auto regs = ht::labelRegions(defBody, rig.asHtRig());
        const ht::Rig hr = rig.asHtRig();
        ht::Camera cam = ht::makeCamera("front", 0, 0, W, H);
        fitOne(cam, body);
        for (const wp4::Pose& pose : suite) {
            const auto g = wp4::poseGlobals(rig, pose);
            const auto posed = wp4::skinPositions(body, sw, wp4::skinMatrices(rig, g));
            ht::RenderSurface s2{&ctx.body, &posed, &regs, &sw, &hr};
            ht::writePngFile(outDir + "/images/" + nm + "-" + pose.name + "-clay.png", ht::renderSurface(s2, cam, ht::ShadeMode::Clay, false));
        }
        say("[extra-suite] " + nm + " rendered " + std::to_string(suite.size()) + " poses");
    }

    // ---- Core vs Full comparison -------------------------------------------
    std::string cmpCsv = "profile,pose,bones,flipped,edgeP99,volumeRatio,cpuMs,weightDiscarded,fistExpressible\n";
    for (const wp4::Pose& pose : wp4::comparisonPoses()) {
        for (int pi = 0; pi < 2; ++pi) {
            const wp4::RigV2& rig = pi == 0 ? core0 : full0;
            const ht::SkinWeights sw = wp4::generateWeights(ctx.body, ctx.topo, rig, wp4::WeightMethod::Region, opt);
            const double t0 = ht::nowMs();
            const auto g = wp4::poseGlobals(rig, pose);
            const auto posed = wp4::skinPositions(ctx.body.pos, sw, wp4::skinMatrices(rig, g));
            const double ms = ht::nowMs() - t0;
            const auto dq = ht::compareSurfaces(ctx.body, posed, ctx.topo);
            const bool fistOk = rig.find("finger.02.01.L") >= 0;
            cmpCsv += fmt("%s,%s,%zu,%ld,%.4f,%.4f,%.2f,%.5f,%d\n", wp4::profileName(rig.profile), pose.name.c_str(),
                          rig.bones.size(), dq.flippedFaces, dq.edgeRatioP99, dq.volumeRatio, ms, sw.massLostByCompression, fistOk);
            const ht::Rig hr = rig.asHtRig();
            ht::RenderSurface s2{&ctx.body, &posed, &regions0, &sw, &hr};
            ht::writePngFile(outDir + "/images/cmp-" + std::string(wp4::profileName(rig.profile)) + "-" + pose.name + "-clay.png",
                             ht::renderSurface(s2, front, ht::ShadeMode::Clay, false));
        }
    }
    say("[compare] core20 vs full92 done");

    // ---- reports -----------------------------------------------------------
    ht::writeFile(outDir + "/deformation-metrics.csv", defCsv);
    ht::writeFile(outDir + "/region-metrics.csv", regCsv);
    ht::writeFile(outDir + "/shape-sync.csv", syncCsv);
    ht::writeFile(outDir + "/core-vs-full.csv", cmpCsv);
    ht::writeFile(outDir + "/assertions.md", assertMd);

    std::string sk = "# WP4 skeleton definitions (generated)\n\n";
    sk += fmt("rig version: %s\n\n", wp4::kRigVersion);
    for (int pi = 0; pi < 2; ++pi) {
        const wp4::RigV2& rig = pi == 0 ? core0 : full0;
        sk += "## " + std::string(wp4::profileName(rig.profile)) + " (" + std::to_string(rig.bones.size()) + " bones)\n\n";
        sk += "| semantic id | parent | role | deform group | head | tail | length(m) |\n|---|---|---|---|---|---|---|\n";
        for (size_t b = 0; b < rig.bones.size(); ++b) {
            const ht::RigBone& bb = rig.bones[b];
            const std::string par = bb.parent >= 0 ? rig.bones[static_cast<size_t>(bb.parent)].name : "-";
            sk += fmt("| %s | %s | %s | %s | (%.3f,%.3f,%.3f) | (%.3f,%.3f,%.3f) | %.4f |\n", bb.name.c_str(), par.c_str(),
                      ht::roleName(bb.role), rig.deformGroup[b].c_str(), bb.head.x, bb.head.y, bb.head.z, bb.tail.x, bb.tail.y, bb.tail.z,
                      (bb.tail - bb.head).length());
        }
        sk += "\n";
    }
    ht::writeFile(outDir + "/skeleton-definitions.md", sk);

    std::string jt = "joint,x,y,z\n";
    for (const auto& kv : js0.group) jt += fmt("%s,%.6f,%.6f,%.6f\n", kv.first.c_str(), kv.second.x, kv.second.y, kv.second.z);
    jt += fmt("root-anchor,%.6f,%.6f,%.6f\n", js0.rootHead.x, js0.rootHead.y, js0.rootHead.z);
    ht::writeFile(outDir + "/joint-table.csv", jt);

    std::string dv = "bone,headDev,tailDev\n";
    for (const auto& d : dev) dv += fmt("%s,%.6f,%.6f\n", d.bone.c_str(), d.headDev, d.tailDev);
    ht::writeFile(outDir + "/joint-deviation-vs-reference.csv", dv);

    std::string wd = "# WP4 weight diagnostics (generated)\n\n";
    wd += "| method | unbound | maxInf | minSum | maxSum | discarded | crossSideMass | crossSideVerts | farBound | meanDomDist | smoothL1 | genMs |\n";
    wd += "|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    auto row = [&](const char* n, const wp4::WeightDiagnostics& d) {
        wd += fmt("| %s | %ld | %d | %.4f | %.4f | %.5f | %.5f | %ld | %ld | %.4f | %.4f | %.0f |\n", n, d.unbound,
                  d.maxInfluences, d.minSum, d.maxSum, d.discardedMass, d.crossSideMass, d.crossSideVertices, d.farBoundVertices,
                  d.meanDominantDistance, d.smoothnessL1, d.generationMs);
    };
    row("distance", dDist); row("heat", dHeat); row("region", dReg);
    ht::writeFile(outDir + "/weight-diagnostics.md", wd);

    std::string wt = "bone,distanceMass,regionMass\n";
    const auto smD = wp4::boneWeightSummaries(full0, wDist), smR = wp4::boneWeightSummaries(full0, wReg);
    for (size_t b = 0; b < smD.size(); ++b) wt += fmt("%s,%.4f,%.4f\n", smD[b].bone.c_str(), smD[b].mass, smR[b].mass);
    ht::writeFile(outDir + "/weight-table.csv", wt);

    std::string pi = "# WP4 pose inputs (Bone Local Transforms, generated)\n\n";
    for (const wp4::Pose& p : suite) {
        pi += "## " + p.name + "\n\n";
        if (p.rotations.empty()) pi += "(bind / no rotation)\n\n";
        for (const auto& r : p.rotations) pi += fmt("- %s axis %c %+.1f deg\n", r.bone.c_str(), "XYZ"[r.axis], r.angleDeg);
        pi += "\n";
    }
    ht::writeFile(outDir + "/pose-inputs.md", pi);

    ht::writeFile(outDir + "/run.log", log);
    say(fmt("[done] assertions failed=%ld bindErr=%.2e maxDev=%.5f", assertFail, bindErr, maxDev));
    ht::writeFile(outDir + "/run.log", log);
    // bindErr floor ~3.7e-8 m is float-weight rounding (positions are double);
    // 1e-6 m (1 micrometre) is exact recovery for practical purposes.
    return (vf.ok && vc.ok && bindErr < 1e-6 && assertFail == 0) ? 0 : 3;
}