#include "ht_template.h"

#include <chrono>
#include <cstdio>
#include <cstring>

namespace pae::ht {

namespace {

constexpr char kMagic[4] = {'P', 'A', 'H', 'T'};
// 1: first host-proof container. 2: per-vertex weight fallback flag added.
constexpr uint32_t kVersion = 2;

struct Writer {
    std::string bytes;
    void u8(uint8_t v) { bytes += static_cast<char>(v); }
    void u16(uint16_t v) { for (int i = 0; i < 2; ++i) bytes += static_cast<char>((v >> (8 * i)) & 0xFF); }
    void u32(uint32_t v) { for (int i = 0; i < 4; ++i) bytes += static_cast<char>((v >> (8 * i)) & 0xFF); }
    void i32(int32_t v) { u32(static_cast<uint32_t>(v)); }
    void f32(float v) { uint32_t raw; std::memcpy(&raw, &v, 4); u32(raw); }
    void f64(double v) { uint64_t raw; std::memcpy(&raw, &v, 8); for (int i = 0; i < 8; ++i) bytes += static_cast<char>((raw >> (8 * i)) & 0xFF); }
    void str(const std::string& s) { u32(static_cast<uint32_t>(s.size())); bytes += s; }
    void d3(const D3& v) { f64(v.x); f64(v.y); f64(v.z); }
};

struct Reader {
    const std::string& bytes;
    size_t pos = 0;
    bool ok = true;
    explicit Reader(const std::string& b) : bytes(b) {}
    bool need(size_t n) {
        if (!ok || pos + n > bytes.size()) { ok = false; return false; }
        return true;
    }
    uint8_t u8() { if (!need(1)) return 0; return static_cast<uint8_t>(bytes[pos++]); }
    uint32_t u32() {
        if (!need(4)) return 0;
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(static_cast<uint8_t>(bytes[pos++])) << (8 * i);
        return v;
    }
    uint16_t u16() {
        if (!need(2)) return 0;
        uint16_t v = 0;
        for (int i = 0; i < 2; ++i) v |= static_cast<uint16_t>(static_cast<uint8_t>(bytes[pos++])) << (8 * i);
        return v;
    }
    int32_t i32() { return static_cast<int32_t>(u32()); }
    float f32() { const uint32_t raw = u32(); float v = 0.0f; std::memcpy(&v, &raw, 4); return v; }
    double f64() {
        if (!need(8)) return 0.0;
        uint64_t raw = 0;
        for (int i = 0; i < 8; ++i) raw |= static_cast<uint64_t>(static_cast<uint8_t>(bytes[pos++])) << (8 * i);
        double v = 0.0;
        std::memcpy(&v, &raw, 8);
        return v;
    }
    std::string str() {
        const uint32_t n = u32();
        if (!need(n)) return {};
        std::string s = bytes.substr(pos, n);
        pos += n;
        return s;
    }
    D3 d3() { const double x = f64(), y = f64(), z = f64(); return {x, y, z}; }
};

void writeWeights(Writer& w, const SkinWeights& sw) {
    w.u8(static_cast<uint8_t>(sw.maxInfluences));
    w.str(sw.source);
    w.str(sw.license);
    w.str(sw.generator);
    w.u32(static_cast<uint32_t>(sw.vertexCount));
    for (int v = 0; v < sw.vertexCount; ++v) {
        w.u8(sw.count[static_cast<size_t>(v)]);
        for (int k = 0; k < sw.count[static_cast<size_t>(v)]; ++k) {
            w.u16(sw.bone[static_cast<size_t>(v)][static_cast<size_t>(k)]);
            w.f32(sw.w[static_cast<size_t>(v)][static_cast<size_t>(k)]);
        }
    }
    // Which vertices did NOT get their weight from the source set (format 2).
    for (int v = 0; v < sw.vertexCount; ++v)
        w.u8(v < static_cast<int>(sw.fallback.size()) ? sw.fallback[static_cast<size_t>(v)] : 0);
}

bool readWeights(Reader& r, SkinWeights& sw) {
    sw = SkinWeights{};
    sw.maxInfluences = r.u8();
    if (sw.maxInfluences > 4) return false;
    sw.source = r.str();
    sw.license = r.str();
    sw.generator = r.str();
    sw.vertexCount = static_cast<int>(r.u32());
    if (sw.vertexCount < 0 || sw.vertexCount > 100000000) return false;
    sw.bone.assign(static_cast<size_t>(sw.vertexCount), {0, 0, 0, 0});
    sw.w.assign(static_cast<size_t>(sw.vertexCount), {0, 0, 0, 0});
    sw.count.assign(static_cast<size_t>(sw.vertexCount), 0);
    for (int v = 0; v < sw.vertexCount; ++v) {
        const uint8_t n = r.u8();
        if (n > sw.maxInfluences) return false;
        sw.count[static_cast<size_t>(v)] = n;
        for (int k = 0; k < n; ++k) {
            sw.bone[static_cast<size_t>(v)][static_cast<size_t>(k)] = r.u16();
            sw.w[static_cast<size_t>(v)][static_cast<size_t>(k)] = r.f32();
        }
    }
    sw.fallback.assign(static_cast<size_t>(sw.vertexCount), 0);
    for (int v = 0; v < sw.vertexCount; ++v) {
        const uint8_t rule = r.u8();
        if (rule > static_cast<uint8_t>(WeightFallback::NearestBone)) return false;
        sw.fallback[static_cast<size_t>(v)] = rule;
        if (rule != static_cast<uint8_t>(WeightFallback::Source) && sw.count[static_cast<size_t>(v)] == 0) return false;
    }
    return r.ok;
}

} // namespace

std::string serializeTemplate(const HumanTemplate& t) {
    Writer w;
    for (char c : kMagic) w.u8(static_cast<uint8_t>(c));
    w.u32(kVersion);
    w.str(t.name);
    w.str(t.topologyVersion);
    w.str(t.units);
    w.str(t.upAxis);
    w.str(t.forwardAxis);

    // Topology: positions, UVs, polygon faces with per-corner UV indices.
    w.u32(static_cast<uint32_t>(t.topology.pos.size()));
    for (const D3& p : t.topology.pos) w.d3(p);
    w.u32(static_cast<uint32_t>(t.topology.uv.size()));
    for (const D2& uv : t.topology.uv) { w.f64(uv.u); w.f64(uv.v); }
    w.u32(static_cast<uint32_t>(t.topology.faces.size()));
    for (const PolyFace& f : t.topology.faces) {
        w.u8(static_cast<uint8_t>(f.v.size()));
        for (size_t k = 0; k < f.v.size(); ++k) {
            w.u32(static_cast<uint32_t>(f.v[k]));
            w.i32(f.vt[k]);
        }
    }

    // Auxiliary (helper + joint) vertices.
    w.u32(static_cast<uint32_t>(t.auxPositions.size()));
    for (const D3& p : t.auxPositions) w.d3(p);

    // Regions: one label per stable vertex id.
    w.u32(static_cast<uint32_t>(t.regions.size()));
    for (Region r : t.regions) w.u8(static_cast<uint8_t>(r));

    // Landmarks.
    w.u32(static_cast<uint32_t>(t.landmarks.size()));
    for (const Landmark& l : t.landmarks) {
        w.u8(static_cast<uint8_t>(l.kind));
        w.str(l.name);
        w.d3(l.position);
        w.str(l.source);
        w.str(l.confidence);
    }

    // Shape targets.
    w.u32(static_cast<uint32_t>(t.shapeTargets.size()));
    for (const TemplateShapeTarget& s : t.shapeTargets) {
        w.str(s.id);
        w.str(s.name);
        w.str(s.region);
        w.str(s.relPath);
        w.str(s.sha256);
        w.f64(s.rangeMin);
        w.f64(s.rangeMax);
        w.u8(s.side);
        w.u32(static_cast<uint32_t>(s.vertex.size()));
        for (size_t i = 0; i < s.vertex.size(); ++i) {
            w.u32(static_cast<uint32_t>(s.vertex[i]));
            w.d3(s.delta[i]);
        }
    }

    // Rig V2.
    w.str(t.rig.name);
    w.str(t.rig.source);
    w.str(t.rig.license);
    w.i32(t.rig.formatVersion);
    w.u32(static_cast<uint32_t>(t.rig.bones.size()));
    for (const RigBone& b : t.rig.bones) {
        w.str(b.name);
        w.i32(b.parent);
        w.d3(b.head);
        w.d3(b.tail);
        w.d3(b.planeNormal);
        w.u8(static_cast<uint8_t>(b.role));
        w.u8(b.deform ? 1 : 0);
        w.str(b.headJoint);
        w.str(b.tailJoint);
    }

    writeWeights(w, t.weights);

    // Provenance.
    w.str(t.provenance.sourceRepo);
    w.str(t.provenance.sourceCommit);
    w.str(t.provenance.sourceLicenseSummary);
    w.str(t.provenance.converterVersion);
    w.str(t.provenance.convertedAtUtc);
    w.f64(t.provenance.unitScaleToMetres);
    w.str(t.provenance.unitNote);
    w.u32(static_cast<uint32_t>(t.provenance.files.size()));
    for (const SourceFile& f : t.provenance.files) {
        w.str(f.path);
        w.str(f.sha256);
        w.i32(static_cast<int32_t>(f.sizeBytes));
        w.str(f.role);
        w.str(f.license);
        w.str(f.basis);
    }
    return w.bytes;
}

bool parseTemplate(const std::string& bytes, HumanTemplate& t, std::string& error) {
    Reader r(bytes);
    for (char c : kMagic)
        if (static_cast<char>(r.u8()) != c) { error = "bad magic"; return false; }
    if (r.u32() != kVersion) { error = "unsupported template version"; return false; }
    t = HumanTemplate{};
    t.name = r.str();
    t.topologyVersion = r.str();
    t.units = r.str();
    t.upAxis = r.str();
    t.forwardAxis = r.str();

    const uint32_t vertexCount = r.u32();
    if (vertexCount > 100000000) { error = "vertex count out of range"; return false; }
    t.topology.pos.reserve(vertexCount);
    for (uint32_t i = 0; i < vertexCount; ++i) t.topology.pos.push_back(r.d3());
    const uint32_t uvCount = r.u32();
    for (uint32_t i = 0; i < uvCount; ++i) { const double u = r.f64(), v = r.f64(); t.topology.uv.push_back({u, v}); }
    const uint32_t faceCount = r.u32();
    for (uint32_t f = 0; f < faceCount; ++f) {
        const uint8_t n = r.u8();
        if (n < 3) { error = "face with < 3 corners"; return false; }
        PolyFace face;
        for (uint8_t k = 0; k < n; ++k) {
            const uint32_t vi = r.u32();
            const int32_t ti = r.i32();
            if (vi >= vertexCount) { error = "face vertex index out of range"; return false; }
            face.v.push_back(static_cast<int>(vi));
            face.vt.push_back(ti);
        }
        t.topology.faces.push_back(face);
    }
    const uint32_t auxCount = r.u32();
    for (uint32_t i = 0; i < auxCount; ++i) t.auxPositions.push_back(r.d3());
    const uint32_t allVertices = vertexCount + auxCount;
    const uint32_t regionCount = r.u32();
    for (uint32_t i = 0; i < regionCount; ++i) {
        const uint8_t v = r.u8();
        if (v >= static_cast<uint8_t>(Region::Count)) { error = "bad region label"; return false; }
        t.regions.push_back(static_cast<Region>(v));
    }
    const uint32_t landmarkCount = r.u32();
    for (uint32_t i = 0; i < landmarkCount; ++i) {
        Landmark l;
        const uint8_t kind = r.u8();
        if (kind > 2) { error = "bad landmark kind"; return false; }
        l.kind = static_cast<Landmark::Kind>(kind);
        l.name = r.str();
        l.position = r.d3();
        l.source = r.str();
        l.confidence = r.str();
        t.landmarks.push_back(l);
    }
    const uint32_t targetCount = r.u32();
    for (uint32_t i = 0; i < targetCount; ++i) {
        TemplateShapeTarget s;
        s.id = r.str();
        s.name = r.str();
        s.region = r.str();
        s.relPath = r.str();
        s.sha256 = r.str();
        s.rangeMin = r.f64();
        s.rangeMax = r.f64();
        s.side = r.u8();
        const uint32_t n = r.u32();
        s.vertex.reserve(n);
        s.delta.reserve(n);
        for (uint32_t k = 0; k < n; ++k) {
            const uint32_t vi = r.u32();
            s.delta.push_back(r.d3());
            if (vi >= allVertices) { error = "target vertex index out of range"; return false; }
            s.vertex.push_back(static_cast<int>(vi));
        }
        t.shapeTargets.push_back(s);
    }
    t.rig.name = r.str();
    t.rig.source = r.str();
    t.rig.license = r.str();
    t.rig.formatVersion = r.i32();
    const uint32_t boneCount = r.u32();
    for (uint32_t i = 0; i < boneCount; ++i) {
        RigBone b;
        b.name = r.str();
        b.parent = r.i32();
        b.head = r.d3();
        b.tail = r.d3();
        b.planeNormal = r.d3();
        const uint8_t role = r.u8();
        if (role >= static_cast<uint8_t>(Role::Count)) { error = "bad bone role"; return false; }
        b.role = static_cast<Role>(role);
        b.deform = r.u8() != 0;
        b.headJoint = r.str();
        b.tailJoint = r.str();
        // Parents may be listed after children (the source order is not a
        // topological order); only the range and self-reference are checked here.
        if (b.parent < -1 || b.parent >= static_cast<int>(boneCount) || b.parent == static_cast<int>(i)) { error = "bad bone parent"; return false; }
        t.rig.bones.push_back(b);
    }
    if (!readWeights(r, t.weights)) { error = "bad weights block"; return false; }
    // Weights cover body + auxiliary vertices (one entry per stable id).
    if (t.weights.vertexCount != static_cast<int>(vertexCount + t.auxPositions.size())) { error = "weights do not cover the topology"; return false; }
    for (int v = 0; v < t.weights.vertexCount; ++v)
        for (int k = 0; k < t.weights.count[static_cast<size_t>(v)]; ++k)
            if (t.weights.bone[static_cast<size_t>(v)][static_cast<size_t>(k)] >= boneCount) {
                error = "weight references unknown bone";
                return false;
            }
    t.provenance.sourceRepo = r.str();
    t.provenance.sourceCommit = r.str();
    t.provenance.sourceLicenseSummary = r.str();
    t.provenance.converterVersion = r.str();
    t.provenance.convertedAtUtc = r.str();
    t.provenance.unitScaleToMetres = r.f64();
    t.provenance.unitNote = r.str();
    const uint32_t fileCount = r.u32();
    for (uint32_t i = 0; i < fileCount; ++i) {
        SourceFile f;
        f.path = r.str();
        f.sha256 = r.str();
        f.sizeBytes = r.i32();
        f.role = r.str();
        f.license = r.str();
        f.basis = r.str();
        t.provenance.files.push_back(f);
    }
    if (!r.ok) { error = "truncated template"; return false; }
    if (r.pos != bytes.size()) { error = "trailing bytes after template"; return false; }
    return true;
}

} // namespace pae::ht
