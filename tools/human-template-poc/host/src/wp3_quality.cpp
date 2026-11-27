#include "wp3_quality.h"

#include "procedural/MeshData.h"
#include "procedural/MeshTopology.h"
#include "procedural/humanoid/canonical/CanonicalTopology.h"

#include <algorithm>
#include <cmath>

using namespace pae::ht;

namespace pae::wp3 {

QualityReport evaluateQuality(const PolyMesh& body, const std::vector<D3>& positions,
                              const Topology& topo, const std::vector<D3>* base) {
    QualityReport q;
    const auto t0 = nowMs();

    // Triangulate (0-2 diagonal for quads, fan otherwise) into engine MeshData.
    pae::MeshData mesh;
    mesh.vertices.resize(positions.size());
    std::vector<D3> accum(positions.size(), D3{});
    for (size_t i = 0; i < positions.size(); ++i) {
        pae::Vertex v;
        v.position = pae::Vec3(static_cast<float>(positions[i].x),
                               static_cast<float>(positions[i].y),
                               static_cast<float>(positions[i].z));
        mesh.vertices[i] = v;
    }
    auto tri = [&](int ia, int ib, int ic) {
        const D3& a = positions[static_cast<size_t>(ia)];
        const D3& b = positions[static_cast<size_t>(ib)];
        const D3& c = positions[static_cast<size_t>(ic)];
        const D3 fn = (b - a).cross(c - a);
        accum[static_cast<size_t>(ia)] += fn;
        accum[static_cast<size_t>(ib)] += fn;
        accum[static_cast<size_t>(ic)] += fn;
        mesh.indices.push_back(static_cast<uint32_t>(ia));
        mesh.indices.push_back(static_cast<uint32_t>(ib));
        mesh.indices.push_back(static_cast<uint32_t>(ic));
        // signed volume contribution
        q.volume += a.dot(b.cross(c)) / 6.0;
    };
    for (const PolyFace& f : body.faces) {
        if (f.v.size() == 4) {
            tri(f.v[0], f.v[1], f.v[2]);
            tri(f.v[0], f.v[2], f.v[3]);
        } else {
            for (size_t k = 1; k + 1 < f.v.size(); ++k) tri(f.v[0], f.v[k], f.v[k + 1]);
        }
    }
    q.volume = std::fabs(q.volume);
    pae::Aabb bounds;
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const D3 n = accum[i].normalized();
        mesh.vertices[i].normal = pae::Vec3(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z));
        bounds.expand(mesh.vertices[i].position);
    }
    mesh.bounds = bounds;
    q.bboxX = bounds.size().x; q.bboxY = bounds.size().y; q.bboxZ = bounds.size().z;

    const pae::TopologyReport t = pae::analyzeTopology(mesh);
    q.triangles = t.triangles;
    q.vertices = t.vertices;
    q.components = t.components;
    q.boundaryEdges = t.boundaryEdges;
    q.nonManifoldEdges = t.nonManifoldEdges;
    q.degenerateTriangles = t.degenerateTriangles;
    q.duplicateVertices = t.duplicateVertices;
    q.invalidNormals = t.invalidNormals;

    const pae::canonical::SelfIntersectionReport si = pae::canonical::detectSelfIntersections(mesh);
    q.selfIntersectingPairs = si.intersectingPairs;
    q.candidatePairs = si.candidatePairs;

    if (base) {
        const DeformationQuality d = compareSurfaces(body, *base, topo);
        q.flippedFaces = d.flippedFaces;
        q.edgeRatioP99 = d.edgeRatioP99;
        q.edgeRatioMin = d.minEdgeRatio;
        q.edgeRatioMax = d.maxEdgeRatio;
        q.volumeRatio = d.volumeRatio;
    }

    // Edge length distribution over unique edges.
    std::vector<double> edges;
    edges.reserve(topo.edges.size());
    for (const auto& e : topo.edges) {
        // ht Topology edge: (a, b, useCount), a < b
        edges.push_back((positions[static_cast<size_t>(e[0])] - positions[static_cast<size_t>(e[1])]).length());
    }
    if (!edges.empty()) {
        std::sort(edges.begin(), edges.end());
        q.edgeMin = edges.front();
        q.edgeMax = edges.back();
        q.edgeP50 = edges[edges.size() / 2];
        q.edgeP99 = edges[static_cast<size_t>(static_cast<double>(edges.size()) * 0.99)];
    }
    q.ms = nowMs() - t0;
    return q;
}

} // namespace pae::wp3