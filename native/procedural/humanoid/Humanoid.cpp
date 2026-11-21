#include "Humanoid.h"

#include "core/diagnostics/Log.h"
#include "procedural/geometry/CatmullRom.h"
#include "skeleton/weights/WeightGenerator.h"
#include "procedural/primitives/Primitives.h"
#include "procedural/surfaces/ParametricSurfaces.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace pae {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kFootHeight = 0.08f;

float clampParam(float value, float lo, float hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

float smoothstep(float edge0, float edge1, float x) {
    const float t = clampParam((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}


struct Part {
    MeshData mesh;
    std::vector<Vec3> path;
    std::vector<uint16_t> pathBones;
    uint16_t materialId = 0;
};

Vec3 materialColor(uint16_t materialId) {
    switch (materialId) {
        case 0: return {0.95f, 0.75f, 0.62f}; // skin
        case 1: return {0.22f, 0.13f, 0.10f}; // hair
        case 2: return {0.25f, 0.42f, 0.72f}; // cloth
        case 3: return {0.97f, 0.97f, 0.92f}; // eye
        case 4: return {0.05f, 0.04f, 0.06f}; // ink (M11: iris/pupil/mouth)
        default: return {0.8f, 0.8f, 0.8f};
    }
}

BodyJoints deriveJoints(const BodyParameters& p, float scale) {
    BodyJoints j;
    const float footHeight = kFootHeight * scale;
    const float hipY = (p.upperLegLength + p.lowerLegLength) * scale + footHeight;
    const float torsoTop = hipY + p.torsoLength * scale;
    const float neckTop = torsoTop + p.neckLength * scale;
    const float headHalf = p.headHeight * scale * 0.5f;

    j.root = {0.0f, 0.0f, 0.0f};
    j.hips = {0.0f, hipY, 0.0f};
    j.spine = {0.0f, hipY + p.torsoLength * scale * 0.35f, 0.0f};
    j.chest = {0.0f, hipY + p.torsoLength * scale * 0.78f, 0.0f};
    j.neck = {0.0f, torsoTop, 0.0f};
    j.head = {0.0f, neckTop + headHalf, 0.0f};

    const float shoulderX = p.shoulderWidth * scale * 0.5f;
    const float upperArm = p.upperArmLength * scale;
    const float lowerArm = p.lowerArmLength * scale;
    const float armAngle = 24.0f * kPi / 180.0f;
    const float forearmAngle = 10.0f * kPi / 180.0f;

    j.leftShoulder = {shoulderX, j.chest.y, 0.0f};
    j.leftElbow = j.leftShoulder + Vec3{std::sin(armAngle) * upperArm, -std::cos(armAngle) * upperArm, 0.0f};
    j.leftWrist = j.leftElbow + Vec3{std::sin(forearmAngle) * lowerArm, -std::cos(forearmAngle) * lowerArm, 0.0f};
    j.leftHand = j.leftWrist + Vec3{0.02f * scale, -0.06f * scale, 0.0f};

    j.rightShoulder = {-shoulderX, j.chest.y, 0.0f};
    j.rightElbow = j.rightShoulder + Vec3{-std::sin(armAngle) * upperArm, -std::cos(armAngle) * upperArm, 0.0f};
    j.rightWrist = j.rightElbow + Vec3{-std::sin(forearmAngle) * lowerArm, -std::cos(forearmAngle) * lowerArm, 0.0f};
    j.rightHand = j.rightWrist + Vec3{-0.02f * scale, -0.06f * scale, 0.0f};

    const float hipX = p.hipWidth * scale * 0.5f;
    const float upperLeg = p.upperLegLength * scale;
    const float lowerLeg = p.lowerLegLength * scale;
    j.leftHip = {hipX, hipY, 0.0f};
    j.leftKnee = {hipX * 1.05f, hipY - upperLeg, 0.0f};
    j.leftAnkle = {hipX * 1.05f, hipY - upperLeg - lowerLeg, 0.0f};
    j.leftToe = j.leftAnkle + Vec3{0.0f, -footHeight * 0.5f, p.footLength * scale * 0.6f};

    j.rightHip = {-hipX, hipY, 0.0f};
    j.rightKnee = {-hipX * 1.05f, hipY - upperLeg, 0.0f};
    j.rightAnkle = {-hipX * 1.05f, hipY - upperLeg - lowerLeg, 0.0f};
    j.rightToe = j.rightAnkle + Vec3{0.0f, -footHeight * 0.5f, p.footLength * scale * 0.6f};
    return j;
}

MeshData translated(const MeshData& source, const Vec3& offset) {
    MeshData result = source;
    for (Vertex& vertex : result.vertices) vertex.position = vertex.position + offset;
    Aabb bounds;
    for (const Vertex& vertex : result.vertices) bounds.expand(vertex.position);
    result.bounds = bounds;
    return result;
}

} // namespace

BodyParameters clampBodyParameters(const BodyParameters& params) {
    BodyParameters p = params;
    p.height = clampParam(p.height, 0.8f, 2.2f);
    p.headScale = clampParam(p.headScale, 0.6f, 1.8f);
    p.headWidth = clampParam(p.headWidth, 0.10f, 0.40f) * p.headScale;
    p.headHeight = clampParam(p.headHeight, 0.12f, 0.48f) * p.headScale;
    p.shoulderWidth = clampParam(p.shoulderWidth, 0.20f, 0.80f);
    p.chestWidth = clampParam(p.chestWidth, 0.20f, 0.70f);
    p.waistWidth = clampParam(p.waistWidth, 0.16f, 0.60f);
    p.hipWidth = clampParam(p.hipWidth, 0.18f, 0.60f);
    p.torsoLength = clampParam(p.torsoLength, 0.25f, 0.90f);
    p.neckLength = clampParam(p.neckLength, 0.04f, 0.20f);
    p.upperArmLength = clampParam(p.upperArmLength, 0.12f, 0.55f);
    p.lowerArmLength = clampParam(p.lowerArmLength, 0.12f, 0.55f);
    p.armRadius = clampParam(p.armRadius, 0.025f, 0.12f);
    p.upperLegLength = clampParam(p.upperLegLength, 0.18f, 0.75f);
    p.lowerLegLength = clampParam(p.lowerLegLength, 0.18f, 0.75f);
    p.legRadius = clampParam(p.legRadius, 0.035f, 0.16f);
    p.handScale = clampParam(p.handScale, 0.5f, 1.8f);
    p.footLength = clampParam(p.footLength, 0.12f, 0.40f);
    p.footWidth = clampParam(p.footWidth, 0.06f, 0.20f);
    p.stylization = clampParam(p.stylization, 0.5f, 2.0f);
    p.detailLevel = clampParam(p.detailLevel, 0u, 3u);
    p.torsoShape.chestDepth = clampParam(p.torsoShape.chestDepth, 0.6f, 1.6f);
    p.torsoShape.chestUpperCurve = clampParam(p.torsoShape.chestUpperCurve, 0.0f, 1.0f);
    p.torsoShape.chestLowerCurve = clampParam(p.torsoShape.chestLowerCurve, 0.0f, 1.0f);
    p.torsoShape.chestTilt = clampParam(p.torsoShape.chestTilt, -0.05f, 0.05f);
    p.torsoShape.chestVolume = clampParam(p.torsoShape.chestVolume, 0.0f, 2.0f);
    p.torsoShape.waistPinch = clampParam(p.torsoShape.waistPinch, 0.0f, 1.0f);
    p.torsoShape.backFlatness = clampParam(p.torsoShape.backFlatness, 0.0f, 1.0f);
    p.pelvisShape.hipFlare = clampParam(p.pelvisShape.hipFlare, 0.0f, 1.0f);
    p.pelvisShape.hipDepth = clampParam(p.pelvisShape.hipDepth, 0.15f, 0.6f);
    p.pelvisShape.waistHipBlend = clampParam(p.pelvisShape.waistHipBlend, 0.0f, 1.0f);
    p.pelvisShape.pelvicTilt = clampParam(p.pelvisShape.pelvicTilt, -0.05f, 0.05f);
    p.armShape.deltoidVolume = clampParam(p.armShape.deltoidVolume, 0.9f, 1.6f);
    p.armShape.upperArmWidth = clampParam(p.armShape.upperArmWidth, 0.8f, 1.4f);
    p.armShape.elbowShape = clampParam(p.armShape.elbowShape, 0.8f, 1.1f);
    p.armShape.forearmCurve = clampParam(p.armShape.forearmCurve, -0.05f, 0.05f);
    p.armShape.forearmEllipse = clampParam(p.armShape.forearmEllipse, 0.7f, 1.0f);
    p.armShape.wristPinch = clampParam(p.armShape.wristPinch, 0.5f, 0.9f);
    p.legShape.thighVolume = clampParam(p.legShape.thighVolume, 0.9f, 1.4f);
    p.legShape.kneeShape = clampParam(p.legShape.kneeShape, 0.75f, 1.0f);
    p.legShape.calfCurve = clampParam(p.legShape.calfCurve, -0.06f, 0.06f);
    p.legShape.calfVolume = clampParam(p.legShape.calfVolume, 0.9f, 1.3f);
    p.legShape.anklePinch = clampParam(p.legShape.anklePinch, 0.4f, 0.8f);
    p.legShape.legEllipse = clampParam(p.legShape.legEllipse, 0.8f, 1.0f);
    p.neckShape.radiusFactor = clampParam(p.neckShape.radiusFactor, 0.3f, 0.6f);
    p.neckShape.taper = clampParam(p.neckShape.taper, 0.7f, 1.0f);
    return p;
}

HumanoidResult generateHumanoid(const BodyParameters& params, bool weightHeatmap) {
    const auto genStart = std::chrono::steady_clock::now();
    const BodyParameters p = clampBodyParameters(params);
    HumanoidResult result;
    const float estimatedHeight = p.upperLegLength + p.lowerLegLength + kFootHeight +
                                  p.torsoLength + p.neckLength + p.headHeight;
    const float scale = clampParam(p.height / std::fmax(estimatedHeight, 0.1f), 0.5f, 1.8f);
    const BodyJoints joints = deriveJoints(p, scale);
    result.joints = joints;
    result.skeleton = buildHumanoidSkeleton(joints);

    const uint32_t radial = 12u + p.detailLevel * 6u;
    const uint32_t rings = radial;
    const int splineSamples = 3 + static_cast<int>(p.detailLevel);
    std::vector<Part> parts;

    // ---- Torso: smooth S-curve spline loft, 9 anatomy stations (M11). ----
    {
        Part part;
        part.materialId = 2;
        const TorsoShape& ts = p.torsoShape;
        const PelvisShape& ps = p.pelvisShape;
        const float hipW = p.hipWidth * scale;
        const float waistW = p.waistWidth * scale;
        const float chestW = p.chestWidth * scale;
        const float shoulderW = p.shoulderWidth * scale;
        const float T = p.torsoLength * scale;
        const float chestDepth = chestW * ts.chestDepth;
        const float hipDepth = hipW * ps.hipDepth;
        std::vector<LoftSection> sections;
        const auto at = [&](float fraction) { return joints.hips.y + T * fraction; };
        // Station radii: pelvis flare -> waist pinch -> chest volume -> shoulder line.
        const float waistRx = waistW * 0.5f * (1.0f - 0.30f * ts.waistPinch);
        const float waistRz = waistW * 0.30f * (1.0f - 0.20f * ts.waistPinch);
        const float chestRx = chestW * 0.5f * (0.92f + 0.10f * ts.chestVolume);
        const float chestRz = chestDepth * (0.86f + 0.18f * ts.chestVolume);
        const float hipRx = hipW * 0.5f * (0.92f + 0.14f * ps.hipFlare);
        sections.push_back({{0.0f, at(-0.10f) + ps.pelvicTilt * 0.4f, hipDepth * 0.10f},
                            hipRx, hipDepth * 1.02f, 0.0f});
        sections.push_back({{0.0f, at(0.14f), hipDepth * 0.06f},
                            lerp(hipRx, waistRx, ps.waistHipBlend * 0.5f), hipDepth * 0.78f, 0.0f});
        sections.push_back({{0.0f, at(0.38f), 0.0f}, waistRx, waistRz, 0.0f});
        sections.push_back({{0.0f, at(0.60f), ts.chestTilt * 0.35f},
                            chestW * 0.5f * (0.86f + 0.14f * ts.chestLowerCurve),
                            chestDepth * (0.80f + 0.14f * ts.chestLowerCurve), 0.0f});
        sections.push_back({{0.0f, at(0.80f) + ts.chestTilt, 0.0f}, chestRx, chestRz, 0.0f});
        sections.push_back({{0.0f, at(0.93f) + ts.chestTilt * 0.6f, 0.0f},
                            chestW * 0.5f * (0.90f + 0.14f * ts.chestUpperCurve),
                            chestDepth * (0.80f + 0.12f * ts.chestUpperCurve), 0.0f});
        sections.push_back({{0.0f, at(1.00f), 0.0f}, shoulderW * 0.42f, chestDepth * 0.58f, 0.0f});
        part.mesh = generateSplineLoft(sections, radial, splineSamples);
        part.path = {joints.hips, joints.spine, joints.chest, joints.neck};
        part.pathBones = {BoneHips, BoneSpine, BoneChest, BoneNeck};
        parts.push_back(part);
    }

    // ---- Head, face and hair (M10 base; M11-E replaces with FaceGenerator). ----
    {
        Part head;
        head.materialId = 0;
        const Vec3 headRadii{p.headWidth * scale * 0.5f, p.headHeight * scale * 0.5f,
                             p.headWidth * scale * 0.42f};
        head.mesh = translated(generateEllipsoid(headRadii, radial, rings), joints.head);
        head.path = {joints.head};
        head.pathBones = {BoneHead};
        parts.push_back(head);

        Part hair;
        hair.materialId = 1;
        hair.mesh = translated(generateEllipsoid(
            headRadii * 1.09f + Vec3{0.003f, 0.006f, 0.003f}, radial, rings),
            joints.head + Vec3{0.0f, headRadii.y * 0.24f, -headRadii.z * 0.16f});
        hair.path = {joints.head};
        hair.pathBones = {BoneHead};
        parts.push_back(hair);

        Part fringe;
        fringe.materialId = 1;
        fringe.mesh = translated(generateEllipsoid(
            Vec3{p.headWidth * scale * 0.47f, p.headHeight * scale * 0.12f,
                 p.headWidth * scale * 0.14f}, radial, rings / 2),
            joints.head + Vec3{0.0f, headRadii.y * 0.52f, headRadii.z * 0.58f});
        fringe.path = {joints.head};
        fringe.pathBones = {BoneHead};
        parts.push_back(fringe);

        const float headH = p.headHeight * scale;
        const float eyeRadius = p.headWidth * scale * 0.095f;
        for (int side = -1; side <= 1; side += 2) {
            const Vec3 eyeCenter{
                side * p.headWidth * scale * 0.21f,
                joints.head.y + headH * 0.02f,
                headRadii.z * 0.86f
            };
            Part eye;
            eye.materialId = 3;
            eye.mesh = translated(generateUvSphere(eyeRadius, radial, rings / 2), eyeCenter);
            eye.path = {eyeCenter};
            eye.pathBones = {BoneHead};
            parts.push_back(eye);

            Part iris;
            iris.materialId = 4;
            iris.mesh = translated(generateEllipsoid(
                Vec3{eyeRadius * 0.58f, eyeRadius * 0.58f, eyeRadius * 0.35f}, radial, rings / 2),
                eyeCenter + Vec3{0.0f, 0.0f, eyeRadius * 0.80f});
            iris.path = {eyeCenter};
            iris.pathBones = {BoneHead};
            parts.push_back(iris);

            Part glint;
            glint.materialId = 3;
            glint.mesh = translated(generateUvSphere(eyeRadius * 0.18f, 10, 6),
                eyeCenter + Vec3{-side * eyeRadius * 0.28f, eyeRadius * 0.38f, eyeRadius * 0.97f});
            glint.path = {eyeCenter};
            glint.pathBones = {BoneHead};
            parts.push_back(glint);

            Part brow;
            brow.materialId = 1;
            brow.mesh = translated(generateEllipsoid(
                Vec3{eyeRadius * 0.90f, eyeRadius * 0.22f, eyeRadius * 0.28f}, radial, rings / 2),
                eyeCenter + Vec3{0.0f, eyeRadius * 1.55f, -eyeRadius * 0.25f});
            brow.path = {eyeCenter};
            brow.pathBones = {BoneHead};
            parts.push_back(brow);
        }

        Part nose;
        nose.materialId = 0;
        nose.mesh = translated(generateEllipsoid(
            Vec3{p.headWidth * scale * 0.055f, p.headWidth * scale * 0.070f,
                 p.headWidth * scale * 0.080f}, radial / 2, rings / 2),
            joints.head + Vec3{0.0f, -headH * 0.10f, headRadii.z * 0.94f});
        nose.path = {joints.head};
        nose.pathBones = {BoneHead};
        parts.push_back(nose);

        Part mouth;
        mouth.materialId = 4;
        mouth.mesh = translated(generateEllipsoid(
            Vec3{p.headWidth * scale * 0.105f, p.headWidth * scale * 0.024f,
                 p.headWidth * scale * 0.030f}, radial / 2, rings / 2),
            joints.head + Vec3{0.0f, -headH * 0.30f, headRadii.z * 0.90f});
        mouth.path = {joints.head};
        mouth.pathBones = {BoneHead};
        parts.push_back(mouth);
    }

    // ---- Neck: embedded spline sweep with taper (M11). ----
    {
        Part part;
        part.materialId = 0;
        const NeckShape& ns = p.neckShape;
        const float neckRadius = p.neckLength * scale * ns.radiusFactor;
        const Vec3 chestTop = joints.chest + Vec3{0.0f, (joints.neck.y - joints.chest.y) * 0.55f, 0.0f};
        const Vec3 headBase = joints.head - Vec3{0.0f, p.headHeight * scale * 0.30f, 0.0f};
        const std::vector<SweepRing> rings = {
            {chestTop + Vec3{0.0f, -0.01f * scale, 0.0f}, neckRadius * 1.12f, {1.0f, 1.0f}, 0.0f},
            {joints.neck, neckRadius, {1.0f, 1.0f}, 0.0f},
            {headBase, neckRadius * ns.taper, {1.0f, 1.0f}, 0.0f},
        };
        part.mesh = generateSplineSweep(rings, radial, splineSamples, false, false, false);
        part.path = {chestTop, joints.neck, headBase};
        part.pathBones = {BoneChest, BoneNeck, BoneHead};
        parts.push_back(part);
    }

    // ---- Arms: 6-ring embedded spline sweeps with anatomy profile (M11). ----
    auto addArm = [&](bool left) {
        const int side = left ? 1 : -1;
        const ArmShape& as = p.armShape;
        const Vec3 shoulder = left ? joints.leftShoulder : joints.rightShoulder;
        const Vec3 elbow = left ? joints.leftElbow : joints.rightElbow;
        const Vec3 wrist = left ? joints.leftWrist : joints.rightWrist;
        const Vec3 inward = Vec3{-side * 0.05f * scale, 0.015f * scale, 0.01f * scale};
        const Vec3 biceps = (shoulder + elbow) * 0.5f + Vec3{0.0f, 0.0f, -0.004f * scale};
        const Vec3 forearmMid = (elbow + wrist) * 0.5f + Vec3{0.0f, 0.0f, as.forearmCurve * side};
        const float armR = p.armRadius * scale;
        const std::vector<SweepRing> rings = {
            {shoulder + inward, armR * as.deltoidVolume * 0.92f, {1.0f, 0.95f}, 0.0f},
            {shoulder + Vec3{side * 0.008f * scale, 0.004f * scale, 0.0f},
             armR * as.deltoidVolume, {1.05f, 0.95f}, 0.0f},
            {biceps, armR * as.upperArmWidth, {1.0f, 0.92f}, 0.0f},
            {elbow, armR * as.elbowShape, {0.98f, 0.94f}, 0.0f},
            {forearmMid, armR * 1.06f, {1.0f, as.forearmEllipse}, 0.0f},
            {wrist, armR * as.wristPinch, {1.0f, 0.95f}, 0.0f},
        };
        Part part;
        part.materialId = 2;
        part.mesh = generateSplineSweep(rings, radial, splineSamples, false, false, false);
        part.path = {shoulder + Vec3{side * -0.02f * scale, 0.02f * scale, 0.0f},
                     shoulder, elbow, wrist,
                     left ? joints.leftHand : joints.rightHand};
        part.pathBones = {left ? BoneLeftUpperArm : BoneRightUpperArm,
                          left ? BoneLeftUpperArm : BoneRightUpperArm,
                          left ? BoneLeftLowerArm : BoneRightLowerArm,
                          left ? BoneLeftLowerArm : BoneRightLowerArm,
                          left ? BoneLeftHand : BoneRightHand};
        parts.push_back(part);
    };
    addArm(true);
    addArm(false);

    // ---- Legs: 5-ring embedded spline sweeps (thigh/knee/calf/ankle). ----
    auto addLeg = [&](bool left) {
        const LegShape& ls = p.legShape;
        const int side = left ? 1 : -1;
        const Vec3 hip = left ? joints.leftHip : joints.rightHip;
        const Vec3 knee = left ? joints.leftKnee : joints.rightKnee;
        const Vec3 ankle = left ? joints.leftAnkle : joints.rightAnkle;
        const Vec3 thighMid = (hip + knee) * 0.5f + Vec3{0.004f * scale, 0.0f, 0.0f};
        const Vec3 calfMid = (knee + ankle) * 0.45f + Vec3{0.0f, 0.0f, -ls.calfCurve};
        const float legR = p.legRadius * scale;
        const std::vector<SweepRing> rings = {
            {hip + Vec3{-side * 0.055f * scale, 0.10f * scale, 0.0f},
             legR * 1.05f, {0.88f, 0.92f}, 0.0f},
            {thighMid, legR * ls.thighVolume, {1.0f, ls.legEllipse}, 0.0f},
            {knee, legR * ls.kneeShape, {0.98f, 0.95f}, 0.0f},
            {calfMid, legR * ls.calfVolume, {0.95f, 1.02f}, 0.0f},
            {ankle + Vec3{0.0f, 0.02f * scale, 0.0f}, legR * ls.anklePinch, {0.90f, 0.92f}, 0.0f},
        };
        Part part;
        part.materialId = 2;
        part.mesh = generateSplineSweep(rings, radial, splineSamples, false, false, false);
        part.path = {hip + Vec3{0.0f, 0.03f * scale, 0.0f}, hip, knee, ankle,
                     left ? joints.leftToe : joints.rightToe};
        part.pathBones = {left ? BoneLeftUpperLeg : BoneRightUpperLeg,
                          left ? BoneLeftUpperLeg : BoneRightUpperLeg,
                          left ? BoneLeftLowerLeg : BoneRightLowerLeg,
                          left ? BoneLeftLowerLeg : BoneRightLowerLeg,
                          left ? BoneLeftFoot : BoneRightFoot};
        parts.push_back(part);
    };
    addLeg(true);
    addLeg(false);

    // ---- Hands (mitten + thumb) and feet. ----
    auto addHand = [&](const Vec3& wrist, const Vec3& hand, bool left) {
        const int side = left ? 1 : -1;
        Part part;
        part.materialId = 0;
        const Vec3 radii{0.062f * scale * p.handScale, 0.092f * scale * p.handScale,
                         0.038f * scale * p.handScale};
        part.mesh = translated(generateEllipsoid(radii, radial, radial / 2), hand);
        part.path = {wrist, hand};
        part.pathBones = {left ? BoneLeftLowerArm : BoneRightLowerArm,
                          left ? BoneLeftHand : BoneRightHand};
        parts.push_back(part);

        Part thumb;
        thumb.materialId = 0;
        thumb.mesh = translated(generateEllipsoid(
            Vec3{0.030f * scale * p.handScale, 0.046f * scale * p.handScale,
                 0.030f * scale * p.handScale}, radial / 2, radial / 2),
            hand + Vec3{-side * 0.052f * scale * p.handScale, 0.012f * scale, 0.012f * scale});
        thumb.path = {hand};
        thumb.pathBones = {left ? BoneLeftHand : BoneRightHand};
        parts.push_back(thumb);
    };
    addHand(joints.leftWrist, joints.leftHand, true);
    addHand(joints.rightWrist, joints.rightHand, false);

    auto addFoot = [&](const Vec3& ankle, const Vec3& toe, bool left) {
        Part part;
        part.materialId = 2;
        const Vec3 center = ankle + Vec3{0.0f, -kFootHeight * scale * 0.4f,
                                        p.footLength * scale * 0.22f};
        part.mesh = translated(generateRoundedBox(
            {p.footWidth * scale, kFootHeight * scale, p.footLength * scale},
            kFootHeight * scale * 0.35f, p.detailLevel + 1), center);
        part.path = {ankle, toe};
        part.pathBones = {left ? BoneLeftFoot : BoneRightFoot,
                          left ? BoneLeftToe : BoneRightToe};
        parts.push_back(part);
    };
    addFoot(joints.leftAnkle, joints.leftToe, true);
    addFoot(joints.rightAnkle, joints.rightToe, false);

    for (Part& part : parts) {
        weights::PartSpec spec;
        spec.mesh = &part.mesh;
        spec.path = &part.path;
        spec.pathBones = &part.pathBones;
        spec.materialId = part.materialId;
        weights::assignWeights(spec, result.skeleton, weightHeatmap);
    }

    MeshData combined;
    for (const Part& part : parts) {
        const uint32_t firstVertex = static_cast<uint32_t>(combined.vertices.size());
        const uint32_t firstIndex = static_cast<uint32_t>(combined.indices.size());
        combined.vertices.insert(combined.vertices.end(), part.mesh.vertices.begin(), part.mesh.vertices.end());
        for (uint32_t index : part.mesh.indices) combined.indices.push_back(firstVertex + index);
        SubMesh sub;
        sub.firstIndex = firstIndex;
        sub.indexCount = static_cast<uint32_t>(part.mesh.indices.size());
        sub.materialId = part.materialId;
        combined.subMeshes.push_back(sub);
    }
    Aabb bounds;
    for (const Vertex& vertex : combined.vertices) bounds.expand(vertex.position);
    combined.bounds = bounds;
    result.mesh = combined;
    result.valid = !combined.vertices.empty() && !combined.indices.empty();

    const auto genEnd = std::chrono::steady_clock::now();
    PAE_LOGI("PAE_GEOMETRY", "generateHumanoid: %u verts, %u tris, %.2f ms, detail=%u",
             static_cast<unsigned>(result.mesh.vertices.size()),
             static_cast<unsigned>(result.mesh.indices.size() / 3),
             std::chrono::duration<float, std::milli>(genEnd - genStart).count(),
             static_cast<unsigned>(p.detailLevel));
    return result;
}

} // namespace pae
