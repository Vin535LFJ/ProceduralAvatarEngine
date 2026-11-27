#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <cmath>
#include <cstring>

namespace pae::human_template {

double D3::length() const { return std::sqrt(dot(*this)); }
D3 D3::normalized() const {
    const double l = length();
    return l > 1e-15 ? *this * (1.0 / l) : D3{};
}

const char* regionName(Region r) {
    switch (r) {
        case Region::Head: return "head";
        case Region::Neck: return "neck";
        case Region::Thorax: return "thorax";
        case Region::Abdomen: return "abdomen";
        case Region::Pelvis: return "pelvis";
        case Region::ShoulderGirdle: return "shoulder-girdle";
        case Region::UpperArm: return "upper-arm";
        case Region::Forearm: return "forearm";
        case Region::Hand: return "hand";
        case Region::Thigh: return "thigh";
        case Region::LowerLeg: return "lower-leg";
        case Region::Foot: return "foot";
        default: return "unknown";
    }
}

namespace {

constexpr char kMagic[4] = {'P', 'A', 'H', 'T'};

struct Writer {
    std::string bytes;
    void u8(uint8_t v) { bytes += static_cast<char>(v); }
    void u16(uint16_t v) { for (int i = 0; i < 2; ++i) bytes += static_cast<char>((v >> (8 * i)) & 0xFF); }
    void u32(uint32_t v) { for (int i = 0; i < 4; ++i) bytes += static_cast<char>((v >> (8 * i)) & 0xFF); }
    void i32(int32_t v) { u32(static_cast<uint32_t>(v)); }
    void i64(int64_t v) {
        const uint64_t raw = static_cast<uint64_t>(v);
        for (int i = 0; i < 8; ++i) bytes += static_cast<char>((raw >> (8 * i)) & 0xFF);
    }
    void f32(float v) { uint32_t raw; std::memcpy(&raw, &v, 4); u32(raw); }
    void f64(double v) { uint64_t raw; std::memcpy(&raw, &v, 8); for (int i = 0; i < 8; ++i) bytes += static_cast<char>((raw >> (8 * i)) & 0xFF); }
    void str(const std::string& s) { u32(static_cast<uint32_t>(s.size())); bytes += s; }
    void d3(const D3& v) { f64(v.x); f64(v.y); f64(v.z); }
};

struct Reader {
    const std::string& bytes;
    size_t pos = 0;
    bool ok = true;
    explicit Reader(const std::string& b, size_t start = 0) : bytes(b), pos(start) {}
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
    int64_t i64() {
        if (!need(8)) return 0;
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(static_cast<uint8_t>(bytes[pos++])) << (8 * i);
        return static_cast<int64_t>(v);
    }
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

// --- SHA-256 (ported verbatim from the host prototype ht_common.cpp) ---------
struct Sha256 {
    uint32_t h[8];
    uint64_t bitLength = 0;
    uint8_t block[64];
    size_t blockUsed = 0;

    Sha256() {
        static const uint32_t init[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
        for (int i = 0; i < 8; ++i) h[i] = init[i];
    }
    static uint32_t rotr(uint32_t v, int n) { return (v >> n) | (v << (32 - n)); }
    void transform(const uint8_t* p) {
        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(p[i * 4]) << 24) | (static_cast<uint32_t>(p[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(p[i * 4 + 2]) << 8) | static_cast<uint32_t>(p[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t t1 = hh + S1 + ch + k[i] + w[i];
            const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    void update(const uint8_t* data, size_t size) {
        bitLength += static_cast<uint64_t>(size) * 8;
        for (size_t i = 0; i < size; ++i) {
            block[blockUsed++] = data[i];
            if (blockUsed == 64) { transform(block); blockUsed = 0; }
        }
    }
    void finalBytes(uint8_t out[32]) {
        const uint64_t messageBits = bitLength;
        uint8_t pad = 0x80;
        update(&pad, 1);
        pad = 0x00;
        while (blockUsed != 56) update(&pad, 1);
        uint8_t lengthBytes[8];
        for (int i = 0; i < 8; ++i) lengthBytes[i] = static_cast<uint8_t>(messageBits >> (56 - i * 8));
        for (int i = 0; i < 8; ++i) {
            block[blockUsed++] = lengthBytes[i];
            if (blockUsed == 64) { transform(block); blockUsed = 0; }
        }
        for (int i = 0; i < 8; ++i)
            for (int shift = 24; shift >= 0; shift -= 8)
                out[i * 4 + (24 - shift) / 8] = static_cast<uint8_t>(h[i] >> shift);
    }
};

} // namespace

std::string sha256Hex(const uint8_t* data, size_t size) {
    Sha256 sha;
    sha.update(data, size);
    uint8_t digest[32];
    sha.finalBytes(digest);
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (int i = 0; i < 32; ++i) {
        out += hex[digest[i] >> 4];
        out += hex[digest[i] & 15];
    }
    return out;
}

std::string serializeTemplate(const HumanTemplate& t) {
    Writer w;
    w.str(t.name);
    w.str(t.topologyVersion);
    w.str(t.units);
    w.str(t.upAxis);
    w.str(t.forwardAxis);

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

    w.u32(static_cast<uint32_t>(t.auxPositions.size()));
    for (const D3& p : t.auxPositions) w.d3(p);

    w.u32(static_cast<uint32_t>(t.regions.size()));
    for (Region r : t.regions) w.u8(static_cast<uint8_t>(r));

    w.u32(static_cast<uint32_t>(t.landmarks.size()));
    for (const Landmark& l : t.landmarks) {
        w.u8(static_cast<uint8_t>(l.kind));
        w.str(l.name);
        w.d3(l.position);
        w.str(l.source);
        w.str(l.confidence);
    }

    w.u32(static_cast<uint32_t>(t.shapeTargets.size()));
    for (const ShapeTarget& s : t.shapeTargets) {
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

    // Provenance: v3 has NO conversion timestamp (D1 reproducibility fix).
    w.str(t.provenance.sourceRepo);
    w.str(t.provenance.sourceCommit);
    w.str(t.provenance.sourceLicenseSummary);
    w.str(t.provenance.converterVersion);
    w.f64(t.provenance.unitScaleToMetres);
    w.str(t.provenance.unitNote);
    w.u32(static_cast<uint32_t>(t.provenance.files.size()));
    for (const SourceFile& f : t.provenance.files) {
        w.str(f.path);
        w.str(f.sha256);
        w.i64(f.sizeBytes);
        w.str(f.role);
        w.str(f.license);
        w.str(f.basis);
    }

    // Header: magic + version + SHA-256(payload).
    Sha256 sha;
    sha.update(reinterpret_cast<const uint8_t*>(w.bytes.data()), w.bytes.size());
    uint8_t digest[32];
    sha.finalBytes(digest);
    std::string out;
    out.reserve(40 + w.bytes.size());
    for (char c : kMagic) out += c;
    Writer hw;
    hw.u32(kTemplateVersion);
    out += hw.bytes;
    out.append(reinterpret_cast<const char*>(digest), 32);
    out += w.bytes;
    return out;
}

bool parseTemplate(const std::string& bytes, HumanTemplate& t, std::string& error) {
    if (bytes.size() < 40) { error = "truncated template header"; return false; }
    for (int i = 0; i < 4; ++i)
        if (bytes[static_cast<size_t>(i)] != kMagic[i]) { error = "bad magic"; return false; }
    Reader header(bytes);
    header.pos = 4;
    if (header.u32() != kTemplateVersion) { error = "unsupported template version"; return false; }
    const std::string storedHash = bytes.substr(8, 32);
    const std::string actualHash = sha256Hex(reinterpret_cast<const uint8_t*>(bytes.data()) + 40, bytes.size() - 40);
    static const char* hex = "0123456789abcdef";
    std::string storedHex;
    storedHex.reserve(64);
    for (unsigned char c : storedHash) {
        storedHex += hex[c >> 4];
        storedHex += hex[c & 15];
    }
    if (storedHex != actualHash) { error = "payload hash mismatch"; return false; }

    Reader r(bytes, 40);
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
        ShapeTarget s;
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
        if (role >= static_cast<uint8_t>(BoneRole::Count)) { error = "bad bone role"; return false; }
        b.role = static_cast<BoneRole>(role);
        b.deform = r.u8() != 0;
        b.headJoint = r.str();
        b.tailJoint = r.str();
        if (b.parent < -1 || b.parent >= static_cast<int>(boneCount) || b.parent == static_cast<int>(i)) {
            error = "bad bone parent";
            return false;
        }
        t.rig.bones.push_back(b);
    }
    if (!readWeights(r, t.weights)) { error = "bad weights block"; return false; }
    if (t.weights.vertexCount != static_cast<int>(vertexCount + t.auxPositions.size())) {
        error = "weights do not cover the topology";
        return false;
    }
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
    t.provenance.unitScaleToMetres = r.f64();
    t.provenance.unitNote = r.str();
    const uint32_t fileCount = r.u32();
    for (uint32_t i = 0; i < fileCount; ++i) {
        SourceFile f;
        f.path = r.str();
        f.sha256 = r.str();
        f.sizeBytes = r.i64();
        f.role = r.str();
        f.license = r.str();
        f.basis = r.str();
        t.provenance.files.push_back(f);
    }
    if (!r.ok) { error = "truncated template"; return false; }
    if (r.pos != bytes.size()) { error = "trailing bytes after template"; return false; }
    return true;
}

} // namespace pae::human_template