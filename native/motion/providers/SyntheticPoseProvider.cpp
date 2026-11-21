#include "SyntheticPoseProvider.h"

#include "skeleton/Skinning.h"

#include <cmath>

namespace pae {
namespace {

Quat zRotation(float degrees) {
    const float half = degrees * 3.14159265f / 360.0f;
    return {0.0f, 0.0f, std::sin(half), std::cos(half)};
}

Quat xRotation(float degrees) {
    const float half = degrees * 3.14159265f / 360.0f;
    return {std::sin(half), 0.0f, 0.0f, std::cos(half)};
}

void addLandmark(PoseFrame& frame, uint16_t id, const Vec3& position, float confidence) {
    PoseLandmark landmark;
    landmark.id = id;
    landmark.position = position;
    landmark.visibility = confidence;
    landmark.presence = confidence;
    landmark.valid = true;
    frame.landmarks[frame.landmarkCount++] = landmark;
}

} // namespace

SyntheticPoseProvider::SyntheticPoseProvider(const BodyJoints& joints, const Skeleton& skeleton)
    : joints_(joints), skeleton_(skeleton) {}

bool SyntheticPoseProvider::start() {
    running_ = true;
    return true;
}

void SyntheticPoseProvider::stop() {
    running_ = false;
}

void SyntheticPoseProvider::rotationsForAction(int action, std::array<Quat, BoneCount>& rotations) {
    rotations.fill(Quat{});
    switch (action) {
        case 1:
            rotations[BoneLeftUpperArm] = zRotation(75.0f);
            break;
        case 2:
            rotations[BoneLeftUpperArm] = zRotation(40.0f);
            rotations[BoneLeftLowerArm] = zRotation(-85.0f);
            break;
        case 3:
            rotations[BoneLeftUpperArm] = zRotation(75.0f);
            rotations[BoneRightUpperArm] = zRotation(-75.0f);
            break;
        case 4:
            rotations[BoneLeftUpperLeg] = xRotation(-25.0f);
            rotations[BoneLeftLowerLeg] = xRotation(55.0f);
            rotations[BoneRightUpperLeg] = xRotation(-25.0f);
            rotations[BoneRightLowerLeg] = xRotation(55.0f);
            break;
        case 5:
            rotations[BoneLeftUpperLeg] = xRotation(-50.0f);
            rotations[BoneLeftLowerLeg] = xRotation(20.0f);
            break;
        default:
            break;
    }
}

bool SyntheticPoseProvider::pollLatest(PoseFrame& out) {
    if (!running_) return false;
    out = PoseFrame{};
    out.timestampNs = timestampNs_;
    out.coordinateSpace = CoordinateSpace::AvatarLocal;
    out.poseConfidence = hasPerson_ ? confidence_ : 0.0f;
    out.hasPerson = hasPerson_;
    out.revision = ++revision_;
    out.imageWidth = 640;
    out.imageHeight = 480;
    if (!hasPerson_) return true;

    std::array<Quat, BoneCount> rotations{};
    rotationsForAction(action_, rotations);
    const PoseResult pose = computePose(skeleton_, rotations);
    auto posedHead = [&](uint16_t bone) {
        const Vec3 bindHead = skeleton_.bones[bone].head;
        const Vec4 transformed = pose.palette[bone].transform(Vec4{bindHead, 1.0f});
        return Vec3{transformed.x, transformed.y, transformed.z};
    };

    addLandmark(out, LkNose, posedHead(BoneHead) + Vec3{0.0f, 0.03f, 0.06f}, confidence_);
    addLandmark(out, LkLeftShoulder, posedHead(BoneLeftShoulder), confidence_);
    addLandmark(out, LkRightShoulder, posedHead(BoneRightShoulder), confidence_);
    addLandmark(out, LkLeftElbow, posedHead(BoneLeftLowerArm), confidence_);
    addLandmark(out, LkRightElbow, posedHead(BoneRightLowerArm), confidence_);
    addLandmark(out, LkLeftWrist, posedHead(BoneLeftHand), confidence_);
    addLandmark(out, LkRightWrist, posedHead(BoneRightHand), confidence_);
    addLandmark(out, LkLeftHip, posedHead(BoneLeftUpperLeg), confidence_);
    addLandmark(out, LkRightHip, posedHead(BoneRightUpperLeg), confidence_);
    addLandmark(out, LkLeftKnee, posedHead(BoneLeftLowerLeg), confidence_);
    addLandmark(out, LkRightKnee, posedHead(BoneRightLowerLeg), confidence_);
    addLandmark(out, LkLeftAnkle, posedHead(BoneLeftFoot), confidence_);
    addLandmark(out, LkRightAnkle, posedHead(BoneRightFoot), confidence_);
    addLandmark(out, LkLeftHeel, posedHead(BoneLeftFoot) + Vec3{0.0f, -0.03f, -0.04f}, confidence_);
    addLandmark(out, LkRightHeel, posedHead(BoneRightFoot) + Vec3{0.0f, -0.03f, -0.04f}, confidence_);
    addLandmark(out, LkLeftFootIndex, posedHead(BoneLeftToe), confidence_);
    addLandmark(out, LkRightFootIndex, posedHead(BoneRightToe), confidence_);
    return true;
}

} // namespace pae
