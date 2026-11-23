#include "procedural/shape/TopologyAudit.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace pae::shape {
namespace {

constexpr double kPi = 3.14159265358979323846;

double angleBetween(const Vec3& a, const Vec3& b) {
    const double la = std::sqrt(a.lengthSq());
    const double lb = std::sqrt(b.lengthSq());
    if (la < 1e-12 || lb < 1e-12) return 0.0;
    double c = (a.x * b.x + a.y * b.y + a.z * b.z) / (la * lb);
    c = std::max(-1.0, std::min(1.0, c));
    return std::acos(c);
}

double percentile(std::vector<double> values, double p) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const double idx = p * static_cast<double>(values.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(idx));
    const size_t hi = static_cast<size_t>(std::ceil(idx));
    const double frac = idx - static_cast<double>(lo);
    return values[lo] * (1.0 - frac) + values[hi] * frac;
}

} // namespace

TopologyAudit auditTopology(const MeshData& mesh, const std::vector<CropBoundary>& crops) {
    TopologyAudit audit;
    audit.base = analyzeTopology(mesh);
    audit.registeredCrops = crops;

    // Boundary edges: referenced by exactly one triangle.
    std::map<std::pair<uint32_t, uint32_t>, int> edgeCount;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t tri[3] = {mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]};
        for (int e = 0; e < 3; ++e) {
            const uint32_t v0 = tri[e];
            const uint32_t v1 = tri[(e + 1) % 3];
            edgeCount[{std::min(v0, v1), std::max(v0, v1)}] += 1;
        }
    }

    // Group boundary edges into connected loops over the boundary-vertex graph.
    std::map<uint32_t, std::vector<uint32_t>> boundaryAdjacency;
    int boundaryEdgeTotal = 0;
    for (const auto& edge : edgeCount) {
        if (edge.second != 1) continue;
        ++boundaryEdgeTotal;
        boundaryAdjacency[edge.first.first].push_back(edge.first.second);
        boundaryAdjacency[edge.first.second].push_back(edge.first.first);
    }

    std::set<uint32_t> visited;
    for (const auto& entry : boundaryAdjacency) {
        if (visited.count(entry.first)) continue;
        std::vector<uint32_t> stack{entry.first};
        visited.insert(entry.first);
        std::set<uint32_t> loopVertices;
        int loopEdges = 0;
        Vec3 centroid{0.0f, 0.0f, 0.0f};
        float minY = 1e30f;
        float maxY = -1e30f;
        float minRadius = 1e30f;
        float maxRadius = -1e30f;
        bool allDegreeTwo = true;

        while (!stack.empty()) {
            const uint32_t v = stack.back();
            stack.pop_back();
            loopVertices.insert(v);
            const auto degree = boundaryAdjacency[v].size();
            if (degree != 2) allDegreeTwo = false;
            const Vec3& p = mesh.vertices[v].position;
            centroid = centroid + p;
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
            const float radius = std::hypot(p.x, p.z);
            minRadius = std::min(minRadius, radius);
            maxRadius = std::max(maxRadius, radius);
            for (uint32_t next : boundaryAdjacency[v]) {
                ++loopEdges;
                if (visited.insert(next).second) stack.push_back(next);
            }
        }
        loopEdges /= 2;  // each edge counted from both ends

        BoundaryLoop loop;
        loop.edgeCount = loopEdges;
        loop.vertexCount = static_cast<int>(loopVertices.size());
        loop.closed = allDegreeTwo;
        loop.centroid = centroid * (1.0f / static_cast<float>(loopVertices.size()));
        loop.minY = minY;
        loop.maxY = maxY;
        loop.minRadius = minRadius;
        loop.maxRadius = maxRadius;
        audit.boundaryLoops.push_back(loop);
    }

    // Match each registered crop against the boundary loops it should explain.
    // A crop ring lives at a single height, so a loop whose height band
    // contains the crop height and whose vertex count matches is the crop.
    audit.allCropsClosed = true;
    audit.cropsMatchLoops = true;
    std::vector<bool> loopUsed(audit.boundaryLoops.size(), false);
    for (const CropBoundary& crop : crops) {
        int matched = 0;
        for (size_t i = 0; i < audit.boundaryLoops.size(); ++i) {
            if (loopUsed[i]) continue;
            const BoundaryLoop& loop = audit.boundaryLoops[i];
            const bool heightMatches = crop.y >= loop.minY - 0.02f && crop.y <= loop.maxY + 0.02f;
            if (heightMatches && loop.vertexCount == crop.vertexCount) {
                loopUsed[i] = true;
                ++matched;
                audit.registeredBoundaryEdges += loop.edgeCount;
            }
        }
        if (matched != 1) {
            audit.cropsMatchLoops = false;
        }
        if (!crop.closed) audit.allCropsClosed = false;
    }
    audit.unregisteredBoundaryEdges = boundaryEdgeTotal - audit.registeredBoundaryEdges;

    audit.clean = audit.base.components == 1 && audit.base.nonManifoldEdges == 0 &&
                  audit.base.degenerateTriangles == 0 && audit.base.duplicateVertices == 0 &&
                  audit.base.invalidNormals == 0 && audit.unregisteredBoundaryEdges == 0 &&
                  audit.cropsMatchLoops && audit.allCropsClosed;

    audit.volumeM3 = meshVolume(mesh);
    audit.volumeClosed = boundaryEdgeTotal == 0;
    return audit;
}

FaceQuality measureFaceQuality(const MeshData& mesh,
                                const std::vector<std::array<uint32_t, 4>>& quads) {
    FaceQuality quality;
    quality.quadCount = static_cast<int>(quads.size());
    if (quads.empty()) return quality;

    double aspectSum = 0.0;
    double angleSum = 0.0;
    for (const auto& quad : quads) {
        Vec3 edges[4];
        double lengths[4];
        double minLength = 1e30;
        double maxLength = 0.0;
        for (int e = 0; e < 4; ++e) {
            const Vec3& p = mesh.vertices[quad[e]].position;
            const Vec3& q = mesh.vertices[quad[(e + 1) % 4]].position;
            edges[e] = q - p;
            lengths[e] = std::sqrt(edges[e].lengthSq());
            minLength = std::min(minLength, lengths[e]);
            maxLength = std::max(maxLength, lengths[e]);
        }
        const double aspect = minLength > 1e-9 ? maxLength / minLength : 1e9;
        aspectSum += aspect;
        quality.maxAspectRatio = std::max(quality.maxAspectRatio, aspect);

        // Smallest interior angle of the quad (at any corner).
        double minAngle = 180.0;
        for (int c = 0; c < 4; ++c) {
            const Vec3& prev = edges[(c + 3) % 4];
            const Vec3& next = edges[c];
            const double deg = angleBetween(prev, next) * 180.0 / kPi;
            minAngle = std::min(minAngle, deg);
        }
        angleSum += minAngle;
        quality.minMinAngleDeg = std::min(quality.minMinAngleDeg, minAngle);
        if (aspect > 10.0 || minAngle < 15.0) ++quality.sliverCount;
    }
    const double n = static_cast<double>(quads.size());
    quality.meanAspectRatio = aspectSum / n;
    quality.meanMinAngleDeg = angleSum / n;
    return quality;
}

EdgeFlow measureEdgeFlow(const MeshData& mesh,
                          const std::vector<std::array<uint32_t, 4>>& quads) {
    EdgeFlow flow;
    if (quads.empty()) return flow;

    std::vector<double> circumferential;
    std::vector<double> longitudinal;
    int orthogonal = 0;

    for (const auto& quad : quads) {
        // Quad centre and its radial direction (the local "around the body"
        // direction). The body axis is +Y, so the circumferential direction at
        // a vertex is the horizontal tangent of the circle around the axis.
        Vec3 centre{0.0f, 0.0f, 0.0f};
        for (int k = 0; k < 4; ++k) centre = centre + mesh.vertices[quad[k]].position;
        centre = centre * 0.25f;

        // Edge 0 = ring direction (a -> b), edge 1 = column direction (b -> c).
        const Vec3& p0 = mesh.vertices[quad[0]].position;
        const Vec3& p1 = mesh.vertices[quad[1]].position;
        const Vec3& p2 = mesh.vertices[quad[2]].position;
        const Vec3 ringEdge = p1 - p0;
        const Vec3 columnEdge = p2 - p1;

        // Radial direction at the quad centre (horizontal).
        Vec3 radial{centre.x, 0.0f, centre.z};
        const float radialLen = std::sqrt(radial.lengthSq());
        Vec3 circDir{-radial.z, 0.0f, radial.x};
        const float circLen = std::sqrt(circDir.lengthSq());
        if (radialLen < 1e-9 || circLen < 1e-9) continue;
        circDir = circDir * (1.0f / circLen);

        const double ringDeg = angleBetween(ringEdge, circDir) * 180.0 / kPi;
        // The column edge should run along the body axis (+Y).
        const double columnDeg = angleBetween(columnEdge, Vec3{0.0f, 1.0f, 0.0f}) * 180.0 / kPi;
        circumferential.push_back(std::min(ringDeg, 180.0 - ringDeg));
        longitudinal.push_back(std::min(columnDeg, 180.0 - columnDeg));

        const double between = angleBetween(ringEdge, columnEdge) * 180.0 / kPi;
        const double deviation = std::min(between, 180.0 - between);
        if (std::fabs(deviation - 90.0) < 15.0) ++orthogonal;
    }

    flow.edgeCount = static_cast<int>(circumferential.size());
    if (flow.edgeCount == 0) return flow;
    double sumC = 0.0;
    double sumL = 0.0;
    for (size_t i = 0; i < circumferential.size(); ++i) {
        sumC += circumferential[i];
        sumL += longitudinal[i];
    }
    const double n = static_cast<double>(flow.edgeCount);
    flow.meanCircumferentialDeg = sumC / n;
    flow.meanLongitudinalDeg = sumL / n;
    flow.p95CircumferentialDeg = percentile(circumferential, 0.95);
    flow.p95LongitudinalDeg = percentile(longitudinal, 0.95);
    flow.orthogonalQuadRatio = static_cast<double>(orthogonal) / n;
    return flow;
}

double meshVolume(const MeshData& mesh) {
    double total = 0.0;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const Vec3& a = mesh.vertices[mesh.indices[i]].position;
        const Vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const Vec3& c = mesh.vertices[mesh.indices[i + 2]].position;
        total += (a.x * (b.y * c.z - b.z * c.y) -
                  a.y * (b.x * c.z - b.z * c.x) +
                  a.z * (b.x * c.y - b.y * c.x));
    }
    return total / 6.0;
}

} // namespace pae::shape
