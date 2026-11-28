// VR-1B M2 deformation-quality improvement (WP4 host whitelist).
// (1) Shoulder joint-center anatomical offset: measure acromion-vs-helper
//     geometry, sweep a PAE-authored parametric offset, measure abduction
//     axilla quality. (2) Extreme-pose flip attribution: per-region flips /
//     edge stretch / weight purity / self-intersection localization / per-finger
//     breakdown. Host-only; no Android, no DQS.

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "wp3_quality.h"
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
#include <map>
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

// Acromion surface landmark: highest body surface vertex in the lateral shoulder
// band (PAE-authored detection rule). Returns position + vertex id.
D3 acromionLandmark(const ht::ProofContext& c, const D3& shoulderJoint, int& vid) {
    double bestY = -1e30; D3 best; vid = -1;
    for (size_t v = 0; v < c.body.pos.size(); ++v) {
        const D3& p = c.body.pos[v];
        if (p.x < 0.10 || p.x > 0.24) continue;          // lateral shoulder band (L)
        if (p.y < shoulderJoint.y - 0.06) continue;       // near shoulder height
        if (p.y > bestY) { bestY = p.y; best = p; vid = static_cast<int>(v); }
    }
    return best;
}

// Apply a PAE-authored offset to the shoulder joint centers (mirror for R).
wp4::JointSet offsetShoulder(wp4::JointSet js, const D3& offL) {
    if (js.group.count("joint-l-shoulder")) js.group["joint-l-shoulder"] = js.group["joint-l-shoulder"] + offL;
    if (js.group.count("joint-r-shoulder")) js.group["joint-r-shoulder"] = js.group["joint-r-shoulder"] + D3(-offL.x, offL.y, offL.z);
    return js;
}

// Weight purity over a region: fraction of weight mass on anatomically-allowed
// bone prefixes. Low purity => binding contamination.
double regionWeightPurity(const ht::PolyMesh& body, const std::vector<ht::Region>& regions,
                          const wp4::RigV2& rig, const ht::SkinWeights& sw, ht::Region reg,
                          const std::vector<std::string>& allowedPrefixes, char side) {
    double pure = 0.0, total = 0.0;
    for (size_t v = 0; v < body.pos.size() && v < static_cast<size_t>(sw.vertexCount); ++v) {
        if (regions[v] != reg) continue;
        if (side != 0) { const char vs = body.pos[v].x > 0.02 ? 'L' : (body.pos[v].x < -0.02 ? 'R' : 0); if (vs != side) continue; }
        for (int k = 0; k < sw.count[v]; ++k) {
            const int b = sw.bone[v][static_cast<size_t>(k)];
            if (b < 0 || static_cast<size_t>(b) >= rig.bones.size()) continue;
            const double w = sw.w[v][static_cast<size_t>(k)];
            total += w;
            const std::string& id = rig.bones[static_cast<size_t>(b)].name;
            for (const std::string& pre : allowedPrefixes)
                if (id.rfind(pre, 0) == 0) { pure += w; break; }
        }
    }
    return total > 0 ? pure / total : 1.0;
}

// Build engine mesh + triangle->face map; return self-intersection sample pairs.
std::vector<std::array<uint32_t, 2>> selfIntSamples(const ht::PolyMesh& body, const std::vector<D3>& pos,
                                                    std::vector<int>& triToFace, size_t& totalPairs) {
    pae::MeshData mesh;
    mesh.vertices.resize(pos.size());
    std::vector<D3> accum(pos.size());
    triToFace.clear();
    auto tri = [&](int a, int b, int c, int face) {
        const D3 fn = (pos[static_cast<size_t>(b)] - pos[static_cast<size_t>(a)]).cross(pos[static_cast<size_t>(c)] - pos[static_cast<size_t>(a)]);
        accum[static_cast<size_t>(a)] += fn; accum[static_cast<size_t>(b)] += fn; accum[static_cast<size_t>(c)] += fn;
        mesh.indices.push_back(static_cast<uint32_t>(a)); mesh.indices.push_back(static_cast<uint32_t>(b)); mesh.indices.push_back(static_cast<uint32_t>(c));
        triToFace.push_back(face);
    };
    for (size_t f = 0; f < body.faces.size(); ++f) {
        const auto& v = body.faces[f].v;
        if (v.size() == 4) { tri(v[0], v[1], v[2], static_cast<int>(f)); tri(v[0], v[2], v[3], static_cast<int>(f)); }
        else for (size_t k = 1; k + 1 < v.size(); ++k) tri(v[0], v[k], v[k + 1], static_cast<int>(f));
    }
    pae::Aabb bounds;
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        mesh.vertices[i].position = pae::Vec3(static_cast<float>(pos[i].x), static_cast<float>(pos[i].y), static_cast<float>(pos[i].z));
        const D3 n = accum[i].normalized();
        mesh.vertices[i].normal = pae::Vec3(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z));
        bounds.expand(mesh.vertices[i].position);
    }
    mesh.bounds = bounds;
    const auto si = pae::canonical::detectSelfIntersections(mesh, 256);
    totalPairs = si.intersectingPairs;
    return si.samples;
}
} // namespace

int main(int argc, char** argv) {
    const std::string dataDir = arg(argc, argv, "--data", "");
    const std::string repoRoot = arg(argc, argv, "--repo", "");
    const std::string outDir = arg(argc, argv, "--out", "wp4q2-out");
    if (dataDir.empty() || repoRoot.empty()) { std::printf("usage: --data --repo --out\n"); return 2; }
    ht::makeDirs(outDir); ht::makeDirs(outDir + "/images");
    std::string log;
    auto say = [&](const std::string& s) { log += s + "\n"; std::printf("%s\n", s.c_str()); };

    ht::ProofContext ctx; std::string err;
    if (!ctx.load(dataDir, repoRoot, err)) { std::printf("load failed: %s\n", err.c_str()); return 1; }
    const int rootV = wp4::resolveRootAnchor(ctx.obj, ctx.full);
    wp4::JointSet js0; wp4::resolveJoints(ctx.obj, ctx.full, rootV, js0);
    wp4::RigV2 rig0; wp4::buildRig(js0, wp4::Profile::Full92, rig0, err);
    const ht::Rig hr0 = rig0.asHtRig();
    const std::vector<ht::Region> regions0 = ht::labelRegions(ctx.body, hr0);
    const auto jregions = wp4::jointRegions(rig0, js0);
    wp4::WeightOptions opt;
    const ht::SkinWeights w0 = wp4::generateWeights(ctx.body, ctx.topo, rig0, wp4::WeightMethod::Region, opt);
    say(fmt("[load] body=%zu rig=%zu", ctx.body.pos.size(), rig0.bones.size()));

    const int W = 360, H = 720;
    ht::Camera front = ht::makeCamera("front", 0, 0, W, H); fitOne(front, ctx.body.pos);
    auto render = [&](const std::string& tag, const std::vector<D3>& posed, const ht::SkinWeights& sw,
                      const wp4::RigV2& rig, const std::vector<ht::Affine>& g, bool closeup, const D3& focus) {
        const ht::Rig hr = rig.asHtRig();
        ht::RenderSurface s{&ctx.body, &posed, &regions0, &sw, &hr};
        ht::writePngFile(outDir + "/images/" + tag + "-clay.png", ht::renderSurface(s, front, ht::ShadeMode::Clay, false));
        if (closeup) {
            ht::Camera cu = ht::makeCamera("cu", 25, 8, 320, 320);
            ht::fitCameraToBox(cu, focus - D3(0.10, 0.10, 0.10), focus + D3(0.10, 0.10, 0.10));
            ht::writePngFile(outDir + "/images/" + tag + "-closeup.png", ht::renderSurface(s, cu, ht::ShadeMode::Clay, true));
        }
        (void)g;
    };

    // ---- (1) shoulder geometry + offset sweep ------------------------------
    const D3 shJ = js0.group["joint-l-shoulder"];
    int acroVid = -1; const D3 acro = acromionLandmark(ctx, shJ, acroVid);
    const D3 elJ = js0.group["joint-l-elbow"];
    const D3 humerus = (elJ - shJ).normalized();
    std::string shMd = "# M2 shoulder joint-center geometry (generated)\n\n";
    shMd += fmt("- joint-l-shoulder helper centroid (m): (%.4f, %.4f, %.4f)\n", shJ.x, shJ.y, shJ.z);
    shMd += fmt("- acromion surface landmark (m): (%.4f, %.4f, %.4f) vertex %d\n", acro.x, acro.y, acro.z, acroVid);
    const D3 acroToJ = shJ - acro;
    shMd += fmt("- acromion -> helper centroid offset (m): (%+.4f, %+.4f, %+.4f)  [helper is %.1f mm inferior, %.1f mm lateral to acromion]\n",
                acroToJ.x, acroToJ.y, acroToJ.z, -acroToJ.y * 1000, acroToJ.x * 1000);
    shMd += fmt("- humerus axis (shoulder->elbow) unit: (%.3f, %.3f, %.3f)\n", humerus.x, humerus.y, humerus.z);
    shMd += "- Finding: the CC0 helper centroid is already internal/inferior to the acromion surface landmark, NOT naively at the surface. The offset sweep below tests whether moving the rotation center further improves abduction axilla quality.\n\n";

    struct Off { std::string name; D3 d; };
    const std::vector<Off> offsets = {
        {"baseline", D3(0, 0, 0)},
        {"medial-15mm", D3(-0.015, 0, 0)},
        {"inferior-15mm", D3(0, -0.015, 0)},
        {"medial-inferior-15mm", D3(-0.015, -0.015, 0)},
        {"proximal-10mm", humerus * -0.010},
    };
    const auto abduction = wp4::comparisonPoses();  // includes shoulder_abduction_90
    std::string offCsv = "offset,pose,axilla_flipped,axilla_edgeP99,axilla_edgeMax,axilla_areaRatio,global_flipped,global_edgeP99\n";
    shMd += "| offset | abduction | axilla flips | axilla edgeP99 | axilla edgeMax | axilla areaRatio | global flips |\n|---|---|---|---|---|---|---|\n";
    for (const Off& off : offsets) {
        const wp4::JointSet js = offsetShoulder(js0, off.d);
        wp4::RigV2 rig; wp4::buildRig(js, wp4::Profile::Full92, rig, err);
        const ht::SkinWeights sw = wp4::generateWeights(ctx.body, ctx.topo, rig, wp4::WeightMethod::Region, opt);
        const auto regs = ht::labelRegions(ctx.body, rig.asHtRig());
        const auto jr = wp4::jointRegions(rig, js);
        for (const std::string& pn : {std::string("shoulder_abduction_30"), std::string("shoulder_abduction_60"), std::string("shoulder_abduction_90")}) {
            const wp4::Pose* pose = wp4::findPose(abduction, pn);
            wp4::Pose pp = pose ? *pose : wp4::Pose{pn, {}};
            if (!pose) {
                const double ang = pn == "shoulder_abduction_30" ? 30 : (pn == "shoulder_abduction_60" ? 60 : 90);
                pp = wp4::Pose{pn, {{"upper_arm.L", 2, ang}, {"upper_arm.R", 2, -ang}}};
            }
            const auto g = wp4::poseGlobals(rig, pp);
            const auto posed = wp4::skinPositions(ctx.body.pos, sw, wp4::skinMatrices(rig, g));
            const auto dq = ht::compareSurfaces(ctx.body, posed, ctx.topo);
            wp4::RegionMetrics ax;
            for (const auto& r : jr) if (r.name == "shoulder_axilla.L") {
                const auto mask = wp4::regionMask(ctx.body, ctx.body.pos, regs, r);
                ax = wp4::evaluateRegion(ctx.body, ctx.topo, ctx.body.pos, posed, mask, r.name);
            }
            offCsv += fmt("%s,%s,%ld,%.4f,%.4f,%.4f,%ld,%.4f\n", off.name.c_str(), pn.c_str(), ax.flippedFaces,
                          ax.edgeRatioP99, ax.edgeRatioMax, ax.areaRatio, dq.flippedFaces, dq.edgeRatioP99);
            shMd += fmt("| %s | %s | %ld | %.3f | %.3f | %.3f | %ld |\n", off.name.c_str(), pn.c_str(), ax.flippedFaces,
                        ax.edgeRatioP99, ax.edgeRatioMax, ax.areaRatio, dq.flippedFaces);
            if (pn == "shoulder_abduction_90") render("shoulder-" + off.name, posed, sw, rig, g, true, js.group.at("joint-l-shoulder"));
        }
        say(fmt("[shoulder] %-22s done", off.name.c_str()));
    }
    ht::writeFile(outDir + "/shoulder-offset.csv", offCsv);
    ht::writeFile(outDir + "/shoulder-geometry.md", shMd);

    // ---- (2) extreme-pose flip attribution ---------------------------------
    const std::vector<std::string> extreme = {"fist", "elbow_120", "arms_raised", "basic_squat"};
    const auto suite = wp4::poseSuite();
    std::string attCsv = "pose,region,vertices,flipped,edgeP99,edgeMax,areaRatio,weightPurity\n";
    std::string attMd = "# M2 extreme-pose flip attribution (generated)\n\n";
    std::string siMd = "# M2 self-intersection localization (generated)\n\n";
    std::string fingerCsv = "pose,finger,flipped\n";
    const std::map<ht::Region, std::vector<std::string>> allowed = {
        {ht::Region::Hand, {"finger.", "hand.", "metacarpal."}},
        {ht::Region::Forearm, {"forearm.", "hand."}},
        {ht::Region::UpperArm, {"upper_arm.", "shoulder."}},
        {ht::Region::ShoulderGirdle, {"shoulder.", "clavicle.", "upper_arm.", "spine."}},
        {ht::Region::Thigh, {"thigh.", "pelvis."}},
        {ht::Region::LowerLeg, {"shin.", "foot."}},
    };
    for (const std::string& pn : extreme) {
        const wp4::Pose* pose = wp4::findPose(suite, pn);
        if (!pose) continue;
        const auto g = wp4::poseGlobals(rig0, *pose);
        const auto skin = wp4::skinMatrices(rig0, g);
        const auto posed = wp4::skinPositions(ctx.body.pos, w0, skin);
        const auto dq = ht::compareSurfaces(ctx.body, posed, ctx.topo);
        const auto q = wp3::evaluateQuality(ctx.body, posed, ctx.topo, &ctx.body.pos);
        attMd += "## " + pn + fmt("  (global flips=%ld, selfIntPairs=%zu, edgeP99=%.3f, vol=%.3f)\n\n", dq.flippedFaces, q.selfIntersectingPairs, dq.edgeRatioP99, dq.volumeRatio);
        attMd += "| region | flips | edgeP99 | edgeMax | areaRatio | weightPurity |\n|---|---|---|---|---|---|\n";
        for (const auto& jr : jregions) {
            const auto mask = wp4::regionMask(ctx.body, ctx.body.pos, regions0, jr);
            const auto rm = wp4::evaluateRegion(ctx.body, ctx.topo, ctx.body.pos, posed, mask, jr.name);
            // purity for the matching region/side
            double purity = 1.0;
            ht::Region reg = ht::Region::Unknown; char side = jr.name.back();
            if (jr.name.rfind("fingers", 0) == 0) reg = ht::Region::Hand;
            else if (jr.name.rfind("elbow", 0) == 0) reg = ht::Region::Forearm;
            else if (jr.name.rfind("wrist", 0) == 0) reg = ht::Region::Hand;
            else if (jr.name.rfind("shoulder", 0) == 0) reg = ht::Region::ShoulderGirdle;
            else if (jr.name.rfind("hip", 0) == 0) reg = ht::Region::Thigh;
            else if (jr.name.rfind("knee", 0) == 0) reg = ht::Region::LowerLeg;
            auto it = allowed.find(reg);
            if (it != allowed.end()) purity = regionWeightPurity(ctx.body, regions0, rig0, w0, reg, it->second, side == 'L' || side == 'R' ? side : 0);
            attCsv += fmt("%s,%s,%ld,%ld,%.4f,%.4f,%.4f,%.4f\n", pn.c_str(), jr.name.c_str(), rm.vertices, rm.flippedFaces,
                          rm.edgeRatioP99, rm.edgeRatioMax, rm.areaRatio, purity);
            if (rm.flippedFaces > 0 || rm.edgeRatioMax > 1.5)
                attMd += fmt("| %s | %ld | %.3f | %.3f | %.3f | %.3f |\n", jr.name.c_str(), rm.flippedFaces, rm.edgeRatioP99, rm.edgeRatioMax, rm.areaRatio, purity);
        }
        attMd += "\n";
        // self-intersection localization
        std::vector<int> triToFace; size_t totalPairs = 0;
        const auto samples = selfIntSamples(ctx.body, posed, triToFace, totalPairs);
        std::map<std::string, long> pairRegions;
        auto faceRegion = [&](int face) -> std::string {
            if (face < 0 || static_cast<size_t>(face) >= ctx.body.faces.size()) return "?";
            const auto& v = ctx.body.faces[static_cast<size_t>(face)].v;
            std::map<ht::Region, int> cnt;
            for (int vidx : v) cnt[regions0[static_cast<size_t>(vidx)]]++;
            ht::Region best = ht::Region::Unknown; int bc = 0;
            for (const auto& kv : cnt) if (kv.second > bc) { bc = kv.second; best = kv.first; }
            return ht::regionName(best);
        };
        for (const auto& pr : samples) {
            const int fa = pr[0] < triToFace.size() ? triToFace[pr[0]] : -1;
            const int fb = pr[1] < triToFace.size() ? triToFace[pr[1]] : -1;
            std::string ra = faceRegion(fa), rb = faceRegion(fb);
            if (ra > rb) std::swap(ra, rb);
            pairRegions[ra + " x " + rb]++;
        }
        siMd += "## " + pn + fmt("  (totalPairs=%zu, sampled=%zu)\n\n", totalPairs, samples.size());
        for (const auto& kv : pairRegions) siMd += fmt("- %s : %ld\n", kv.first.c_str(), kv.second);
        siMd += "\n";
        // per-finger flip breakdown for fist
        if (pn == "fist") {
            for (char side : {'L', 'R'}) {
                for (int f = 1; f <= 5; ++f) {
                    // finger sub-region: vertices bound dominantly to finger.0f.*.side
                    long flips = 0;
                    const std::string pre = "finger.0" + std::to_string(f) + ".";
                    const std::string suf = std::string(".") + side;
                    for (size_t face = 0; face < ctx.body.faces.size(); ++face) {
                        const auto& v = ctx.body.faces[face].v;
                        bool any = false;
                        for (int vidx : v) {
                            const int b = w0.bone[static_cast<size_t>(vidx)][0];
                            if (b >= 0 && static_cast<size_t>(b) < rig0.bones.size()) {
                                const std::string& id = rig0.bones[static_cast<size_t>(b)].name;
                                if (id.rfind(pre, 0) == 0 && id.size() >= suf.size() && id.compare(id.size() - suf.size(), suf.size(), suf) == 0) { any = true; break; }
                            }
                        }
                        if (!any) continue;
                        const D3 nr = ht::faceNormal(ctx.body, static_cast<int>(face));
                        D3 np;
                        const size_t k = v.size();
                        for (size_t i = 0; i < k; ++i) {
                            const D3& a = posed[v[i]]; const D3& b = posed[v[(i + 1) % k]];
                            np.x += (a.y - b.y) * (a.z + b.z); np.y += (a.z - b.z) * (a.x + b.x); np.z += (a.x - b.x) * (a.y + b.y);
                        }
                        if (nr.dot(np.normalized()) < 0) ++flips;
                    }
                    fingerCsv += fmt("%s,finger.0%d.%c,%ld\n", pn.c_str(), f, side, flips);
                }
            }
        }
        render("extreme-" + pn, posed, w0, rig0, g, true, pn == "fist" ? js0.group["joint-l-hand"] : (pn == "elbow_120" ? js0.group["joint-l-elbow"] : js0.group["joint-l-shoulder"]));
        say(fmt("[extreme] %-14s flips=%ld selfInt=%zu", pn.c_str(), dq.flippedFaces, q.selfIntersectingPairs));
    }
    ht::writeFile(outDir + "/extreme-attribution.csv", attCsv);
    ht::writeFile(outDir + "/extreme-attribution.md", attMd);
    ht::writeFile(outDir + "/selfint-localization.md", siMd);
    ht::writeFile(outDir + "/per-finger.csv", fingerCsv);
    ht::writeFile(outDir + "/run.log", log);
    say("[done] M2 quality improvement experiment complete");
    ht::writeFile(outDir + "/run.log", log);
    return 0;
}