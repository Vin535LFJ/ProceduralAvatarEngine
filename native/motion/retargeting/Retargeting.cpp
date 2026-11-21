#include "Retargeting.h"

#include <algorithm>
#include <cmath>

namespace pae {
namespace {

float clampFloat(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

Quat constrainJoint(uint16_t bone, const Quat& rotation) {
    Quat q = rotation.normalized();
    float angle = 2.0f * std::acos(clampFloat(q.w, -1.0f, 1.0f));
    if (angle > 3.14159265f) angle = 2.0f * 3.14159265f - angle;
    Vec3 axis{q.x, q.y, q.z};
    const float axisLength = axis.length();
    if (axisLength < 1e-5f) return {};
    axis = axis * (1.0f / axisLength);

    float maxAngle = 3.14159265f;
    bool hingeZ = false;
    bool hingeX = false;
    switch (bone) {
        case BoneLeftLowerArm:
        case BoneRightLowerArm:
            maxAngle = 2.6f; hingeZ = true; break;
        case BoneLeftLowerLeg:
        case BoneRightLowerLeg:
            maxAngle = 2.4f; hingeX = true; break;
        case BoneLeftUpperArm:
        case BoneRightUpperArm:
            maxAngle = 1.9f; break;
        case BoneLeftUpperLeg:
        case BoneRightUpperLeg:
            maxAngle = 1.6f; break;
        case BoneSpine:
        case BoneChest:
        case BoneNeck:
            maxAngle = 0.5f; break;
        default:
            break;
    }
    if (hingeZ) {
        axis = std::fabs(axis.z) > 0.2f ? Vec3{0.0f, 0.0f, axis.z > 0.0f ? 1.0f : -1.0f} : axis;
    } else if (hingeX) {
        axis = std::fabs(axis.x) > 0.2f ? Vec3{axis.x > 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f} : axis;
    }
    const float limited = std::fmin(angle, maxAngle);
    const float half = limited * 0.5f;
    return {axis.x * std::sin(half), axis.y * std::sin(half),
            axis.z * std::sin(half), std::cos(half)};
}

} // namespace

PoseFrame convertPoseFrameToLocal(const PoseFrame& frame, const BodyJoints& reference) {
    PoseFrame result = frame;
    if (frame.coordinateSpace == CoordinateSpace::AvatarLocal ||
        frame.coordinateSpace == CoordinateSpace::EngineWorld) {
        return result;
    }
    result.coordinateSpace = CoordinateSpace::AvatarLocal;
    if (frame.coordinateSpace != CoordinateSpace::NormalizedImage &&
        frame.coordinateSpace != CoordinateSpace::PixelImage) {
        return result;
    }
    const float width = static_cast<float>(std::max(frame.imageWidth, 1u));
    const float height = static_cast<float>(std::max(frame.imageHeight, 1u));
    const float shoulderSpan = std::fabs(reference.leftShoulder.x - reference.rightShoulder.x);
    const float torso = std::fabs(reference.chest.y - reference.hips.y);
    for (uint32_t i = 0; i < frame.landmarkCount; ++i) {
        PoseLandmark& landmark = result.landmarks[i];
        if (!landmark.valid) continue;
        float nx = frame.coordinateSpace == CoordinateSpace::PixelImage
                 ? landmark.position.x / width : landmark.position.x;
        float ny = frame.coordinateSpace == CoordinateSpace::PixelImage
                 ? landmark.position.y / height : landmark.position.y;
        if (frame.mirrored) nx = 1.0f - nx;
        landmark.position = {
            (nx - 0.5f) * shoulderSpan * 2.1f,
            (0.5f - ny) * torso * 2.2f + reference.hips.y,
            landmark.position.z * 0.25f
        };
    }
    return result;
}

std::array<Quat, BoneCount> retargetPose(const PoseFrame& frame,
                                         const Skeleton& skeleton,
                                         LandmarkFilters& filters,
                                         float dt) {
    std::array<Quat, BoneCount> result{};
    if (!frame.hasPerson || frame.landmarkCount == 0) return result;

    const float confidence = clampFloat(frame.poseConfidence, 0.0f, 1.0f);
    std::array<Vec3, LandmarkCount> positions{};
    std::array<bool, LandmarkCount> valid{};
    for (const PoseLandmark& landmark : frame.landmarks) {
        if (!landmark.valid || landmark.id >= LandmarkCount) continue;
        const float weight = 0.4f + 0.6f * confidence * landmark.visibility;
        (void)weight;
        positions[landmark.id] = {
            filters[landmark.id * 3 + 0].filter(landmark.position.x, dt),
            filters[landmark.id * 3 + 1].filter(landmark.position.y, dt),
            filters[landmark.id * 3 + 2].filter(landmark.position.z, dt)
        };
        valid[landmark.id] = true;
    }

    auto direction = [&](uint16_t from, uint16_t to, Vec3& out) {
        if (!valid[from] || !valid[to]) return false;
        const Vec3 delta = positions[to] - positions[from];
        if (delta.lengthSq() < 1e-8f) return false;
        out = delta.normalized();
        return true;
    };

    std::array<Quat, BoneCount> worldTargets{};
    std::array<bool, BoneCount> hasTarget{};
    auto addPair = [&](uint16_t bone, uint16_t from, uint16_t to) {
        Vec3 target;
        if (!direction(from, to, target)) return;
        const BoneDefinition& def = skeleton.bones[bone];
        const Vec3 bindDirection = (def.tail - def.head).normalized();
        worldTargets[bone] = Quat::fromToRotation(bindDirection, target);
        hasTarget[bone] = true;
    };

    addPair(BoneLeftUpperArm, LkLeftShoulder, LkLeftElbow);
    addPair(BoneLeftLowerArm, LkLeftElbow, LkLeftWrist);
    addPair(BoneRightUpperArm, LkRightShoulder, LkRightElbow);
    addPair(BoneRightLowerArm, LkRightElbow, LkRightWrist);
    addPair(BoneLeftUpperLeg, LkLeftHip, LkLeftKnee);
    addPair(BoneLeftLowerLeg, LkLeftKnee, LkLeftAnkle);
    addPair(BoneLeftFoot, LkLeftAnkle, LkLeftFootIndex);
    addPair(BoneRightUpperLeg, LkRightHip, LkRightKnee);
    addPair(BoneRightLowerLeg, LkRightKnee, LkRightAnkle);
    addPair(BoneRightFoot, LkRightAnkle, LkRightFootIndex);

    if (valid[LkLeftHip] && valid[LkRightHip] && valid[LkLeftShoulder] && valid[LkRightShoulder]) {
        const Vec3 hipsMid = (positions[LkLeftHip] + positions[LkRightHip]) * 0.5f;
        const Vec3 shouldersMid = (positions[LkLeftShoulder] + positions[LkRightShoulder]) * 0.5f;
        const Vec3 bindSpine = (skeleton.bones[BoneSpine].tail - skeleton.bones[BoneSpine].head).normalized();
        const Vec3 bindChest = (skeleton.bones[BoneChest].tail - skeleton.bones[BoneChest].head).normalized();
        const Vec3 spineDir = shouldersMid - hipsMid;
        if (spineDir.lengthSq() > 1e-8f) {
            worldTargets[BoneSpine] = Quat::fromToRotation(bindSpine, spineDir.normalized());
            hasTarget[BoneSpine] = true;
        }
        if (valid[LkNose]) {
            const Vec3 chestDir = positions[LkNose] - shouldersMid;
            if (chestDir.lengthSq() > 1e-8f) {
                worldTargets[BoneChest] = Quat::fromToRotation(bindChest, chestDir.normalized());
                hasTarget[BoneChest] = true;
            }
        }
    }

    std::array<Quat, BoneCount> worldAccum{};
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        const Quat target = hasTarget[bone] ? worldTargets[bone] : Quat{};
        Quat local = bone == 0 ? target : worldAccum[skeleton.bones[bone].parent].inverse() * target;
        local = constrainJoint(bone, local);
        result[bone] = local;
        const int16_t parent = skeleton.bones[bone].parent;
        worldAccum[bone] = parent >= 0 ? worldAccum[parent] * local : local;
    }
    return result;
}

} // namespace pae
