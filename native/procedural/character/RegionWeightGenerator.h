#pragma once

#include "AnatomicalRegions.h"
#include "procedural/MeshData.h"
#include "skeleton/Skeleton.h"

#include <vector>

namespace pae::character {

struct RegionWeightReport {
    bool valid = false;
    size_t vertices = 0;
    size_t invalidVertices = 0;
    size_t crossSideViolations = 0;
    float maxNormalizationError = 0.0f;
};

RegionWeightReport assignRegionWeights(MeshData& mesh,
                                       const std::vector<RegionId>& vertexRegions,
                                       const Skeleton& skeleton,
                                       bool heatmap);

} // namespace pae::character