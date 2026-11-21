#pragma once

#include "motion/filtering/OneEuroFilter.h"
#include "motion/poseframe/PoseFrame.h"
#include "skeleton/Skeleton.h"

#include <array>

namespace pae {

using LandmarkFilters = std::array<OneEuroFilter, LandmarkCount * 3>;

// Converts camera/provider coordinates into the engine's metric AvatarLocal space.
PoseFrame convertPoseFrameToLocal(const PoseFrame& frame, const BodyJoints& reference);

// Produces local rotations for the 22-bone skeleton. Filters landmark positions
// before solving directions and applies joint limits.
std::array<Quat, BoneCount> retargetPose(const PoseFrame& localFrame,
                                         const Skeleton& skeleton,
                                         LandmarkFilters& filters,
                                         float dt);

} // namespace pae
