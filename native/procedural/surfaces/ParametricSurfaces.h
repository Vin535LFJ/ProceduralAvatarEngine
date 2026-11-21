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
    float roll = 0.0f; // M11: cross-section rotation around the section plane normal (radians)
};

// M11: fully parameterized sweep ring. sectionScale multiplies the base
// radius into an ellipse (x = PTF normal axis, z = PTF binormal axis);
// roll rotates the section around the tangent.
struct SweepRing {
    Vec3 point{0.0f, 0.0f, 0.0f};
    float radius = 0.1f;
    Vec2 sectionScale{1.0f, 1.0f};
    float roll = 0.0f;
};

// Stable frames along a centerline; avoids fixed-cross-product flipping.
std::vector<SurfaceFrame> computeParallelTransportFrames(const std::vector<Vec3>& points);

// M11 legacy path (kept for compatibility): one ring per input point,
// circular section, no caps.
MeshData generateSweepSurface(const std::vector<Vec3>& centerline,
                              const std::vector<float>& radii,
                              uint32_t radialSegments);

MeshData generateLoftSurface(const std::vector<LoftSection>& sections,
                             uint32_t radialSegments);

// M11: spline-resampled sweep with elliptical, rollable sections,
// taper-aware normals, optional end caps and correct UV seam.
// splineSamplesPerSegment >= 1; closedRing welds first/last ring
// (rings.front().point should equal rings.back().point).
MeshData generateSplineSweep(const std::vector<SweepRing>& rings,
                             uint32_t radialSegments, int splineSamplesPerSegment,
                             bool capStart = false, bool capEnd = false,
                             bool closedRing = false);

// M11: spline-resampled loft (centers/radii/roll interpolated through the
// sections). Winding is normalized against the section travel direction.
MeshData generateSplineLoft(const std::vector<LoftSection>& sections,
                            uint32_t radialSegments, int splineSamplesPerSection);

} // namespace pae
