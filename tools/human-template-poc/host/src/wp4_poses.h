#pragma once

// WP4-HOST-RIG-01 (WP4-D): static pose suite expressed as Bone Local Transforms.
// No hand-edited meshes: every pose is a deterministic set of local rotations.
// Signs are side-aware and verified by direction assertions in the runner.

#include "wp4_skin.h"

#include <string>
#include <vector>

namespace pae::wp4 {

// The full required suite (plan §3). Angles in degrees.
std::vector<Pose> poseSuite();

// A representative subset used for the 9-shape sync sweep (cheap).
std::vector<Pose> syncSubset();

// Core-vs-Full comparison poses (plan §6).
std::vector<Pose> comparisonPoses();

const Pose* findPose(const std::vector<Pose>& suite, const std::string& name);

} // namespace pae::wp4