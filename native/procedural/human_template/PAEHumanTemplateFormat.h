#pragma once

// PAEHumanTemplate format v3 (WP1 productization of the host-proof container).
//
// Lineage: tools/human-template-poc/host/src/ht_template.* ("PAHT" v2, host
// proof evidence, frozen). v3 changes, per VR-1B decision D1 and WP2:
//   1. The provenance `convertedAtUtc` field is REMOVED from the byte stream.
//      It was the only nondeterministic field; conversion time belongs in the
//      compiler log, not in the content hash. v3 files are byte-reproducible.
//   2. A 32-byte SHA-256 of the payload follows magic+version, so the loader
//      can reject corrupted files before parsing (WP2 acceptance requirement).
//   3. SourceFile.sizeBytes is i64 (v2 truncated to i32).
//
// The container stores QUAD topology with stable vertex ids, face-corner UVs,
// region labels, landmarks, shape targets, the PAE Rig V2 definition, Top-4
// skin weights with per-vertex fallback flags, and full asset provenance.
// Little-endian, byte-at-a-time, no padding. Pure data: no asset paths, no GL,
// no MakeHuman code — compilable for both host tests and Android.

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace pae::human_template {

constexpr uint32_t kTemplateVersion = 3;

struct D3 {
    double x = 0.0, y = 0.0, z = 0.0;
    D3() = default;
    D3(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}
    D3 operator+(const D3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    D3 operator-(const D3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    D3 operator*(double s) const { return {x * s, y * s, z * s}; }
    D3& operator+=(const D3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    double dot(const D3& o) const { return x * o.x + y * o.y + z * o.z; }
    D3 cross(const D3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    double length() const;
    D3 normalized() const;
};

struct D2 {
    double u = 0.0, v = 0.0;
};

struct PolyFace {
    std::vector<int> v;   // 0-based stable vertex ids
    std::vector<int> vt;  // 0-based UV ids, -1 = absent
};

struct PolyMesh {
    std::vector<D3> pos;  // metres, stable id order
    std::vector<D2> uv;
    std::vector<PolyFace> faces;
};

enum class Region : uint8_t {
    Head, Neck, Thorax, Abdomen, Pelvis, ShoulderGirdle, UpperArm, Forearm, Hand,
    Thigh, LowerLeg, Foot, Count, Unknown
};
const char* regionName(Region r);

enum class BoneRole : uint8_t {
    Root, Pelvis, Spine, Neck, Head, Clavicle, Shoulder, UpperArm, Forearm, Hand, Finger,
    Thigh, Shin, Foot, Toe, Breast, Face, Count
};

struct RigBone {
    std::string name;
    int parent = -1;
    D3 head, tail;
    D3 planeNormal;
    BoneRole role = BoneRole::Face;
    bool deform = true;
    std::string headJoint, tailJoint;
};

struct Rig {
    std::string name;
    std::string source;
    std::string license;
    int formatVersion = 2;
    std::vector<RigBone> bones;
};

enum class WeightFallback : uint8_t { Source = 0, KeptAncestor = 1, NearestBone = 2 };

struct SkinWeights {
    int vertexCount = 0;
    int maxInfluences = 4;
    std::string source;
    std::string license;
    std::string generator;
    std::vector<std::array<uint16_t, 4>> bone;
    std::vector<std::array<float, 4>> w;
    std::vector<uint8_t> count;
    std::vector<uint8_t> fallback;
};

struct Landmark {
    enum class Kind : uint8_t { JointCenter = 0, Surface = 1, Derived = 2 };
    Kind kind = Kind::JointCenter;
    std::string name;
    D3 position;
    std::string source;
    std::string confidence;
};

struct ShapeTarget {
    std::string id, name, region, relPath, sha256;
    double rangeMin = 0.0, rangeMax = 1.0;
    uint8_t side = 0;  // 0 centre, 1 left, 2 right, 3 left+right pair
    std::vector<int> vertex;
    std::vector<D3> delta;
};

struct SourceFile {
    std::string path;
    std::string sha256;
    int64_t sizeBytes = 0;
    std::string role;
    std::string license;
    std::string basis;
};

struct Provenance {
    std::string sourceRepo;
    std::string sourceCommit;
    std::string sourceLicenseSummary;
    std::string converterVersion;
    double unitScaleToMetres = 0.1;
    std::string unitNote;
    std::vector<SourceFile> files;
};

struct HumanTemplate {
    std::string name = "PAEHumanTemplate";
    std::string topologyVersion;
    std::string units = "m";
    std::string upAxis = "+Y";
    std::string forwardAxis = "+Z";
    PolyMesh topology;
    std::vector<D3> auxPositions;  // helper + joint vertices (never rendered)
    std::vector<Region> regions;
    std::vector<Landmark> landmarks;
    std::vector<ShapeTarget> shapeTargets;
    Rig rig;
    SkinWeights weights;
    Provenance provenance;
};

// Serialize to the v3 byte stream (deterministic: identical input -> identical
// bytes). Parse validates magic, version, payload SHA-256, every index range
// and the weight block; any trailing or truncated byte is an error.
std::string serializeTemplate(const HumanTemplate& t);
bool parseTemplate(const std::string& bytes, HumanTemplate& out, std::string& error);

// Self-contained SHA-256 (ported from the host prototype; no external deps).
std::string sha256Hex(const uint8_t* data, size_t size);
inline std::string sha256Hex(const std::string& bytes) {
    return sha256Hex(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
}

} // namespace pae::human_template