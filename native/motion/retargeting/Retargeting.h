#pragma once

#include "motion/filtering/OneEuroFilter.h"
#include "motion/poseframe/PoseFrame.h"
#include "skeleton/Skeleton.h"

#include <array>

namespace pae {

using LandmarkFilters = std::array<OneEuroFilter, LandmarkCount * 3>;

// M11-H: persistent retargeting state — last accepted landmark positions and
// bone direction targets. Missing landmarks hold their last accepted values
// (short-term) instead of collapsing to identity, and low-confidence
// landmarks shrink toward the held position instead of jumping.
struct RetargetingState {
    std::array<Vec3, LandmarkCount> lastPositions{};
    std::array<int64_t, LandmarkCount> lastValidNs{};
    std::array<Quat, BoneCount> lastTargets{};
    std::array<char, BoneCount> hasTargets{};
    std::array<char, LandmarkCount> everSeen{};

    void reset() {
        lastPositions.fill(Vec3{});
        lastValidNs.fill(0);
        lastTargets.fill(Quat{});
        hasTargets.fill(0);
        everSeen.fill(0);
    }
};

// Converts camera/provider coordinates into the engine's metric AvatarLocal space.
PoseFrame convertPoseFrameToLocal(const PoseFrame& frame, const BodyJoints& reference);

// Produces local rotations for the 22-bone skeleton. Filters landmark
// positions before solving directions and applies joint limits. Confidence
// and landmark visibility gate how far each observation moves the pose;
// missing landmarks hold their last accepted direction for a short window.
std::array<Quat, BoneCount> retargetPose(const PoseFrame& localFrame,
                                         const Skeleton& skeleton,
                                         LandmarkFilters& filters,
                                         float dt, int64_t nowNs,
                                         RetargetingState& state);

} // namespace pae
