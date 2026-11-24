#pragma once

#include "procedural/MeshData.h"
#include "procedural/humanoid/FaceGenerator.h"
#include "procedural/humanoid/HairGenerator.h"
#include "procedural/surfaces/ParametricSurfaces.h"
#include "procedural/surfaces/ParametricSurfaces.h"
#include "skeleton/Skeleton.h"

namespace pae {

// M11: fine-grained anatomy controls. Every field participates in geometry
// generation (see Humanoid.cpp); defaults produce the reference cartoon body.
struct TorsoShape {
    float chestDepth = 1.0f;      // chest thickness multiplier on chestWidth
    float chestUpperCurve = 0.35f;// upper-chest fullness toward the shoulders
    float chestLowerCurve = 0.25f;// lower ribcage rounding
    float chestTilt = 0.012f;     // chest volume forward offset (m)
    float chestVolume = 1.0f;     // chest prominence multiplier
    float waistPinch = 0.45f;     // how far the waist narrows between chest/hips
    float backFlatness = 0.4f;    // back (-z) flattening; front keeps depth
};

struct PelvisShape {
    float hipFlare = 0.55f;       // sideways hip flare at the pelvis base
    float hipDepth = 0.36f;       // pelvis front-back depth ratio on hipWidth
    float waistHipBlend = 0.5f;   // waist->hip transition fullness
    float pelvicTilt = 0.0f;      // pelvis volume forward offset (m)
};

struct ArmShape {
    float deltoidVolume = 1.18f;  // shoulder cap radius factor
    float upperArmWidth = 1.02f;  // biceps ring radius factor
    float elbowShape = 0.94f;     // elbow ring radius factor
    float forearmCurve = 0.018f;  // forearm spindle forward offset (m)
    float forearmEllipse = 0.90f; // forearm z-section scale (flat upper arm)
    float wristPinch = 0.68f;     // wrist radius factor
};

struct LegShape {
    float thighVolume = 1.12f;    // thigh ring radius factor
    float kneeShape = 0.88f;      // knee ring radius factor
    float calfCurve = 0.030f;     // calf backward bulge offset (m)
    float calfVolume = 1.10f;     // calf ring radius factor
    float anklePinch = 0.58f;     // ankle ring radius factor
    float legEllipse = 0.94f;     // leg z-section scale
};

struct NeckShape {
    float radiusFactor = 0.42f;   // neck radius relative to neckLength
    float taper = 0.85f;          // top radius / bottom radius
};

struct ClothingParams {
    float thickness = 0.012f;   // shell thickness over the body surface (m)
    float ease = 0.010f;        // extra ease so the cloth floats over skin
    float shirtHem = 0.92f;     // shirt hem position (fraction of torso)
    float sleeveLength = 0.55f; // fraction of the upper arm covered
    float pantsLength = 1.0f;   // fraction of the legs covered (1 = ankles)
};

struct BodyParameters {
    // Adult natural proportions. The M10 cartoon defaults (1.12x head,
    // short limbs) are retained through BodyParameters overrides; these
    // defaults target a realistic adult silhouette at ~1.75 m.
    float height = 1.75f;
    float headScale = 1.0f;       // natural adult head-to-body ratio
    float headWidth = 0.168f;
    float headHeight = 0.235f;
    float shoulderWidth = 0.465f;  // bideltoid, natural adult male
    float chestWidth = 0.365f;
    float waistWidth = 0.285f;
    float hipWidth = 0.345f;
    float torsoLength = 0.565f;
    float neckLength = 0.082f;
    float upperArmLength = 0.325f;
    float lowerArmLength = 0.295f;
    float armRadius = 0.056f;
    float upperLegLength = 0.455f;
    float lowerLegLength = 0.445f;
    float legRadius = 0.078f;
    float handScale = 1.0f;
    float footLength = 0.255f;
    float footWidth = 0.095f;
    float stylization = 1.0f;
    uint32_t detailLevel = 2;
    uint64_t seed = 20261124;

    // M11 anatomy groups — all fields drive the generators.
    TorsoShape torsoShape{};
    PelvisShape pelvisShape{};
    ArmShape armShape{};
    LegShape legShape{};
    NeckShape neckShape{};
    face::FaceParams face{};
    hair::HairParams hair{};
    ClothingParams clothing{};
};

struct HumanoidResult {
    MeshData mesh;
    Skeleton skeleton;
    BodyJoints joints;
    bool valid = false;
};

BodyParameters clampBodyParameters(const BodyParameters& params);
BodyJoints deriveJoints(const BodyParameters& params, float scale);
HumanoidResult generateHumanoid(const BodyParameters& params, bool weightHeatmap = false);

// M11-F: shared anatomy profiles (clothing shells reuse these exact rings).
std::vector<LoftSection> computeTorsoSections(const BodyParameters& params,
                                              const BodyJoints& joints, float scale);
std::vector<SweepRing> computeLegRings(const BodyParameters& params,
                                       const BodyJoints& joints, float scale, bool left);
std::vector<SweepRing> computeArmRings(const BodyParameters& params,
                                       const BodyJoints& joints, float scale, bool left);;

// M11-F: shared anatomy profiles (clothing shells reuse these exact rings).
std::vector<LoftSection> computeTorsoSections(const BodyParameters& params,
                                              const BodyJoints& joints, float scale);
std::vector<SweepRing> computeLegRings(const BodyParameters& params,
                                       const BodyJoints& joints, float scale, bool left);
std::vector<SweepRing> computeArmRings(const BodyParameters& params,
                                       const BodyJoints& joints, float scale, bool left);;

} // namespace pae
