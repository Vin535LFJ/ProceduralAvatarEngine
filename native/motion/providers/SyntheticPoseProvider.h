#pragma once

#include "motion/providers/IPoseProvider.h"

#include <array>

namespace pae {

// Deterministic provider used when no camera/SDK is available.
// 0 idle, 1 left arm raise, 2 left elbow bend, 3 both arms horizontal,
// 4 squat/knee bend, 5 left leg raise.
class SyntheticPoseProvider : public IPoseProvider {
public:
    SyntheticPoseProvider() = default;
    SyntheticPoseProvider(const BodyJoints& joints, const Skeleton& skeleton);

    bool start() override;
    void stop() override;
    bool pollLatest(PoseFrame& out) override;

    void setAction(int action) { action_ = action; }
    void setTimestampNs(int64_t timestampNs) { timestampNs_ = timestampNs; }
    void setConfidence(float confidence) { confidence_ = confidence; }
    void setHasPerson(bool hasPerson) { hasPerson_ = hasPerson; }
    static void rotationsForAction(int action, std::array<Quat, BoneCount>& rotations);

private:
    BodyJoints joints_;
    Skeleton skeleton_;
    int action_ = 0;
    int64_t timestampNs_ = 0;
    float confidence_ = 0.95f;
    bool hasPerson_ = true;
    bool running_ = false;
    uint64_t revision_ = 0;
};

} // namespace pae
