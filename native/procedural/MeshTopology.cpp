#include "MeshTopology.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>

namespace pae {

TopologyReport analyzeTopology(const MeshData& mesh) {
    TopologyReport report;
    report.vertices = mesh.vertices.size();
    report.triangles = static_cast<long>(mesh.indices.size() / 3);

    // Duplicate vertex positions (exact float equality).
    std::map<std::pair<float, std::pair<float, float>>, int> positionMap;
    for (const Vertex& v : mesh.vertices) {
        const auto key = std::make_pair(v.position.x, std::make_pair(v.position.y, v.position.z));
        auto it = positionMap.find(key);
        if (it != positionMap.end()) ++it->second;
        else positionMap[key] = 1;
    }
    for (const auto& entry : positionMap) {
        if (entry.second > 1) ++report.duplicateVertices;
    }

    // Invalid normals.
    for (const Vertex& v : mesh.vertices) {
        if (!std::isfinite(v.normal.x) || !std::isfinite(v.normal.y) || !std::isfinite(v.normal.z) ||
            std::fabs(v.normal.lengthSq() - 1.0f) > 1e-2f) {
            ++report.invalidNormals;
        }
    }

    // Edge reference counting + triangle adjacency + degenerate census.
    std::map<std::pair<uint32_t, uint32_t>, int> edgeCount;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t a = mesh.indices[i], b = mesh.indices[i + 1], c = mesh.indices[i + 2];
        if (a >= mesh.vertices.size() || b >= mesh.vertices.size() || c >= mesh.vertices.size())
            continue;
        const Vec3& pa = mesh.vertices[a].position;
        const Vec3& pb = mesh.vertices[b].position;
        const Vec3& pc = mesh.vertices[c].position;
        if ((pb - pa).cross(pc - pa).lengthSq() < 1e-14f) ++report.degenerateTriangles;
        for (int e = 0; e < 3; ++e) {
            const uint32_t v0 = e == 0 ? a : e == 1 ? b : c;
            const uint32_t v1 = e == 0 ? b : e == 1 ? c : a;
            const auto key = std::make_pair(std::min(v0, v1), std::max(v0, v1));
            ++edgeCount[key];
        }
    }
    for (const auto& edge : edgeCount) {
        if (edge.second == 1) {
            ++report.boundaryEdges;
            if (report.boundarySamples.size() < 8 &&
                edge.first.first < mesh.vertices.size() &&
                edge.first.second < mesh.vertices.size()) {
                report.boundarySamples.push_back(
                    (mesh.vertices[edge.first.first].position +
                     mesh.vertices[edge.first.second].position) * 0.5f);
            }
        }
        if (edge.second > 2) ++report.nonManifoldEdges;
    }

    // Connected components over vertex adjacency (edges referenced by >= 2
    // triangles connect their endpoints).
    std::vector<std::vector<uint32_t>> adjacency(mesh.vertices.size());
    for (const auto& edge : edgeCount) {
        if (edge.second < 2) continue;
        adjacency[edge.first.first].push_back(edge.first.second);
        adjacency[edge.first.second].push_back(edge.first.first);
    }
    std::vector<char> visited(mesh.vertices.size(), 0);
    for (size_t start = 0; start < mesh.vertices.size(); ++start) {
        if (visited[start] || adjacency[start].empty()) continue;
        ++report.components;
        std::vector<size_t> queue{start};
        visited[start] = 1;
        while (!queue.empty()) {
            const size_t v = queue.back();
            queue.pop_back();
            for (uint32_t next : adjacency[v]) {
                if (!visited[next]) {
                    visited[next] = 1;
                    queue.push_back(next);
                }
            }
        }
    }
    return report;
}

} // namespace pae
