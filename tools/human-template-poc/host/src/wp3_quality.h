#pragma once

// VR-1B WP3-HOST-01: geometry quality evaluation for shape variants.
// Reuses PAE's own checkers (analyzeTopology, detectSelfIntersections) by
// compiling them read-only into this host tool; no native/ source is modified.

#include "ht_common.h"
#include "ht_obj.h"
#include "ht_topology.h"

#include <string>
#include <vector>

namespace pae::wp3 {

struct QualityReport {
    // topology (PAE analyzeTopology on the triangulated body)
    long triangles = 0;
    size_t vertices = 0;
    int components = 0, boundaryEdges = 0, nonManifoldEdges = 0;
    int degenerateTriangles = 0, duplicateVertices = 0, invalidNormals = 0;
    // self-intersection (PAE canonical::detectSelfIntersections)
    size_t selfIntersectingPairs = 0, candidatePairs = 0;
    // deformation vs base (ht compareSurfaces)
    long flippedFaces = 0;
    double edgeRatioP99 = 1.0, edgeRatioMin = 1.0, edgeRatioMax = 1.0, volumeRatio = 1.0;
    // absolute surface volume (m^3, divergence theorem) and bbox
    double volume = 0.0;
    double bboxX = 0.0, bboxY = 0.0, bboxZ = 0.0;
    // edge length distribution (m)
    double edgeMin = 0.0, edgeP50 = 0.0, edgeP99 = 0.0, edgeMax = 0.0;
    double ms = 0.0;
};

// Triangulate the body (0-2 diagonal), recompute area-weighted normals, run
// all checkers. `base` may be null (skip deformation comparison).
QualityReport evaluateQuality(const ht::PolyMesh& body, const std::vector<ht::D3>& positions,
                              const ht::Topology& topo, const std::vector<ht::D3>* base);

} // namespace pae::wp3