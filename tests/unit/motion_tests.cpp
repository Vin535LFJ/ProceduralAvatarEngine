#include "mini_test.hpp"

#include "motion/MotionRuntime.h"
#include "motion/retargeting/Retargeting.h"
#include "procedural/humanoid/Humanoid.h"

using namespace pae;

namespace {

Vec3 xyz(const Vec4& v) { return {v.x, v.y, v.z}; }

PoseFrame makeNormalizedFrame() {
    PoseFrame frame;
    frame.coordinateSpace = CoordinateSpace::NormalizedImage;
    frame.imageWidth = 640;
    frame.imageHeight = 480;
    frame.hasPerson = true;
    frame.poseConfidence = 0.9f;
    frame.landmarkCount = 2;
    PoseLandmark left;
    left.id = LkLeftShoulder;
    left.position = {0.25f, 0.5f, 0.0f};
    left.valid = true;
    PoseLandmark right;
    right.id = LkRightShoulder;
    right.position = {0.75f, 0.5f, 0.0f};
    right.valid = true;
    frame.landmarks[0] = left;
    frame.landmarks[1] = right;
    return frame;
}

} // namespace

PAE_TEST_CASE(synthetic_provider_is_deterministic_and_varies_by_action) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    SyntheticPoseProvider provider(humanoid.joints, humanoid.skeleton);
    EXPECT_TRUE(provider.start());
    provider.setTimestampNs(123456789);
    provider.setAction(1);
    PoseFrame a, b;
    EXPECT_TRUE(provider.pollLatest(a));
    EXPECT_TRUE(provider.pollLatest(b));
    EXPECT_TRUE(a.revision != b.revision);
    EXPECT_TRUE(a.landmarkCount == b.landmarkCount);
    for (uint32_t i = 0; i < a.landmarkCount; ++i) {
        EXPECT_NEAR(a.landmarks[i].position.x, b.landmarks[i].position.x, 1e-6f);
        EXPECT_NEAR(a.landmarks[i].position.y, b.landmarks[i].position.y, 1e-6f);
    }

    provider.setAction(0);
    PoseFrame idle;
    EXPECT_TRUE(provider.pollLatest(idle));
    provider.setAction(1);
    PoseFrame raised;
    EXPECT_TRUE(provider.pollLatest(raised));
    const PoseLandmark* idleWrist = idle.find(LkLeftWrist);
    const PoseLandmark* raisedWrist = raised.find(LkLeftWrist);
    EXPECT_TRUE(idleWrist != nullptr && raisedWrist != nullptr);
    EXPECT_TRUE(raisedWrist->position.y > idleWrist->position.y + 0.15f);
    provider.stop();
}

PAE_TEST_CASE(normalized_coordinate_conversion_handles_mirror) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    const PoseFrame frame = makeNormalizedFrame();
    const PoseFrame local = convertPoseFrameToLocal(frame, humanoid.joints);
    const PoseLandmark* left = local.find(LkLeftShoulder);
    const PoseLandmark* right = local.find(LkRightShoulder);
    EXPECT_TRUE(left != nullptr && right != nullptr);
    EXPECT_TRUE(left->position.x < 0.0f);
    EXPECT_TRUE(right->position.x > 0.0f);

    PoseFrame mirrored = frame;
    mirrored.mirrored = true;
    const PoseFrame flipped = convertPoseFrameToLocal(mirrored, humanoid.joints);
    const PoseLandmark* flippedLeft = flipped.find(LkLeftShoulder);
    EXPECT_TRUE(flippedLeft != nullptr);
    EXPECT_TRUE(flippedLeft->position.x > 0.0f);
}

PAE_TEST_CASE(retarget_recovers_arm_raise_and_leg_raise) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    MotionRuntime runtime;
    EXPECT_TRUE(runtime.initialize(humanoid.skeleton, humanoid.joints));
    int64_t now = 0;
    for (int i = 0; i < 90; ++i) {
        now += 16666666;
        runtime.update(now, 0);
    }
    const PoseResult idlePose = runtime.update(now + 16666666, 0);
    const Vec3 idleWrist = xyz(idlePose.palette[BoneLeftHand].transform(
        Vec4{humanoid.skeleton.bones[BoneLeftHand].head, 1.0f}));
    for (int i = 0; i < 90; ++i) {
        now += 16666666;
        runtime.update(now, 1);
    }
    const PoseResult raisedPose = runtime.update(now + 16666666, 1);
    const Vec3 raisedWrist = xyz(raisedPose.palette[BoneLeftHand].transform(
        Vec4{humanoid.skeleton.bones[BoneLeftHand].head, 1.0f}));
    EXPECT_TRUE(raisedWrist.y > idleWrist.y + 0.12f);

    runtime.resetFilters();
    for (int i = 0; i < 90; ++i) {
        now += 16666666;
        runtime.update(now, 5);
    }
    const PoseResult legPose = runtime.update(now + 16666666, 5);
    const Vec3 ankle = xyz(legPose.palette[BoneLeftFoot].transform(
        Vec4{humanoid.skeleton.bones[BoneLeftFoot].head, 1.0f}));
    EXPECT_TRUE(ankle.z > humanoid.joints.leftAnkle.z + 0.05f);
}

namespace {

PoseFrame makeRaisedArmFrame(int64_t timestampNs) {
    PoseFrame frame;
    frame.coordinateSpace = CoordinateSpace::NormalizedImage;
    frame.timestampNs = timestampNs;
    frame.imageWidth = 640;
    frame.imageHeight = 480;
    frame.hasPerson = true;
    frame.poseConfidence = 0.95f;
    frame.landmarkCount = 3;
    PoseLandmark leftShoulder;
    leftShoulder.id = LkLeftShoulder;
    leftShoulder.position = {0.25f, 0.5f, 0.0f};
    leftShoulder.valid = true;
    PoseLandmark leftElbow;
    leftElbow.id = LkLeftElbow;
    leftElbow.position = {0.25f, 0.3f, 0.0f}; // image y grows downward: above shoulder
    leftElbow.valid = true;
    PoseLandmark leftWrist;
    leftWrist.id = LkLeftWrist;
    leftWrist.position = {0.25f, 0.1f, 0.0f};
    leftWrist.valid = true;
    frame.landmarks[0] = leftShoulder;
    frame.landmarks[1] = leftElbow;
    frame.landmarks[2] = leftWrist;
    return frame;
}

bool isNearIdentity(const PoseResult& pose) {
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                if (std::fabs(pose.palette[bone].at(c, r) - (c == r ? 1.0f : 0.0f)) > 2e-2f) {
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace

PAE_TEST_CASE(motion_reset_clears_external_pose_state) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    MotionRuntime runtime;
    EXPECT_TRUE(runtime.initialize(humanoid.skeleton, humanoid.joints));
    runtime.setExternalEnabled(true);

    // Video-style frames with monotonically increasing PTS drive the pose up.
    int64_t pts = 0;
    for (int i = 0; i < 60; ++i) {
        pts += 33333333; // 30 fps video timeline
        runtime.pushExternalFrame(makeRaisedArmFrame(pts));
        const PoseResult pose = runtime.update(pts, -1);
        const Vec3 wrist = xyz(pose.palette[BoneLeftHand].transform(
            Vec4{humanoid.skeleton.bones[BoneLeftHand].head, 1.0f}));
        if (i == 0) {
            EXPECT_TRUE(wrist.y < humanoid.joints.leftWrist.y + 0.3f);
        }
        if (i == 59) {
            EXPECT_TRUE(wrist.y > humanoid.joints.leftWrist.y + 0.1f);
        }
    }

    // Simulated seek: clear the filters, then render without new frames —
    // the palette must return to identity (no stale smoothing residue).
    runtime.resetFilters();
    const PoseResult afterReset = runtime.update(pts + 33333333, -1);
    EXPECT_TRUE(isNearIdentity(afterReset));

    // After reset the next frame re-converges from a clean state.
    for (int i = 0; i < 30; ++i) {
        pts += 33333333;
        runtime.pushExternalFrame(makeRaisedArmFrame(pts));
        runtime.update(pts, -1);
    }
    const PoseResult recovered = runtime.update(pts, -1);
    const Vec3 wrist = xyz(recovered.palette[BoneLeftHand].transform(
        Vec4{humanoid.skeleton.bones[BoneLeftHand].head, 1.0f}));
    EXPECT_TRUE(wrist.y > humanoid.joints.leftWrist.y + 0.05f);
}

PAE_TEST_CASE(dropout_and_low_confidence_are_controlled) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    MotionRuntime runtime;
    EXPECT_TRUE(runtime.initialize(humanoid.skeleton, humanoid.joints));
    int64_t now = 0;
    for (int i = 0; i < 60; ++i) {
        now += 16666666;
        runtime.update(now, 1);
    }
    const uint64_t before = runtime.droppedFrames();
    runtime.setTestHasPerson(false);
    for (int i = 0; i < 180; ++i) {
        now += 16666666;
        runtime.update(now, 1);
    }
    EXPECT_TRUE(runtime.droppedFrames() > before);
    const PoseResult faded = runtime.update(now + 16666666, 1);
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                EXPECT_NEAR(faded.palette[bone].at(c, r), c == r ? 1.0f : 0.0f, 2e-2f);
            }
        }
    }

    MotionRuntime lowConfidence;
    EXPECT_TRUE(lowConfidence.initialize(humanoid.skeleton, humanoid.joints));
    lowConfidence.setTestConfidence(0.1f);
    int64_t other = 0;
    for (int i = 0; i < 60; ++i) {
        other += 16666666;
        lowConfidence.update(other, 1);
    }
    EXPECT_TRUE(lowConfidence.droppedFrames() > 0);
}

int main() {
    return test::runAllTests();
}
