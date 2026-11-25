#include "CanonicalTopology.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace pae::canonical {
namespace {

struct IntersectionTriangle {
    std::array<uint32_t, 3> vertices{};
    std::array<Vec3, 3> points{};
    Vec3 center{};
    Aabb bounds;
};

struct IntersectionNode {
    Aabb bounds;
    int32_t left = -1;
    int32_t right = -1;
    uint32_t begin = 0;
    uint32_t count = 0;
};

bool overlap(const Aabb& a, const Aabb& b, float epsilon) {
    return a.valid && b.valid &&
           a.max.x + epsilon >= b.min.x && b.max.x + epsilon >= a.min.x &&
           a.max.y + epsilon >= b.min.y && b.max.y + epsilon >= a.min.y &&
           a.max.z + epsilon >= b.min.z && b.max.z + epsilon >= a.min.z;
}

bool sharesVertex(const IntersectionTriangle& a, const IntersectionTriangle& b) {
    for (uint32_t av : a.vertices) {
        for (uint32_t bv : b.vertices) {
            if (av == bv) return true;
        }
    }
    return false;
}

bool pointInTriangle(const Vec3& p, const IntersectionTriangle& triangle, float epsilon) {
    const Vec3 v0 = triangle.points[1] - triangle.points[0];
    const Vec3 v1 = triangle.points[2] - triangle.points[0];
    const Vec3 v2 = p - triangle.points[0];
    const float d00 = v0.dot(v0);
    const float d01 = v0.dot(v1);
    const float d11 = v1.dot(v1);
    const float d20 = v2.dot(v0);
    const float d21 = v2.dot(v1);
    const float denominator = d00 * d11 - d01 * d01;
    if (std::fabs(denominator) < 1e-16f) return false;
    const float v = (d11 * d20 - d01 * d21) / denominator;
    const float w = (d00 * d21 - d01 * d20) / denominator;
    const float u = 1.0f - v - w;
    return u >= -epsilon && v >= -epsilon && w >= -epsilon;
}

bool segmentHitsTriangle(const Vec3& a, const Vec3& b,
                         const IntersectionTriangle& triangle, const Vec3& normal,
                         float epsilon) {
    const float da = normal.dot(a - triangle.points[0]);
    const float db = normal.dot(b - triangle.points[0]);
    if ((da > epsilon && db > epsilon) || (da < -epsilon && db < -epsilon)) return false;
    if (std::fabs(da) <= epsilon && std::fabs(db) <= epsilon) return false;
    const float denominator = da - db;
    if (std::fabs(denominator) < 1e-12f) return false;
    const float t = da / denominator;
    if (t < -epsilon || t > 1.0f + epsilon) return false;
    return pointInTriangle(a + (b - a) * t, triangle, 1e-5f);
}

float orient2d(const Vec3& a, const Vec3& b, const Vec3& c, int dropAxis) {
    if (dropAxis == 0) return (b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y);
    if (dropAxis == 1) return (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool coplanarTrianglesIntersect(const IntersectionTriangle& a, const IntersectionTriangle& b,
                                const Vec3& normal) {
    const Vec3 magnitude{std::fabs(normal.x), std::fabs(normal.y), std::fabs(normal.z)};
    const int dropAxis = magnitude.x >= magnitude.y && magnitude.x >= magnitude.z ? 0
                         : magnitude.y >= magnitude.z ? 1 : 2;
    float scale = 0.0f;
    for (const IntersectionTriangle* triangle : {&a, &b}) {
        for (size_t i = 0; i < 3; ++i) {
            scale = std::fmax(scale,
                (triangle->points[i] - triangle->points[(i + 1) % 3]).length());
        }
    }
    if (scale < 1e-12f) return false;
    const Vec3 origin = a.points[0];
    const float inverseScale = 1.0f / scale;
    Vec3 pointsA[3];
    Vec3 pointsB[3];
    for (size_t i = 0; i < 3; ++i) {
        pointsA[i] = (a.points[i] - origin) * inverseScale;
        pointsB[i] = (b.points[i] - origin) * inverseScale;
    }
    constexpr float epsilon = 1e-7f;
    const auto separated = [dropAxis, epsilon](const Vec3* pointsA, const Vec3* pointsB,
                                               const Vec3& p, const Vec3& q) {
        float minA = std::numeric_limits<float>::max();
        float maxA = -std::numeric_limits<float>::max();
        float minB = std::numeric_limits<float>::max();
        float maxB = -std::numeric_limits<float>::max();
        for (size_t i = 0; i < 3; ++i) {
            const float valueA = orient2d(p, q, pointsA[i], dropAxis);
            const float valueB = orient2d(p, q, pointsB[i], dropAxis);
            minA = std::fmin(minA, valueA);
            maxA = std::fmax(maxA, valueA);
            minB = std::fmin(minB, valueB);
            maxB = std::fmax(maxB, valueB);
        }
        return maxA <= minB + epsilon || maxB <= minA + epsilon;
    };
    for (size_t i = 0; i < 3; ++i) {
        if (separated(pointsA, pointsB, pointsA[i], pointsA[(i + 1) % 3]) ||
            separated(pointsA, pointsB, pointsB[i], pointsB[(i + 1) % 3])) return false;
    }
    return true;
}

bool trianglesIntersect(const IntersectionTriangle& a, const IntersectionTriangle& b) {
    float scale = 0.0f;
    for (const IntersectionTriangle* triangle : {&a, &b}) {
        for (size_t i = 0; i < 3; ++i) {
            scale = std::fmax(scale,
                (triangle->points[i] - triangle->points[(i + 1) % 3]).length());
        }
    }
    if (!(scale > 0.0f) || !std::isfinite(scale)) return false;

    // Pair-local normalization keeps tolerances invariant under scale and translation.
    IntersectionTriangle localA = a;
    IntersectionTriangle localB = b;
    const float inverseScale = 1.0f / scale;
    for (size_t i = 0; i < 3; ++i) {
        localA.points[i] = (a.points[i] - a.points[0]) * inverseScale;
        localB.points[i] = (b.points[i] - a.points[0]) * inverseScale;
    }
    constexpr float epsilon = 1e-7f;
    const Vec3 normalA = (localA.points[1] - localA.points[0]).cross(localA.points[2] - localA.points[0]);
    const Vec3 normalB = (localB.points[1] - localB.points[0]).cross(localB.points[2] - localB.points[0]);
    const float lengthA = normalA.length();
    const float lengthB = normalB.length();
    if (!(lengthA > 0.0f) || !(lengthB > 0.0f) ||
        !std::isfinite(lengthA) || !std::isfinite(lengthB)) return false;
    const Vec3 unitNormalA = normalA * (1.0f / lengthA);
    const Vec3 unitNormalB = normalB * (1.0f / lengthB);
    float distanceA[3];
    float distanceB[3];
    bool positiveA = false, negativeA = false, positiveB = false, negativeB = false;
    for (size_t i = 0; i < 3; ++i) {
        distanceA[i] = unitNormalA.dot(localB.points[i] - localA.points[0]);
        distanceB[i] = unitNormalB.dot(localA.points[i] - localB.points[0]);
        positiveA = positiveA || distanceA[i] > epsilon;
        negativeA = negativeA || distanceA[i] < -epsilon;
        positiveB = positiveB || distanceB[i] > epsilon;
        negativeB = negativeB || distanceB[i] < -epsilon;
    }
    const bool coplanarA = !positiveA && !negativeA;
    const bool coplanarB = !positiveB && !negativeB;
    if (coplanarA && coplanarB) return coplanarTrianglesIntersect(localA, localB, normalA);
    if (sharesVertex(localA, localB)) return false;
    if ((positiveA != negativeA) || (positiveB != negativeB)) return false;
    for (size_t i = 0; i < 3; ++i) {
        if (segmentHitsTriangle(localA.points[i], localA.points[(i + 1) % 3], localB, unitNormalB, epsilon) ||
            segmentHitsTriangle(localB.points[i], localB.points[(i + 1) % 3], localA, unitNormalA, epsilon)) {
            return true;
        }
    }
    return false;
}

} // namespace

SelfIntersectionReport detectSelfIntersections(const MeshData& mesh, size_t maxSamples) {
    SelfIntersectionReport report;
    const size_t triangleCount = mesh.indices.size() / 3;
    if (triangleCount < 2) return report;
    std::vector<IntersectionTriangle> triangles;
    triangles.reserve(triangleCount);
    for (size_t i = 0; i < triangleCount; ++i) {
        IntersectionTriangle triangle;
        triangle.vertices = {mesh.indices[i * 3], mesh.indices[i * 3 + 1], mesh.indices[i * 3 + 2]};
        if (triangle.vertices[0] >= mesh.vertices.size() ||
            triangle.vertices[1] >= mesh.vertices.size() ||
            triangle.vertices[2] >= mesh.vertices.size()) continue;
        for (size_t corner = 0; corner < 3; ++corner) {
            triangle.points[corner] = mesh.vertices[triangle.vertices[corner]].position;
            if (!triangle.points[corner].finite()) return report;
            triangle.bounds.expand(triangle.points[corner]);
            triangle.center = triangle.center + triangle.points[corner];
        }
        triangle.center = triangle.center * (1.0f / 3.0f);
        triangles.push_back(triangle);
    }
    if (triangles.size() < 2) return report;

    std::vector<uint32_t> order(triangles.size());
    for (uint32_t i = 0; i < order.size(); ++i) order[i] = i;
    std::vector<IntersectionNode> nodes;
    nodes.reserve(triangles.size() * 2);
    auto build = [&](auto&& self, uint32_t begin, uint32_t count) -> int32_t {
        Aabb bounds;
        Aabb centers;
        for (uint32_t i = begin; i < begin + count; ++i) {
            const IntersectionTriangle& triangle = triangles[order[i]];
            bounds.expand(triangle.bounds.min);
            bounds.expand(triangle.bounds.max);
            centers.expand(triangle.center);
        }
        if (count <= 8) {
            const int32_t index = static_cast<int32_t>(nodes.size());
            nodes.push_back({bounds, -1, -1, begin, count});
            return index;
        }
        const Vec3 extent = centers.size();
        const int axis = extent.x >= extent.y && extent.x >= extent.z ? 0
                       : extent.y >= extent.z ? 1 : 2;
        const uint32_t middle = begin + count / 2;
        std::nth_element(order.begin() + begin, order.begin() + middle, order.begin() + begin + count,
            [&](uint32_t a, uint32_t b) {
                const Vec3& ca = triangles[a].center;
                const Vec3& cb = triangles[b].center;
                return axis == 0 ? ca.x < cb.x : axis == 1 ? ca.y < cb.y : ca.z < cb.z;
            });
        const int32_t left = self(self, begin, count / 2);
        const int32_t right = self(self, middle, count - count / 2);
        const int32_t index = static_cast<int32_t>(nodes.size());
        nodes.push_back({bounds, left, right, begin, count});
        return index;
    };
    const int32_t root = build(build, 0, static_cast<uint32_t>(order.size()));
    auto query = [&](auto&& self, int32_t a, int32_t b) -> void {
        if (a < 0 || b < 0 || !overlap(nodes[a].bounds, nodes[b].bounds, 1e-7f)) return;
        const bool leafA = nodes[a].left < 0;
        const bool leafB = nodes[b].left < 0;
        if (a == b) {
            if (leafA) {
                for (uint32_t i = nodes[a].begin; i < nodes[a].begin + nodes[a].count; ++i) {
                    for (uint32_t j = i + 1; j < nodes[a].begin + nodes[a].count; ++j) {
                        const uint32_t idA = order[i], idB = order[j];
                        if (!overlap(triangles[idA].bounds, triangles[idB].bounds, 1e-7f)) continue;
                        ++report.candidatePairs;
                        if (trianglesIntersect(triangles[idA], triangles[idB])) {
                            ++report.intersectingPairs;
                            if (report.samples.size() < maxSamples) report.samples.push_back({idA, idB});
                        }
                    }
                }
            } else {
                self(self, nodes[a].left, nodes[a].left);
                self(self, nodes[a].right, nodes[a].right);
                self(self, nodes[a].left, nodes[a].right);
            }
            return;
        }
        if (leafA && leafB) {
            for (uint32_t i = nodes[a].begin; i < nodes[a].begin + nodes[a].count; ++i) {
                for (uint32_t j = nodes[b].begin; j < nodes[b].begin + nodes[b].count; ++j) {
                    const uint32_t idA = order[i], idB = order[j];
                    ++report.candidatePairs;
                    if (sharesVertex(triangles[idA], triangles[idB])) continue;
                    if (trianglesIntersect(triangles[idA], triangles[idB])) {
                        ++report.intersectingPairs;
                        if (report.samples.size() < maxSamples) report.samples.push_back({idA, idB});
                    }
                }
            }
        } else if (leafA) {
            self(self, a, nodes[b].left);
            self(self, a, nodes[b].right);
        } else if (leafB) {
            self(self, nodes[a].left, b);
            self(self, nodes[a].right, b);
        } else {
            self(self, nodes[a].left, nodes[b].left);
            self(self, nodes[a].left, nodes[b].right);
            self(self, nodes[a].right, nodes[b].left);
            self(self, nodes[a].right, nodes[b].right);
        }
    };
    query(query, root, root);
    return report;
}

} // namespace pae::canonical
