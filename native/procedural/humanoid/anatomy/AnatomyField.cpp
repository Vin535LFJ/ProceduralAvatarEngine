#include "AnatomyField.h"
#include "AnatomyFeatures.h"

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

// Super-ellipsoid: same normalised-radius approximation as sdEllipsoid, with
// an exponent that squares the section off (a chest that is fuller across the
// shoulders than a pure ellipsoid can express).
float sdSuperellipsoid(Vec3 p, Vec3 c, Vec3 r, float squash) {
    const Vec3 q{(p.x - c.x) / r.x, (p.y - c.y) / r.y, (p.z - c.z) / r.z};
    const float k = std::min(std::min(r.x, r.y), r.z);
    const float d = q.length();
    const float e = std::clamp(squash, 1.0f, 2.2f);
    return k * (std::pow(d, e) - 1.0f);
}

// Muscle belly: a tube from origin to insertion whose radius swells at
// shape.bellyAt and tapers back to the tendon radii at both ends, with an
// elliptical + squashed + rolled + laterally pinched cross-section. This is
// what makes a pec a fan, a biceps a spindle and a forearm a split mass.
float sdMuscleFan(Vec3 p, Vec3 a, Vec3 b, float ra, float rBelly, float rb,
                  const MuscleShape& shape) {
    const Vec3 ab = b - a;
    const float len = ab.length();
    if (len < 1e-6f) return (p - a).length() - ra;
    const Vec3 axis = ab * (1.0f / len);
    const Vec3 d = p - a;
    const float along = std::clamp(d.dot(axis), 0.0f, len);

    // Radius profile: origin -> belly -> insertion, eased with smoothstep.
    const float ba = std::clamp(shape.bellyAt, 0.05f, 0.95f) * len;
    float r;
    if (along < ba) {
        const float u = ba > 1e-6f ? along / ba : 1.0f;
        const float sm = u * u * (3.0f - 2.0f * u);
        r = ra + (rBelly - ra) * sm;
    } else {
        const float denom = len - ba;
        const float u = denom > 1e-6f ? (along - ba) / denom : 1.0f;
        const float sm = u * u * (3.0f - 2.0f * u);
        r = rBelly + (rb - rBelly) * sm;
    }

    // Section frame: u = perpendicular to the axis (chosen so it does not
    // collapse near the vertical), v = axis x u.
    Vec3 u = std::fabs(axis.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f}.cross(axis)
                                     : Vec3{1.0f, 0.0f, 0.0f}.cross(axis);
    u = u.normalized(Vec3{1.0f, 0.0f, 0.0f});
    Vec3 v = axis.cross(u).normalized(Vec3{0.0f, 0.0f, 1.0f});
    if (shape.roll != 0.0f) {
        const float cr = std::cos(shape.roll), sr = std::sin(shape.roll);
        const Vec3 up = u * cr + v * sr;
        v = v * cr - u * sr;
        u = up;
    }

    const Vec3 radial = d - axis * along;
    const float rx = std::max(1e-4f, r * shape.sectionScale.x);
    const float rz = std::max(1e-4f, r * shape.sectionScale.y);
    float qu = radial.dot(u) / rx;
    float qv = radial.dot(v) / rz;
    float q = std::sqrt(qu * qu + qv * qv);
    if (shape.squash > 1.0f + 1e-4f) q = std::pow(q, shape.squash);
    float dist = std::min(rx, rz) * (q - 1.0f);
    // Lateral pinch: sides of the section push inward, carving the separations
    // between neighbouring muscle groups (rectus segments, forearm groups).
    if (shape.groove > 0.0f) {
        const float side = std::min(1.0f, std::fabs(qu));
        dist += std::min(rx, rz) * shape.groove * side * side * 0.30f;
    }
    return dist;
}

// Two grooves that cut a segment separation into a mass (e.g. the midline
// between the rectus segments).
float sdMidlineGroove(Vec3 p, Vec3 a, Vec3 b, float ra, float rBelly, float rb,
                      const MuscleShape& shape, float midlineOffset) {
    return sdMuscleFan(p, a + Vec3{midlineOffset, 0.0f, 0.0f},
                       b + Vec3{midlineOffset, 0.0f, 0.0f}, ra, rBelly, rb, shape);
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
    Vec3 lumbarA;
    Vec3 lumbarB;
    float lumbarRa;
    float lumbarRb;
    Vec3 clavicleA;
    Vec3 clavicleB;
    float clavicleR;
    Vec3 deltoidC;
    Vec3 deltoidR;
    Vec3 deltoidAntC;
    Vec3 deltoidAntR;
    Vec3 deltoidPostC;
    Vec3 deltoidPostR;
    Vec3 trapA;
    Vec3 trapB;
    float trapRa;
    float trapRbBelly;
    float trapRb;
    Vec3 pecA;
    Vec3 pecB;
    float pecRa;
    float pecRbBelly;
    float pecRb;
    Vec3 upperArmA;
    Vec3 upperArmB;
    float upperArmRa;
    float upperArmRb;
    Vec3 bicepsA;
    Vec3 bicepsB;
    float bicepsRa;
    float bicepsRbBelly;
    float bicepsRb;
    Vec3 tricepsA;
    Vec3 tricepsB;
    float tricepsRa;
    float tricepsRbBelly;
    float tricepsRb;
    Vec3 brachialisA;
    Vec3 brachialisB;
    float brachialisRa;
    float brachialisRb;
    Vec3 elbowC;
    float elbowR;
    Vec3 forearmA;
    Vec3 forearmB;
    float forearmRa;
    float forearmRb;
    Vec3 forearmFlexA;
    Vec3 forearmFlexB;
    float flexRa;
    float flexRbBelly;
    float flexRb;
    Vec3 forearmExtA;
    Vec3 forearmExtB;
    float extRa;
    float extRbBelly;
    float extRb;
    Vec3 forearmSpindleC;
    float forearmSpindleR;
    Vec3 rectusA;
    Vec3 rectusB;
    float rectusRa;
    float rectusRb;
    Vec3 intercostalA;
    Vec3 intercostalB;
    float intercostalRa;
    float intercostalRb;
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
    m.backCarveR = 1.80f;

    // Left clavicle ridge: sternum -> shoulder.
    m.clavicleA = Vec3{0.02f * t, j.chest.y + 0.045f * t, 0.048f * t};
    m.clavicleB = Vec3{(j.leftShoulder.x - 0.02f) * q.clavicleWidth,
                       j.leftShoulder.y - 0.008f * t * q.shoulderSlope,
                       0.028f * t};
    m.clavicleR = 0.016f * t * q.clavicleWidth;

    // Deltoid: caps the shoulder joint, slightly outboard, volume-driven.
    // Base radius 0.070 (was 0.058) so the shoulder reads as a rounded cap
    // instead of a narrow shelf above the ribcage. M13 adds the anterior and
    // posterior heads so the cap has a front/back falloff instead of being a
    // single symmetric ball.
    m.deltoidC = Vec3{j.leftShoulder.x + 0.016f * t * q.deltoidVolume,
                      j.leftShoulder.y - 0.022f * t * q.shoulderSlope, 0.0f};
    m.deltoidR = Vec3{0.070f * t * q.deltoidVolume, 0.088f * t * q.deltoidVolume,
                      0.072f * t * q.deltoidVolume};
    m.deltoidAntC = m.deltoidC - Vec3{0.016f, 0.020f, 0.020f} * t;
    m.deltoidAntR = Vec3{0.044f * t, 0.058f * t, 0.040f * t};
    m.deltoidPostC = m.deltoidC + Vec3{0.016f, 0.020f, -0.020f} * t;
    m.deltoidPostR = Vec3{0.048f * t, 0.058f * t, 0.040f * t};

    // Trapezius: broad sheet from the neck base to the shoulder spine, so the
    // neck-to-shoulder transition has a soft muscular slope.
    m.trapA = Vec3{0.045f * t, j.neck.y - 0.055f * t, -0.045f * t};
    m.trapB = Vec3{j.leftShoulder.x * 0.72f, j.chest.y + 0.030f * t, -0.032f * t};
    m.trapRa = 0.055f * t * q.shoulderWidth;
    m.trapRbBelly = 0.062f * t * q.shoulderWidth;
    m.trapRb = 0.040f * t * q.shoulderWidth;

    // Pectoralis major: a thin, wide fan from the sternum onto the humerus.
    // sternal origin sits on the chest midline, insertion on the inner arm.
    m.pecA = Vec3{0.035f * t, j.chest.y - 0.020f * t, 0.075f * t * q.chestDepth};
    m.pecB = Vec3{j.leftShoulder.x * 0.55f, j.chest.y - 0.080f * t, 0.020f * t};
    m.pecRa = 0.026f * t;                  // sternal tendon
    m.pecRbBelly = 0.062f * t * q.chestWidth; // belly thickness (thickness, not width)
    m.pecRb = 0.024f * t;                  // humeral tendon

    // Upper arm: deltoid underside -> elbow.
    m.upperArmA = m.deltoidC + Vec3{0.010f * t, -0.025f * t * q.shoulderSlope, 0.0f};
    m.upperArmB = j.leftElbow + Vec3{-0.008f * t * q.elbowWidth, 0.0f, 0.004f * t};
    m.upperArmRa = 0.047f * t * q.upperArmVolume;
    m.upperArmRb = 0.041f * t * q.elbowWidth;
    m.elbowC = m.upperArmB;
    m.elbowR = 0.041f * t * q.elbowWidth;

    // Biceps (anterior upper-arm belly) and triceps (posterior), both anchored
    // shoulder -> elbow so they follow the arm direction.
    m.bicepsA = Vec3{j.leftShoulder.x * 0.45f, j.chest.y - 0.030f * t, 0.020f * t};
    m.bicepsB = Vec3{m.upperArmB.x, m.upperArmB.y, 0.020f * t};
    m.bicepsRa = 0.026f * t;
    m.bicepsRbBelly = 0.052f * t * q.upperArmVolume;
    m.bicepsRb = 0.028f * t;

    m.tricepsA = Vec3{j.leftShoulder.x * 0.45f, j.chest.y - 0.030f * t, -0.020f * t};
    m.tricepsB = Vec3{m.upperArmB.x, m.upperArmB.y, -0.020f * t};
    m.tricepsRa = 0.026f * t;
    m.tricepsRbBelly = 0.050f * t * q.upperArmVolume;
    m.tricepsRb = 0.030f * t;

    // Brachialis: small lateral head between deltoid and elbow.
    m.brachialisA = Vec3{j.leftShoulder.x * 0.55f, j.chest.y - 0.055f * t, -0.005f * t};
    m.brachialisB = Vec3{m.upperArmB.x, m.upperArmB.y, -0.005f * t};
    m.brachialisRa = 0.028f * t;
    m.brachialisRb = 0.032f * t;

    // Forearm: flexor group (medial/belly side) + extensor group (lateral).
    m.forearmA = m.upperArmB + Vec3{0.006f * t, -0.010f * t, 0.002f * t};
    m.forearmB = j.leftWrist + Vec3{0.006f * t * q.forearmLength, -0.010f * t, 0.0f};
    m.forearmRa = 0.044f * t * q.forearmVolume;
    m.forearmRb = 0.032f * t * q.wristWidth;
    m.forearmSpindleC = (m.forearmA + m.forearmB) * 0.5f + Vec3{0.0f, 0.008f * t, 0.0f};
    m.forearmSpindleR = 0.046f * t * q.forearmVolume;

    // Flexor sits on the medial (body-side) edge of the forearm and gets the
    // thick belly; extensor is the flatter lateral mass.
    m.forearmFlexA = Vec3{m.forearmA.x - 0.004f * t, m.forearmA.y, m.forearmA.z + 0.012f * t};
    m.forearmFlexB = Vec3{m.forearmB.x, m.forearmB.y, m.forearmB.z + 0.010f * t};
    m.flexRa = 0.026f * t;
    m.flexRbBelly = 0.046f * t * q.forearmVolume;
    m.flexRb = 0.020f * t;
    m.forearmExtA = Vec3{m.forearmA.x + 0.004f * t, m.forearmA.y, m.forearmA.z - 0.012f * t};
    m.forearmExtB = Vec3{m.forearmB.x, m.forearmB.y, m.forearmB.z - 0.010f * t};
    m.extRa = 0.026f * t;
    m.extRbBelly = 0.040f * t * q.forearmVolume;
    m.extRb = 0.020f * t;

    // Rectus abdominis: two narrow ridges on the front midline, so the belly
    // has segment structure instead of one smooth dome.
    m.rectusA = Vec3{0.016f * t, j.chest.y - 0.055f * t, 0.075f * t * q.chestDepth};
    m.rectusB = Vec3{0.014f * t, j.hips.y + 0.050f * t, 0.062f * t * q.waistDepth};
    m.rectusRa = 0.014f * t;
    m.rectusRb = 0.030f * t;

    // Intercostal grooves: shallow separations between rib levels on the front.
    m.intercostalA = Vec3{0.140f * t, j.spine.y - 0.010f * t, 0.020f * t};
    m.intercostalB = Vec3{0.120f * t, j.chest.y - 0.050f * t, 0.020f * t};
    m.intercostalRa = 0.010f * t;
    m.intercostalRb = 0.010f * t;

    // Lumbar: the lower-back mass that carries the waist into the hips.
    m.lumbarA = Vec3{0.0f, j.spine.y - 0.055f * t, -0.045f * t};
    m.lumbarB = Vec3{0.0f, j.hips.y + 0.075f * t, -0.040f * t};
    m.lumbarRa = 0.070f * t * q.waistWidth * (1.0f - 0.15f * q.waistCurvature);
    m.lumbarRb = 0.060f * t * q.pelvisWidth;

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
        // P0 Shoulder/UpperArm fix: the deltoid follows the humerus; the
        // Shoulder bone now represents the clavicle/shoulder girdle only.
        case BodyRegion::Deltoid: return left ? BoneLeftUpperArm : BoneRightUpperArm;
        case BodyRegion::UpperArm: return left ? BoneLeftUpperArm : BoneRightUpperArm;
        case BodyRegion::Forearm: return left ? BoneLeftLowerArm : BoneRightLowerArm;
        case BodyRegion::Neck: return BoneNeck;
        case BodyRegion::Head: return BoneHead;
    }
    return BoneHips;
}

} // namespace pae::anatomy
