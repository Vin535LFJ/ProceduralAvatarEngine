#pragma once

// Rig V2 prototype (variable bone count, dynamic palette, configurable
// influence count) and skin weights, host-only. Nothing here touches the
// production LegacyRigV1 (22 BoneId / UBO[22] / 64 B packing).

#include "ht_common.h"
#include "ht_json.h"
#include "ht_obj.h"
#include "ht_topology.h"

#include <map>

namespace pae::ht {

// PAE anatomical role of a bone. Independent of the source naming scheme.
enum class Role : uint8_t {
    Root, Pelvis, Spine, Neck, Head, Clavicle, Shoulder, UpperArm, Forearm, Hand, Finger,
    Thigh, Shin, Foot, Toe, Breast, Face, Count
};
const char* roleName(Role r);
Role roleOfBoneName(const std::string& name);
bool isFacialRole(Role r);  // Face or Breast: excluded from the PAE deform rig

enum class Region : uint8_t {
    Head, Neck, Thorax, Abdomen, Pelvis, ShoulderGirdle, UpperArm, Forearm, Hand,
    Thigh, LowerLeg, Foot, Count, Unknown
};
constexpr int kRegionCount = static_cast<int>(Region::Count);
const char* regionName(Region r);
Region regionOfRole(Role r);

struct RigBone {
    std::string name;
    int parent = -1;
    D3 head, tail;
    D3 planeNormal;      // roll reference (normal of the source rotation plane); zero when absent
    Role role = Role::Face;
    bool deform = true;
    std::string headJoint, tailJoint;  // landmark provenance (joint helper name)
};

struct Rig {
    std::string name;
    std::string source;
    std::string license;
    int formatVersion = 2;
    std::vector<RigBone> bones;
    int find(const std::string& boneName) const;
    std::vector<int> ancestors(int bone) const;  // nearest first, excluding itself
};

// --- MakeHuman .mhskel -------------------------------------------------------
struct RigFileJoint {
    std::string name;
    std::vector<long> objVertexIndices;  // 1-based OBJ indices, as stored in the file
};
struct RigFileBone {
    std::string name, headJoint, tailJoint, parent;
    std::array<std::string, 3> planeJoints;
};
struct RigFile {
    std::string path, sha256;
    std::string name, license, description, weightsFile;
    long version = 0;
    std::vector<RigFileBone> bones;
    std::vector<RigFileJoint> joints;
    long planeCount = 0;
};
bool readRigFile(const std::string& path, RigFile& out, std::string& error);

// Joint position = mean of the referenced vertices on the CURRENT surface,
// so the rig follows every shape change of the template.
bool resolveJoints(const RigFile& file, const std::vector<D3>& positions, std::map<std::string, D3>& out,
                   int& unresolvedCount);

// Names of the minimal locomation set used as the "Rig V2 core" variant.
std::vector<std::string> coreBoneNames();

// Builds a PAE Rig V2 from the rig file on the current surface.
//   coreOnly=false : every non-facial bone of the source rig
//   coreOnly=true  : the minimal set in coreBoneNames(); dropped bones are
//                    re-parented to their nearest kept ancestor
bool buildRigV2(const RigFile& file, const std::map<std::string, D3>& joints, bool coreOnly, Rig& out);

// Heuristic anatomical region label per vertex: nearest bone by
// radius-normalised distance (surface distance / bone radius), mapped to a
// region through the bone's PAE role.
std::vector<Region> labelRegions(const PolyMesh& body, const Rig& rig);
std::vector<double> boneRadii(const PolyMesh& body, const Rig& rig);  // mean of 16 nearest vertex distances
// For each bone: number of body vertices for which it is the nearest bone
// (radius-normalised). Diagnostic for the region labels.
std::vector<long> nearestBoneCounts(const PolyMesh& body, const Rig& rig);

// --- Skin weights -------------------------------------------------------------
// How a vertex got its weight for this rig. "Source" = taken from the weight
// set as loaded/generated; the other two are the deterministic fallback for
// vertices that set leaves without any influence.
enum class WeightFallback : uint8_t { Source = 0, KeptAncestor = 1, NearestBone = 2 };
const char* weightFallbackName(WeightFallback rule);

struct SkinWeights {
    int vertexCount = 0;
    int maxInfluences = 4;
    std::string source;      // file / generator description
    std::string license;     // when loaded from a licensed asset
    std::string generator;   // "makehuman-mhw", "pae-distance", "pae-heat", "remap"
    double generationMs = 0.0;
    std::vector<std::array<uint16_t, 4>> bone;
    std::vector<std::array<float, 4>> w;
    std::vector<uint8_t> count;
    std::vector<uint8_t> fallback;  // one WeightFallback per vertex
    // statistics collected before Top-N compression
    double meanInfluencesBeforeCompression = 0.0;
    double massLostByCompression = 0.0;
    long entriesRead = 0;
    long unknownBoneEntries = 0;
    long outOfRangeEntries = 0;
    int detectedIndexBase = 0;
};
// hierarchy + restPositions switch on the fallback for vertices whose source
// weights all sit on bones this rig does not keep (facial/breast bones):
//   KeptAncestor  every dropped bone is replaced by its nearest kept ancestor
//                 in the source hierarchy (facial bones -> head), masses merged
//   NearestBone   no usable source entry at all: nearest bone segment at rest
// A vertex that already has a kept influence is never touched. Without these
// two arguments such vertices stay unweighted (count 0).
bool readMhWeights(const std::string& path, const Rig& rig, int vertexCount, SkinWeights& out, std::string& error,
                   const RigFile* hierarchy = nullptr, const std::vector<D3>* restPositions = nullptr);
long countFallback(const SkinWeights& sw, WeightFallback rule, int vertexLimit = -1);
// PAE generators work on the body mesh (they need its graph) and then give
// every auxiliary vertex its nearest bone with weight 1.
SkinWeights generateDistanceWeights(const PolyMesh& body, const Topology& topo, const Rig& rig, int smoothingIterations);
SkinWeights generateHeatWeights(const PolyMesh& body, const Topology& topo, const Rig& rig, double diffusionTime,
                                int iterations, double power);
void extendWeightsToAux(SkinWeights& bodyWeights, const std::vector<D3>& fullPositions, const Rig& rig);
SkinWeights remapToRig(const SkinWeights& src, const Rig& from, const Rig& to);

struct WeightStats {
    double meanInfluences = 0.0;
    double maxInfluences = 0.0;
    double minSum = 0.0, maxSum = 0.0;
    long verticesWithoutWeight = 0;
};
WeightStats weightStats(const SkinWeights& sw, int vertexLimit = -1);
// L1 difference between two weight sets (0 = identical), mean over vertices.
double weightL1Difference(const SkinWeights& a, const Rig& rigA, const SkinWeights& b, const Rig& rigB);
// Fraction of vertices whose dominant bone is the same in both sets.
double dominantBoneAgreement(const SkinWeights& a, const Rig& rigA, const SkinWeights& b, const Rig& rigB);

// --- Linear blend skinning -----------------------------------------------------
struct BoneRotation {
    int bone = -1;
    D3 axis;          // unit axis in the REST world frame
    double angleDeg = 0.0;
};
std::vector<Affine> poseGlobals(const Rig& rig, const std::vector<BoneRotation>& rotations);
std::vector<Affine> skinMatrices(const Rig& rig, const std::vector<Affine>& globals);
std::vector<D3> lbsSkin(const std::vector<D3>& rest, const SkinWeights& sw, const std::vector<Affine>& skin);

} // namespace pae::ht
