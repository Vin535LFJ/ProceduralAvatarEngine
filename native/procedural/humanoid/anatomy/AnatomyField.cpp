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

// Bounding sphere of a feature, inflated by its blend radius. A point outside
// (radius + k) has SDF > k, and smooth-union(x, y, k) equals min(x, y)
// whenever |x - y| >= k — so the whole feature can be skipped without
// changing the composed field. Most sampled cells are far from most features,
// so this is what keeps the M13 feature system affordable.
struct Bound { Vec3 center; float radius; };

Bound boundEllipsoid(Vec3 c, Vec3 r) { return Bound{c, r.length()}; }

Bound boundSegment(Vec3 a, Vec3 b, float r) {
    return Bound{(a + b) * 0.5f, (a - b).length() * 0.5f + r};
}

// True when the feature's primitive provably cannot influence p.
bool prune(const Bound& b, float k, Vec3 p) {
    const float reach = b.radius + k;
    return (p - b.center).lengthSq() > reach * reach;
}

// Union that skips the b-primitive when a bounding test proves it inert.
float prunedUnion(float a, float b, float k, const Bound& bound, Vec3 p) {
    if (prune(bound, k, p)) return a;
    return sdSmoothUnion(a, b, k);
}

// Single-sided evaluation happens inside makeUpperBodyField, where the
// joint-anchored primitive set and its bounding spheres are resolved once.

Vec3 mirrorX(Vec3 p) { return {-p.x, p.y, p.z}; }

} // namespace

float upperBodySdf(const HumanAnatomyParameters& params, const BodyJoints& joints, Vec3 p) {
    return makeUpperBodyField(params, joints)(p);
}

std::function<float(Vec3)> makeUpperBodyField(const HumanAnatomyParameters& params,
                                             const BodyJoints& joints) {
    // Resolve the joint-anchored primitive set once; both mirrored sides and
    // every later evaluation reuse it.
    const RegionPrimitives m = buildPrimitives(params, joints);
    const Bound ribBound = boundEllipsoid(m.ribCageC, m.ribCageR);
    const Bound abdomenBound = boundEllipsoid(m.abdomenC, m.abdomenR);
    const Bound pelvisBound = boundEllipsoid(m.pelvisC, m.pelvisR);
    const Bound backBound = Bound{m.backCarveC, m.backCarveR};
    const Bound deltoidBound = boundEllipsoid(m.deltoidC, m.deltoidR);
    const Bound clavicleBound = boundSegment(m.clavicleA, m.clavicleB, m.clavicleR);
    const Bound upperArmBound = boundSegment(m.upperArmA, m.upperArmB, m.upperArmRa);
    const Bound forearmBound = boundSegment(m.forearmA, m.forearmB, m.forearmRa);
    const Bound armGroupBound = Bound{(m.upperArmA + m.forearmB) * 0.5f,
                                      (m.upperArmA - m.forearmB).length() * 0.5f
                                          + m.forearmSpindleR};
    const Bound neckBound = boundSegment(m.neckA, m.neckB, m.neckR);
    const Bound headBound = boundEllipsoid(m.headC, m.headR);
    const float smoothness = params.smoothness;
    const float kShoulder = (0.06f + 0.06f * params.axillaDepth) * smoothness;

    auto side = [=](Vec3 p) {
        // Finite "far" sentinel: a real infinity would turn the smooth-blend
        // term a*h into 0*inf = NaN and poison the whole field.
        constexpr float kFar = 1e6f;
        float torso = prune(ribBound, 0.055f * smoothness, p)
            ? kFar
            : sdEllipsoid(p, m.ribCageC, m.ribCageR);
        torso = prunedUnion(torso, sdEllipsoid(p, m.abdomenC, m.abdomenR),
                            0.055f * smoothness, abdomenBound, p);
        torso = prunedUnion(torso, sdEllipsoid(p, m.pelvisC, m.pelvisR),
                            0.05f * smoothness, pelvisBound, p);
        if (!prune(backBound, 0.05f * smoothness, p)) {
            torso = sdSmoothSubtraction(torso, sdSphere(p, m.backCarveC, m.backCarveR),
                                        0.05f * smoothness);
        }
        if (!prune(Bound{deltoidBound.center, deltoidBound.radius + clavicleBound.radius},
                   kShoulder, p)) {
            float shoulder = sdEllipsoid(p, m.deltoidC, m.deltoidR);
            shoulder = prunedUnion(shoulder, sdCapsule(p, m.clavicleA, m.clavicleB, m.clavicleR),
                                   0.022f * smoothness, clavicleBound, p);
            torso = sdSmoothUnion(torso, shoulder, kShoulder);
        }
        if (!prune(armGroupBound, 0.045f * smoothness, p)) {
            float arm = sdRoundCone(p, m.upperArmA, m.upperArmRa, m.upperArmB, m.upperArmRb);
            arm = prunedUnion(arm, sdSphere(p, m.elbowC, m.elbowR), 0.028f * smoothness,
                              Bound{m.elbowC, m.elbowR}, p);
            arm = prunedUnion(arm, sdRoundCone(p, m.forearmA, m.forearmRa, m.forearmB,
                                               m.forearmRb),
                              0.032f * smoothness,
                              Bound{forearmBound.center,
                                    (m.forearmA - m.forearmB).length() * 0.5f + m.forearmRa},
                              p);
            arm = prunedUnion(arm, sdSphere(p, m.forearmSpindleC, m.forearmSpindleR),
                              0.040f * smoothness,
                              Bound{m.forearmSpindleC, m.forearmSpindleR}, p);
            torso = sdSmoothUnion(torso, arm, 0.045f * smoothness);
        }
        torso = prunedUnion(torso, sdCapsule(p, m.neckA, m.neckB, m.neckR),
                            0.05f * smoothness,
                            neckBound, p);
        torso = prunedUnion(torso, sdEllipsoid(p, m.headC, m.headR), 0.04f * smoothness,
                            headBound, p);
        return torso;
    };
    // NOTE: the mirror is always evaluated — the L/R symmetry of the result
    // depends on min(side(p), side(-p)) and must not be short-circuited.
    return [side](Vec3 p) {
        return std::min(side(p), side(mirrorX(p)));
    };
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
