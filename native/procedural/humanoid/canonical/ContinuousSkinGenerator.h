#pragma once

#include "procedural/humanoid/Humanoid.h"
#include "procedural/humanoid/canonical/CanonicalSkinGenerator.h"
#include "skeleton/Skeleton.h"

namespace pae::canonical {

struct ContinuousSkinResult {
    CanonicalSkinResult canonical;
    Skeleton skeleton;
    BodyJoints joints;
    double generationMs = 0.0;
    double extractionMs = 0.0;
    double weightMs = 0.0;
    double selfIntersectionMs = 0.0;
    size_t repairedBoundaryHoles = 0;
};

ContinuousSkinResult generateContinuousSkin(const BodyParameters& params, bool weightHeatmap = false);

} // namespace pae::canonical
