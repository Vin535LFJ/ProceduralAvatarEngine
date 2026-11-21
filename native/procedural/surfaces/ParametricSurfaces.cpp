#include "ParametricSurfaces.h"

#include <algorithm>
#include <cmath>

namespace pae {
namespace {

MeshData finalizeSurface(MeshData& mesh) {
    Aabb bounds;
    for (const Vertex& v : mesh.vertices) bounds.expand(v.position);
    mesh.bounds = bounds;
    if (mesh.subMeshes.empty()) {
        SubMesh sub;
        sub.indexCount = static_cast<uint32_t>(mesh.indices.size());
        mesh.subMeshes.push_back(sub);
    }
    return mesh;
}

} // namespace

std::vector<SurfaceFrame> computeParallelTransportFrames(const std::vector<Vec3>& points) {
    std::vector<SurfaceFrame> frames;
    const size_t count = points.size();
    if (count < 2) return frames;
    frames.resize(count);

    std::vector<Vec3> tangents(count);
    for (size_t i = 0; i < count; ++i) {
        if (i == 0) tangents[i] = (points[1] - points[0]).normalized({0.0f, 0.0f, 1.0f});
        else if (i + 1 == count) tangents[i] = (points[i] - points[i - 1]).normalized({0.0f, 0.0f, 1.0f});
        else tangents[i] = (points[i + 1] - points[i - 1]).normalized({0.0f, 0.0f, 1.0f});
    }

    const Vec3 reference = std::fabs(tangents[0].dot({0.0f, 1.0f, 0.0f})) > 0.9f
                               ? Vec3{1.0f, 0.0f, 0.0f}
                               : Vec3{0.0f, 1.0f, 0.0f};
    Vec3 normal = (reference - tangents[0] * reference.dot(tangents[0])).normalized({1.0f, 0.0f, 0.0f});
    frames[0].tangent = tangents[0];
    frames[0].normal = normal;
    frames[0].binormal = tangents[0].cross(normal).normalized();

    for (size_t i = 1; i < count; ++i) {
        const Quat rotation = Quat::fromToRotation(tangents[i - 1], tangents[i]);
        normal = rotation.rotate(normal);
        normal = (normal - tangents[i] * normal.dot(tangents[i])).normalized({1.0f, 0.0f, 0.0f});
        frames[i].tangent = tangents[i];
        frames[i].normal = normal;
        frames[i].binormal = tangents[i].cross(normal).normalized();
    }
    return frames;
}

MeshData generateSweepSurface(const std::vector<Vec3>& centerline,
                              const std::vector<float>& radii,
                              uint32_t radialSegments) {
    MeshData mesh;
    if (centerline.size() < 2 || radii.size() != centerline.size() || radialSegments < 3) return mesh;
    const std::vector<SurfaceFrame> frames = computeParallelTransportFrames(centerline);
    const uint32_t segments = radialSegments;
    mesh.vertices.reserve(centerline.size() * segments);
    for (size_t i = 0; i < centerline.size(); ++i) {
        const float radius = std::fmax(radii[i], 1e-4f);
        for (uint32_t s = 0; s < segments; ++s) {
            const float angle = 2.0f * 3.14159265f * static_cast<float>(s) / static_cast<float>(segments);
            const float c = std::cos(angle), sn = std::sin(angle);
            const Vec3 radial = frames[i].normal * c + frames[i].binormal * sn;
            Vertex vertex;
            vertex.position = centerline[i] + radial * radius;
            vertex.normal = radial.normalized();
            vertex.uv = {static_cast<float>(s) / static_cast<float>(segments),
                         static_cast<float>(i) / static_cast<float>(centerline.size() - 1)};
            mesh.vertices.push_back(vertex);
        }
    }
    for (size_t i = 0; i + 1 < centerline.size(); ++i) {
        for (uint32_t s = 0; s < segments; ++s) {
            const uint32_t next = (s + 1) % segments;
            const uint32_t a = static_cast<uint32_t>(i) * segments + s;
            const uint32_t b = static_cast<uint32_t>(i) * segments + next;
            const uint32_t c = static_cast<uint32_t>(i + 1) * segments + s;
            const uint32_t d = static_cast<uint32_t>(i + 1) * segments + next;
            mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
        }
    }
    return finalizeSurface(mesh);
}

MeshData generateLoftSurface(const std::vector<LoftSection>& sections,
                             uint32_t radialSegments) {
    MeshData mesh;
    if (sections.size() < 2 || radialSegments < 3) return mesh;
    const uint32_t segments = radialSegments;
    mesh.vertices.reserve(sections.size() * segments);
    for (const LoftSection& section : sections) {
        const float rx = std::fmax(section.radiusX, 1e-4f);
        const float rz = std::fmax(section.radiusZ, 1e-4f);
        for (uint32_t s = 0; s < segments; ++s) {
            const float angle = 2.0f * 3.14159265f * static_cast<float>(s) / static_cast<float>(segments);
            const float c = std::cos(angle), sn = std::sin(angle);
            Vertex vertex;
            vertex.position = section.center + Vec3{c * rx, 0.0f, sn * rz};
            vertex.normal = Vec3{c / rx, 0.0f, sn / rz}.normalized({0.0f, 1.0f, 0.0f});
            vertex.uv = {static_cast<float>(s) / static_cast<float>(segments), 0.0f};
            mesh.vertices.push_back(vertex);
        }
    }
    for (size_t i = 0; i + 1 < sections.size(); ++i) {
        for (uint32_t s = 0; s < segments; ++s) {
            const uint32_t next = (s + 1) % segments;
            const uint32_t a = static_cast<uint32_t>(i) * segments + s;
            const uint32_t b = static_cast<uint32_t>(i) * segments + next;
            const uint32_t c = static_cast<uint32_t>(i + 1) * segments + s;
            const uint32_t d = static_cast<uint32_t>(i + 1) * segments + next;
            mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
        }
    }
    return finalizeSurface(mesh);
}

} // namespace pae
