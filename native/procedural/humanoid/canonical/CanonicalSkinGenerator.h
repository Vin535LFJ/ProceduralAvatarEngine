#pragma once

#include "procedural/MeshData.h"
#include "procedural/MeshTopology.h"
#include "procedural/humanoid/canonical/CanonicalTopology.h"

#include <array>
#include <cstdint>
#include <vector>

namespace pae::canonical {

struct BoundaryLoop {
    int regionId = -1;
    int pairId = -1;
    uint32_t anchor = 0;
    int direction = 1;
    std::vector<uint32_t> vertexIds;
    std::vector<float> distance;
    float totalLength = 0.0f;
};

struct WeightReport {
    size_t vertices = 0;
    size_t invalidVertices = 0;
    float maxNormalizationError = 0.0f;
    size_t top4Reductions = 0;
};

std::vector<BoundaryLoop> extractBoundaryLoops(const MeshData& mesh);
std::vector<Vec3> resampleBoundary(const MeshData& mesh, const BoundaryLoop& loop,
                                   uint32_t sampleCount);
BoundaryLoop orientBoundary(const MeshData& mesh, const BoundaryLoop& loop, bool reverse);
std::vector<uint32_t> matchBoundaryVertices(const MeshData& mesh, const BoundaryLoop& reference,
                                            const BoundaryLoop& candidate,
                                            uint32_t commonSamples);

struct WeldResult {
    bool valid = false;
    size_t weldedPairs = 0;
    float maxPositionError = 0.0f;
};

WeldResult weldBoundaryLoops(MeshData& mesh, const BoundaryLoop& reference,
                             const BoundaryLoop& candidate);

void mergeVertexWeights(Vertex& destination, const Vertex& source);
WeightReport validateWeights(const MeshData& mesh);

struct UvReport {
    size_t charts = 0;
    size_t atlasColumns = 0;
    size_t atlasRows = 0;
    size_t renderVertices = 0;
    size_t invalidUvVertices = 0;
    size_t outOfRangeUvVertices = 0;
    size_t zeroAreaUvTriangles = 0;
    float minUvArea = 0.0f;
    float maxUvArea = 0.0f;
};

struct RenderResult {
    bool valid = false;
    MeshData mesh;
    std::vector<uint32_t> renderToCanonical;
    size_t splitVertices = 0;
    UvReport uv;
};

struct CanonicalSkinResult {
    bool valid = false;
    MeshData mesh;
    std::vector<BoundaryLoop> boundaryLoops;
    TopologyReport topology;
    SelfIntersectionReport selfIntersections;
    WeightReport weights;
};

RenderResult buildRenderMesh(const CanonicalSkinResult& canonical, float weldTolerance = 1e-5f);

} // namespace pae::canonical
