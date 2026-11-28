// M2-POSE-FRAME-AUDIT-01: 22-pose Before/After regression.
// Before = legacy world-axis rotation formula (the reproduced bug).
// After  = fixed local-unit-axis formula (wp4::poseGlobals).
// Identical base mesh / rig / joints / Region weights / pose inputs / masks /
// camera. Host-only; no Android, no DQS, no weight/joint/mesh change.

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "ht_topology.h"
#include "wp4_poses.h"
#include "wp4_quality.h"
#include "wp4_rig.h"
#include "wp4_skin.h"
#include "wp4_weights.h"

#include "procedural/MeshData.h"
#include "procedural/humanoid/canonical/CanonicalTopology.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace pae;
using ht::D3;
using ht::Affine;
using ht::fmt;

namespace {
std::string arg(int argc, char** argv, const std::string& k, const std::string& d) {
    for (int i = 1; i + 1 < argc; ++i) if (k == argv[i]) return argv[i + 1];
    return d;
}
// BEFORE: the legacy buggy formula — rotation axis = rig.ax/ay/az (REST WORLD
// axes) right-multiplied onto restLocal. Reproduced here verbatim for comparison.
std::vector<Affine> poseGlobalsLegacy(const wp4::RigV2& rig, const wp4::Pose& pose) {
    const size_t n = rig.bones.size();
    std::vector<Affine> local(n);
    for (size_t b = 0; b < n; ++b) {
        Affine rl = Affine::identity();
        for (const auto& r : pose.rotations) {
            if (r.bone != rig.bones[b].name || r.angleDeg == 0.0) continue;
            const D3 axis = r.axis == 0 ? rig.ax[b] : (r.axis == 1 ? rig.ay[b] : rig.az[b]);
            rl = rl * Affine::rotation(axis, r.angleDeg * 3.14159265358979323846 / 180.0);
        }
        local[b] = rig.restLocal[b] * rl;
    }
    std::vector<Affine> globals(n);
    std::vector<int> state(n, 0);
    for (size_t b = 0; b < n; ++b) {
        std::vector<int> chain; int cur = static_cast<int>(b);
        while (cur >= 0 && state[static_cast<size_t>(cur)] != 2) {
            if (state[static_cast<size_t>(cur)] == 1) break;
            state[static_cast<size_t>(cur)] = 1; chain.push_back(cur); cur = rig.bones[static_cast<size_t>(cur)].parent;
        }
        Affine g = cur >= 0 ? globals[static_cast<size_t>(cur)] : Affine::identity();
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) { g = g * local[static_cast<size_t>(*it)]; globals[static_cast<size_t>(*it)] = g; state[static_cast<size_t>(*it)] = 2; }
    }
    return globals;
}
size_t selfIntPairs(const ht::PolyMesh& body, const std::vector<D3>& pos) {
    pae::MeshData mesh; mesh.vertices.resize(pos.size());
    std::vector<D3> accum(pos.size());
    auto tri = [&](int a, int b, int c) {
        const D3 fn = (pos[b] - pos[a]).cross(pos[c] - pos[a]);
        accum[a] += fn; accum[b] += fn; accum[c] += fn;
        mesh.indices.push_back(a); mesh.indices.push_back(b); mesh.indices.push_back(c);
    };
    for (const auto& f : body.faces) {
        if (f.v.size() == 4) { tri(f.v[0], f.v[1], f.v[2]); tri(f.v[0], f.v[2], f.v[3]); }
        else for (size_t k = 1; k + 1 < f.v.size(); ++k) tri(f.v[0], f.v[k], f.v[k + 1]);
    }
    pae::Aabb bounds;
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        mesh.vertices[i].position = pae::Vec3(static_cast<float>(pos[i].x), static_cast<float>(pos[i].y), static_cast<float>(pos[i].z));
        const D3 n = accum[i].normalized();
        mesh.vertices[i].normal = pae::Vec3(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z));
        bounds.expand(mesh.vertices[i].position);
    }
    mesh.bounds = bounds;
    return pae::canonical::detectSelfIntersections(mesh, 64).intersectingPairs;
}
struct EdgePct { double p95 = 1, p99 = 1, max = 1; };
EdgePct edgePercentiles(const ht::PolyMesh& body, const std::vector<D3>& rest, const std::vector<D3>& posed, const ht::Topology& topo) {
    std::vector<double> r;
    for (const auto& e : topo.edges) {
        const double lr = (rest[e[1]] - rest[e[0]]).length();
        const double lp = (posed[e[1]] - posed[e[0]]).length();
        if (lr > 1e-9) r.push_back(lp / lr);
    }
    EdgePct out;
    if (!r.empty()) {
        std::sort(r.begin(), r.end());
        out.max = r.back();
        out.p99 = r[static_cast<size_t>(std::min<double>(r.size() - 1, r.size() * 0.99))];
        out.p95 = r[static_cast<size_t>(std::min<double>(r.size() - 1, r.size() * 0.95))];
    }
    return out;
}
void fitOne(ht::Camera& cam, const std::vector<D3>& pos) {
    std::vector<ht::Camera> v{cam}; std::vector<const std::vector<D3>*> s{&pos}; ht::fitSharedFrames(v, s); cam = v[0];
}
} // namespace

int main(int argc, char** argv) {
    const std::string dataDir = arg(argc, argv, "--data", "");
    const std::string repoRoot = arg(argc, argv, "--repo", "");
    const std::string outDir = arg(argc, argv, "--out", "wp4frame-out");
    if (dataDir.empty() || repoRoot.empty()) { std::printf("usage: --data --repo --out\n"); return 2; }
    ht::makeDirs(outDir); ht::makeDirs(outDir + "/images");
    std::string log;
    auto say = [&](const std::string& s) { log += s + "\n"; std::printf("%s\n", s.c_str()); };

    ht::ProofContext ctx; std::string err;
    if (!ctx.load(dataDir, repoRoot, err)) { std::printf("load failed: %s\n", err.c_str()); return 1; }
    const int rootV = wp4::resolveRootAnchor(ctx.obj, ctx.full);
    wp4::JointSet js; wp4::resolveJoints(ctx.obj, ctx.full, rootV, js);
    wp4::RigV2 rig; wp4::buildRig(js, wp4::Profile::Full92, rig, err);  // v2.1 default
    wp4::WeightOptions opt;
    const ht::SkinWeights w = wp4::generateWeights(ctx.body, ctx.topo, rig, wp4::WeightMethod::Region, opt);
    const auto regs = ht::labelRegions(ctx.body, rig.asHtRig());
    const auto jr = wp4::jointRegions(rig, js);
    const ht::Rig hr = rig.asHtRig();
    const double bindErr = wp4::bindRecoveryError(ctx.body.pos, w, rig);
    say(fmt("[load] body=%zu rig=%zu bindErr=%.3e", ctx.body.pos.size(), rig.bones.size(), bindErr));

    const int W = 360, H = 720;
    ht::Camera front = ht::makeCamera("front", 0, 0, W, H); fitOne(front, ctx.body.pos);
    auto render = [&](const std::string& tag, const std::vector<D3>& posed, bool wire, bool closeup, const D3& focus) {
        ht::RenderSurface s{&ctx.body, &posed, &regs, &w, &hr};
        ht::writePngFile(outDir + "/images/" + tag + (wire ? "-wire.png" : "-clay.png"),
                         ht::renderSurface(s, front, wire ? ht::ShadeMode::Wireframe : ht::ShadeMode::Clay, false));
        if (closeup) {
            ht::Camera cu = ht::makeCamera("cu", 25, 8, 320, 320);
            ht::fitCameraToBox(cu, focus - D3(0.11, 0.11, 0.11), focus + D3(0.11, 0.11, 0.11));
            ht::writePngFile(outDir + "/images/" + tag + "-closeup.png", ht::renderSurface(s, cu, ht::ShadeMode::Clay, true));
        }
    };

    const auto suite = wp4::poseSuite();
    std::string csv = "pose,variant,flips,flipRate,selfInt,minNormalDot,edgeP95,edgeP99,edgeMax,volumeRatio,cpuMs";
    for (const auto& r : jr) csv += "," + r.name + "_flips," + r.name + "_edgeMax," + r.name + "_areaRatio";
    csv += "\n";
    std::string md = "# M2-POSE-FRAME-AUDIT-01 22-pose Before/After (generated)\n\n";
    md += "Before = legacy world-axis formula (bug); After = fixed local-unit-axis. Identical mesh/rig/weights/poses/masks.\n\n";
    md += "| pose | flips B->A | flipRate B->A | selfInt B->A | edgeP99 B->A | volRatio B->A |\n|---|---|---|---|---|---|\n";

    const std::vector<std::string> keyPoses = {"bind", "fist", "finger_curl", "elbow_90", "elbow_120",
        "shoulder_abduction_60", "shoulder_abduction_90", "arms_raised", "basic_squat"};
    auto isKey = [&](const std::string& n) { return std::find(keyPoses.begin(), keyPoses.end(), n) != keyPoses.end(); };

    for (const wp4::Pose& pose : suite) {
        const double t0 = ht::nowMs();
        const auto gB = poseGlobalsLegacy(rig, pose);
        const auto posedB = wp4::skinPositions(ctx.body.pos, w, wp4::skinMatrices(rig, gB));
        const double msB = ht::nowMs() - t0;
        const double t1 = ht::nowMs();
        const auto gA = wp4::poseGlobals(rig, pose);
        const auto posedA = wp4::skinPositions(ctx.body.pos, w, wp4::skinMatrices(rig, gA));
        const double msA = ht::nowMs() - t1;
        const auto dqB = ht::compareSurfaces(ctx.body, posedB, ctx.topo);
        const auto dqA = ht::compareSurfaces(ctx.body, posedA, ctx.topo);
        const size_t siB = selfIntPairs(ctx.body, posedB);
        const size_t siA = selfIntPairs(ctx.body, posedA);
        const auto epB = edgePercentiles(ctx.body, ctx.body.pos, posedB, ctx.topo);
        const auto epA = edgePercentiles(ctx.body, ctx.body.pos, posedA, ctx.topo);
        const double frB = static_cast<double>(dqB.flippedFaces) / ctx.body.faces.size();
        const double frA = static_cast<double>(dqA.flippedFaces) / ctx.body.faces.size();
        auto row = [&](const std::string& var, const std::vector<D3>& posed, const ht::DeformationQuality& dq, size_t si, const EdgePct& ep, double ms) {
            csv += fmt("%s,%s,%ld,%.5f,%zu,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f", pose.name.c_str(), var.c_str(), dq.flippedFaces,
                       static_cast<double>(dq.flippedFaces) / ctx.body.faces.size(), si, dq.minNormalDot, ep.p95, ep.p99, ep.max, dq.volumeRatio, ms);
            for (const auto& r : jr) {
                const auto mask = wp4::regionMask(ctx.body, ctx.body.pos, regs, r);
                const auto rm = wp4::evaluateRegion(ctx.body, ctx.topo, ctx.body.pos, posed, mask, r.name);
                csv += fmt(",%ld,%.4f,%.4f", rm.flippedFaces, rm.edgeRatioMax, rm.areaRatio);
            }
            csv += "\n";
        };
        row("before", posedB, dqB, siB, epB, msB);
        row("after", posedA, dqA, siA, epA, msA);
        md += fmt("| %s | %ld->%ld | %.4f->%.4f | %zu->%zu | %.3f->%.3f | %.3f->%.3f |\n",
                  pose.name.c_str(), dqB.flippedFaces, dqA.flippedFaces, frB, frA, siB, siA, epB.p99, epA.p99, dqB.volumeRatio, dqA.volumeRatio);
        if (isKey(pose.name)) {
            const D3 focus = pose.name.rfind("elbow", 0) == 0 ? js.group.at("joint-l-elbow")
                : (pose.name.rfind("shoulder", 0) == 0 || pose.name == "arms_raised" ? js.group.at("joint-l-shoulder")
                : (pose.name == "basic_squat" ? js.group.at("joint-l-knee") : js.group.at("joint-l-hand")));
            render(pose.name + "-before", posedB, false, true, focus);
            render(pose.name + "-after", posedA, false, true, focus);
            render(pose.name + "-before", posedB, true, false, focus);
            render(pose.name + "-after", posedA, true, false, focus);
        }
        say(fmt("[pose] %-26s flips %ld->%ld selfInt %zu->%zu edgeP99 %.3f->%.3f", pose.name.c_str(),
                dqB.flippedFaces, dqA.flippedFaces, siB, siA, epB.p99, epA.p99));
    }
    ht::writeFile(outDir + "/pose-before-after.csv", csv);
    ht::writeFile(outDir + "/pose-before-after.md", md);
    ht::writeFile(outDir + "/run.log", log);
    say("[done] 22-pose Before/After complete");
    ht::writeFile(outDir + "/run.log", log);
    return 0;
}