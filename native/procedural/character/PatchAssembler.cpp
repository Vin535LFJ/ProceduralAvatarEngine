#include "PatchAssembler.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <unordered_map>
#include <utility>

namespace pae::character {
namespace {

struct BoundaryRef {
    size_t patch = 0;
    size_t boundary = 0;
};

struct FaceEdge {
    uint32_t face = 0;
    int direction = 0;
};

bool orientAssembledMesh(MeshData& mesh, int* componentCount) {
    struct EdgeRecord {
        uint64_t key = 0;
        FaceEdge edge;
    };
    const size_t faceCount = mesh.indices.size() / 3;
    std::vector<EdgeRecord> edges;
    edges.reserve(faceCount * 3);
    for (uint32_t face = 0; face < faceCount; ++face) {
        const uint32_t ids[3] = {mesh.indices[face * 3], mesh.indices[face * 3 + 1],
                                 mesh.indices[face * 3 + 2]};
        for (uint32_t edge = 0; edge < 3; ++edge) {
            const uint32_t a = ids[edge];
            const uint32_t b = ids[(edge + 1) % 3];
            const uint64_t key = (static_cast<uint64_t>(std::min(a, b)) << 32) |
                                 std::max(a, b);
            edges.push_back({key, {face, a < b ? 1 : -1}});
        }
    }
    std::sort(edges.begin(), edges.end(), [](const EdgeRecord& a, const EdgeRecord& b) {
        return a.key < b.key;
    });
    std::vector<uint32_t> offsets(faceCount + 1, 0);
    for (size_t begin = 0; begin < edges.size();) {
        size_t end = begin + 1;
        while (end < edges.size() && edges[end].key == edges[begin].key) ++end;
        if (end - begin == 2) {
            ++offsets[edges[begin].edge.face + 1];
            ++offsets[edges[begin + 1].edge.face + 1];
        }
        begin = end;
    }
    for (size_t i = 1; i < offsets.size(); ++i) offsets[i] += offsets[i - 1];
    std::vector<uint32_t> neighbourFace(offsets.back());
    std::vector<uint8_t> neighbourFlip(offsets.back());
    std::vector<uint32_t> cursor(offsets.begin(), offsets.end() - 1);
    for (size_t begin = 0; begin < edges.size();) {
        size_t end = begin + 1;
        while (end < edges.size() && edges[end].key == edges[begin].key) ++end;
        if (end - begin == 2) {
            const FaceEdge a = edges[begin].edge;
            const FaceEdge b = edges[begin + 1].edge;
            const bool flip = a.direction == b.direction;
            neighbourFace[cursor[a.face]] = b.face;
            neighbourFlip[cursor[a.face]++] = flip ? 1u : 0u;
            neighbourFace[cursor[b.face]] = a.face;
            neighbourFlip[cursor[b.face]++] = flip ? 1u : 0u;
        }
        begin = end;
    }
    std::vector<int> parity(faceCount, -1);
    std::vector<std::vector<uint32_t>> faceComponents;
    for (uint32_t seed = 0; seed < faceCount; ++seed) {
        if (parity[seed] >= 0) continue;
        parity[seed] = 0;
        std::vector<uint32_t> component;
        std::vector<uint32_t> pending{seed};
        while (!pending.empty()) {
            const uint32_t face = pending.back();
            pending.pop_back();
            component.push_back(face);
            for (uint32_t i = offsets[face]; i < offsets[face + 1]; ++i) {
                const uint32_t neighbour = neighbourFace[i];
                const int next = parity[face] ^ (neighbourFlip[i] ? 1 : 0);
                if (parity[neighbour] < 0) {
                    parity[neighbour] = next;
                    pending.push_back(neighbour);
                } else if (parity[neighbour] != next) {
                    return false;
                }
            }
        }
        faceComponents.push_back(std::move(component));
    }
    for (uint32_t face = 0; face < faceCount; ++face) {
        if (parity[face] == 1) std::swap(mesh.indices[face * 3 + 1], mesh.indices[face * 3 + 2]);
    }
    for (const auto& component : faceComponents) {
        double volume6 = 0.0;
        for (uint32_t face : component) {
            const Vec3& a = mesh.vertices[mesh.indices[face * 3]].position;
            const Vec3& b = mesh.vertices[mesh.indices[face * 3 + 1]].position;
            const Vec3& c = mesh.vertices[mesh.indices[face * 3 + 2]].position;
            volume6 += a.dot(b.cross(c));
        }
        if (volume6 >= 0.0) continue;
        for (uint32_t face : component) {
            std::swap(mesh.indices[face * 3 + 1], mesh.indices[face * 3 + 2]);
        }
    }
    if (componentCount) *componentCount = static_cast<int>(faceComponents.size());
    return true;
}

} // namespace

PatchAssemblyResult assemblePatches(const std::vector<SurfacePatch>& patches) {
    PatchAssemblyResult result;
    std::unordered_map<std::string, std::vector<BoundaryRef>> boundaryGroups;
    std::set<uint64_t> sharedStableIds;
    std::unordered_map<uint64_t, Vec3> sharedPositions;
    std::unordered_map<uint64_t, size_t> sharedOwners;

    for (size_t patchIndex = 0; patchIndex < patches.size(); ++patchIndex) {
        const SurfacePatch& patch = patches[patchIndex];
        if (!patch.valid || patch.stableIds.size() != patch.mesh.vertices.size() ||
            patch.vertexRegions.size() != patch.mesh.vertices.size()) {
            result.error = "invalid patch: " + patch.id;
            return result;
        }
        std::set<uint64_t> localStableIds;
        for (size_t vertex = 0; vertex < patch.mesh.vertices.size(); ++vertex) {
            if (!localStableIds.insert(patch.stableIds[vertex]).second) {
                result.error = "duplicate stable id in patch: " + patch.id +
                               " vertex=" + std::to_string(vertex) +
                               " id=" + std::to_string(patch.stableIds[vertex]);
                return result;
            }
        }
        for (size_t boundaryIndex = 0; boundaryIndex < patch.boundaries.size(); ++boundaryIndex) {
            const PatchBoundary& boundary = patch.boundaries[boundaryIndex];
            if (boundary.vertexIds.size() < 3) {
                result.error = "short boundary: " + boundary.id;
                return result;
            }
            for (uint32_t local : boundary.vertexIds) {
                if (local >= patch.mesh.vertices.size()) {
                    result.error = "boundary vertex out of range: " + boundary.id +
                                   " patch=" + patch.id + " local=" + std::to_string(local);
                    return result;
                }
            }
            boundaryGroups[boundary.id].push_back({patchIndex, boundaryIndex});
        }
    }

    for (const auto& entry : boundaryGroups) {
        const std::vector<BoundaryRef>& refs = entry.second;
        if (refs.size() == 1) {
            const PatchBoundary& boundary = patches[refs[0].patch].boundaries[refs[0].boundary];
            if (!boundary.crop) {
                result.error = "unpaired internal boundary: " + entry.first;
                return result;
            }
            continue;
        }
        if (refs.size() != 2) {
            result.error = "boundary must have one or two owners: " + entry.first;
            return result;
        }
        const PatchBoundary& a = patches[refs[0].patch].boundaries[refs[0].boundary];
        const PatchBoundary& b = patches[refs[1].patch].boundaries[refs[1].boundary];
        if (a.crop || b.crop || a.vertexIds.size() != b.vertexIds.size()) {
            result.error = "incompatible shared boundary: " + entry.first;
            return result;
        }
        const SurfacePatch& patchA = patches[refs[0].patch];
        const SurfacePatch& patchB = patches[refs[1].patch];
        std::unordered_map<uint64_t, Vec3> positionsA;
        std::unordered_map<uint64_t, Vec3> positionsB;
        for (uint32_t local : a.vertexIds) {
            positionsA[patchA.stableIds[local]] = patchA.mesh.vertices[local].position;
        }
        for (uint32_t local : b.vertexIds) {
            positionsB[patchB.stableIds[local]] = patchB.mesh.vertices[local].position;
        }
        if (positionsA.size() != positionsB.size()) {
            result.error = "boundary stable-id mismatch: " + entry.first;
            return result;
        }
        for (const auto& item : positionsA) {
            const auto found = positionsB.find(item.first);
            if (found == positionsB.end() ||
                (item.second - found->second).length() > 1e-4f) {
                result.error = "boundary C0/stable-id mismatch: " + entry.first;
                return result;
            }
            const auto owner = sharedOwners.find(item.first);
            if (owner != sharedOwners.end() && owner->second != refs[0].patch) {
                result.error = "stable id shared by more than two patches: " + entry.first;
                return result;
            }
            sharedOwners[item.first] = refs[0].patch;
            sharedPositions[item.first] = item.second;
            sharedStableIds.insert(item.first);
        }
    }

    std::vector<uint32_t> vertexMap;
    size_t totalVertices = 0;
    for (const SurfacePatch& patch : patches) totalVertices += patch.mesh.vertices.size();
    vertexMap.assign(totalVertices, UINT32_MAX);
    std::unordered_map<uint64_t, uint32_t> finalByStableId;
    size_t offset = 0;
    for (const SurfacePatch& patch : patches) {
        for (size_t local = 0; local < patch.mesh.vertices.size(); ++local, ++offset) {
            const uint64_t stableId = patch.stableIds[local];
            const auto found = finalByStableId.find(stableId);
            if (found != finalByStableId.end()) {
                if (sharedStableIds.count(stableId) == 0) {
                    result.error = "unexpected duplicate stable id: " + patch.id;
                    return result;
                }
                vertexMap[offset] = found->second;
                continue;
            }
            const uint32_t finalIndex = static_cast<uint32_t>(result.mesh.vertices.size());
            result.mesh.vertices.push_back(patch.mesh.vertices[local]);
            result.stableIds.push_back(stableId);
            result.vertexRegions.push_back(patch.vertexRegions[local]);
            finalByStableId[stableId] = finalIndex;
            vertexMap[offset] = finalIndex;
        }
    }

    offset = 0;
    std::set<std::array<uint32_t, 3>> faces;
    for (const SurfacePatch& patch : patches) {
        for (size_t i = 0; i + 2 < patch.mesh.indices.size(); i += 3) {
            const uint32_t source[3] = {patch.mesh.indices[i], patch.mesh.indices[i + 1],
                                        patch.mesh.indices[i + 2]};
            uint32_t triangle[3]{};
            for (size_t corner = 0; corner < 3; ++corner) {
                if (source[corner] >= patch.mesh.vertices.size()) {
                    result.error = "index out of range in patch: " + patch.id;
                    return result;
                }
                triangle[corner] = vertexMap[offset + source[corner]];
                if (triangle[corner] == UINT32_MAX) {
                    result.error = "unmapped vertex in patch: " + patch.id;
                    return result;
                }
            }
            if (triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[2] == triangle[0]) {
                result.error = "degenerate assembled triangle: " + patch.id;
                return result;
            }
            std::array<uint32_t, 3> sorted = {triangle[0], triangle[1], triangle[2]};
            std::sort(sorted.begin(), sorted.end());
            if (!faces.insert(sorted).second) {
                result.error = "duplicate assembled face: " + patch.id;
                return result;
            }
            result.mesh.indices.insert(result.mesh.indices.end(), triangle, triangle + 3);
        }
        offset += patch.mesh.vertices.size();
    }

    if (!orientAssembledMesh(result.mesh, &result.topology.components)) {
        result.error = "assembled winding is inconsistent";
        return result;
    }
    std::vector<uint64_t> edgeKeys;
    edgeKeys.reserve(result.mesh.indices.size());
    for (size_t i = 0; i + 2 < result.mesh.indices.size(); i += 3) {
        const uint32_t triangle[3] = {result.mesh.indices[i], result.mesh.indices[i + 1],
                                      result.mesh.indices[i + 2]};
        for (size_t edge = 0; edge < 3; ++edge) {
            const uint32_t a = triangle[edge];
            const uint32_t b = triangle[(edge + 1) % 3];
            edgeKeys.push_back((static_cast<uint64_t>(std::min(a, b)) << 32) |
                               std::max(a, b));
        }
    }
    std::sort(edgeKeys.begin(), edgeKeys.end());
    result.topology.vertices = result.mesh.vertices.size();
    result.topology.triangles = static_cast<long>(result.mesh.indices.size() / 3);
    for (size_t begin = 0; begin < edgeKeys.size();) {
        size_t end = begin + 1;
        while (end < edgeKeys.size() && edgeKeys[end] == edgeKeys[begin]) ++end;
        const size_t count = end - begin;
        if (count == 1) ++result.topology.boundaryEdges;
        if (count > 2) {
            ++result.topology.nonManifoldEdges;
            result.error = "assembled edge incidence failure count=" +
                           std::to_string(count) + " key=" + std::to_string(edgeKeys[begin]);
            return result;
        }
        begin = end;
    }

    offset = 0;
    for (const SurfacePatch& patch : patches) {
        for (PatchBoundary boundary : patch.boundaries) {
            for (uint32_t& local : boundary.vertexIds) {
                if (offset + local >= vertexMap.size()) {
                    result.error = "boundary remap out of range: " + boundary.id +
                                   " patch=" + patch.id + " local=" + std::to_string(local) +
                                   " offset=" + std::to_string(offset);
                    return result;
                }
                local = vertexMap[offset + local];
                if (local >= result.mesh.vertices.size()) {
                    result.error = "boundary remap invalid final index: " + boundary.id +
                                   " patch=" + patch.id + " final=" + std::to_string(local);
                    return result;
                }
            }
            const size_t edgeCount = boundary.closed ? boundary.vertexIds.size()
                                                      : boundary.vertexIds.size() - 1;
            for (size_t i = 0; i < edgeCount; ++i) {
                const uint32_t a = boundary.vertexIds[i];
                const uint32_t b = boundary.vertexIds[(i + 1) % boundary.vertexIds.size()];
                const uint32_t lo = std::min(a, b);
                const uint32_t hi = std::max(a, b);
                const uint64_t key = (static_cast<uint64_t>(lo) << 32) | hi;
                const auto found = std::lower_bound(edgeKeys.begin(), edgeKeys.end(), key);
                const uint32_t expected = boundary.crop ? 1u : 2u;
                const bool present = found != edgeKeys.end() && *found == key;
                const size_t actual = present
                    ? static_cast<size_t>(std::upper_bound(found, edgeKeys.end(), key) - found) : 0;
                if (!present || actual != expected) {
                    std::string ids;
                    for (uint32_t id : boundary.vertexIds) ids += std::to_string(id) + ",";
                    result.error = "boundary edge incidence failure: " + boundary.id +
                                   " expected=" + std::to_string(expected) +
                                   " actual=" + (present ? std::to_string(actual) : "missing") +
                                   " edge=" + std::to_string(a) + "-" + std::to_string(b) +
                                   " final_vertices=" + std::to_string(result.mesh.vertices.size()) +
                                   " ids=" + ids;
                    return result;
                }
            }
            result.boundaries.push_back(std::move(boundary));
        }
        offset += patch.mesh.vertices.size();
    }

    for (const Vertex& vertex : result.mesh.vertices) result.mesh.bounds.expand(vertex.position);
    result.mesh.subMeshes.push_back({0, static_cast<uint32_t>(result.mesh.indices.size()), 0});
    result.valid = !result.mesh.vertices.empty() && !result.mesh.indices.empty() &&
                   result.stableIds.size() == result.mesh.vertices.size() &&
                   result.vertexRegions.size() == result.mesh.vertices.size();
    return result;
}

} // namespace pae::character
