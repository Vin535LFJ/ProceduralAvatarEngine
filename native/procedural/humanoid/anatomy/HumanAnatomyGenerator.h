#pragma once

// M12-A: unified upper-body generator — the composed anatomy SDF field is
// extracted into ONE continuous, watertight mesh (Surface Nets), weights are
// derived from the region fields, and a Human Anatomy Report is produced.
// This is the production path (not a PoC): the SDF IS the body surface.

#include "core/math/Math.h"
#include "procedural/MeshData.h"
#include "procedural/humanoid/anatomy/AnatomyField.h"
#include "procedural/humanoid/Humanoid.h"
#include "skeleton/Skeleton.h"

namespace pae::anatomy {

struct AnatomyResolution {
    int x = 72;  // cells across the body width
    int y = 112; // cells across the body height
    int z = 44;  // cells across the body depth
};

struct AnatomyResult {
    MeshData mesh;      // ONE continuous upper-body surface (bind pose)
    Skeleton skeleton;  // standard 22-bone protocol
    BodyJoints joints;
    bool valid = false;
};

// Generates the unified upper body (hips -> head, arms to the wrists) as a
// single extracted surface aligned with the 22-bone bind skeleton.
AnatomyResult generateAnatomyUpperBody(const HumanAnatomyParameters& params,
                                       const BodyParameters& bodyParams,
                                       const AnatomyResolution& resolution = {});

} // namespace pae::anatomy
