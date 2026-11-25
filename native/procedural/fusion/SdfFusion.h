#pragma once

// M11: local SDF fusion utilities. Surface Nets (dual contouring lite)
// extracts a quad-connected, orientation-corrected mesh from a signed
// distance function over a local volume — used for joint blend patches and
// (M13) for the upper-body implicit surface.

#include "core/math/Math.h"
#include "procedural/MeshData.h"

#include <array>
#include <functional>
#include <cstdint>

namespace pae::fusion {

// M13: resolution is per-axis. The body is far wider than it is deep, so a
// single scalar forces a cubic lattice and wastes sample density on the
// thin axes (or starves the wide ones).
struct FusionVolume {
    Vec3 center{0.0f, 0.0f, 0.0f};              // world center of the volume
    Vec3 size{1.0f, 1.0f, 1.0f};                // full extent per axis
    std::array<int, 3> resolution{20, 20, 20}; // cells per axis (n+1 samples)
};

// SDF signature: negative inside the solid, positive outside.
using SdfField = std::function<float(Vec3)>;

// Extract a surface with per-vertex normals taken from the SDF gradient.
// Quads are triangulated; winding is corrected against the gradient so all
// faces point outward. The dual-lattice quad connectivity (v0..v3 per face)
// can optionally be captured for topology analysis (edge flow / quad-ness),
// which the triangulated output alone cannot express.
MeshData extractSurfaceNets(const FusionVolume& volume, const SdfField& sdf,
                            std::vector<std::array<uint32_t, 4>>* quadIndices = nullptr,
                            bool discardDegenerateQuads = false);

// SDF primitives (world-space).
float sdSphere(Vec3 p, Vec3 center, float radius);
float sdEllipsoidApprox(Vec3 p, Vec3 center, Vec3 radii);
float sdCapsule(Vec3 p, Vec3 a, Vec3 b, float radius);
float sdSmoothUnion(float a, float b, float k);
float sdSmoothSubtraction(float a, float b, float k);

} // namespace pae::fusion
