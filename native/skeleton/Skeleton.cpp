#include "Skeleton.h"

namespace pae {
namespace {

const char* const kBoneNames[BoneCount] = {
    "Root", "Hips", "Spine", "Chest", "Neck", "Head",
    "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
    "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
    "LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "LeftToe",
    "RightUpperLeg", "RightLowerLeg", "RightFoot", "RightToe"
};

} // namespace

const char* boneName(uint16_t id) {
    return id < BoneCount ? kBoneNames[id] : "Invalid";
}

Skeleton buildHumanoidSkeleton(const BodyJoints& j) {
    Skeleton skeleton;
    auto set = [&skeleton](uint16_t id, int16_t parent, const Vec3& head, const Vec3& tail) {
        BoneDefinition& bone = skeleton.bones[id];
        bone.id = id;
        bone.name = kBoneNames[id];
        bone.parent = parent;
        bone.head = head;
        bone.tail = tail;
    };

    set(BoneRoot, -1, j.root, j.hips);
    set(BoneHips, BoneRoot, j.hips, j.spine);
    set(BoneSpine, BoneHips, j.spine, j.chest);
    set(BoneChest, BoneSpine, j.chest, j.neck);
    // M11: the neck ends at the skull base; the head stub covers the whole
    // skull so skull vertices are dominated by the head bone. Directions are
    // unchanged (+Y), so retargeting behaviour is identical.
    set(BoneNeck, BoneChest, j.neck, j.neck + (j.head - j.neck) * 0.75f);
    set(BoneHead, BoneNeck, j.head, j.head + (j.head - j.neck) * 1.2f);

    // P0: Shoulder is the clavicle/shoulder-girdle segment from the chest to
    // the glenohumeral joint; UpperArm starts at that joint and ends at the
    // elbow. BoneId ABI is unchanged.
    set(BoneLeftShoulder, BoneChest, j.chest, j.leftShoulder);
    set(BoneLeftUpperArm, BoneLeftShoulder, j.leftShoulder, j.leftElbow);
    set(BoneLeftLowerArm, BoneLeftUpperArm, j.leftElbow, j.leftWrist);
    set(BoneLeftHand, BoneLeftLowerArm, j.leftWrist, j.leftHand);

    set(BoneRightShoulder, BoneChest, j.chest, j.rightShoulder);
    set(BoneRightUpperArm, BoneRightShoulder, j.rightShoulder, j.rightElbow);
    set(BoneRightLowerArm, BoneRightUpperArm, j.rightElbow, j.rightWrist);
    set(BoneRightHand, BoneRightLowerArm, j.rightWrist, j.rightHand);

    set(BoneLeftUpperLeg, BoneHips, j.leftHip, j.leftKnee);
    set(BoneLeftLowerLeg, BoneLeftUpperLeg, j.leftKnee, j.leftAnkle);
    set(BoneLeftFoot, BoneLeftLowerLeg, j.leftAnkle, j.leftToe);
    set(BoneLeftToe, BoneLeftFoot, j.leftToe, j.leftToe + Vec3{0.0f, 0.0f, 0.08f});

    set(BoneRightUpperLeg, BoneHips, j.rightHip, j.rightKnee);
    set(BoneRightLowerLeg, BoneRightUpperLeg, j.rightKnee, j.rightAnkle);
    set(BoneRightFoot, BoneRightLowerLeg, j.rightAnkle, j.rightToe);
    set(BoneRightToe, BoneRightFoot, j.rightToe, j.rightToe + Vec3{0.0f, 0.0f, 0.08f});

    for (uint16_t i = 0; i < BoneCount; ++i) {
        const BoneDefinition& bone = skeleton.bones[i];
        const Vec3 parentHead = bone.parent >= 0 ? skeleton.bones[bone.parent].head : Vec3{};
        skeleton.bindLocal[i] = Mat4::translation(bone.head - parentHead);
    }
    skeleton.bindGlobal[0] = skeleton.bindLocal[0];
    for (uint16_t i = 1; i < BoneCount; ++i) {
        const int16_t parent = skeleton.bones[i].parent;
        skeleton.bindGlobal[i] = skeleton.bindGlobal[parent] * skeleton.bindLocal[i];
    }
    for (uint16_t i = 0; i < BoneCount; ++i) {
        if (!skeleton.bindGlobal[i].inverse(skeleton.inverseBind[i])) {
            skeleton.inverseBind[i] = Mat4{};
        }
    }
    return skeleton;
}

} // namespace pae
