#include "AnatomyField.h"

#include <algorithm>
#include <cmath>

namespace pae::anatomy {

namespace {

constexpr float kPi = 3.14159265f;

float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

float sdEllipsoid(Vec3 p, Vec3 c, Vec3 r) {
    const Vec3 q{(p.x - c.x) / r.x, (p.y - c.y) / r.y, (p.z - c.z) / r.z};
    const float k = std::min(std::min(r.x, r.y), r.z);
    return k * (q.length() - 1.0f);
}

// Capsule with independent end radii (round cone) — tapered limbs.
float sdRoundCone(Vec3 p, Vec3 a, float ra, Vec3 b, float rb) {
    const Vec3 ab = b - a;
    const float l2 = ab.lengthSq();
    const float h = l2 > 1e-10f ? std::clamp((p - a).dot(ab) / l2, 0.0f, 1.0f) : 0.0f;
    const Vec3 closest = a + ab * h;
    const float radius = ra + (rb - ra) * h;
    return (p - closest).length() - radius;
}

float sdSmoothUnion(float a, float b, float k) {
    const float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return b * (1.0f - h) + a * h - k * h * (1.0f - h);
}

float sdSmoothSubtraction(float a, float b, float k) {
    const float nb = -b;
    const float h = std::clamp(0.5f - 0.5f * (nb - a) / k, 0.0f, 1.0f);
    return nb * (1.0f - h) + a * h - k * h * (1.0f - h);
}

float sdSphere(Vec3 p, Vec3 c, float r) {
    return (p - c).length() - r;
}

float sdCapsule(Vec3 p, Vec3 a, Vec3 b, float r) {
    return sdRoundCone(p, a, r, b, r);
}

// Region primitives anchored to the BodyJoints bind landmarks. Width/depth
// controls come from HumanAnatomyParameters; only the LEFT (+X) paired
// regions are modeled — evaluation mirrors for the right side.
struct RegionPrimitives {
    Vec3 ribCageC;
    Vec3 ribCageR;
    Vec3 abdomenC;
    Vec3 abdomenR;
    Vec3 pelvisC;
    Vec3 pelvisR;
    Vec3 backCarveC;
    float backCarveR;
    Vec3 clavicleA;
    Vec3 clavicleB;
    float clavicleR;
    Vec3 deltoidC;
    Vec3 deltoidR;
    Vec3 upperArmA;
    Vec3 upperArmB;
    float upperArmRa;
    float upperArmRb;
    Vec3 elbowC;
    float elbowR;
    Vec3 forearmA;
    Vec3 forearmB;
    float forearmRa;
    float forearmRb;
    Vec3 forearmSpindleC;
    float forearmSpindleR;
    Vec3 neckA;
    Vec3 neckB;
    float neckR;
    Vec3 headC;
    Vec3 headR;
};

RegionPrimitives buildPrimitives(const HumanAnatomyParameters& q, const BodyJoints& j) {
    RegionPrimitives m;
    const float t = 1.0f; // joints already carry the figure scale

    // Torso column: chest spans spine->chest, abdomen spans to the hips.
    // Width/depth parameters are MULTIPLIERS over these base half-extents.
    m.ribCageC = Vec3{0.0f, j.spine.y + (j.chest.y - j.spine.y) * 0.55f, 0.012f * t};
    m.ribCageR = Vec3{0.185f * q.chestWidth,
                      (j.chest.y - j.spine.y) * 0.52f + 0.03f * t,
                      0.125f * q.chestDepth} *
                 (0.92f + 0.08f * q.ribCageVolume);
    m.abdomenC = Vec3{0.0f, j.hips.y + (j.spine.y - j.hips.y) * 0.55f, 0.004f * t};
    m.abdomenR = Vec3{0.150f * q.waistWidth * (1.0f - 0.22f * q.waistCurvature),
                      (j.spine.y - j.hips.y) * 0.55f + 0.03f * t,
                      0.100f * q.waistDepth} *
                 (0.92f + 0.08f * q.abdominalVolume);
    m.pelvisC = Vec3{0.0f, j.hips.y + 0.012f * t, 0.0f};
    m.pelvisR = Vec3{0.170f * q.pelvisWidth, 0.105f * t, 0.115f * q.pelvisWidth};

    // Back flatten: a huge sphere centered far behind slices the torso back
    // flat at z ~= -0.13 (roundness shifts the plane slightly forward).
    m.backCarveC = Vec3{0.0f, m.ribCageC.y, -1.95f + q.backDepth * 0.05f};
    m.backCarveR = 1.87f;

    // Left clavicle ridge: sternum -> shoulder.
    m.clavicleA = Vec3{0.02f * t, j.chest.y + 0.045f * t, 0.048f * t};
    m.clavicleB = Vec3{(j.leftShoulder.x - 0.02f) * q.clavicleWidth,
                       j.leftShoulder.y - 0.008f * t * q.shoulderSlope,
                       0.028f * t};
    m.clavicleR = 0.016f * t * q.clavicleWidth;

    // Deltoid: caps the shoulder joint, slightly outboard, volume-driven.
    // Base radius 0.070 (was 0.058) so the shoulder reads as a rounded cap
    // instead of a narrow shelf above the ribcage.
    m.deltoidC = Vec3{j.leftShoulder.x + 0.016f * t * q.deltoidVolume,
                      j.leftShoulder.y - 0.022f * t * q.shoulderSlope, 0.0f};
    m.deltoidR = Vec3{0.070f * t * q.deltoidVolume, 0.088f * t * q.deltoidVolume,
                      0.072f * t * q.deltoidVolume};

    // Upper arm: deltoid underside -> elbow.
    m.upperArmA = m.deltoidC + Vec3{0.010f * t, -0.025f * t * q.shoulderSlope, 0.0f};
    m.upperArmB = j.leftElbow + Vec3{-0.008f * t * q.elbowWidth, 0.0f, 0.004f * t};
    m.upperArmRa = 0.047f * t * q.upperArmVolume;
    m.upperArmRb = 0.041f * t * q.elbowWidth;
    m.elbowC = m.upperArmB;
    m.elbowR = 0.041f * t * q.elbowWidth;

    // Forearm: spindle elbow -> wrist.
    m.forearmA = m.upperArmB + Vec3{0.006f * t, -0.010f * t, 0.002f * t};
    m.forearmB = j.leftWrist + Vec3{0.006f * t * q.forearmLength, -0.010f * t, 0.0f};
    m.forearmRa = 0.044f * t * q.forearmVolume;
    m.forearmRb = 0.032f * t * q.wristWidth;
    m.forearmSpindleC = (m.forearmA + m.forearmB) * 0.5f + Vec3{0.0f, 0.008f * t, 0.0f};
    m.forearmSpindleR = 0.046f * t * q.forearmVolume;

    // Neck + head placeholder.
    m.neckA = Vec3{0.0f, j.chest.y + 0.055f * t, 0.005f * t};
    m.neckB = Vec3{0.0f, j.head.y - 0.055f * t, 0.006f * t};
    m.neckR = 0.046f * t;
    m.headC = j.head + Vec3{0.0f, 0.02f * t, 0.0f};
    m.headR = Vec3{0.105f * t, 0.125f * t, 0.098f * t};
    return m;
}

// Single-sided evaluation (left/+X paired regions). Wrap with the mirror.
float evaluateSide(const HumanAnatomyParameters& q, const BodyJoints& j, Vec3 p) {
    RegionPrimitives m = buildPrimitives(q, j);

    float torso = sdEllipsoid(p, m.ribCageC, m.ribCageR);
    torso = sdSmoothUnion(torso, sdEllipsoid(p, m.abdomenC, m.abdomenR), 0.055f * q.smoothness);
    torso = sdSmoothUnion(torso, sdEllipsoid(p, m.pelvisC, m.pelvisR), 0.05f * q.smoothness);
    torso = sdSmoothSubtraction(torso, sdSphere(p, m.backCarveC, m.backCarveR),
                                0.05f * q.smoothness);

    // Shoulder: deltoid fuses with the widest k — the key M12 blend.
    float shoulder = sdEllipsoid(p, m.deltoidC, m.deltoidR);
    shoulder = sdSmoothUnion(shoulder,
                             sdCapsule(p, m.clavicleA, m.clavicleB, m.clavicleR),
                             0.022f * q.smoothness);
    float body = sdSmoothUnion(torso, shoulder,
                               (0.06f + 0.06f * q.axillaDepth) * q.smoothness);

    // Arm: tapered capsules + spindle with the tightest blends.
    float arm = sdRoundCone(p, m.upperArmA, m.upperArmRa, m.upperArmB, m.upperArmRb);
    arm = sdSmoothUnion(arm, sdSphere(p, m.elbowC, m.elbowR), 0.028f * q.smoothness);
    arm = sdSmoothUnion(arm, sdRoundCone(p, m.forearmA, m.forearmRa, m.forearmB, m.forearmRb),
                        0.032f * q.smoothness);
    arm = sdSmoothUnion(arm, sdSphere(p, m.forearmSpindleC, m.forearmSpindleR),
                        0.040f * q.smoothness);
    body = sdSmoothUnion(body, arm, 0.045f * q.smoothness);

    body = sdSmoothUnion(body, sdCapsule(p, m.neckA, m.neckB, m.neckR), 0.05f * q.smoothness);
    body = sdSmoothUnion(body, sdEllipsoid(p, m.headC, m.headR), 0.04f * q.smoothness);
    return body;
}

Vec3 mirrorX(Vec3 p) { return {-p.x, p.y, p.z}; }

} // namespace

float upperBodySdf(const HumanAnatomyParameters& params, const BodyJoints& joints, Vec3 p) {
    // Mirror evaluation: paired regions are modeled on +X; the min against
    // the x-mirrored field guarantees perfect left/right symmetry.
    const float left = evaluateSide(params, joints, p);
    const float right = evaluateSide(params, joints, mirrorX(p));
    return std::min(left, right);
}

std::array<float, RegionCount> regionFields(const HumanAnatomyParameters& params,
                                            const BodyJoints& joints, Vec3 p) {
    RegionPrimitives m = buildPrimitives(params, joints);
    std::array<float, RegionCount> out{};
    out[static_cast<size_t>(BodyRegion::RibCage)] = sdEllipsoid(p, m.ribCageC, m.ribCageR);
    out[static_cast<size_t>(BodyRegion::Abdomen)] = sdEllipsoid(p, m.abdomenC, m.abdomenR);
    out[static_cast<size_t>(BodyRegion::Pelvis)] = sdEllipsoid(p, m.pelvisC, m.pelvisR);
    out[static_cast<size_t>(BodyRegion::Clavicle)] =
        std::min(sdCapsule(p, m.clavicleA, m.clavicleB, m.clavicleR),
                 sdCapsule(mirrorX(p), mirrorX(m.clavicleA), mirrorX(m.clavicleB),
                           m.clavicleR));
    out[static_cast<size_t>(BodyRegion::Deltoid)] =
        std::min(sdEllipsoid(p, m.deltoidC, m.deltoidR),
                 sdEllipsoid(mirrorX(p), mirrorX(m.deltoidC), m.deltoidR));
    out[static_cast<size_t>(BodyRegion::UpperArm)] =
        std::min(sdRoundCone(p, m.upperArmA, m.upperArmRa, m.upperArmB, m.upperArmRb),
                 sdRoundCone(mirrorX(p), mirrorX(m.upperArmA), m.upperArmRa,
                             mirrorX(m.upperArmB), m.upperArmRb));
    out[static_cast<size_t>(BodyRegion::Forearm)] =
        std::min(sdRoundCone(p, m.forearmA, m.forearmRa, m.forearmB, m.forearmRb),
                 sdRoundCone(mirrorX(p), mirrorX(m.forearmA), m.forearmRa,
                             mirrorX(m.forearmB), m.forearmRb));
    out[static_cast<size_t>(BodyRegion::Neck)] = sdCapsule(p, m.neckA, m.neckB, m.neckR);
    out[static_cast<size_t>(BodyRegion::Head)] = sdEllipsoid(p, m.headC, m.headR);
    return out;
}

uint16_t regionBone(BodyRegion region, int side) {
    const bool left = side > 0;
    switch (region) {
        case BodyRegion::RibCage: return BoneChest;
        case BodyRegion::Abdomen: return BoneSpine;
        case BodyRegion::Pelvis: return BoneHips;
        case BodyRegion::Clavicle: return BoneChest;
        case BodyRegion::Deltoid: return left ? BoneLeftShoulder : BoneRightShoulder;
        case BodyRegion::UpperArm: return left ? BoneLeftUpperArm : BoneRightUpperArm;
        case BodyRegion::Forearm: return left ? BoneLeftLowerArm : BoneRightLowerArm;
        case BodyRegion::Neck: return BoneNeck;
        case BodyRegion::Head: return BoneHead;
    }
    return BoneHips;
}

} // namespace pae::anatomy
