#include "ParametricSurfaces.h"
#include "procedural/geometry/CatmullRom.h"

#include <algorithm>
#include <cmath>

namespace pae {
namespace {

constexpr float kTwoPi = 6.2831853f;

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
    frames[0].binormal = tangents[0].cross(normal).normalized({0.0f, 1.0f, 0.0f});

    for (size_t i = 1; i < count; ++i) {
        const Quat rotation = Quat::fromToRotation(tangents[i - 1], tangents[i]);
        normal = rotation.rotate(normal);
        normal = (normal - tangents[i] * normal.dot(tangents[i])).normalized({1.0f, 0.0f, 0.0f});
        frames[i].tangent = tangents[i];
        frames[i].normal = normal;
        frames[i].binormal = tangents[i].cross(normal).normalized({0.0f, 1.0f, 0.0f}); // M11: fallback
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

MeshData generateSplineSweep(const std::vector<SweepRing>& rings,
                             uint32_t radialSegments, int splineSamplesPerSegment,
                             bool capStart, bool capEnd, bool closedRing) {
    MeshData mesh;
    if (rings.size() < 2 || radialSegments < 3 || splineSamplesPerSegment < 1) return mesh;

    std::vector<Vec3> points;
    std::vector<float> radii, scaleX, scaleZ, rolls;
    points.reserve(rings.size());
    radii.reserve(rings.size());
    for (const SweepRing& ring : rings) {
        points.push_back(ring.point);
        radii.push_back(ring.radius);
        scaleX.push_back(std::fmax(ring.sectionScale.x, 1e-4f));
        scaleZ.push_back(std::fmax(ring.sectionScale.y, 1e-4f));
        rolls.push_back(ring.roll);
    }
    std::vector<Vec3> sampled = spline::resampleCatmullRom(points, splineSamplesPerSegment);
    std::vector<float> rS = spline::resampleCatmullRomScalar(radii, splineSamplesPerSegment);
    std::vector<float> sxS = spline::resampleCatmullRomScalar(scaleX, splineSamplesPerSegment);
    std::vector<float> szS = spline::resampleCatmullRomScalar(scaleZ, splineSamplesPerSegment);
    std::vector<float> rollS = spline::resampleCatmullRomScalar(rolls, splineSamplesPerSegment);
    if (closedRing && sampled.size() > 2) sampled.pop_back(); // weld first/last ring

    const size_t ringCount = sampled.size();
    if (ringCount < 2) return mesh;
    const std::vector<SurfaceFrame> frames = computeParallelTransportFrames(sampled);
    const uint32_t columns = radialSegments + 1; // duplicated seam column

    mesh.vertices.reserve(ringCount * columns);
    for (size_t i = 0; i < ringCount; ++i) {
        const float radius = std::fmax(rS[i], 1e-4f);
        const float sx = std::fmax(sxS[i], 1e-4f);
        const float sz = std::fmax(szS[i], 1e-4f);
        float taper = 0.0f; // dr/dl along the resampled centerline
        if (ringCount > 2 && i > 0 && i + 1 < ringCount) {
            const float rPrev = std::fmax(rS[i - 1], 1e-4f) *
                std::sqrt(std::fmax(sxS[i - 1] * szS[i - 1], 1e-8f));
            const float rNext = std::fmax(rS[i + 1], 1e-4f) *
                std::sqrt(std::fmax(sxS[i + 1] * szS[i + 1], 1e-8f));
            const float dl = std::fmax((sampled[i + 1] - sampled[i - 1]).length(), 1e-5f);
            taper = (rNext - rPrev) / dl;
        }
        for (uint32_t s = 0; s <= radialSegments; ++s) {
            const float angle = kTwoPi * static_cast<float>(s) / static_cast<float>(radialSegments)
                              + rollS[i];
            const float c = std::cos(angle), sn = std::sin(angle);
            const Vec3 radial = frames[i].normal * (c * sx) + frames[i].binormal * (sn * sz);
            Vertex vertex;
            vertex.position = sampled[i] + radial * radius;
            // Analytic ellipse normal tilted against the local taper (cone rule).
            const Vec3 radialNormal = frames[i].normal * (c / sx) + frames[i].binormal * (sn / sz);
            vertex.normal = (radialNormal - frames[i].tangent * taper).normalized({0.0f, 1.0f, 0.0f});
            vertex.uv = {static_cast<float>(s) / static_cast<float>(radialSegments),
                         static_cast<float>(i) / static_cast<float>(ringCount > 1 ? ringCount - 1 : 1)};
            mesh.vertices.push_back(vertex);
        }
    }

    const size_t quadRings = closedRing ? ringCount : ringCount - 1;
    for (size_t i = 0; i < quadRings; ++i) {
        const size_t j = (i + 1) % ringCount;
        for (uint32_t s = 0; s < radialSegments; ++s) {
            const uint32_t a = static_cast<uint32_t>(i) * columns + s;
            const uint32_t b = static_cast<uint32_t>(i) * columns + s + 1;
            const uint32_t c = static_cast<uint32_t>(j) * columns + s;
            const uint32_t d = static_cast<uint32_t>(j) * columns + s + 1;
            mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
        }
    }

    auto addCap = [&](size_t ringIndex, const Vec3& centerPoint, const Vec3& capNormal, bool flip) {
        const uint32_t centerIndex = static_cast<uint32_t>(mesh.vertices.size());
        Vertex center;
        center.position = centerPoint;
        center.normal = capNormal;
        center.uv = {0.5f, 0.5f};
        mesh.vertices.push_back(center);
        const uint32_t base = static_cast<uint32_t>(ringIndex) * columns;
        for (uint32_t s = 0; s < radialSegments; ++s) {
            const uint32_t v0 = base + s;
            const uint32_t v1 = base + s + 1;
            if (flip) mesh.indices.insert(mesh.indices.end(), {centerIndex, v1, v0});
            else mesh.indices.insert(mesh.indices.end(), {centerIndex, v0, v1});
        }
    };
    if (capStart) addCap(0, sampled.front(), frames.front().tangent * -1.0f, false);
    if (capEnd) addCap(ringCount - 1, sampled.back(), frames.back().tangent, true);

    return finalizeSurface(mesh);
}

MeshData generateSplineLoft(const std::vector<LoftSection>& sections,
                            uint32_t radialSegments, int splineSamplesPerSection) {
    MeshData mesh;
    if (sections.size() < 2 || radialSegments < 3 || splineSamplesPerSection < 1) return mesh;

    std::vector<Vec3> centers;
    std::vector<float> rxs, rzs, rolls;
    for (const LoftSection& section : sections) {
        centers.push_back(section.center);
        rxs.push_back(std::fmax(section.radiusX, 1e-4f));
        rzs.push_back(std::fmax(section.radiusZ, 1e-4f));
        rolls.push_back(section.roll);
    }
    std::vector<Vec3> sampled = spline::resampleCatmullRom(centers, splineSamplesPerSection);
    std::vector<float> rxS = spline::resampleCatmullRomScalar(rxs, splineSamplesPerSection);
    std::vector<float> rzS = spline::resampleCatmullRomScalar(rzs, splineSamplesPerSection);
    std::vector<float> rollS = spline::resampleCatmullRomScalar(rolls, splineSamplesPerSection);
    const size_t ringCount = sampled.size();
    const uint32_t columns = radialSegments + 1;

    // Taper (dr/dy) for normal tilt; travel direction for winding.
    for (size_t i = 0; i < ringCount; ++i) {
        const float rx = std::fmax(rxS[i], 1e-4f);
        const float rz = std::fmax(rzS[i], 1e-4f);
        float taper = 0.0f;
        if (ringCount > 2 && i > 0 && i + 1 < ringCount) {
            const float dl = std::fmax(sampled[i + 1].y - sampled[i - 1].y, 1e-5f);
            taper = (std::fmax(rxS[i + 1], 1e-4f) - std::fmax(rxS[i - 1], 1e-4f)) / dl;
        }
        const float cosRoll = std::cos(rollS[i]), sinRoll = std::sin(rollS[i]);
        for (uint32_t s = 0; s <= radialSegments; ++s) {
            const float angle = kTwoPi * static_cast<float>(s) / static_cast<float>(radialSegments);
            const float c = std::cos(angle), sn = std::sin(angle);
            const float ex = c * rx, ez = sn * rz;
            Vertex vertex;
            vertex.position = sampled[i] + Vec3{ex * cosRoll + ez * sinRoll, 0.0f,
                                                -ex * sinRoll + ez * cosRoll};
            const Vec3 baseNormal{c / rx, 0.0f, sn / rz};
            Vec3 normal{baseNormal.x * cosRoll + baseNormal.z * sinRoll, 0.0f,
                        -baseNormal.x * sinRoll + baseNormal.z * cosRoll};
            vertex.normal = (normal + Vec3{0.0f, -taper * c, 0.0f}).normalized({0.0f, 1.0f, 0.0f});
            vertex.uv = {static_cast<float>(s) / static_cast<float>(radialSegments),
                         static_cast<float>(i) / static_cast<float>(ringCount > 1 ? ringCount - 1 : 1)};
            mesh.vertices.push_back(vertex);
        }
    }

    // Winding normalized against travel direction.
    const float travelY = sampled.back().y - sampled.front().y;
    const bool flip = travelY < 0.0f;
    for (size_t i = 0; i + 1 < ringCount; ++i) {
        for (uint32_t s = 0; s < radialSegments; ++s) {
            const uint32_t a = static_cast<uint32_t>(i) * columns + s;
            const uint32_t b = static_cast<uint32_t>(i) * columns + s + 1;
            const uint32_t c = static_cast<uint32_t>(i + 1) * columns + s;
            const uint32_t d = static_cast<uint32_t>(i + 1) * columns + s + 1;
            if (!flip) mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
            else mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
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
        const float cosRoll = std::cos(section.roll), sinRoll = std::sin(section.roll);
        for (uint32_t s = 0; s < segments; ++s) {
            const float angle = kTwoPi * static_cast<float>(s) / static_cast<float>(segments);
            const float c = std::cos(angle), sn = std::sin(angle);
            const float ex = c * rx, ez = sn * rz;
            Vertex vertex;
            vertex.position = section.center + Vec3{ex * cosRoll + ez * sinRoll, 0.0f,
                                                    -ex * sinRoll + ez * cosRoll};
            const Vec3 baseNormal{c / rx, 0.0f, sn / rz};
            vertex.normal = Vec3{baseNormal.x * cosRoll + baseNormal.z * sinRoll, 0.0f,
                                 -baseNormal.x * sinRoll + baseNormal.z * cosRoll}
                                .normalized({0.0f, 1.0f, 0.0f});
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
