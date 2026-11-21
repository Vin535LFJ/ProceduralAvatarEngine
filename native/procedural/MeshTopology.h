#pragma once

// M12: mesh topology analysis — distinguishes "buffer merged" from "actually
// connected" geometry. Used by the Human Anatomy Report and tests.

#include "procedural/MeshData.h"

namespace pae {

struct TopologyReport {
    long triangles = 0;
    size_t vertices = 0;
    int components = 0;            // connected vertex components
    int boundaryEdges = 0;         // edges referenced by exactly 1 triangle
    int nonManifoldEdges = 0;      // edges referenced by more than 2 triangles
    int degenerateTriangles = 0;   // zero-area triangles
    int duplicateVertices = 0;     // vertices with identical positions
    int invalidNormals = 0;        // non-unit or non-finite normals
    std::vector<Vec3> boundarySamples; // midpoints of up to 8 boundary edges
};

TopologyReport analyzeTopology(const MeshData& mesh);

} // namespace pae
