#pragma once

// WP4-HOST-RIG-01 (WP4-C): CPU linear blend skinning driven by Bone Local
// Transforms. Host-only. Every deformation starts from the rest mesh; results
// are never accumulated. First version is LBS only (no DQS / corrective).

#include "ht_common.h"
#include "ht_rig.h"
#include "wp4_rig.h"

#include <string>
#include <vector>

namespace pae::wp4 {

// A rotation about a bone's own rest local axis, applied at its head.
struct LocalRotation {
    std::string bone;   // semantic id
    int axis = 0;       // 0 = local X, 1 = local Y, 2 = local Z
    double angleDeg = 0.0;
};

struct Pose {
    std::string name;
    std::vector<LocalRotation> rotations;
};

// G_pose[b] = G_pose[parent] * restLocal[b] * R_local[b].
std::vector<ht::Affine> poseGlobals(const RigV2& rig, const Pose& pose);
// S[b] = G_pose[b] * restGlobal[b]^-1  (bind pose => identity).
std::vector<ht::Affine> skinMatrices(const RigV2& rig, const std::vector<ht::Affine>& globals);

std::vector<ht::D3> skinPositions(const std::vector<ht::D3>& rest, const ht::SkinWeights& sw,
                                  const std::vector<ht::Affine>& skin);
// Rigid transforms: inverse-transpose = rotation part. Weighted, renormalised.
std::vector<ht::D3> skinNormals(const std::vector<ht::D3>& restNormals, const ht::SkinWeights& sw,
                                const std::vector<ht::Affine>& skin);

// Max |v' - v| over the body when posed at bind (must be ~0).
double bindRecoveryError(const std::vector<ht::D3>& rest, const ht::SkinWeights& sw, const RigV2& rig);

} // namespace pae::wp4