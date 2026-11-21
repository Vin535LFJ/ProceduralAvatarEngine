#include "Humanoid.h"

#include "procedural/primitives/Primitives.h"
#include "procedural/surfaces/ParametricSurfaces.h"

#include <algorithm>
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

Vec3 boneColor(uint16_t bone) {
    const float r = static_cast<float>(bone % 5) / 4.0f;
    const float g = static_cast<float>((bone / 5) % 5) / 4.0f;
    const float b = static_cast<float>((bone / 25) % 3) / 2.0f;
    return {0.25f + 0.75f * r, 0.25f + 0.75f * g, 0.25f + 0.75f * b};
}

Vec3 materialColor(uint16_t materialId) {
    switch (materialId) {
        case 0: return {0.95f, 0.75f, 0.62f}; // skin
        case 1: return {0.22f, 0.13f, 0.10f}; // hair
        case 2: return {0.25f, 0.42f, 0.72f}; // cloth
        case 3: return {0.97f, 0.97f, 0.92f}; // eye
        default: return {0.8f, 0.8f, 0.8f};
    }
}

void assignWeights(Part& part, bool heatmap) {
    if (part.path.empty() || part.path.size() != part.pathBones.size()) return;
    for (Vertex& vertex : part.mesh.vertices) {
        uint16_t bestBone = part.pathBones[0];
        float bestT = 0.0f;
        float bestDistance = std::numeric_limits<float>::max();
        uint16_t secondBone = bestBone;
        float secondWeight = 0.0f;

        if (part.path.size() == 1) {
            bestBone = part.pathBones[0];
        } else {
            for (size_t i = 0; i + 1 < part.path.size(); ++i) {
                const Vec3 ab = part.path[i + 1] - part.path[i];
                const float abLen2 = ab.lengthSq();
                const float rawT = abLen2 > 1e-10f
                    ? (vertex.position - part.path[i]).dot(ab) / abLen2
                    : 0.0f;
                const float t = clampParam(rawT, 0.0f, 1.0f);
                const Vec3 closest = part.path[i] + ab * t;
                const float distance = (vertex.position - closest).lengthSq();
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestT = t;
                    bestBone = part.pathBones[i];
                    secondBone = part.pathBones[i + 1];
                }
            }
            secondWeight = smoothstep(0.18f, 0.82f, bestT);
        }

        const float primaryWeight = 1.0f - secondWeight;
        vertex.joints = {bestBone, secondBone, 0, 0};
        vertex.weights = {primaryWeight, secondWeight, 0.0f, 0.0f};

        if (heatmap) {
            const Vec3 a = materialColor(0);
            const Vec3 c0 = boneColor(bestBone), c1 = boneColor(secondBone);
            vertex.color = Vec4{
                (c0.x * primaryWeight + c1.x * secondWeight) * 0.8f + a.x * 0.2f,
                (c0.y * primaryWeight + c1.y * secondWeight) * 0.8f + a.y * 0.2f,
                (c0.z * primaryWeight + c1.z * secondWeight) * 0.8f + a.z * 0.2f,
                static_cast<float>(part.materialId)
            };
        } else {
            const Vec3 tint = materialColor(part.materialId);
            const float shade = 0.85f + 0.15f * clampParam(vertex.normal.y * 0.5f + 0.5f, 0.0f, 1.0f);
            vertex.color = {tint.x * shade, tint.y * shade, tint.z * shade,
                            static_cast<float>(part.materialId)};
        }
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
    return p;
}

HumanoidResult generateHumanoid(const BodyParameters& params, bool weightHeatmap) {
    const BodyParameters p = clampBodyParameters(params);
    HumanoidResult result;
    const float estimatedHeight = p.upperLegLength + p.lowerLegLength + kFootHeight +
                                  p.torsoLength + p.neckLength + p.headHeight;
    const float scale = clampParam(p.height / std::fmax(estimatedHeight, 0.1f), 0.5f, 1.8f);
    const BodyJoints joints = deriveJoints(p, scale);
    result.joints = joints;
    result.skeleton = buildHumanoidSkeleton(joints);

    const uint32_t radial = 12u + p.detailLevel * 6u;
    const uint32_t rings = 10u + p.detailLevel * 6u;
    std::vector<Part> parts;

    // Torso: smooth S-curve loft through seven sections (M10: replaces the
    // four-section boxy loft with a rounded chest, tapered shoulder line).
    {
        Part part;
        part.materialId = 2;
        const float hipW = p.hipWidth * scale;
        const float waistW = p.waistWidth * scale;
        const float chestW = p.chestWidth * scale;
        const float shoulderW = p.shoulderWidth * scale;
        const float dt = (joints.spine.y - joints.hips.y) * 0.5f;
        const float dc = (joints.chest.y - joints.spine.y) * 0.5f;
        const float ds = (joints.neck.y - joints.chest.y) * 0.45f;
        std::vector<LoftSection> sections;
        sections.push_back({{0.0f, joints.hips.y, 0.0f}, hipW * 0.50f, hipW * 0.34f});
        sections.push_back({{0.0f, joints.hips.y + dt, 0.0f}, hipW * 0.47f, hipW * 0.35f});
        sections.push_back({{0.0f, joints.spine.y, 0.0f}, waistW * 0.46f, waistW * 0.34f});
        sections.push_back({{0.0f, joints.spine.y + dc, 0.0f}, chestW * 0.48f, chestW * 0.40f});
        sections.push_back({{0.0f, joints.chest.y, 0.0f}, chestW * 0.52f, chestW * 0.44f});
        sections.push_back({{0.0f, joints.chest.y + ds, 0.0f}, chestW * 0.48f, chestW * 0.40f});
        sections.push_back({{0.0f, joints.neck.y, 0.0f}, shoulderW * 0.42f, chestW * 0.30f});
        part.mesh = generateLoftSurface(sections, radial);
        part.path = {joints.hips, joints.spine, joints.chest, joints.neck};
        part.pathBones = {BoneHips, BoneSpine, BoneChest, BoneNeck};
        parts.push_back(part);
    }

    // Head, face and hair (M10: expressive cartoon face + fringe).
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

        // Fringe band across the forehead line.
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

            // Iris+pupil disc (ink material) protruding through the sclera
            // front, with a small highlight glint.
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

            // Eyebrow in hair colour.
            Part brow;
            brow.materialId = 1;
            brow.mesh = translated(generateEllipsoid(
                Vec3{eyeRadius * 0.90f, eyeRadius * 0.22f, eyeRadius * 0.28f}, radial, rings / 2),
                eyeCenter + Vec3{0.0f, eyeRadius * 1.55f, -eyeRadius * 0.25f});
            brow.path = {eyeCenter};
            brow.pathBones = {BoneHead};
            parts.push_back(brow);
        }

        // Nose (subtle) and mouth line.
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

    // Neck.
    {
        Part part;
        part.materialId = 0;
        const float neckRadius = p.neckLength * scale * 0.42f;
        std::vector<Vec3> centerline = {joints.chest, joints.neck, joints.head};
        std::vector<float> radii = {neckRadius * 1.2f, neckRadius, neckRadius * 0.85f};
        part.mesh = generateSweepSurface(centerline, radii, radial);
        part.path = centerline;
        part.pathBones = {BoneChest, BoneNeck, BoneHead};
        parts.push_back(part);
    }

    // Arms and legs with continuous sweeps and smooth taper.
    auto addLimb = [&](const Vec3& a, const Vec3& b, const Vec3& c,
                       float radius, uint16_t bone0, uint16_t bone1, uint16_t bone2) {
        Part part;
        part.materialId = 2;
        part.mesh = generateSweepSurface({a, b, c},
                                         {radius * 1.12f, radius * 0.96f, radius * 0.70f}, radial);
        part.path = {a, b, c};
        part.pathBones = {bone0, bone1, bone2};
        parts.push_back(part);
    };
    addLimb(joints.leftShoulder, joints.leftElbow, joints.leftWrist,
            p.armRadius * scale, BoneLeftUpperArm, BoneLeftLowerArm, BoneLeftHand);
    addLimb(joints.rightShoulder, joints.rightElbow, joints.rightWrist,
            p.armRadius * scale, BoneRightUpperArm, BoneRightLowerArm, BoneRightHand);
    addLimb(joints.leftHip, joints.leftKnee, joints.leftAnkle,
            p.legRadius * scale, BoneLeftUpperLeg, BoneLeftLowerLeg, BoneLeftFoot);
    addLimb(joints.rightHip, joints.rightKnee, joints.rightAnkle,
            p.legRadius * scale, BoneRightUpperLeg, BoneRightLowerLeg, BoneRightFoot);

    // Joint transition volumes prevent visible gaps at large rotations.
    // Radii stay close to the local sweep radius so the spheres blend in
    // instead of bulging (M10).
    auto addJoint = [&](const Vec3& center, float radius, uint16_t bone, uint16_t materialId) {
        Part part;
        part.materialId = materialId;
        part.mesh = translated(generateUvSphere(radius, radial, rings / 2), center);
        part.path = {center};
        part.pathBones = {bone};
        parts.push_back(part);
    };
    addJoint(joints.leftShoulder, p.armRadius * scale * 1.22f, BoneLeftShoulder, 2);
    addJoint(joints.rightShoulder, p.armRadius * scale * 1.22f, BoneRightShoulder, 2);
    addJoint(joints.leftElbow, p.armRadius * scale * 1.02f, BoneLeftLowerArm, 2);
    addJoint(joints.rightElbow, p.armRadius * scale * 1.02f, BoneRightLowerArm, 2);
    addJoint(joints.leftHip, p.legRadius * scale * 1.22f, BoneLeftUpperLeg, 2);
    addJoint(joints.rightHip, p.legRadius * scale * 1.22f, BoneRightUpperLeg, 2);
    addJoint(joints.leftKnee, p.legRadius * scale * 1.04f, BoneLeftLowerLeg, 2);
    addJoint(joints.rightKnee, p.legRadius * scale * 1.04f, BoneRightLowerLeg, 2);

    // Hands (mitten + thumb hint) and feet.
    auto addHand = [&](const Vec3& wrist, const Vec3& hand, bool left) {
        const int side = left ? 1 : -1;
        Part part;
        part.materialId = 0;
        const Vec3 radii{0.062f * scale * p.handScale, 0.092f * scale * p.handScale,
                         0.038f * scale * p.handScale};
        part.mesh = translated(generateEllipsoid(radii, radial, rings / 2), hand);
        part.path = {wrist, hand};
        part.pathBones = {left ? BoneLeftLowerArm : BoneRightLowerArm,
                          left ? BoneLeftHand : BoneRightHand};
        parts.push_back(part);

        Part thumb;
        thumb.materialId = 0;
        thumb.mesh = translated(generateEllipsoid(
            Vec3{0.030f * scale * p.handScale, 0.046f * scale * p.handScale,
                 0.030f * scale * p.handScale}, radial / 2, rings / 2),
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

    for (Part& part : parts) assignWeights(part, weightHeatmap);

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
    return result;
}

} // namespace pae
