#pragma once

// G2 (SPEC 8.3 Layer B / 10.1): topology audit for the PoC-A canonical mesh.
//
// The production TopologyReport (procedural/MeshTopology.h) is deliberately
// left untouched: it counts boundary EDGES but cannot say whether those edges
// form one clean loop or a shattered mess. G2 has to distinguish two very
// different situations:
//
//   - a REGISTERED crop boundary (the T2 shoulder root) - legitimate, the
//     mesh is intentionally open there;
//   - an UNREGISTERED crack - a defect.
//
// So this module extracts the boundary loops and matches them against the
// crop boundaries the template registered.

#include "procedural/MeshData.h"
#include "procedural/MeshTopology.h"
#include "procedural/shape/QuadTemplate.h"

#include <string>
#include <vector>

namespace pae::shape {

// One connected boundary loop (a chain of edges that belong to exactly one
// triangle).
struct BoundaryLoop {
    int edgeCount = 0;
    int vertexCount = 0;
    bool closed = false;      // every vertex in the loop has degree 2
    Vec3 centroid{};
    float minY = 0.0f;
    float maxY = 0.0f;
    float minRadius = 0.0f;   // horizontal radius around the body axis
    float maxRadius = 0.0f;
};

// Quad face quality, measured on the canonical quad grid (not the
// triangulated render mesh).
struct FaceQuality {
    int quadCount = 0;
    double meanAspectRatio = 0.0;   // longest edge / shortest edge
    double maxAspectRatio = 0.0;
    double meanMinAngleDeg = 0.0;   // smallest interior angle, degrees
    double minMinAngleDeg = 0.0;
    int sliverCount = 0;             // aspect ratio > 10 or min angle < 15 deg
};

// Edge flow: how well the quad grid's edges follow the anatomy. A canonical
// body quad grid has two families of edges - circumferential (around the
// body) and longitudinal (along the body). The metric is the angular
// deviation of each edge from the local circumferential direction.
struct EdgeFlow {
    int edgeCount = 0;
    double meanCircumferentialDeg = 0.0;
    double p95CircumferentialDeg = 0.0;
    double meanLongitudinalDeg = 0.0;
    double p95LongitudinalDeg = 0.0;
    // Fraction of quads whose two edge families are close to orthogonal
    // (the signature of a well-formed quad grid).
    double orthogonalQuadRatio = 0.0;
};

struct TopologyAudit {
    TopologyReport base;                     // production counters
    std::vector<BoundaryLoop> boundaryLoops;
    std::vector<CropBoundary> registeredCrops;
    int registeredBoundaryEdges = 0;   // boundary edges explained by a crop
    int unregisteredBoundaryEdges = 0; // boundary edges NOT explained by a crop
    bool allCropsClosed = false;       // every registered crop is one closed loop
    bool cropsMatchLoops = false;      // each crop ring maps 1:1 onto a boundary loop
    bool clean = false;                // no crack, no island, no non-manifold edge

    FaceQuality faces;
    EdgeFlow flow;
    double volumeM3 = 0.0;             // signed triangle sum (see caveat in .cpp)
    bool volumeClosed = false;         // false when a crop leaves the surface open
};

// Extracts the boundary loops of a mesh and matches them against the crop
// boundaries a template registered.
TopologyAudit auditTopology(const MeshData& mesh,
                            const std::vector<CropBoundary>& crops);

// Quad face quality for the canonical grid. `quads` are index quadruples.
FaceQuality measureFaceQuality(const MeshData& mesh,
                                const std::vector<std::array<uint32_t, 4>>& quads);

// Edge-flow statistics for the canonical grid.
EdgeFlow measureEdgeFlow(const MeshData& mesh,
                          const std::vector<std::array<uint32_t, 4>>& quads);

// Signed volume of a triangle mesh. For a mesh with an open crop boundary the
// value is the volume of the surface closed by the crop plane - usable for
// relative comparison, not as an absolute body volume.
double meshVolume(const MeshData& mesh);

} // namespace pae::shape
