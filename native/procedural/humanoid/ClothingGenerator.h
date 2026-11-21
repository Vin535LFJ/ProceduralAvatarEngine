#pragma once

// M11-F: procedural clothing shells. Shirts/pants are independent loft/sweep
// meshes built over the exact body profile rings (shared anatomy functions)
// with thickness + ease, so the cloth follows the body and deforms with it.

#include "procedural/MeshData.h"
#include "procedural/humanoid/Humanoid.h"

#include <vector>

namespace pae::clothing {

struct ClothingPiece {
    MeshData mesh;
    std::vector<Vec3> path;
    std::vector<uint16_t> pathBones;
    uint16_t materialId = 2; // 2 = shirt (cloth), 5 = pants
};

// Generates shirt (body + sleeves + collar) and pants (pelvis + legs).
std::vector<ClothingPiece> generateClothing(const BodyParameters& params,
                                            const BodyJoints& joints, float scale,
                                            const ClothingParams& cp);

} // namespace pae::clothing
