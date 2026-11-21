#include "Primitives.h"

#include <algorithm>
#include <cmath>

namespace pae {
namespace {

uint32_t atLeast(uint32_t value, uint32_t minimum) {
    return value < minimum ? minimum : value;
}

void finalizeMesh(MeshData& mesh) {
    Aabb bounds;
    for (const Vertex& vertex : mesh.vertices) bounds.expand(vertex.position);
    mesh.bounds = bounds;
    if (!mesh.subMeshes.empty()) return;
    SubMesh sub;
    sub.firstIndex = 0;
    sub.indexCount = static_cast<uint32_t>(mesh.indices.size());
    mesh.subMeshes.push_back(sub);
}

MeshData buildGrid(uint32_t ringCount, uint32_t segments,
                   const std::vector<Vec3>& positions,
                   const std::vector<Vec3>& normals,
                   bool reverseWinding) {
    MeshData mesh;
    mesh.vertices.resize(positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        mesh.vertices[i].position = positions[i];
        mesh.vertices[i].normal = normals[i].normalized({0.0f, 1.0f, 0.0f});
        mesh.vertices[i].uv = {
            static_cast<float>(i % segments) / static_cast<float>(segments),
            static_cast<float>(i / segments) / static_cast<float>(ringCount - 1)
        };
    }
    for (uint32_t r = 0; r + 1 < ringCount; ++r) {
        for (uint32_t s = 0; s < segments; ++s) {
            const uint32_t next = (s + 1) % segments;
            const uint32_t a = r * segments + s;
            const uint32_t b = r * segments + next;
            const uint32_t c = (r + 1) * segments + s;
            const uint32_t d = (r + 1) * segments + next;
            if (reverseWinding) {
                mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
            } else {
                mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
            }
        }
    }
    finalizeMesh(mesh);
    return mesh;
}

} // namespace

MeshData generateUvSphere(float radius, uint32_t segments, uint32_t rings) {
    segments = atLeast(segments, 3);
    rings = atLeast(rings, 2);
    const float r = std::fmax(radius, 1e-4f);
    const uint32_t ringCount = rings + 1;
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    positions.reserve(ringCount * segments);
    normals.reserve(ringCount * segments);
    for (uint32_t i = 0; i < ringCount; ++i) {
        const float theta = static_cast<float>(i) * 3.14159265f / static_cast<float>(rings);
        const float sy = std::cos(theta);
        const float sr = std::sin(theta);
        for (uint32_t j = 0; j < segments; ++j) {
            const float phi = 2.0f * 3.14159265f * static_cast<float>(j) / static_cast<float>(segments);
            const Vec3 n{sr * std::cos(phi), sy, sr * std::sin(phi)};
            normals.push_back(n);
            positions.push_back(n * r);
        }
    }
    return buildGrid(ringCount, segments, positions, normals, false);
}

MeshData generateEllipsoid(const Vec3& radii, uint32_t segments, uint32_t rings) {
    const Vec3 safe{std::fmax(radii.x, 1e-4f), std::fmax(radii.y, 1e-4f), std::fmax(radii.z, 1e-4f)};
    segments = atLeast(segments, 3);
    rings = atLeast(rings, 2);
    const uint32_t ringCount = rings + 1;
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    for (uint32_t i = 0; i < ringCount; ++i) {
        const float theta = static_cast<float>(i) * 3.14159265f / static_cast<float>(rings);
        for (uint32_t j = 0; j < segments; ++j) {
            const float phi = 2.0f * 3.14159265f * static_cast<float>(j) / static_cast<float>(segments);
            const Vec3 unit{std::sin(theta) * std::cos(phi), std::cos(theta),
                            std::sin(theta) * std::sin(phi)};
            positions.push_back({unit.x * safe.x, unit.y * safe.y, unit.z * safe.z});
            normals.push_back({unit.x / safe.x, unit.y / safe.y, unit.z / safe.z});
        }
    }
    return buildGrid(ringCount, segments, positions, normals, false);
}

MeshData generateCapsule(float radius, float height, uint32_t segments,
                         uint32_t capRings, uint32_t bodyRings) {
    const float r = std::fmax(radius, 1e-4f);
    const float h = std::fmax(height, 0.0f);
    segments = atLeast(segments, 3);
    capRings = atLeast(capRings, 2);
    bodyRings = atLeast(bodyRings, 1);
    const uint32_t totalRings = capRings * 2 + bodyRings + 1;
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    positions.reserve(totalRings * segments);
    normals.reserve(totalRings * segments);

    for (uint32_t ring = 0; ring < totalRings; ++ring) {
        float y = 0.0f;
        float radial = r;
        float normalY = 0.0f;
        if (ring < capRings) {
            const float t = static_cast<float>(capRings - ring) / static_cast<float>(capRings);
            const float angle = t * 1.57079633f;
            radial = r * std::cos(angle);
            normalY = -std::sin(angle);
            y = -h * 0.5f - r * std::sin(angle);
        } else if (ring < capRings + bodyRings + 1) {
            const float t = static_cast<float>(ring - capRings) / static_cast<float>(bodyRings);
            radial = r;
            normalY = 0.0f;
            y = -h * 0.5f + t * h;
        } else {
            const float t = static_cast<float>(ring - capRings - bodyRings) / static_cast<float>(capRings);
            const float angle = t * 1.57079633f;
            radial = r * std::cos(angle);
            normalY = std::sin(angle);
            y = h * 0.5f + r * std::sin(angle);
        }
        for (uint32_t s = 0; s < segments; ++s) {
            const float phi = 2.0f * 3.14159265f * static_cast<float>(s) / static_cast<float>(segments);
            const Vec3 radialDir{std::cos(phi), 0.0f, std::sin(phi)};
            positions.push_back({radialDir.x * radial, y, radialDir.z * radial});
            const float radialNormal = std::sqrt(std::fmax(0.0f, 1.0f - normalY * normalY));
            const Vec3 n{radialDir.x * radialNormal, normalY, radialDir.z * radialNormal};
            normals.push_back(n.normalized({0.0f, 1.0f, 0.0f}));
        }
    }
    return buildGrid(totalRings, segments, positions, normals, true);
}

MeshData generateRoundedBox(const Vec3& size, float cornerRadius, uint32_t subdivisions) {
    const Vec3 half{std::fmax(size.x, 1e-3f) * 0.5f,
                    std::fmax(size.y, 1e-3f) * 0.5f,
                    std::fmax(size.z, 1e-3f) * 0.5f};
    const float radius = std::fmin(std::fmax(cornerRadius, 1e-4f),
                                   std::fmin(half.x, std::fmin(half.y, half.z)) * 0.99f);
    subdivisions = atLeast(subdivisions, 1);
    const uint32_t n = subdivisions + 1;

    struct Face { Vec3 normal, u, v, half; };
    const Face faces[6] = {
        {{1,0,0},  {0,1,0},  {0,0,1},  {half.x, half.y, half.z}},
        {{-1,0,0}, {0,0,1},  {0,1,0},  {half.x, half.y, half.z}},
        {{0,1,0},  {0,0,1},  {1,0,0},  {half.x, half.y, half.z}},
        {{0,-1,0}, {1,0,0},  {0,0,1},  {half.x, half.y, half.z}},
        {{0,0,1},  {1,0,0},  {0,1,0},  {half.x, half.y, half.z}},
        {{0,0,-1}, {0,1,0},  {1,0,0},  {half.x, half.y, half.z}}
    };

    MeshData mesh;
    mesh.vertices.reserve(6 * n * n);
    for (const Face& face : faces) {
        const Vec3 coreLimit{
            std::fmax(0.0f, half.x - radius),
            std::fmax(0.0f, half.y - radius),
            std::fmax(0.0f, half.z - radius)
        };
        const float extentU = std::fabs(face.u.x) * half.x + std::fabs(face.u.y) * half.y + std::fabs(face.u.z) * half.z;
        const float extentV = std::fabs(face.v.x) * half.x + std::fabs(face.v.y) * half.y + std::fabs(face.v.z) * half.z;
        for (uint32_t i = 0; i < n; ++i) {
            for (uint32_t j = 0; j < n; ++j) {
                const float a = -extentU + 2.0f * extentU * static_cast<float>(i) / static_cast<float>(subdivisions);
                const float b = -extentV + 2.0f * extentV * static_cast<float>(j) / static_cast<float>(subdivisions);
                const Vec3 plane = face.normal * (std::fabs(face.normal.x) * half.x +
                                                   std::fabs(face.normal.y) * half.y +
                                                   std::fabs(face.normal.z) * half.z) + face.u * a + face.v * b;
                const Vec3 core{
                    clamp(plane.x, -coreLimit.x, coreLimit.x),
                    clamp(plane.y, -coreLimit.y, coreLimit.y),
                    clamp(plane.z, -coreLimit.z, coreLimit.z)
                };
                const Vec3 delta = plane - core;
                const Vec3 normal = delta.lengthSq() > 1e-12f ? delta.normalized() : face.normal;
                Vertex vertex;
                vertex.position = core + normal * radius;
                vertex.normal = normal;
                vertex.uv = {static_cast<float>(i) / static_cast<float>(subdivisions),
                             static_cast<float>(j) / static_cast<float>(subdivisions)};
                mesh.vertices.push_back(vertex);
            }
        }
    }
    const uint32_t faceVertices = n * n;
    for (uint32_t f = 0; f < 6; ++f) {
        const uint32_t base = f * faceVertices;
        for (uint32_t i = 0; i < subdivisions; ++i) {
            for (uint32_t j = 0; j < subdivisions; ++j) {
                const uint32_t a = base + i * n + j;
                const uint32_t b = base + (i + 1) * n + j;
                const uint32_t c = base + i * n + j + 1;
                const uint32_t d = base + (i + 1) * n + j + 1;
                mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
            }
        }
    }
    finalizeMesh(mesh);
    return mesh;
}

} // namespace pae
