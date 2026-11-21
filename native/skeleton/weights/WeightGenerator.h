#pragma once

// M11-D: region-aware skinning weights with cross-part continuity.
//
// Primary influence candidates come from the part centerline path; skeleton
// hierarchy neighbours (parents/children of the path bones) extend the
// candidate set so vertices near part boundaries blend continuously into the
// neighbouring region instead of snapping at the part seam. Weights use
// inverse-square distance over the closest bone segment, keep the strongest
// four influences and renormalize.

#include "procedural/MeshData.h"
#include "skeleton/Skeleton.h"

#include <vector>

namespace pae::weights {

struct PartSpec {
    MeshData* mesh = nullptr;
    const std::vector<Vec3>* path = nullptr;          // centerline in bind space
    const std::vector<uint16_t>* pathBones = nullptr; // bones along the centerline
    uint16_t materialId = 0;
};

// Palette colour for a bone in heatmap mode.
Vec3 boneColor(uint16_t bone);

void assignWeights(PartSpec& part, const Skeleton& skeleton, bool heatmap);

} // namespace pae::weights
