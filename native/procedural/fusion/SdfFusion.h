#pragma once

// M11: local SDF fusion utilities. Surface Nets (dual contouring lite)
// extracts a quad-connected, orientation-corrected mesh from a signed
// distance function over a small local volume — used only for joint
// blend patches (shoulder/hip/neck), never for the whole body.

#include "core/math/Math.h"
#include "procedural/MeshData.h"

#include <functional>

namespace pae::fusion {

struct FusionVolume {
    Vec3 center{0.0f, 0.0f, 0.0f}; // world center of the local volume
    Vec3 size{1.0f, 1.0f, 1.0f};   // full extent per axis
    int resolution = 20;           // cells per axis (grid is resolution+1 samples)
};

// SDF signature: negative inside the solid, positive outside.
using SdfField = std::function<float(Vec3)>;

// Extract a surface with per-vertex normals taken from the SDF gradient.
// Quads are triangulated; winding is corrected against the gradient so all
// faces point outward.
MeshData extractSurfaceNets(const FusionVolume& volume, const SdfField& sdf);

// SDF primitives (world-space).
float sdSphere(Vec3 p, Vec3 center, float radius);
float sdEllipsoidApprox(Vec3 p, Vec3 center, Vec3 radii);
float sdCapsule(Vec3 p, Vec3 a, Vec3 b, float radius);
float sdSmoothUnion(float a, float b, float k);
float sdSmoothSubtraction(float a, float b, float k);

} // namespace pae::fusion
