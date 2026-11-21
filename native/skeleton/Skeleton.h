#pragma once

#include "core/math/Math.h"

#include <array>
#include <cstdint>

namespace pae {

enum BoneId : uint16_t {
    BoneRoot = 0,
    BoneHips = 1,
    BoneSpine = 2,
    BoneChest = 3,
    BoneNeck = 4,
    BoneHead = 5,
    BoneLeftShoulder = 6,
    BoneLeftUpperArm = 7,
    BoneLeftLowerArm = 8,
    BoneLeftHand = 9,
    BoneRightShoulder = 10,
    BoneRightUpperArm = 11,
    BoneRightLowerArm = 12,
    BoneRightHand = 13,
    BoneLeftUpperLeg = 14,
    BoneLeftLowerLeg = 15,
    BoneLeftFoot = 16,
    BoneLeftToe = 17,
    BoneRightUpperLeg = 18,
    BoneRightLowerLeg = 19,
    BoneRightFoot = 20,
    BoneRightToe = 21,
    BoneCount = 22
};

struct BodyJoints {
    Vec3 root{0.0f, 0.0f, 0.0f};
    Vec3 hips{0.0f, 0.9f, 0.0f};
    Vec3 spine{0.0f, 1.1f, 0.0f};
    Vec3 chest{0.0f, 1.35f, 0.0f};
    Vec3 neck{0.0f, 1.5f, 0.0f};
    Vec3 head{0.0f, 1.62f, 0.0f};
    Vec3 leftShoulder{0.2f, 1.35f, 0.0f};
    Vec3 leftElbow{0.4f, 1.1f, 0.0f};
    Vec3 leftWrist{0.5f, 0.85f, 0.0f};
    Vec3 leftHand{0.53f, 0.78f, 0.0f};
    Vec3 rightShoulder{-0.2f, 1.35f, 0.0f};
    Vec3 rightElbow{-0.4f, 1.1f, 0.0f};
    Vec3 rightWrist{-0.5f, 0.85f, 0.0f};
    Vec3 rightHand{-0.53f, 0.78f, 0.0f};
    Vec3 leftHip{0.1f, 0.9f, 0.0f};
    Vec3 leftKnee{0.11f, 0.5f, 0.0f};
    Vec3 leftAnkle{0.11f, 0.08f, 0.0f};
    Vec3 leftToe{0.11f, 0.04f, 0.18f};
    Vec3 rightHip{-0.1f, 0.9f, 0.0f};
    Vec3 rightKnee{-0.11f, 0.5f, 0.0f};
    Vec3 rightAnkle{-0.11f, 0.08f, 0.0f};
    Vec3 rightToe{-0.11f, 0.04f, 0.18f};
};

struct BoneDefinition {
    uint16_t id = 0;
    const char* name = "";
    int16_t parent = -1;
    Vec3 head{};
    Vec3 tail{};
};

struct Skeleton {
    std::array<BoneDefinition, BoneCount> bones{};
    std::array<Mat4, BoneCount> bindLocal{};
    std::array<Mat4, BoneCount> bindGlobal{};
    std::array<Mat4, BoneCount> inverseBind{};
};

Skeleton buildHumanoidSkeleton(const BodyJoints& joints);
const char* boneName(uint16_t id);

} // namespace pae
