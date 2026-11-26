#pragma once

// Surface-mesh topology audit (element quality, manifoldness, winding, UV
// seams). Works on polygon meshes; quads are evaluated through their
// corner angles and through the triangles of the shorter diagonal split.

#include "ht_common.h"
#include "ht_obj.h"

#include <map>

namespace pae::ht {

struct Topology {
    int vertexCount = 0;
    int faceCount = 0;
    std::vector<std::array<int, 3>> edges;  // (a, b, useCount), a < b
    std::vector<std::vector<int>> edgeFaces;
    std::vector<std::vector<int>> vertexNeighbours;  // unique, sorted
    std::vector<std::vector<int>> vertexFaces;
    std::vector<int> componentOfVertex;
    int componentCount = 0;
    int isolatedVertexCount = 0;

    int edgeIndex(int a, int b, const std::map<std::pair<int, int>, int>& lookup) const;
};

void buildTopology(const PolyMesh& mesh, Topology& topo);
std::map<std::pair<int, int>, int> edgeLookup(const Topology& topo);

D3 faceNormal(const PolyMesh& mesh, int face);
double faceArea(const PolyMesh& mesh, int face);

struct Stats {
    double min = 0, p01 = 0, p50 = 0, p99 = 0, max = 0, mean = 0;
    long count = 0;
};

Stats summarize(std::vector<double> values);

struct TopologyAudit {
    long vertexCount = 0;
    long faceCount = 0;
    long quadCount = 0, triCount = 0, ngonCount = 0;
    long edgeCount = 0;
    long boundaryEdgeCount = 0;
    long nonManifoldEdgeCount = 0;
    long inconsistentWindingEdgeCount = 0;
    long duplicateFaceCount = 0;
    long degenerateFaceCount = 0;
    long zeroAreaFaceCount = 0;
    long eulerChi = 0;
    int componentCount = 0;
    int boundaryLoopCount = 0;
    int isolatedVertexCount = 0;
    int valenceMin = 0, valenceMax = 0;
    std::map<int, long> valenceHistogram;
    Stats edgeLength;
    Stats minCornerAngleDeg;
    Stats triangleMinAngleDeg;
    Stats aspectRatio;      // (a^2+b^2+c^2) / (4*sqrt(3)*A); 1.0 = equilateral
    Stats dihedralDeg;      // 0 = flat
    long dihedralOver60 = 0, dihedralOver90 = 0;
    double surfaceArea = 0;
    double signedVolume = 0;
    bool outwardWinding = false;
    long uvSeamEdgeCount = 0;
    long uvCornerCount = 0;          // sum of face corner count
    long uniqueUvVertexPairCount = 0; // render-mesh vertex count before triangulation
    long triangulatedCount = 0;
    long materialFaceCount = 0;      // faces with a non-empty usemtl
    std::string materialNames;       // distinct usemtl names
};

TopologyAudit auditTopology(const PolyMesh& mesh, const Topology& topo);

// Same-topology comparison between a rest surface and a deformed one
// (shape target or skinning result): normal flips and edge stretch.
struct DeformationQuality {
    long flippedFaces = 0;          // face normal dot(rest normal) < 0
    double minNormalDot = 1.0;      // worst cos(angle) between rest and deformed normals
    long edgesBelow50pct = 0;       // deformed edge < 50 % of rest length
    double minEdgeRatio = 1.0;      // deformed / rest edge length
    double maxEdgeRatio = 1.0;
    double edgeRatioP99 = 1.0;
    double minAngleDeg = 180.0;
    double maxAspect = 0.0;
    double signedVolume = 0.0;
    double volumeRatio = 1.0;       // deformed / rest signed volume
};
DeformationQuality compareSurfaces(const PolyMesh& rest, const std::vector<D3>& deformed, const Topology& topo);

} // namespace pae::ht
