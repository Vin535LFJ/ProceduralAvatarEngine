#pragma once

// M11-E: layered procedural hair. Cap + bangs + side strands + back strands
// built from spline sweeps with elliptical tapering sections. Three presets
// (short / medium / long); every parameter participates in geometry.

#include "core/math/Math.h"
#include "procedural/MeshData.h"

#include <vector>

namespace pae::hair {

struct HairParams {
    int style = 0;            // 0 short, 1 medium, 2 long
    int strandCount = 30;     // layered hair clumps (short, textured)
    float strandWidth = 0.012f;
    float bend = 0.42f;       // outward/downward bend strength
    float backLength = 0.085f; // back hair drop length (m), style-dependent
};

struct HairPiece {
    MeshData mesh;
    Vec3 center{0.0f, 0.0f, 0.0f};
    uint16_t materialId = 1; // hair material
};

// headWidth/headHeight/headDepth shape the scalp; pieces anchor the head bone.
std::vector<HairPiece> generateHair(const HairParams& params,
                                    Vec3 headCenter, float headWidth,
                                    float headHeight, float headDepth,
                                    uint32_t radial);

} // namespace pae::hair
