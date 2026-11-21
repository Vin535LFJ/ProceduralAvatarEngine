#pragma once

#include "motion/providers/ExternalPoseProvider.h"
#include "motion/providers/SyntheticPoseProvider.h"
#include "motion/retargeting/Retargeting.h"
#include "skeleton/Skinning.h"

#include <array>

namespace pae {

// SDK-independent bridge from PoseFrame to the bone palette.
// Holds a synthetic provider by default; a Kotlin/JNI adapter can push frames
// through the same IPoseProvider contract in later phases.
class MotionRuntime {
public:
    bool initialize(const Skeleton& skeleton, const BodyJoints& joints);
    PoseResult update(int64_t timestampNs, int actionMode);
    void resetFilters();
    void setTestConfidence(float confidence) { provider_.setConfidence(confidence); }
    void setTestHasPerson(bool hasPerson) { provider_.setHasPerson(hasPerson); }
    void setExternalEnabled(bool enabled);
    void pushExternalFrame(const PoseFrame& frame);

    const PoseFrame& latestFrame() const { return latestFrame_; }
    bool hasLatestFrame() const { return hasLatest_; }
    uint64_t droppedFrames() const { return droppedFrames_; }

private:
    Skeleton skeleton_;
    BodyJoints joints_;
    SyntheticPoseProvider provider_;
    ExternalPoseProvider external_;
    bool useExternal_ = false;
    LandmarkFilters filters_{};
    std::array<Quat, BoneCount> filtered_{};
    PoseFrame latestFrame_{};
    bool hasLatest_ = false;
    bool initialized_ = false;
    int64_t lastFrameNs_ = 0;
    int64_t lastValidNs_ = 0;
    uint64_t droppedFrames_ = 0;
};

} // namespace pae
