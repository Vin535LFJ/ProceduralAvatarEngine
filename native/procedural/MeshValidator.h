#pragma once

#include "MeshData.h"

namespace pae {

// Validates generated CPU geometry. minTriangleArea allows callers to permit
// deliberately collapsed vertices at poles of analytic primitives.
ValidationReport validateMesh(const MeshData& mesh,
                              float minTriangleArea = 1e-12f,
                              bool requireSkinWeights = true);

} // namespace pae
