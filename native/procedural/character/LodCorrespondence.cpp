#include "LodCorrespondence.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace pae::character {

LodMap buildLodMap(const std::vector<uint64_t>& fromStableIds,
                   const std::vector<uint64_t>& toStableIds,
                   uint32_t fromLevel, uint32_t toLevel) {
    LodMap map;
    map.fromLevel = fromLevel;
    map.toLevel = toLevel;
    map.fromStableIds = fromStableIds;
    map.toStableIds = toStableIds;
    map.toIndices.assign(fromStableIds.size(), UINT32_MAX);
    std::unordered_map<uint64_t, uint32_t> indexById;
    indexById.reserve(toStableIds.size());
    for (uint32_t i = 0; i < toStableIds.size(); ++i) {
        indexById[toStableIds[i]] = i;
    }
    bool identity = fromStableIds.size() == toStableIds.size();
    size_t mapped = 0;
    for (size_t i = 0; i < fromStableIds.size(); ++i) {
        const auto it = indexById.find(fromStableIds[i]);
        if (it == indexById.end()) {
            identity = false;
            continue;
        }
        map.toIndices[i] = it->second;
        if (it->second != i) identity = false;
        ++mapped;
    }
    map.identity = identity && mapped == fromStableIds.size();
    map.valid = mapped == fromStableIds.size();
    return map;
}

namespace {

struct ClosestTrianglePoint {
    float distanceSq = std::numeric_limits<float>::max();
    Vec2 barycentric{};
};

ClosestTrianglePoint closestOnTriangle(const Vec3& point, const Vec3& a,
                                       const Vec3& b, const Vec3& c) {
    ClosestTrianglePoint result;
    const Vec3 ab = b - a;
    const Vec3 ac = c - a;
    const Vec3 ap = point - a;
    const float d1 = ab.dot(ap);
    const float d2 = ac.dot(ap);
    if (d1 <= 0.0f && d2 <= 0.0f) {
        result.distanceSq = ap.lengthSq();
        result.barycentric = {1.0f, 0.0f};
        return result;
    }
    const Vec3 bp = point - b;
    const float d3 = ab.dot(bp);
    const float d4 = ac.dot(bp);
    if (d3 >= 0.0f && d4 <= d3) {
        result.distanceSq = bp.lengthSq();
        result.barycentric = {0.0f, 1.0f};
        return result;
    }
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);
        const Vec3 closest = a + ab * v;
        result.distanceSq = (point - closest).lengthSq();
        result.barycentric = {1.0f - v, v};
        return result;
    }
    const Vec3 cp = point - c;
    const float d5 = ab.dot(cp);
    const float d6 = ac.dot(cp);
    if (d6 >= 0.0f && d5 <= d6) {
        result.distanceSq = cp.lengthSq();
        result.barycentric = {0.0f, 0.0f};
        return result;
    }
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);
        const Vec3 closest = a + ac * w;
        result.distanceSq = (point - closest).lengthSq();
        result.barycentric = {1.0f - w, 0.0f};
        return result;
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        const Vec3 closest = b + (c - b) * w;
        result.distanceSq = (point - closest).lengthSq();
        result.barycentric = {0.0f, 1.0f - w};
        return result;
    }
    const float denominator = 1.0f / (va + vb + vc);
    const float v = vb * denominator;
    const float w = vc * denominator;
    const Vec3 closest = a + ab * v + ac * w;
    result.distanceSq = (point - closest).lengthSq();
    result.barycentric = {1.0f - v - w, v};
    return result;
}

uint64_t faceStableId(const MeshData& mesh, uint32_t face) {
    uint64_t hash = 1469598103934665603ull;
    for (uint32_t corner = 0; corner < 3; ++corner) {
        const uint32_t id = mesh.indices[face * 3 + corner];
        hash ^= id;
        hash *= 1099511628211ull;
    }
    return hash;
}

} // namespace

LodMap buildLodMap(const MeshData& fromMesh, const MeshData& toMesh,
                   uint32_t fromLevel, uint32_t toLevel) {
    LodMap map;
    map.fromLevel = fromLevel;
    map.toLevel = toLevel;
    map.fromStableIds.reserve(fromMesh.vertices.size());
    map.toStableIds.reserve(toMesh.vertices.size());
    map.toIndices.reserve(toMesh.vertices.size());
    map.vertices.reserve(toMesh.vertices.size());
    for (size_t i = 0; i < fromMesh.vertices.size(); ++i) {
        map.fromStableIds.push_back(faceStableId(fromMesh, static_cast<uint32_t>(i)));
    }
    bool valid = fromMesh.indices.size() >= 3 && fromMesh.indices.size() % 3 == 0;
    for (size_t i = 0; i < toMesh.vertices.size(); ++i) {
        ClosestTrianglePoint closest;
        uint32_t parentFace = UINT32_MAX;
        for (size_t face = 0; face + 2 < fromMesh.indices.size(); face += 3) {
            const uint32_t ids[3] = {fromMesh.indices[face], fromMesh.indices[face + 1],
                                     fromMesh.indices[face + 2]};
            if (ids[0] >= fromMesh.vertices.size() || ids[1] >= fromMesh.vertices.size() ||
                ids[2] >= fromMesh.vertices.size()) {
                valid = false;
                continue;
            }
            const ClosestTrianglePoint candidate = closestOnTriangle(
                toMesh.vertices[i].position, fromMesh.vertices[ids[0]].position,
                fromMesh.vertices[ids[1]].position, fromMesh.vertices[ids[2]].position);
            if (candidate.distanceSq < closest.distanceSq) {
                closest = candidate;
                parentFace = static_cast<uint32_t>(face / 3);
            }
        }
        RefinementVertex vertex;
        vertex.stableId = faceStableId(toMesh, static_cast<uint32_t>(i));
        vertex.lodLevel = toLevel;
        vertex.parentFaceStableId = parentFace == UINT32_MAX
            ? 0 : faceStableId(fromMesh, parentFace);
        vertex.barycentric = closest.barycentric;
        vertex.refinementPath = parentFace == UINT32_MAX ? 0 : 1;
        map.toStableIds.push_back(vertex.stableId);
        map.toIndices.push_back(static_cast<uint32_t>(i));
        map.vertices.push_back(vertex);
        if (parentFace == UINT32_MAX || !std::isfinite(closest.distanceSq)) valid = false;
    }
    map.identity = false;
    map.valid = valid;
    return map;
}

} // namespace pae::character