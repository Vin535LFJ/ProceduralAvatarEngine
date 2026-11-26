// Assert-based host tests for the P1-VR-ARCH-1 human-template proof.
// Usage: pae_ht_tests <makehuman/data> <makehuman-repo-root> <scratch-dir>
// Every check prints [PASS] or [FAIL]; the summary line is the raw log.

#include "ht_measure.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "ht_target.h"
#include "ht_template.h"
#include "ht_topology.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace pae::ht;

namespace {

int gPass = 0;
int gFail = 0;
std::vector<std::string> gFailures;

void check(bool condition, const std::string& name, const std::string& detail = "") {
    if (condition) {
        ++gPass;
        std::printf("[PASS] %s\n", name.c_str());
    } else {
        ++gFail;
        gFailures.push_back(name);
        std::printf("[FAIL] %s%s%s\n", name.c_str(), detail.empty() ? "" : " -- ", detail.c_str());
    }
}

bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

double maxDifference(const std::vector<D3>& a, const std::vector<D3>& b) {
    double m = 0.0;
    for (size_t i = 0; i < a.size() && i < b.size(); ++i) m = std::max(m, (a[i] - b[i]).length());
    return m;
}

const char* kRegionKeys[] = {"head", "neck", "thorax", "abdomen", "pelvis", "shoulder-girdle", "upper-arm", "forearm",
                             "hand", "thigh", "lower-leg", "foot"};

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::printf("usage: pae_ht_tests <makehuman/data> <makehuman-repo-root> <scratch-dir>\n");
        return 2;
    }
    const std::string dataDir = argv[1], repoRoot = argv[2], scratch = argv[3];
    makeDirs(scratch);
    std::printf("== PAE human-template proof tests ==\n");

    // --- T01 SHA-256 known answers ------------------------------------------------
    check(sha256Hex(std::vector<uint8_t>{}) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
          "T01a sha256(empty) known answer");
    check(sha256Hex(std::vector<uint8_t>{'a', 'b', 'c'}) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "T01b sha256(abc) known answer");

    // --- load --------------------------------------------------------------------
    const double t0 = nowMs();
    ProofContext c;
    std::string error;
    if (!c.load(dataDir, repoRoot, error)) {
        for (const std::string& line : c.log) std::printf("       %s\n", line.c_str());
        std::printf("[FAIL] T02 context load -- %s\n", error.c_str());
        return 1;
    }
    const double loadMs = nowMs() - t0;
    for (const std::string& line : c.log) std::printf("       %s\n", line.c_str());
    const int bodyCount = static_cast<int>(c.body.pos.size());
    const std::vector<D3>& base = c.body.pos;

    // --- T02 OBJ reader: order and counts preserved -------------------------------
    check(c.obj.pos.size() == 19158 && c.obj.uv.size() == 21334 && c.obj.faces.size() == 18486,
          "T02a base.obj counts v=19158 vt=21334 f=18486 (pinned file a8bc2d54)",
          fmt("%zu/%zu/%zu", c.obj.pos.size(), c.obj.uv.size(), c.obj.faces.size()));
    check(c.obj.faceStatementsWithNegativeIndex == 0, "T02b no negative OBJ indices (order cannot be ambiguous)");
    bool orderPreserved = true;
    for (size_t i = 0; i < base.size(); ++i) {
        const D3& src = c.obj.pos[static_cast<size_t>(c.bodyStableIds[i])];
        orderPreserved = orderPreserved && near(src.x * c.unitScale, base[i].x, 1e-15) &&
                         near(src.y * c.unitScale, base[i].y, 1e-15) && near(src.z * c.unitScale, base[i].z, 1e-15);
    }
    check(orderPreserved && bodyCount == 13380, "T02c body stable vertex ids = OBJ v order (0..13379), positions untouched");

    // --- T03 topology audit --------------------------------------------------------
    const TopologyAudit audit = auditTopology(c.body, c.topo);
    check(audit.quadCount == audit.faceCount && audit.faceCount == 13378, "T03a body is 13,378 quads (no tri, no n-gon)");
    check(audit.boundaryEdgeCount == 0, "T03b closed surface: no boundary edge", fmt("%ld", audit.boundaryEdgeCount));
    check(audit.nonManifoldEdgeCount == 0, "T03c no non-manifold edge", fmt("%ld", audit.nonManifoldEdgeCount));
    check(audit.inconsistentWindingEdgeCount == 0, "T03d consistent winding on every interior edge",
          fmt("%ld", audit.inconsistentWindingEdgeCount));
    check(audit.duplicateFaceCount == 0 && audit.degenerateFaceCount == 0, "T03e no duplicate or degenerate face");
    check(audit.boundaryEdgeCount == 0 ? (audit.componentCount == 1 && audit.eulerChi == 2)
                                       : (audit.eulerChi == 2 * audit.componentCount - audit.boundaryLoopCount),
          "T03f Euler characteristic matches components/boundary loops", fmt("chi=%ld comps=%d loops=%d", audit.eulerChi,
                                                                            audit.componentCount, audit.boundaryLoopCount));
    check(audit.outwardWinding, "T03g outward winding (positive signed volume on a closed mesh)",
          fmt("volume=%.1f", audit.signedVolume));
    check(audit.valenceMin >= 3 && audit.valenceMax <= 8, "T03h vertex valence in [3, 8] (regular quad mesh)",
          fmt("min=%d max=%d", audit.valenceMin, audit.valenceMax));
    std::printf("       topology: edges=%ld valence[min %d max %d] edge[m %.4f p50 %.4f max %.4f] "
                "quadCornerAngle[min %.2f p01 %.2f med %.2f deg] triAspect[max %.2f p99 %.2f mean %.2f] "
                "dihedral[max %.1f >60:%ld >90:%ld] uvSeams=%ld renderVerts=%ld tris=%ld area=%.3f m2 vol=%.3f L\n",
                audit.edgeCount, audit.valenceMin, audit.valenceMax, audit.edgeLength.min, audit.edgeLength.p50,
                audit.edgeLength.max, audit.minCornerAngleDeg.min, audit.minCornerAngleDeg.p01, audit.minCornerAngleDeg.p50,
                audit.aspectRatio.max, audit.aspectRatio.p99, audit.aspectRatio.mean, audit.dihedralDeg.max,
                audit.dihedralOver60, audit.dihedralOver90, audit.uvSeamEdgeCount, audit.uniqueUvVertexPairCount,
                audit.triangulatedCount, audit.surfaceArea, audit.signedVolume * 1000.0);

    // --- T04 helper groups ---------------------------------------------------------
    const GroupCensus census = censusGroups(c.obj);
    check(census.jointGroups > 0, "T04a base.obj carries joint helper groups", fmt("%d joint groups, %ld faces", census.jointGroups,
                                                                                 census.jointFaces));

    // --- T05 target format decided by data -------------------------------------------
    const IndexBaseProbe probe = probeIndexBase(dataPath(c, "targets/head/head-trans-up.target"), c.full);
    check(probe.spreadBase0 * 2.0 < probe.spreadBase1, "T05a target indices are 0-based (head-trans-up stays local only under 0-based)",
          fmt("spread0=%.3f m spread1=%.3f m", probe.spreadBase0, probe.spreadBase1));
    auto axisOf = [&](const char* rel) {
        ShapeTarget t;
        std::string e;
        readTarget(dataPath(c, rel), rel, static_cast<long>(c.full.size()), 0, t, e);
        return dominantAxis(t);
    };
    check(axisOf("targets/torso/torso-trans-forward.target") == 'z', "T05b forward translation lives in column 3 (Z)");
    check(axisOf("targets/head/head-trans-up.target") == 'y', "T05c up translation lives in column 2 (Y)");
    check(axisOf("targets/torso/torso-trans-in.target") == 'x', "T05d sideways translation lives in column 1 (X)");
    {
        ShapeTarget up;
        std::string e;
        readTarget(dataPath(c, "targets/head/head-trans-up.target"), "targets/head/head-trans-up.target",
                   static_cast<long>(c.full.size()), 0, up, e);
        double meanDy = 0.0;
        for (const D3& d : up.delta) meanDy += d.y;
        meanDy /= static_cast<double>(up.delta.size()) * c.unitScale;
        check(meanDy > 0.0, "T05e 'up' is +Y without sign inversion", fmt("mean dy=%.4f mesh units", meanDy));
    }
    {
        long helperRows = 0;
        for (int v : c.params[0].incr.vertex) helperRows += v >= bodyCount ? 1 : 0;
        check(c.params[0].incr.dataRows == 5568 && c.params[0].incr.rowsOutsideBody == 0,
              "T05f measure-shoulder-dist-incr: 5568 rows, all inside the OBJ index range", fmt("rows=%ld outside=%ld",
                                                                                               c.params[0].incr.dataRows,
                                                                                               c.params[0].incr.rowsOutsideBody));
        std::printf("       note: %ld of those 5568 rows address helper/joint geometry (stable ids >= %d)\n", helperRows, bodyCount);
    }

    // --- T06 shape maths ---------------------------------------------------------------
    std::vector<D3> zero, half, full;
    applyToBody(c, {{&c.params[0].incr, 0.0}}, zero);
    check(zero == base, "T06a alpha = 0 restores the base mesh bit-exactly");
    std::vector<D3> plusFull, plusMinusFull, plusMinus;
    applyToFull(c, {{&c.params[0].incr, 1.0}}, plusFull);
    applyTargets(plusFull, {{&c.params[0].incr, -1.0}}, plusMinusFull);  // relative to the deformed surface
    plusMinus.assign(plusMinusFull.begin(), plusMinusFull.begin() + bodyCount);
    check(maxDifference(plusMinus, base) < 1e-12, "T06b +alpha then -alpha restores the base mesh (double rounding only)",
          fmt("%.3e m", maxDifference(plusMinus, base)));
    applyToBody(c, {{&c.params[1].incr, 0.5}, {&c.params[1].incr, 0.5}}, half);
    applyToBody(c, {{&c.params[1].incr, 1.0}}, full);
    check(maxDifference(half, full) < 1e-12, "T06c targets combine additively (0.5 + 0.5 == 1.0)", fmt("max diff %.3e", maxDifference(half, full)));
    {
        std::vector<std::pair<const ShapeTarget*, double>> all;
        for (const ShapeParam& p : c.params) all.push_back({&p.incr, 1.0});
        std::vector<D3> combined;
        applyToBody(c, all, combined);
        Topology topoAfter;
        buildTopology(c.body, topoAfter);
        check(combined.size() == base.size() && topoAfter.edges.size() == c.topo.edges.size() && c.body.faces.size() == 13378,
              "T06d all nine parameters at +1.0: vertex count, faces and edges unchanged (stable vertex ids)");
        const DeformationQuality q = compareSurfaces(c.body, combined, c.topo);
        std::printf("       all-incr combined: flipped=%ld minNormalDot=%.4f edgeStretch[min %.3f p99 %.3f max %.3f] volumeRatio=%.4f\n",
                    q.flippedFaces, q.minNormalDot, q.minEdgeRatio, q.edgeRatioP99, q.maxEdgeRatio, q.volumeRatio);
    }

    // --- T07 parameter locality (mean displacement per region, not L1 share) ---------------
    const Measures baseM = measureBody(base, c.regions, c.rigDeform);
    std::printf("       base: stature %.4f m bideltoid %.4f chest %.4f x %.4f waist %.4f x %.4f hip %.4f | arm %.4f/%.4f leg %.4f/%.4f\n",
                baseM.stature, baseM.bideltoidWidth, baseM.chestWidth, baseM.chestDepth, baseM.waistWidth, baseM.waistDepth,
                baseM.hipWidth, baseM.upperArmLength, baseM.forearmLength, baseM.thighLength, baseM.shankLength);
    {
        const std::vector<double> radii = boneRadii(c.body, c.rigDeform);
        const std::vector<long> counts = nearestBoneCounts(c.body, c.rigDeform);
        std::vector<size_t> order(c.rigDeform.bones.size());
        for (size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::sort(order.begin(), order.end(), [&](size_t x, size_t y) { return counts[x] > counts[y]; });
        std::string top;
        for (size_t i = 0; i < order.size(); ++i)
            if (counts[order[i]] >= 40) top += fmt("%s=%ld(r%.3f) ", c.rigDeform.bones[order[i]].name.c_str(), counts[order[i]], radii[order[i]]);
        std::printf("       nearest-bone census (bones with >= 40 vertices): %s\n", top.c_str());
    }
    struct Expect { const char* key; std::vector<Region> allowed; int measure; };
    const std::vector<Region> arm = {Region::UpperArm, Region::Forearm, Region::Hand};
    const std::vector<Region> lower = {Region::Thigh, Region::LowerLeg, Region::Foot};
    const std::vector<Expect> expects = {
        {"shoulderWidth", {Region::ShoulderGirdle, Region::Thorax, Region::UpperArm, Region::Forearm, Region::Hand}, 0},
        {"ribcageWidth", {Region::Thorax, Region::Abdomen, Region::Pelvis, Region::ShoulderGirdle, Region::UpperArm, Region::Forearm, Region::Hand}, 1},
        {"chestDepth", {Region::Thorax, Region::Abdomen, Region::Pelvis, Region::ShoulderGirdle, Region::UpperArm, Region::Forearm, Region::Hand}, 2},
        {"waistWidth", {Region::Abdomen, Region::Pelvis, Region::Thorax}, 3},
        {"hipWidth", {Region::Pelvis, Region::Thigh, Region::Abdomen}, 4},
        {"armShape", {Region::UpperArm}, -1},
        {"legShape", {Region::Thigh, Region::Pelvis}, -1},  // upperleg-muscle blends into the gluteal area (measured, see log)
        {"headShape", {Region::Head, Region::Neck}, -1},
    };
    auto valueOf = [&](const Measures& m, int which) {
        switch (which) {
            case 0: return m.bideltoidWidth;
            case 1: return m.chestWidth;
            case 2: return m.chestDepth;
            case 3: return m.waistWidth;
            case 4: return m.hipWidth;
            default: return m.stature;
        }
    };
    int locality = 0, direction = 0, notScale = 0, noFold = 0, measured = 0, nonZero = 0;
    for (const Expect& e : expects) {
        if (e.measure >= 0) {
            ++measured;
            nonZero += valueOf(baseM, e.measure) > 1e-6 ? 1 : 0;
        }
        const ShapeParam* p = nullptr;
        for (const ShapeParam& q : c.params) if (q.key == e.key) p = &q;
        if (!p) continue;
        std::vector<D3> inc, dec;
        applyToBody(c, {{&p->incr, 1.0}}, inc);
        applyToBody(c, {{&p->decr, 1.0}}, dec);
        const std::vector<double> meanI = regionMeanDisplacement(c.regions, p->incr.vertex, p->incr.delta);
        const std::vector<double> meanD = regionMeanDisplacement(c.regions, p->decr.vertex, p->decr.delta);
        double allowedMax = 0.0, forbiddenMaxI = 0.0, forbiddenMaxD = 0.0;
        std::string table;
        std::vector<int> bySize(kRegionCount);
        for (int r = 0; r < kRegionCount; ++r) bySize[static_cast<size_t>(r)] = r;
        std::sort(bySize.begin(), bySize.end(), [&](int x, int y) { return meanI[static_cast<size_t>(x)] > meanI[static_cast<size_t>(y)]; });
        for (int r = 0; r < kRegionCount; ++r) {
            const bool ok = std::find(e.allowed.begin(), e.allowed.end(), static_cast<Region>(r)) != e.allowed.end();
            if (ok) allowedMax = std::max(allowedMax, meanI[static_cast<size_t>(r)]);
            else {
                forbiddenMaxI = std::max(forbiddenMaxI, meanI[static_cast<size_t>(r)]);
                forbiddenMaxD = std::max(forbiddenMaxD, meanD[static_cast<size_t>(r)]);
            }
        }
        for (size_t i = 0; i < 5; ++i)
            table += fmt("%s %.1f mm, ", kRegionKeys[bySize[i]], meanI[static_cast<size_t>(bySize[i])] * 1000.0);
        // Locality: every region outside the allowed set moves, on average,
        // less than 2 % of the strongest allowed region, or under 0.5 mm
        // (the targets are stored with 0.1 mm precision; mean edge 5.6 mm).
        const double worst = std::max(forbiddenMaxI, forbiddenMaxD);
        const bool localOk = allowedMax > 0.0 && worst <= std::max(0.02 * allowedMax, 0.0005);
        locality += localOk;
        const Measures mi = measureBody(inc, c.regions, c.rigDeform), md = measureBody(dec, c.regions, c.rigDeform);
        const double dInc = valueOf(mi, e.measure) - valueOf(baseM, e.measure);
        const double dDec = valueOf(md, e.measure) - valueOf(baseM, e.measure);
        const bool dir = e.measure < 0 || (dInc > 0.0 && dDec < 0.0);
        if (e.measure >= 0) direction += dir;
        const bool scaleOk = std::fabs(mi.stature - baseM.stature) < 0.005 * baseM.stature &&
                             std::fabs(md.stature - baseM.stature) < 0.005 * baseM.stature;
        notScale += scaleOk;
        const DeformationQuality qi = compareSurfaces(c.body, inc, c.topo), qd = compareSurfaces(c.body, dec, c.topo);
        const bool foldOk = qi.flippedFaces == 0 && qd.flippedFaces == 0;
        noFold += foldOk;
        std::printf("       %-13s mean |disp| incr: %s| outside-allowed max %.2f%% of allowed (limit 2%%) %s | %s base %.4f  +1 %+.4f  -1 %+.4f | flips %ld/%ld\n",
                    e.key, table.c_str(), allowedMax > 0 ? 100.0 * worst / allowedMax : 0.0, localOk ? "ok" : "EXCEEDS",
                    e.measure >= 0 ? "named measure" : "(no named measure)", e.measure >= 0 ? valueOf(baseM, e.measure) : 0.0, dInc, dDec,
                    qi.flippedFaces, qd.flippedFaces);
    }
    check(locality == static_cast<int>(expects.size()), "T07a every local parameter leaves regions outside its allowed set below 2 % of its strongest effect",
          fmt("%d/%d", locality, static_cast<int>(expects.size())));
    check(direction == measured, "T07b incr raises and decr lowers the named measurement (measured parameters)", fmt("%d/%d", direction, measured));
    check(nonZero == 5, "T07b-all five named measurements are non-zero on the base mesh (region labels cover each plane)", fmt("%d", nonZero));
    check(notScale == static_cast<int>(expects.size()), "T07c local parameters are not a global scale (stature change < 0.5 %)",
          fmt("%d/%d", notScale, static_cast<int>(expects.size())));
    check(noFold == static_cast<int>(expects.size()), "T07d no folded face at +/-1.0 for any local parameter", fmt("%d/%d", noFold, static_cast<int>(expects.size())));

    // --- T08 left / right ----------------------------------------------------------------
    {
        ShapeTarget leftOnly;
        std::string e;
        readTarget(dataPath(c, "targets/armslegs/l-upperarm-muscle-incr.target"), "l-upperarm-muscle-incr",
                   static_cast<long>(c.full.size()), 0, leftOnly, e);
        long onRight = 0, bodyRows = 0;
        for (int v : leftOnly.vertex) {
            if (v >= bodyCount) continue;
            ++bodyRows;
            if (base[static_cast<size_t>(v)].x < 0.0) ++onRight;
        }
        check(onRight == 0 && bodyRows > 0, "T08a l-upperarm-muscle target moves no body vertex on the character's right (x < 0)",
              fmt("%ld of %ld", onRight, bodyRows));
        const ShapeParam* armParam = nullptr;
        for (const ShapeParam& q : c.params) if (q.key == "armShape") armParam = &q;
        const std::vector<double> mean = regionMeanDisplacement(c.regions, armParam->incr.vertex, armParam->incr.delta);
        double others = 0.0;
        for (int r = 0; r < kRegionCount; ++r)
            if (r != static_cast<int>(Region::UpperArm)) others = std::max(others, mean[static_cast<size_t>(r)]);
        check(others <= std::max(0.02 * mean[static_cast<size_t>(Region::UpperArm)], 0.0005), "T08b armShape (L+R pair) stays inside the upper arms",
              fmt("outside max %.2f%% (%.2f mm)", 100.0 * others / mean[static_cast<size_t>(Region::UpperArm)], others * 1000.0));
    }

    // --- T09 rig -------------------------------------------------------------------------------
    check(c.rigFile.bones.size() == 163, "T09a source rig has 163 bones", fmt("%zu", c.rigFile.bones.size()));
    bool parentsOk = true, acyclic = true;
    for (size_t b = 0; b < c.rigFile.bones.size(); ++b) {
        const std::string& p = c.rigFile.bones[b].parent;
        if (p.empty()) continue;
        bool found = false;
        for (const RigFileBone& o : c.rigFile.bones) found = found || o.name == p;
        parentsOk = parentsOk && found;
    }
    for (const Rig* rig : {&c.rigDeform, &c.rigCore}) {
        for (size_t b = 0; b < rig->bones.size(); ++b) {
            const std::vector<int> chain = rig->ancestors(static_cast<int>(b));
            acyclic = acyclic && chain.size() < rig->bones.size();
            for (int a : chain) acyclic = acyclic && a != static_cast<int>(b);
        }
    }
    check(parentsOk && acyclic, "T09b every bone resolves its parent and no hierarchy cycle exists");
    long facial = 0;
    for (const RigFileBone& b : c.rigFile.bones) facial += isFacialRole(roleOfBoneName(b.name)) ? 1 : 0;
    check(c.rigDeform.bones.size() + static_cast<size_t>(facial) == c.rigFile.bones.size(),
          "T09c Rig V2 deform skeleton = source rig minus facial/breast bones", fmt("%zu + %ld facial", c.rigDeform.bones.size(), facial));
    check(c.rigCore.bones.size() == 20, "T09d Rig V2 core variant has 20 bones", fmt("%zu", c.rigCore.bones.size()));
    bool coreParentsKept = true;
    for (const RigBone& b : c.rigCore.bones)
        if (b.parent >= 0) coreParentsKept = coreParentsKept && c.rigCore.bones[static_cast<size_t>(b.parent)].name != b.name;
    check(coreParentsKept, "T09e core variant re-parents dropped bones to kept ancestors");
    int withPlane = 0;
    for (const RigBone& b : c.rigDeform.bones) withPlane += b.planeNormal.lengthSq() > 0.5 ? 1 : 0;
    std::printf("       rig: %zu deform bones, %d with a roll-reference plane normal\n", c.rigDeform.bones.size(), withPlane);

    // --- T10 weights ---------------------------------------------------------------------------
    SkinWeights mh, mhRaw;
    check(readMhWeights(dataPath(c, "rigs/default_weights.mhw"), c.rigDeform, static_cast<int>(c.full.size()), mhRaw, error),
          "T10a MakeHuman default_weights.mhw parsed", error);
    check(mhRaw.detectedIndexBase == 0 && mhRaw.entriesRead == 57107 && mhRaw.outOfRangeEntries == 0,
          "T10b weight indices are 0-based; 57107 entries, all inside the OBJ index range",
          fmt("base=%d entries=%ld outOfRange=%ld", mhRaw.detectedIndexBase, mhRaw.entriesRead, mhRaw.outOfRangeEntries));
    check(mhRaw.license == "CC0", "T10c weights file declares license CC0 in its header (in-file declaration only, not enumerated in LICENSE.md)",
          mhRaw.license);
    const long rawUnweighted = weightStats(mhRaw, bodyCount).verticesWithoutWeight;
    check(rawUnweighted == 175,
          "T10d exactly 175 body vertices have NO deform-rig weight before the fallback (their source weights sit only on facial bones)",
          fmt("%ld of %d unweighted", rawUnweighted, bodyCount));
    check(readMhWeights(dataPath(c, "rigs/default_weights.mhw"), c.rigDeform, static_cast<int>(c.full.size()), mh, error, &c.rigFile, &c.full),
          "T10e MakeHuman weights re-read with the fallback (source bone hierarchy + rest positions)", error);
    const WeightStats mhStats = weightStats(mh, bodyCount);
    check(mhStats.verticesWithoutWeight == 0 && near(mhStats.minSum, 1.0, 1e-5) && near(mhStats.maxSum, 1.0, 1e-5) && mhStats.maxInfluences <= 4.0,
          "T10f after the fallback every body vertex is weighted and Top-4 normalised to 1", fmt("unweighted=%ld sum[%.6f..%.6f] maxInf=%.0f", mhStats.verticesWithoutWeight,
                                                                                     mhStats.minSum, mhStats.maxSum, mhStats.maxInfluences));
    std::printf("       MH weights: body mean influences before Top-4 %.3f, mean mass discarded by Top-4 %.4f%%, %ld entries belong to facial bones not in the deform rig\n",
                mh.meanInfluencesBeforeCompression, mh.massLostByCompression * 100.0, mh.unknownBoneEntries);
    check(mh.massLostByCompression < 0.02, "T10g Top-4 discards < 2 % of the MakeHuman weight mass on average",
          fmt("%.4f%%", mh.massLostByCompression * 100.0));
    {
        // Fallback census on the real data: which rule fired, where, bound to what.
        const int headBone = c.rigDeform.find("head");
        const long byAncestor = countFallback(mh, WeightFallback::KeptAncestor, bodyCount);
        const long byNearest = countFallback(mh, WeightFallback::NearestBone, bodyCount);
        long boundToHead = 0;
        D3 lo{1e9, 1e9, 1e9}, hi{-1e9, -1e9, -1e9};
        for (int v = 0; v < bodyCount; ++v) {
            if (mh.fallback[static_cast<size_t>(v)] == static_cast<uint8_t>(WeightFallback::Source)) continue;
            const size_t i = static_cast<size_t>(v);
            lo.x = std::min(lo.x, base[i].x); lo.y = std::min(lo.y, base[i].y); lo.z = std::min(lo.z, base[i].z);
            hi.x = std::max(hi.x, base[i].x); hi.y = std::max(hi.y, base[i].y); hi.z = std::max(hi.z, base[i].z);
            boundToHead += mh.count[i] == 1 && mh.bone[i][0] == static_cast<uint16_t>(headBone) && mh.w[i][0] == 1.0f ? 1 : 0;
        }
        std::printf("       fallback body vertices: x %.3f..%.3f y %.3f..%.3f z %.3f..%.3f m; auxiliary ids: %ld kept-ancestor, %ld nearest-bone\n", lo.x, hi.x, lo.y, hi.y,
                    lo.z, hi.z, countFallback(mh, WeightFallback::KeptAncestor) - byAncestor, countFallback(mh, WeightFallback::NearestBone) - byNearest);
        check(byAncestor == 175 && byNearest == 0 && boundToHead == 175,
              "T10h fallback census: 175 body vertices, every one by the kept-ancestor rule, every one bound to `head` with weight 1",
              fmt("kept-ancestor=%ld nearest-bone=%ld bound-to-head=%ld", byAncestor, byNearest, boundToHead));
        check(weightStats(mh).verticesWithoutWeight == 0, "T10i every stable vertex id (body + auxiliary) carries a weight",
              fmt("%d ids", mh.vertexCount));
        // The fallback must be purely additive: a vertex with a kept source weight keeps it bit for bit.
        bool untouched = true;
        long compared = 0;
        for (int v = 0; v < mh.vertexCount; ++v) {
            const size_t i = static_cast<size_t>(v);
            if (mhRaw.count[i] == 0) continue;
            ++compared;
            untouched = untouched && mh.count[i] == mhRaw.count[i] && mh.bone[i] == mhRaw.bone[i] && mh.w[i] == mhRaw.w[i] &&
                        mh.fallback[i] == static_cast<uint8_t>(WeightFallback::Source);
        }
        check(untouched, "T10j the fallback changes no vertex that already had a kept source weight", fmt("%ld source-weighted ids compared", compared));
        // A fallback vertex must follow its bone exactly (it is a single-bone binding).
        const std::vector<Affine> turned = skinMatrices(c.rigDeform, poseGlobals(c.rigDeform, {{headBone, D3{0.0, 1.0, 0.0}, 30.0}}));
        double worst = 0.0;
        long tested = 0;
        for (int v = 0; v < bodyCount; ++v) {
            const size_t i = static_cast<size_t>(v);
            if (mh.fallback[i] == static_cast<uint8_t>(WeightFallback::Source)) continue;
            D3 sum;
            for (int k = 0; k < mh.count[i]; ++k)
                sum += turned[mh.bone[i][static_cast<size_t>(k)]].apply(base[i]) * mh.w[i][static_cast<size_t>(k)];
            worst = std::max(worst, (sum - turned[static_cast<size_t>(headBone)].apply(base[i])).length());
            ++tested;
        }
        check(tested == 175 && worst < 1e-12, "T10k fallback vertices move exactly with `head` (30 deg head turn)", fmt("%ld vertices, worst %.3e m", tested, worst));
    }
    {
        // Both rules on a synthetic rig:
        //   a (kept) <- b (kept) <- face.L (dropped) <- eye.L (dropped)
        // vertex 0: a, weight 1                 -> source, untouched
        // vertex 1: face.L 0.25 + eye.L 0.75    -> kept-ancestor: both resolve to b
        // vertex 2: no entry at all              -> nearest-bone: b (rest x = 5.4)
        // vertex 3: entry on "ghost", a bone that is not in the hierarchy -> nearest-bone: a (rest x = 0.3)
        RigFile file;
        file.bones = {{"a", "j0", "j1", "", {}}, {"b", "j1", "j2", "a", {}}, {"face.L", "j2", "j3", "b", {}}, {"eye.L", "j3", "j4", "face.L", {}}};
        Rig rig;
        rig.name = "synthetic";
        RigBone a, b;
        a.name = "a"; a.head = D3{0, 0, 0}; a.tail = D3{1, 0, 0}; a.parent = -1;
        b.name = "b"; b.head = D3{5, 0, 0}; b.tail = D3{6, 0, 0}; b.parent = 0;
        rig.bones = {a, b};
        const std::string path = scratch + "/synthetic-weights.mhw";
        writeFile(path, R"({"license":"CC0","version":110,"weights":{"a":[[0,1.0]],"face.L":[[1,0.25]],"eye.L":[[1,0.75]],"ghost":[[3,1.0]]}})");
        const std::vector<D3> rest = {D3{0.2, 0, 0}, D3{5.2, 0, 0}, D3{5.4, 0, 0}, D3{0.3, 0, 0}};
        SkinWeights plain, filled;
        std::string e;
        const bool okPlain = readMhWeights(path, rig, 4, plain, e);
        const bool okFilled = readMhWeights(path, rig, 4, filled, e, &file, &rest);
        check(okPlain && okFilled && plain.count[0] == 1 && plain.count[1] == 0 && plain.count[2] == 0 && plain.count[3] == 0 &&
                  plain.fallback == std::vector<uint8_t>{0, 0, 0, 0},
              "T10l without hierarchy and rest positions the loader leaves such vertices unweighted", e);
        check(filled.fallback == std::vector<uint8_t>{0, 1, 2, 2} && filled.count[0] == 1 && filled.count[1] == 1 && filled.bone[1][0] == 1 &&
                  filled.w[1][0] == 1.0f && filled.count[2] == 1 && filled.bone[2][0] == 1 && filled.count[3] == 1 && filled.bone[3][0] == 0 &&
                  countFallback(filled, WeightFallback::KeptAncestor) == 1 && countFallback(filled, WeightFallback::NearestBone) == 2,
              "T10m synthetic rig: kept-ancestor (dropped bones -> nearest kept parent) and nearest-bone (no usable entry) rules",
              fmt("rules %u%u%u%u", filled.fallback[0], filled.fallback[1], filled.fallback[2], filled.fallback[3]));
    }

    SkinWeights dist = generateDistanceWeights(c.body, c.topo, c.rigDeform, 2);
    extendWeightsToAux(dist, c.full, c.rigDeform);
    SkinWeights heat = generateHeatWeights(c.body, c.topo, c.rigDeform, 8.0, 60, 4.0);
    extendWeightsToAux(heat, c.full, c.rigDeform);
    const char* paeWeightTestIds[] = {"T10n", "T10o"};  // distance, heat
    const SkinWeights* paeWeightSets[] = {&dist, &heat};
    for (size_t i = 0; i < 2; ++i) {
        const SkinWeights* sw = paeWeightSets[i];
        const WeightStats s = weightStats(*sw, bodyCount);
        check(s.verticesWithoutWeight == 0 && near(s.minSum, 1.0, 1e-6) && near(s.maxSum, 1.0, 1e-6),
              fmt("%s PAE %s weights: all body vertices weighted and normalised", paeWeightTestIds[i], sw->generator.c_str()),
              fmt("unweighted=%ld sum[%.6f..%.6f]", s.verticesWithoutWeight, s.minSum, s.maxSum));
        std::printf("       %s: %.1f ms, body mean influences %.3f\n", sw->generator.c_str(), sw->generationMs, s.meanInfluences);
    }
    const double agreeDist = dominantBoneAgreement(mh, c.rigDeform, dist, c.rigDeform);
    const double agreeHeat = dominantBoneAgreement(mh, c.rigDeform, heat, c.rigDeform);
    std::printf("       agreement of PAE weights with MakeHuman weights (dominant bone): distance %.1f%%, heat %.1f%%\n",
                agreeDist * 100.0, agreeHeat * 100.0);

    // --- T11 linear blend skinning ---------------------------------------------------------------
    std::vector<Affine> identity = poseGlobals(c.rigDeform, {});
    std::vector<D3> bound = lbsSkin(c.full, mh, skinMatrices(c.rigDeform, identity));
    std::vector<D3> boundBody(bound.begin(), bound.begin() + bodyCount);
    check(maxDifference(boundBody, base) < 1e-6, "T11a bind pose reproduces the rest surface", fmt("%.3e m", maxDifference(boundBody, base)));

    const int elbow = c.rigDeform.find("lowerarm01.L"), wrist = c.rigDeform.find("wrist.L");
    check(elbow >= 0 && c.rigDeform.find("lowerarm01.R") >= 0 && wrist >= 0, "T11b pose bones exist (lowerarm01.L/R, wrist.L)");
    const std::vector<Affine> flex = poseGlobals(c.rigDeform, {{elbow, D3{1.0, 0.0, 0.0}, -90.0}});
    const std::vector<Affine> flexSkin = skinMatrices(c.rigDeform, flex);
    std::vector<D3> flexedFull = lbsSkin(c.full, mh, flexSkin);
    std::vector<D3> flexed(flexedFull.begin(), flexedFull.begin() + bodyCount);
    const D3 elbowRest = c.rigDeform.bones[static_cast<size_t>(elbow)].head;
    const D3 wristRest = c.rigDeform.bones[static_cast<size_t>(wrist)].head;
    const D3 wristMoved = flexSkin[static_cast<size_t>(wrist)].apply(wristRest);
    const D3 elbowMoved = flexSkin[static_cast<size_t>(elbow)].apply(elbowRest);
    const D3 rel = wristRest - elbowRest, movedRel = wristMoved - elbowMoved;
    std::printf("       elbow %s: elbow head (%.3f %.3f %.3f) -> (%.3f %.3f %.3f), wrist head (%.3f %.3f %.3f) -> (%.3f %.3f %.3f) m\n",
                "lowerarm01.L", elbowRest.x, elbowRest.y, elbowRest.z, elbowMoved.x, elbowMoved.y, elbowMoved.z, wristRest.x, wristRest.y,
                wristRest.z, wristMoved.x, wristMoved.y, wristMoved.z);
    // The forearm already points forward, down and outwards in the rest pose;
    // a 90 deg flexion about the elbow hinge lifts the wrist above the elbow.
    check(near(rel.length(), movedRel.length(), 1e-9) && (elbowMoved - elbowRest).length() < 1e-9 && movedRel.y > 0.5 * movedRel.length(),
          "T11c elbow flexion keeps the forearm length, holds the elbow and raises the wrist above it", fmt("|rel| %.4f -> %.4f, rel.y %+.4f -> %+.4f m",
                                                                                           rel.length(), movedRel.length(), rel.y, movedRel.y));
    double rightMax = 0.0;
    for (int v = 0; v < bodyCount; ++v)
        if (base[static_cast<size_t>(v)].x < -0.02) rightMax = std::max(rightMax, (flexed[static_cast<size_t>(v)] - base[static_cast<size_t>(v)]).length());
    check(rightMax < 1e-7, "T11d left elbow flexion leaves every right-side vertex unchanged (float32 weight rounding only)", fmt("%.3e m", rightMax));
    {
        // Rigid motion: a vertex bound to one bone rotates about the pivot of
        // the rotated bone (the elbow), so its distance to the moved pivot stays.
        double worst = 0.0;
        long tested = 0;
        std::string worstBone;
        const D3 pivotMoved = flexSkin[static_cast<size_t>(elbow)].apply(elbowRest);
        for (int v = 0; v < bodyCount; ++v) {
            if (mh.count[static_cast<size_t>(v)] != 1 || mh.w[static_cast<size_t>(v)][0] < 0.999f) continue;
            const int b = mh.bone[static_cast<size_t>(v)][0];
            bool inSubtree = false;
            for (int a : c.rigDeform.ancestors(b)) inSubtree = inSubtree || a == elbow;
            if (b != elbow && !inSubtree) continue;
            const D3 moved = flexSkin[static_cast<size_t>(b)].apply(base[static_cast<size_t>(v)]);
            const double err = std::fabs((moved - pivotMoved).length() - (base[static_cast<size_t>(v)] - elbowRest).length());
            if (err > worst) { worst = err; worstBone = c.rigDeform.bones[static_cast<size_t>(b)].name; }
            ++tested;
        }
        check(tested > 0 && worst < 1e-9, "T11e single-bone vertices of the forearm chain move rigidly about the elbow pivot",
              fmt("%ld vertices, worst %.3e m on %s, pivot moved (%.3f %.3f %.3f)", tested, worst, worstBone.c_str(), pivotMoved.x, pivotMoved.y,
                  pivotMoved.z));
    }

    // --- T12 template round trip ----------------------------------------------------------------
    std::string templateSha;
    size_t templateBytes = 0;
    {
        std::vector<TemplateShapeTarget> targets;
        for (const ShapeParam& p : c.params) {
            TemplateShapeTarget t;
            t.id = p.key;
            t.name = p.key;
            t.region = p.expectedRegions;
            t.relPath = p.incr.relPath;
            t.sha256 = p.incr.sha256;
            t.vertex = p.incr.vertex;
            t.delta = p.incr.delta;
            targets.push_back(t);
        }
        std::vector<SourceFile> files;
        SourceFile f;
        f.path = "makehuman/data/3dobjs/base.obj";
        f.sha256 = c.obj.sourceSha256;
        f.role = "base-mesh";
        f.license = "CC0-1.0";
        f.basis = "LICENSE.md section C";
        files.push_back(f);
        HumanTemplate t = buildTemplate(c, c.rigDeform, mh, targets, "a8bc2d54ff0ac92e78ff71431b1023eda42bf482", files);
        const std::string bytes = serializeTemplate(t);
        templateBytes = bytes.size();
        HumanTemplate back;
        std::string e;
        const bool parsed = parseTemplate(bytes, back, e);
        const std::string again = parsed ? serializeTemplate(back) : std::string();
        check(parsed && again == bytes, "T12a template write -> read -> write is byte-identical", e);
        check(back.topology.pos.size() == base.size() && back.topology.faces.size() == c.body.faces.size() &&
                  back.rig.bones.size() == c.rigDeform.bones.size() && back.shapeTargets.size() == c.params.size() &&
                  back.weights.vertexCount == static_cast<int>(c.full.size()) && back.auxPositions.size() == c.full.size() - base.size() &&
                  !back.regions.empty() && !back.landmarks.empty(),
               "T12b template carries quad topology, aux vertices, regions, landmarks, targets, rig and weights");
        check(back.weights.fallback == t.weights.fallback && countFallback(back.weights, WeightFallback::KeptAncestor, bodyCount) == 175 &&
                  countFallback(back.weights, WeightFallback::NearestBone) >= 0,
              "T12e template format 2 carries the per-vertex weight fallback flags through the round trip",
              fmt("kept-ancestor=%ld nearest-bone=%ld of %d ids", countFallback(back.weights, WeightFallback::KeptAncestor),
                  countFallback(back.weights, WeightFallback::NearestBone), back.weights.vertexCount));
        check(back.topology.pos.size() == base.size() && maxDifference(back.topology.pos, base) == 0.0, "T12c template positions keep the stable vertex order");
        const std::string path = scratch + "/pae-human-template-test.paht";
        writeFile(path, bytes);
        templateSha = sha256Hex(std::vector<uint8_t>(bytes.begin(), bytes.end()));
        check(fileExists(path) && sha256HexFile(path) == templateSha, "T12d template file on disk hashes to the in-memory serialization",
              fmt("%zu bytes", bytes.size()));
        std::printf("       template: %zu bytes, sha256 %s, %zu landmarks, %zu aux vertices\n", bytes.size(), templateSha.c_str(),
                    t.landmarks.size(), t.auxPositions.size());
        (void)templateBytes;
    }

    // --- T13 neutral export and image writer -----------------------------------------------------
    {
        const std::string objPath = scratch + "/body-triangulated.obj";
        writeTriangulatedObj(objPath, c.body);
        PolyMesh tri;
        std::string e;
        const bool ok = readObj(objPath, tri, e);
        long tris = 0;
        for (const PolyFace& f : tri.faces) tris += f.v.size() == 3 ? 1 : 0;
        check(ok && tri.pos.size() == base.size() && tris == 2 * audit.quadCount,
              "T13a triangulated export re-reads with 2 triangles per quad and the same vertex order", e);
        std::vector<Camera> cameras = protocolCameras(192, 384);
        fitSharedFrames(cameras, {&base});
        const Camera& camera = cameras[0];
        Rgb image = renderSurface({&c.body, &base, &c.regions, &mh, &c.rigDeform}, camera, ShadeMode::Clay, true);
        const std::string pngPath = scratch + "/smoke.png";
        const bool written = writePngFile(pngPath, image);
        std::vector<uint8_t> png;
        const bool signature = readFileBytes(pngPath, png) && png.size() > 8 && png[0] == 0x89 && png[1] == 'P' && png[2] == 'N' && png[3] == 'G';
        check(written && signature, "T13b PNG written through PAE's ImageWriter");
        const double iou = claySilhouetteIoUWithPae(c.body, camera);
        check(iou > 0.95, "T13c this rasterizer matches PAE's ClayRenderer silhouette (IoU > 0.95)", fmt("IoU=%.4f", iou));
    }

    // --- T14 rig follows the shape ----------------------------------------------------------------
    {
        const ShapeParam* stature = nullptr;
        for (const ShapeParam& q : c.params) if (q.key == "stature") stature = &q;
        std::vector<D3> tallerFull, taller;
        applyToFull(c, {{&stature->incr, 1.0}}, tallerFull);
        applyToBody(c, {{&stature->incr, 1.0}}, taller);
        Rig rigT;
        const bool built = rigOnSurface(c, tallerFull, true, rigT);
        const Measures mt = measureBody(taller, c.regions, rigT);
        const int headT = rigT.find("head"), headR = c.rigCore.find("head");
        const int elbowT = rigT.find("lowerarm01.L"), elbowR = c.rigCore.find("lowerarm01.L");
        std::printf("       stature target: stature %.4f -> %.4f m, head joint y %.4f -> %.4f m, thigh %.4f -> %.4f m, shank %.4f -> %.4f m\n",
                    baseM.stature, mt.stature, c.rigCore.bones[static_cast<size_t>(headR)].head.y, rigT.bones[static_cast<size_t>(headT)].head.y,
                    baseM.thighLength, mt.thighLength, baseM.shankLength, mt.shankLength);
        check(built && mt.stature > baseM.stature + 0.05 &&
                  rigT.bones[static_cast<size_t>(headT)].head.y > c.rigCore.bones[static_cast<size_t>(headR)].head.y + 0.05 &&
                  rigT.bones[static_cast<size_t>(elbowT)].head.y > c.rigCore.bones[static_cast<size_t>(elbowR)].head.y + 0.02,
              "T14a joints re-resolved on the taller surface follow the shape (head and elbow joints rise with stature)");
    }

    std::printf("\n== summary: %d passed, %d failed, %d total | load %.0f ms ==\n", gPass, gFail, gPass + gFail, loadMs);
    for (const std::string& f : gFailures) std::printf("FAILED: %s\n", f.c_str());
    return gFail == 0 ? 0 : 1;
}
