#pragma once

// WP4-HOST-RIG-01 (WP4-A): PAE-owned Rig V2 definition + joint resolver.
// Host-only. The runtime chain resolves every joint from the CC0 base.obj
// `joint-*` helper groups; it never reads default.mhskel / default_weights.mhw
// (both PENDING_CONFIRMATION, research reference only). Nothing here touches
// LegacyRigV1, the PAHT format, or any Android production path.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_rig.h"

#include <map>
#include <string>
#include <vector>

namespace pae::wp4 {

// Stable semantic bone identifier. Independent of source naming and of any
// future GPU palette index. Authored by PAE (see 01-rig-v2-architecture.md).
struct BoneDef {
    const char* id;
    const char* parent;      // "" = root
    ht::Role role;
    const char* headGroup;   // CC0 OBJ joint group; "" => special root anchor
    const char* tailGroup;
    const char* deformGroup; // torso / shoulder.L / arm.L / hand.L / leg.L / foot.L ...
    bool core;               // member of the 20-bone Body Core profile
};

// The frozen PAE authored table (Full Deform Profile v1, 92 bones).
const std::vector<BoneDef>& boneTable();
constexpr const char* kRigVersion = "pae-rig-v2.1";
// v2.2 adopts the anatomical shoulder joint center (see below).
constexpr const char* kRigVersionAnatomical = "pae-rig-v2.2";

// PAE-authored anatomical shoulder offset (metres), STUDIED but NOT adopted by
// default. Moving the shoulder joint center proximal along the humerus axis is
// a TRADE-OFF, not a clear improvement: rigorous fixed-mask verification (same
// skin patch for both rigs) shows fewer face flips at abduction 30/60 deg but
// HIGHER peak axilla edge stretch at all angles and WORSE flips at 90 deg
// (edgeMax 12.75x -> 14.82x, flips 64 -> 70). The earlier "halves stretch"
// result (12.75x -> 5.87x) was a measurement-mask artifact: the axilla mask had
// moved with the joint center, excluding the stretched inferior skin. Available
// for study via anatomicalShoulder=true (rig v2.2); default stays v2.1.
constexpr double kShoulderProximalOffsetM = 0.010;

enum class Profile { Core20, Full92 };
const char* profileName(Profile p);

// Resolved joint positions (metres) on a deformed full surface.
struct JointSet {
    std::map<std::string, ht::D3> group;  // joint group name -> centroid
    ht::D3 rootHead;                      // PAE anchor (stable body vertex)
    int rootHeadVertex = -1;
    std::vector<std::string> unresolved;  // groups missing from the OBJ
    double maxLrAsymmetry = 0.0;          // max |L + mirror(R)| over pairs (m)
    int lrPairs = 0;
};

// Rest-time anchor resolution: pick the body surface vertex nearest the PAE
// authored root anchor. Returned id is then tracked across shape changes.
int resolveRootAnchor(const ht::PolyMesh& obj, const std::vector<ht::D3>& full);

// Resolve all joints on the CURRENT (deformed) full surface. `rootVertex` is
// the stable id from resolveRootAnchor (pass -1 to re-resolve the anchor).
bool resolveJoints(const ht::PolyMesh& obj, const std::vector<ht::D3>& full, int rootVertex, JointSet& out);

// A built, validated rig on a resolved joint set.
struct RigV2 {
    Profile profile = Profile::Full92;
    std::string version = kRigVersion;
    std::vector<ht::RigBone> bones;   // name = semantic id; head/tail/parent/role
    std::vector<ht::D3> ax, ay, az;   // rest local frame axes in world (columns)
    std::vector<ht::Affine> restGlobal, restLocal;
    std::vector<std::string> deformGroup;
    int find(const std::string& id) const;
    ht::Rig asHtRig() const;          // adapter for the reused weight generators
};

struct RigValidation {
    bool ok = false;
    int boneCount = 0;
    bool uniqueIds = true, noCycle = true, parentsExist = true, noZeroLength = true;
    double minBoneLength = 0.0;
    std::vector<std::string> problems;
};
RigValidation validateRig(const RigV2& rig);

// Build a profile rig from resolved joints. Core keeps only core-flagged bones
// and re-parents dropped bones to their nearest kept ancestor. anatomicalShoulder
// (default false = v2.1 helper-centroid rig) optionally moves the shoulder joint
// center kShoulderProximalOffsetM proximal along the humerus axis (v2.2, studied
// but NOT adopted by default — see kShoulderProximalOffsetM note: trade-off).
bool buildRig(const JointSet& js, Profile profile, RigV2& out, std::string& error, bool anatomicalShoulder = false);

// Deviation of the PAE resolver against the PENDING research reference rig
// (host research only; the reference is loaded by the caller, never shipped).
struct JointDeviation {
    std::string bone;
    double headDev = 0.0, tailDev = 0.0;  // metres
};
std::vector<JointDeviation> compareToReference(const RigV2& pae, const ht::Rig& reference);

} // namespace pae::wp4