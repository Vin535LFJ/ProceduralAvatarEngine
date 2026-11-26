#include "ht_proof.h"

#include <algorithm>

namespace pae::ht {

std::string dataPath(const ProofContext& c, const std::string& rel) { return c.dataDir + "/" + rel; }
std::string repoPath(const ProofContext& c, const std::string& rel) { return c.repoRoot + "/" + rel; }

ShapeTarget mergeTargets(const ShapeTarget& a, const ShapeTarget& b, const std::string& name) {
    ShapeTarget m = a;
    m.name = name;
    m.relPath = a.relPath + "+" + b.relPath;
    m.sha256 = a.sha256 + "+" + b.sha256;
    m.vertex.insert(m.vertex.end(), b.vertex.begin(), b.vertex.end());
    m.delta.insert(m.delta.end(), b.delta.begin(), b.delta.end());
    m.dataRows += b.dataRows;
    m.rowsOutsideBody += b.rowsOutsideBody;
    m.commentLines += b.commentLines;
    m.malformedLines += b.malformedLines;
    m.categoryHeaders.insert(m.categoryHeaders.end(), b.categoryHeaders.begin(), b.categoryHeaders.end());
    return m;
}

void applyToFull(const ProofContext& c, const std::vector<std::pair<const ShapeTarget*, double>>& weighted,
                 std::vector<D3>& fullOut) {
    applyTargets(c.full, weighted, fullOut);
}

void applyToBody(const ProofContext& c, const std::vector<std::pair<const ShapeTarget*, double>>& weighted,
                 std::vector<D3>& bodyOut) {
    applyTargets(c.full, weighted, bodyOut);
    bodyOut.resize(c.body.pos.size());
}

bool rigOnSurface(const ProofContext& c, const std::vector<D3>& fullMetres, bool coreOnly, Rig& out) {
    std::map<std::string, D3> joints;
    int unresolved = 0;
    resolveJoints(c.rigFile, fullMetres, joints, unresolved);
    if (unresolved != 0) return false;
    return buildRigV2(c.rigFile, joints, coreOnly, out);
}

GroupCensus censusGroups(const PolyMesh& obj) {
    GroupCensus c;
    for (const std::string& g : obj.groupOrder) {
        const long faces = static_cast<long>(obj.facesOfGroup(g).size());
        if (g.compare(0, 6, "joint-") == 0) {
            ++c.jointGroups;
            c.jointFaces += faces;
        } else if (g.compare(0, 7, "helper-") == 0) {
            ++c.otherHelperGroups;
            c.otherHelperFaces += faces;
            c.otherHelperNames += (c.otherHelperNames.empty() ? "" : ", ") + g.substr(7);
        }
    }
    return c;
}

bool ProofContext::load(const std::string& dataDirectory, const std::string& repoRootDir, std::string& error) {
    const double t0 = nowMs();
    dataDir = dataDirectory;
    repoRoot = repoRootDir;
    const std::string objPath = dataPath(*this, "3dobjs/base.obj");
    if (!readObj(objPath, obj, error)) return false;
    info(fmt("[load] base.obj: v=%zu vt=%zu f=%zu groups=%zu sha256=%s (%.0f ms)", obj.pos.size(), obj.uv.size(),
             obj.faces.size(), obj.groupOrder.size(), obj.sourceSha256.c_str(), nowMs() - t0));

    obj.extractGroup("body", body, bodyStableIds);
    // Stable Vertex ID contract: the body uses the first N `v` lines, untouched.
    bool contiguous = bodyStableIds.size() == body.pos.size();
    for (size_t i = 0; i < bodyStableIds.size() && contiguous; ++i) contiguous = bodyStableIds[i] == static_cast<int>(i);
    if (!contiguous) {
        error = "body group does not use a contiguous prefix of the OBJ vertex order";
        return false;
    }
    for (D3& p : body.pos) p = p * unitScale;
    full = obj.pos;
    for (D3& p : full) p = p * unitScale;
    buildTopology(body, topo);
    info(fmt("[load] body group: %zu stable vertex ids (0..%zu, contiguous prefix = %s), %zu quads", body.pos.size(),
             body.pos.size() - 1, contiguous ? "yes" : "no", body.faces.size()));

    // Index base of target files: decided by data, not by documentation.
    const std::string probeRel = "targets/head/head-trans-up.target";
    const IndexBaseProbe probe = probeIndexBase(dataPath(*this, probeRel), full);
    int indexBase = probe.spreadBase0 <= probe.spreadBase1 ? 0 : 1;
    info(fmt("[targets] index base probe on %s: spread(0-based)=%.3f m spread(1-based)=%.3f m -> using %d-based indices",
             probeRel.c_str(), probe.spreadBase0, probe.spreadBase1, indexBase));
    if (indexBase != 0) {
        error = "unexpected target index base (data says 1-based); stopping before any target is applied";
        return false;
    }

    struct Def {
        const char* key, *label, *expected, *note;
        std::vector<const char*> incr, decr;
    };
    const std::vector<Def> defs = {
        {"shoulderWidth", "Shoulder width", "shoulder-girdle + arms (the arms shift laterally)", "",
         {"targets/measure/measure-shoulder-dist-incr.target"}, {"targets/measure/measure-shoulder-dist-decr.target"}},
        {"ribcageWidth", "Ribcage width", "thorax + attached arms", "",
         {"targets/torso/torso-scale-horiz-incr.target"}, {"targets/torso/torso-scale-horiz-decr.target"}},
        {"chestDepth", "Chest depth", "thorax", "",
         {"targets/torso/torso-scale-depth-incr.target"}, {"targets/torso/torso-scale-depth-decr.target"}},
        {"waistWidth", "Waist width", "abdomen", "",
         {"targets/measure/measure-waist-circ-incr.target"}, {"targets/measure/measure-waist-circ-decr.target"}},
        {"hipWidth", "Hip width", "pelvis, thigh", "",
         {"targets/measure/measure-hips-circ-incr.target"}, {"targets/measure/measure-hips-circ-decr.target"}},
        {"armShape", "Arm shape (muscle)", "upper-arm", "",
         {"targets/armslegs/l-upperarm-muscle-incr.target", "targets/armslegs/r-upperarm-muscle-incr.target"},
         {"targets/armslegs/l-upperarm-muscle-decr.target", "targets/armslegs/r-upperarm-muscle-decr.target"}},
        {"legShape", "Leg shape (muscle)", "thigh", "",
         {"targets/armslegs/l-upperleg-muscle-incr.target", "targets/armslegs/r-upperleg-muscle-incr.target"},
         {"targets/armslegs/l-upperleg-muscle-decr.target", "targets/armslegs/r-upperleg-muscle-decr.target"}},
        {"headShape", "Head shape (width)", "head", "",
         {"targets/head/head-scale-horiz-incr.target"}, {"targets/head/head-scale-horiz-decr.target"}},
        {"stature", "Stature (macro height)", "whole body",
         "standalone macro target used outside its macro combination; anthropometric correctness UNVERIFIED",
         {"targets/macrodetails/height/male-young-averagemuscle-averageweight-maxheight.target"},
         {"targets/macrodetails/height/male-young-averagemuscle-averageweight-minheight.target"}},
    };
    for (const Def& d : defs) {
        ShapeParam p;
        p.key = d.key;
        p.label = d.expected;
        p.expectedRegions = d.expected;
        p.note = d.note;
        auto loadSet = [&](const std::vector<const char*>& files, ShapeTarget& out) -> bool {
            bool first = true;
            for (const char* f : files) {
                ShapeTarget t;
                // Targets address the WHOLE OBJ: helper and joint vertices move too.
                if (!readTarget(dataPath(*this, f), f, static_cast<long>(full.size()), indexBase, t, error)) return false;
                for (D3& dl : t.delta) dl = dl * unitScale;
                if (first) { out = t; first = false; }
                else out = mergeTargets(out, t, out.name);
            }
            return true;
        };
        for (const char* f : d.incr) p.incrFiles.push_back(f);
        for (const char* f : d.decr) p.decrFiles.push_back(f);
        if (!loadSet(d.incr, p.incr) || !loadSet(d.decr, p.decr)) return false;
        params.push_back(p);
        long helperRows = 0;
        for (int v : p.incr.vertex) helperRows += v >= static_cast<int>(body.pos.size()) ? 1 : 0;
        info(fmt("[targets] %-13s incr=%s rows=%ld moved=%ld (helper/joint %ld) | decr=%s rows=%ld moved=%ld", p.key.c_str(),
                 p.incr.relPath.substr(p.incr.relPath.find_last_of('/') + 1).c_str(), p.incr.dataRows, static_cast<long>(p.incr.vertex.size()), helperRows,
                 p.decr.relPath.substr(p.decr.relPath.find_last_of('/') + 1).c_str(), p.decr.dataRows, static_cast<long>(p.decr.vertex.size())));
    }

    // Rig definition and Rig V2 variants.
    const std::string rigPath = dataPath(*this, "rigs/default.mhskel");
    if (!readRigFile(rigPath, rigFile, error)) return false;
    int unresolved = 0;
    resolveJoints(rigFile, full, joints, unresolved);
    info(fmt("[rig] %s v%ld license=%s bones=%zu joints=%zu planes=%ld unresolvedJoints=%d", rigFile.name.c_str(),
             rigFile.version, rigFile.license.c_str(), rigFile.bones.size(), rigFile.joints.size(), rigFile.planeCount, unresolved));
    if (unresolved != 0) {
        error = "rig joints could not be resolved on the base mesh";
        return false;
    }
    if (!buildRigV2(rigFile, joints, false, rigDeform) || !buildRigV2(rigFile, joints, true, rigCore)) {
        error = "failed to build Rig V2";
        return false;
    }
    info(fmt("[rig] PAE Rig V2 deform skeleton: %zu bones; core variant: %zu bones", rigDeform.bones.size(), rigCore.bones.size()));

    regions = labelRegions(body, rigDeform);
    std::vector<long> regionTotals(static_cast<size_t>(kRegionCount), 0);
    for (Region r : regions) ++regionTotals[static_cast<size_t>(r)];
    std::string census = "[regions]";
    for (int r = 0; r < kRegionCount; ++r)
        if (regionTotals[static_cast<size_t>(r)])
            census += fmt(" %s=%ld", regionName(static_cast<Region>(r)), regionTotals[static_cast<size_t>(r)]);
    info(census);
    info(fmt("[load] done in %.0f ms", nowMs() - t0));
    return true;
}

HumanTemplate buildTemplate(const ProofContext& c, const Rig& rig, const SkinWeights& weights,
                            const std::vector<TemplateShapeTarget>& targets, const std::string& sourceCommit,
                            const std::vector<SourceFile>& files) {
    HumanTemplate t;
    t.name = "PAEHumanTemplate";
    t.topologyVersion = "makehuman-base.obj@sha256:" + c.obj.sourceSha256.substr(0, 16);
    t.units = "m";
    t.upAxis = "+Y";
    t.forwardAxis = "+Z";
    t.topology = c.body;  // already in metres, stable vertex id order
    t.auxPositions.assign(c.full.begin() + static_cast<std::ptrdiff_t>(c.body.pos.size()), c.full.end());
    t.regions = c.regions;
    t.shapeTargets = targets;
    t.rig = rig;
    t.weights = weights;
    for (const RigBone& b : rig.bones) {
        for (int which = 0; which < 2; ++which) {
            Landmark l;
            l.kind = Landmark::Kind::JointCenter;
            l.name = which == 0 ? b.headJoint : b.tailJoint;
            l.position = which == 0 ? b.head : b.tail;
            l.source = "rig joint -> mean of base.obj joint-helper cube vertices";
            l.confidence = "source-defined";
            bool duplicate = false;
            for (const Landmark& e : t.landmarks) duplicate = duplicate || e.name == l.name;
            if (!duplicate) t.landmarks.push_back(l);
        }
    }
    // Derived SURFACE landmarks. They are surface extremes, never joint centres.
    auto addExtreme = [&](const char* name, int axis, bool maximum) {
        Landmark l;
        l.kind = Landmark::Kind::Derived;
        l.name = name;
        l.source = "PAE auto-derived surface extreme of the base mesh";
        l.confidence = "auto-derived";
        const std::vector<D3>& p = c.body.pos;
        size_t best = 0;
        for (size_t i = 1; i < p.size(); ++i) {
            const double vi = axis == 0 ? p[i].x : (axis == 1 ? p[i].y : p[i].z);
            const double vb = axis == 0 ? p[best].x : (axis == 1 ? p[best].y : p[best].z);
            if ((maximum && vi > vb) || (!maximum && vi < vb)) best = i;
        }
        l.position = p[best];
        t.landmarks.push_back(l);
    };
    addExtreme("surface-top-of-head", 1, true);
    addExtreme("surface-sole", 1, false);
    addExtreme("surface-left-most", 0, true);
    addExtreme("surface-right-most", 0, false);
    addExtreme("surface-front-most", 2, true);
    addExtreme("surface-back-most", 2, false);

    t.provenance.sourceRepo = "https://github.com/makehumancommunity/makehuman";
    t.provenance.sourceCommit = sourceCommit;
    t.provenance.sourceLicenseSummary =
        "Bundled assets (base mesh, proxies, targets, modifiers, textures, clothes, poses, expressions) are "
        "CC0 1.0 Universal per LICENSE.md section C; source code is AGPL and NOT used. Rig/weights files carry "
        "\"license\": \"CC0\" inside the file but are not individually enumerated in LICENSE.md: "
        "their license status is PENDING_CONFIRMATION (NOT VERIFIED) until the owner rules on it; "
        "host-only evaluation use, not cleared for redistribution.";
    t.provenance.converterVersion = "pae-ht-proof/0.1 (host-only)";
    t.provenance.convertedAtUtc = utcNow();
    t.provenance.unitScaleToMetres = c.unitScale;
    t.provenance.unitNote =
        "Source mesh unit assumed to be 1 dm (scale 0.1 gives an adult stature); official unit statement UNVERIFIED.";
    t.provenance.files = files;
    return t;
}

} // namespace pae::ht
