#pragma once

#include "procedural/MeshData.h"
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

struct BodyParameters {
    float height = 1.70f;
    float headScale = 1.12f;      // enlarged cartoon head-to-body ratio (M10)
    float headWidth = 0.20f;
    float headHeight = 0.25f;
    float shoulderWidth = 0.42f;
    float chestWidth = 0.36f;
    float waistWidth = 0.30f;
    float hipWidth = 0.34f;
    float torsoLength = 0.52f;
    float neckLength = 0.10f;
    float upperArmLength = 0.30f;
    float lowerArmLength = 0.28f;
    float armRadius = 0.062f;
    float upperLegLength = 0.42f;
    float lowerLegLength = 0.40f;
    float legRadius = 0.082f;
    float handScale = 1.15f;
    float footLength = 0.24f;
    float footWidth = 0.10f;
    float stylization = 1.0f;
    uint32_t detailLevel = 1;
    uint64_t seed = 20261121;

    // M11 anatomy groups — all fields drive the generators.
    TorsoShape torsoShape{};
    PelvisShape pelvisShape{};
    ArmShape armShape{};
    LegShape legShape{};
    NeckShape neckShape{};
};

struct HumanoidResult {
    MeshData mesh;
    Skeleton skeleton;
    BodyJoints joints;
    bool valid = false;
};

BodyParameters clampBodyParameters(const BodyParameters& params);
HumanoidResult generateHumanoid(const BodyParameters& params, bool weightHeatmap = false);

} // namespace pae
