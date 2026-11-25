#pragma once

// M12-A/M13: unified upper-body generator — the composed anatomy SDF field is
// extracted into ONE continuous, watertight mesh (Surface Nets), weights are
// derived from the region fields, and a Human Anatomy Report is produced.
// This is the production path (not a PoC): the SDF IS the body surface.
// M13: the sampling volume is anisotropic and the dual-lattice quad
// connectivity is retained (surfaceQuads) for topology analysis.

#include "core/math/Math.h"
#include "procedural/MeshData.h"
#include "procedural/humanoid/anatomy/AnatomyField.h"
#include "procedural/humanoid/Humanoid.h"
#include "skeleton/Skeleton.h"

#include <array>
#include <cstdint>
#include <vector>

namespace pae::anatomy {

struct AnatomyResolution {
    int x = 88;  // cells across the body width
    int y = 132; // cells across the body height
    int z = 52;  // cells across the body depth
};

struct AnatomyResult {
    MeshData mesh;      // ONE continuous upper-body surface (bind pose)
    Skeleton skeleton;  // standard 22-bone protocol
    BodyJoints joints;
    // Dual-lattice quad faces of the extraction (v0..v3). The rendered mesh is
    // triangulated, but these are what a topology analyzer can measure for
    // quad-ness / edge flow.
    std::vector<std::array<uint32_t, 4>> surfaceQuads;
    bool valid = false;
    // True only when the surface is a single watertight, manifold body with
    // valid normals. This is the body acceptance criterion; `valid` alone
    // only says the extraction produced something renderable.
    bool topologyClean = false;
};

// Generates the unified upper body (hips -> head, arms to the wrists) as a
// single extracted surface aligned with the 22-bone bind skeleton.
AnatomyResult generateAnatomyUpperBody(const HumanAnatomyParameters& params,
                                       const BodyParameters& bodyParams,
                                       const AnatomyResolution& resolution = {});

} // namespace pae::anatomy
