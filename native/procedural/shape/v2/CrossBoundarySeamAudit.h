#pragma once

// PoC-SM (round 2): EXPERIMENTAL cross-boundary seam audit.
//
// WHY THIS MODULE EXISTS
//
// measureShoulderRootSeam() (SeamAuditV2.h) compares the face normals of two
// ADJACENT QUADS inside the shoulder-root ring band. That measures the
// polygonal faceting of a ring (a perfect cylinder reads 360/segments degrees)
// plus local ring curvature. It does NOT measure the angle between two
// components that are stitched together, which is what
// docs/foundation/03-geometry-mesh-contract.md describes as
// "seam both sides: position error <= 1e-4 m, normal angle <= 5 deg".
//
// This module is a SEPARATE EXPERIMENT. It never replaces the signed OQ-03
// definition, never changes measureShoulderRootSeam(), and produces no Gate
// judgement. A decision to adopt it as the formal acceptance definition is
// the owner's, not the implementation's.
//
// WHAT IT MEASURES
//
//   INPUT      two patches, each carrying (a) a real triangle mesh with its
//              own winding and (b) an ordered loop of boundary vertex ids.
//   MATCHING   boundary vertices are paired by POSITION within a tolerance,
//              not by index arithmetic, so a shifted/reordered boundary is
//              detected instead of silently accepted.
//   POSITION   max/mean mismatch of the paired boundary vertices.
//   ORDER      whether the pairing preserves the boundary order.
//   WINDING    for every matched edge, the triangle on each side must traverse
//              the shared edge in OPPOSITE directions. A flipped patch is
//              reported and its samples are excluded.
//   ANGLE      for every matched edge, the angle between the two REAL triangle
//              face normals, taken from the mesh index buffer and the actual
//              vertex positions. No hand-supplied normal is used.
//   STATISTIC  max / mean / nearest-rank p95 over the per-edge angles.
//   NA         if a side has no boundary, vertices cannot be paired, or no
//              usable triangle pair exists -> NOT APPLICABLE. This is never
//              reported as PASS.

#include "procedural/MeshData.h"

#include <string>
#include <vector>

namespace pae {
namespace shape {
namespace v2 {

enum class CrossSeamApplicability : int {
    Applicable = 0,
    NotApplicable = 1
};

const char* crossSeamApplicabilityName(CrossSeamApplicability a);

// One side of a candidate seam. `boundary` is the ordered loop of vertex ids
// along the shared edge; `mesh` carries the real triangles and their winding.
struct SeamPatch {
    MeshData mesh;
    std::vector<uint32_t> boundary;
};

struct CrossBoundarySeamMeasurement {
    CrossSeamApplicability applicability = CrossSeamApplicability::NotApplicable;

    int boundaryEdges = 0;             // edges on side A
    int matchedPairs = 0;              // boundary vertices paired by position
    int unmatchedVertices = 0;         // A vertices with no B vertex in tolerance
    int missingTrianglePairs = 0;      // matched edges with no triangle on a side
    int windingInconsistentPairs = 0;  // matched edges traversed the same way
    int usableAngleSamples = 0;        // angles actually used for the statistic
    bool windingVerified = false;
    bool boundaryOrderReversed = false;

    double positionErrorMaxM = 0.0;
    double positionErrorMeanM = 0.0;

    double maxDeg = 0.0;
    double meanDeg = 0.0;
    double p95Deg = 0.0;

    std::vector<std::string> notes;
};

// positionToleranceM: maximum allowed distance for pairing a boundary vertex
// on side A with one on side B. Defaults to the contract's 1e-4 m.
CrossBoundarySeamMeasurement measureCrossBoundarySeam(
    const SeamPatch& sideA,
    const SeamPatch& sideB,
    double positionToleranceM = 1e-4);

// Angle between two face normals in degrees, domain [0, 180]. The full domain
// is kept: a fold above 90 deg must stay visible.
double crossSeamAngleDeg(const Vec3& n0, const Vec3& n1);

} // namespace v2
} // namespace shape
} // namespace pae
