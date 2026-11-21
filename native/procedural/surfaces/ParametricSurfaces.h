#pragma once

#include "procedural/MeshData.h"

#include <vector>

namespace pae {

struct SurfaceFrame {
    Vec3 tangent{0.0f, 0.0f, 1.0f};
    Vec3 normal{1.0f, 0.0f, 0.0f};
    Vec3 binormal{0.0f, 1.0f, 0.0f};
};

struct LoftSection {
    Vec3 center{0.0f, 0.0f, 0.0f};
    float radiusX = 0.1f;
    float radiusZ = 0.1f;
};

// Stable frames along a centerline; avoids fixed-cross-product flipping.
std::vector<SurfaceFrame> computeParallelTransportFrames(const std::vector<Vec3>& points);

MeshData generateSweepSurface(const std::vector<Vec3>& centerline,
                              const std::vector<float>& radii,
                              uint32_t radialSegments);

MeshData generateLoftSurface(const std::vector<LoftSection>& sections,
                             uint32_t radialSegments);

} // namespace pae
