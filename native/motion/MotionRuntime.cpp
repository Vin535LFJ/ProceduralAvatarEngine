#include "MotionRuntime.h"

#include <algorithm>
#include <cmath>

namespace pae {

bool MotionRuntime::initialize(const Skeleton& skeleton, const BodyJoints& joints) {
    skeleton_ = skeleton;
    joints_ = joints;
    provider_ = SyntheticPoseProvider(joints_, skeleton_);
    resetFilters();
    initialized_ = provider_.start() && external_.start();
    return initialized_;
}

void MotionRuntime::setExternalEnabled(bool enabled) {
    useExternal_ = enabled;
    resetFilters();
}

void MotionRuntime::pushExternalFrame(const PoseFrame& frame) {
    external_.push(frame);
}

void MotionRuntime::resetFilters() {
    for (OneEuroFilter& filter : filters_) filter.reset();
    filtered_.fill(Quat{});
    hasLatest_ = false;
    lastFrameNs_ = 0;
    lastValidNs_ = 0;
    external_.clear(); // a seek/replay must not re-consume the pre-seek frame
}

PoseResult MotionRuntime::update(int64_t timestampNs, int actionMode) {
    if (!initialized_) return {};
    PoseFrame frame;
    bool gotFrame = false;
    if (useExternal_) {
        gotFrame = external_.pollLatest(frame);
    } else {
        const int action = actionMode >= 0
            ? actionMode
            : static_cast<int>((static_cast<uint64_t>(timestampNs) / 2000000000ULL) % 6);
        provider_.setAction(action);
        provider_.setTimestampNs(timestampNs);
        gotFrame = provider_.pollLatest(frame);
    }
    if (!gotFrame) {
        ++droppedFrames_;
        return computePose(skeleton_, filtered_);
    }
    latestFrame_ = frame;
    hasLatest_ = true;

    const float dt = lastFrameNs_ > 0 && timestampNs > lastFrameNs_
        ? static_cast<float>(timestampNs - lastFrameNs_) * 1e-9f
        : 1.0f / 60.0f;
    lastFrameNs_ = timestampNs;

    if (!frame.hasPerson || frame.poseConfidence < 0.25f) {
        ++droppedFrames_;
        const float elapsed = lastValidNs_ > 0 && timestampNs > lastValidNs_
            ? static_cast<float>(timestampNs - lastValidNs_) * 1e-9f
            : 0.0f;
        const float fade = 1.0f - std::exp(-elapsed / 0.8f);
        for (Quat& rotation : filtered_) rotation = Quat::slerp(rotation, Quat{}, fade);
        return computePose(skeleton_, filtered_);
    }

    lastValidNs_ = timestampNs;
    const PoseFrame local = convertPoseFrameToLocal(frame, joints_);
    const std::array<Quat, BoneCount> target = retargetPose(local, skeleton_, filters_, dt);
    const float confidence = std::clamp(frame.poseConfidence, 0.0f, 1.0f);
    const float alpha = 1.0f - std::exp(-dt * (2.0f + 8.0f * confidence));
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        filtered_[bone] = Quat::slerp(filtered_[bone], target[bone], alpha);
    }
    return computePose(skeleton_, filtered_);
}

} // namespace pae
