// WP1 Human Template Compiler (host-only; never part of the Android build).
//
// Reuses the validated P1-VR-ARCH-1 loading chain (ht_proof) and emits the
// productized PAHT v3 container (native/procedural/human_template). Per
// ADR-014 decision 7 the DISTRIBUTED template carries a PAE-generated Rig V2
// core (20 bones) and PAE distance weights — the PENDING_CONFIRMATION
// default.mhskel / default_weights.mhw are host-only references and are NOT
// redistributed as rig/weight data.
//
// D1 reproducibility: v3 has no conversion timestamp in the byte stream; the
// conversion time is logged only. Two runs on the same inputs must produce
// byte-identical files (verified by the roundtrip + sha256 log).
//
// Usage: pae_ht_compiler --data <makehuman/data> --repo <makehuman root>
//                        --out <file.paht> [--commit <sha>]

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_rig.h"
#include "ht_template.h"

#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace pae::ht;
namespace ht3 = pae::human_template;

namespace {

void say(const std::string& line) {
    std::printf("%s\n", line.c_str());
    std::fflush(stdout);
}

ht3::D3 mapD3(const D3& v) { return {v.x, v.y, v.z}; }

ht3::HumanTemplate mapTemplate(const HumanTemplate& src) {
    ht3::HumanTemplate t;
    t.name = src.name;
    t.topologyVersion = src.topologyVersion;
    t.units = src.units;
    t.upAxis = src.upAxis;
    t.forwardAxis = src.forwardAxis;
    for (const D3& p : src.topology.pos) t.topology.pos.push_back(mapD3(p));
    for (const D2& uv : src.topology.uv) t.topology.uv.push_back({uv.u, uv.v});
    for (const PolyFace& f : src.topology.faces) {
        ht3::PolyFace nf;
        nf.v = f.v;
        nf.vt = f.vt;
        t.topology.faces.push_back(std::move(nf));
    }
    for (const D3& p : src.auxPositions) t.auxPositions.push_back(mapD3(p));
    for (Region r : src.regions) t.regions.push_back(static_cast<ht3::Region>(static_cast<uint8_t>(r)));
    for (const Landmark& l : src.landmarks) {
        ht3::Landmark nl;
        nl.kind = static_cast<ht3::Landmark::Kind>(static_cast<uint8_t>(l.kind));
        nl.name = l.name;
        nl.position = mapD3(l.position);
        nl.source = l.source;
        nl.confidence = l.confidence;
        t.landmarks.push_back(std::move(nl));
    }
    for (const TemplateShapeTarget& s : src.shapeTargets) {
        ht3::ShapeTarget ns;
        ns.id = s.id;
        ns.name = s.name;
        ns.region = s.region;
        ns.relPath = s.relPath;
        ns.sha256 = s.sha256;
        ns.rangeMin = s.rangeMin;
        ns.rangeMax = s.rangeMax;
        ns.side = s.side;
        ns.vertex = s.vertex;
        for (const D3& d : s.delta) ns.delta.push_back(mapD3(d));
        t.shapeTargets.push_back(std::move(ns));
    }
    t.rig.name = src.rig.name;
    t.rig.source = src.rig.source;
    t.rig.license = src.rig.license;
    t.rig.formatVersion = src.rig.formatVersion;
    for (const RigBone& b : src.rig.bones) {
        ht3::RigBone nb;
        nb.name = b.name;
        nb.parent = b.parent;
        nb.head = mapD3(b.head);
        nb.tail = mapD3(b.tail);
        nb.planeNormal = mapD3(b.planeNormal);
        nb.role = static_cast<ht3::BoneRole>(static_cast<uint8_t>(b.role));
        nb.deform = b.deform;
        nb.headJoint = b.headJoint;
        nb.tailJoint = b.tailJoint;
        t.rig.bones.push_back(std::move(nb));
    }
    t.weights.vertexCount = src.weights.vertexCount;
    t.weights.maxInfluences = src.weights.maxInfluences;
    t.weights.source = src.weights.source;
    t.weights.license = src.weights.license;
    t.weights.generator = src.weights.generator;
    t.weights.bone = src.weights.bone;
    t.weights.w = src.weights.w;
    t.weights.count = src.weights.count;
    t.weights.fallback = src.weights.fallback;
    t.provenance.sourceRepo = src.provenance.sourceRepo;
    t.provenance.sourceCommit = src.provenance.sourceCommit;
    t.provenance.sourceLicenseSummary = src.provenance.sourceLicenseSummary;
    t.provenance.converterVersion = src.provenance.converterVersion;
    // D1: convertedAtUtc deliberately NOT carried into v3.
    t.provenance.unitScaleToMetres = src.provenance.unitScaleToMetres;
    t.provenance.unitNote = src.provenance.unitNote;
    for (const SourceFile& f : src.provenance.files) {
        ht3::SourceFile nf;
        nf.path = f.path;
        nf.sha256 = f.sha256;
        nf.sizeBytes = static_cast<int64_t>(f.sizeBytes);
        nf.role = f.role;
        nf.license = f.license;
        nf.basis = f.basis;
        t.provenance.files.push_back(std::move(nf));
    }
    return t;
}

} // namespace

int main(int argc, char** argv) {
    std::string dataDir, repoRoot, outPath;
    std::string commit = "a8bc2d54ff0ac92e78ff71431b1023eda42bf482";
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string key = argv[i];
        const std::string value = argv[i + 1];
        if (key == "--data") dataDir = value;
        else if (key == "--repo") repoRoot = value;
        else if (key == "--out") outPath = value;
        else if (key == "--commit") commit = value;
    }
    if (dataDir.empty() || repoRoot.empty() || outPath.empty()) {
        std::fprintf(stderr, "usage: pae_ht_compiler --data <makehuman/data> --repo <root> --out <file.paht> [--commit <sha>]\n");
        return 2;
    }

    ProofContext c;
    std::string error;
    if (!c.load(dataDir, repoRoot, error)) {
        say("[error] load failed: " + error);
        return 1;
    }
    for (const std::string& l : c.log) say(l);
    say(fmt("[compiler] body verts=%zu quads=%zu aux=%zu core bones=%zu",
            c.body.pos.size(), c.body.faces.size(), c.full.size() - c.body.pos.size(), c.rigCore.bones.size()));

    // PAE-generated weights on the PAE core rig (ADR-014 decision 7).
    const double w0 = nowMs();
    SkinWeights weights = generateDistanceWeights(c.body, c.topo, c.rigCore, 2);
    extendWeightsToAux(weights, c.full, c.rigCore);
    weights.license = "PAE-generated (no third-party weight data)";
    const WeightStats ws = weightStats(weights);
    say(fmt("[weights] pae-distance core rig: vertices=%d meanInfluences=%.3f minSum=%.4f maxSum=%.4f unweighted=%ld ms=%.0f",
            weights.vertexCount, ws.meanInfluences, ws.minSum, ws.maxSum, ws.verticesWithoutWeight, nowMs() - w0));
    if (ws.verticesWithoutWeight > 0) {
        say("[error] PAE weights leave vertices unweighted; refusing to compile");
        return 1;
    }

    // Shape targets (same 9-parameter table as the host proof).
    std::vector<TemplateShapeTarget> ttargets;
    for (const ShapeParam& p : c.params) {
        TemplateShapeTarget t;
        t.id = p.key;
        t.name = p.key;
        t.region = p.expectedRegions;
        t.relPath = p.incr.relPath;
        t.sha256 = p.incr.sha256;
        t.side = p.incrFiles.size() == 2 ? 3 : 0;
        t.vertex = p.incr.vertex;
        t.delta = p.incr.delta;
        ttargets.push_back(t);
        TemplateShapeTarget d = t;
        d.id = p.key + "-";
        d.relPath = p.decr.relPath;
        d.sha256 = p.decr.sha256;
        d.vertex = p.decr.vertex;
        d.delta = p.decr.delta;
        ttargets.push_back(d);
    }

    // Provenance: CC0 base mesh + targets + license texts are distributed data;
    // the two PENDING files are recorded as host-only references, not data.
    std::vector<SourceFile> files;
    auto addFile = [&](const std::string& rel, const std::string& role, const std::string& license, const std::string& basis) {
        const std::string path = repoPath(c, rel);
        std::vector<uint8_t> bytes;
        if (!readFileBytes(path, bytes)) {
            say("[error] missing provenance file " + rel);
            return;
        }
        SourceFile f;
        f.path = "makehuman/" + rel;
        f.sha256 = sha256Hex(bytes);
        f.sizeBytes = static_cast<long>(bytes.size());
        f.role = role;
        f.license = license;
        f.basis = basis;
        files.push_back(f);
    };
    const std::string cc0Basis = "LICENSE.md section C (bundled assets are CC0); file header states CC0";
    addFile("LICENSE.md", "license-text", "CC0-1.0 (assets) / AGPL (code, unused)", "root license file");
    addFile("LICENSE.ASSETS.md", "license-text", "CC0-1.0", "full CC0 legal text");
    addFile("makehuman/data/3dobjs/base.obj", "base-mesh", "CC0-1.0", cc0Basis);
    for (const ShapeParam& p : c.params) {
        for (const std::string& rel : p.incrFiles) addFile("makehuman/data/" + rel, "shape-target [" + p.key + " +]", "CC0-1.0", cc0Basis);
        for (const std::string& rel : p.decrFiles) addFile("makehuman/data/" + rel, "shape-target [" + p.key + " -]", "CC0-1.0", cc0Basis);
    }
    const std::string pending = "PENDING_CONFIRMATION (CC0 declared in file only)";
    addFile("makehuman/data/rigs/default.mhskel", "host-only-reference (NOT distributed)", pending,
            "Used only to name/organise joint helpers during host compilation; the distributed rig is PAE-generated. Owner confirmation outstanding.");
    addFile("makehuman/data/rigs/default_weights.mhw", "host-only-reference (NOT distributed)", pending,
            "Not used for the distributed weights (PAE distance weights instead). Owner confirmation outstanding.");

    HumanTemplate src = buildTemplate(c, c.rigCore, weights, ttargets, commit, files);
    // Distribution-template license summary (overrides the proof-era text).
    src.provenance.sourceLicenseSummary =
        "Base mesh and shape targets: CC0-1.0 (LICENSE.md section C + per-file headers). "
        "Rig and weights: PAE-generated, no third-party rig/weight data redistributed. "
        "default.mhskel / default_weights.mhw: host-only references, PENDING_CONFIRMATION, not distributed.";
    src.rig.source = "PAE HumanRigV2 core (20 bones), joint centres from CC0 joint-helper geometry; source skeleton naming as host-only reference";
    src.rig.license = "PAE-generated";
    src.provenance.converterVersion = "pae_ht_compiler/1.0 (PAHT v3, ADR-014)";

    ht3::HumanTemplate t = mapTemplate(src);
    const std::string bytes = ht3::serializeTemplate(t);
    ht3::HumanTemplate back;
    if (!ht3::parseTemplate(bytes, back, error)) {
        say("[error] v3 roundtrip parse failed: " + error);
        return 1;
    }
    if (ht3::serializeTemplate(back) != bytes) {
        say("[error] v3 roundtrip is not byte-identical");
        return 1;
    }
    if (!writeFile(outPath, bytes)) {
        say("[error] cannot write " + outPath);
        return 1;
    }
    const std::string sha = ht3::sha256Hex(bytes);
    say(fmt("[template-v3] %zu bytes sha256 %s roundtrip=byte-identical", bytes.size(), sha.c_str()));
    say(fmt("[template-v3] body=%zu quads=%zu aux=%zu landmarks=%zu targets=%zu bones=%zu regions=%zu",
            t.topology.pos.size(), t.topology.faces.size(), t.auxPositions.size(),
            t.landmarks.size(), t.shapeTargets.size(), t.rig.bones.size(), t.regions.size()));
    say("[template-v3] convertedAtUtc (log only, NOT in file): " + utcNow());
    say("[done] " + outPath);
    return 0;
}