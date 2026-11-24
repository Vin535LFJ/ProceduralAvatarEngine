#include "HairGenerator.h"

#include "procedural/primitives/Primitives.h"
#include "procedural/surfaces/ParametricSurfaces.h"

#include <algorithm>
#include <cmath>

namespace pae::hair {

namespace {

constexpr float kPi = 3.14159265f;

MeshData translateMesh(const MeshData& source, const Vec3& offset) {
    MeshData result = source;
    for (Vertex& vertex : result.vertices) vertex.position = vertex.position + offset;
    Aabb bounds;
    for (const Vertex& vertex : result.vertices) bounds.expand(vertex.position);
    result.bounds = bounds;
    return result;
}

// One hair strand: spline sweep from the scalp point, bending outward then
// down, elliptical section tapering to a tip.
MeshData strand(const Vec3& scalpPoint, const Vec3& outward, float length,
                float width, float bend, uint32_t radial) {
    const Vec3 p0 = scalpPoint;
    const Vec3 p1 = scalpPoint + outward * (0.02f + 0.05f * bend);
    const Vec3 p2 = p1 + Vec3{outward.x * 0.25f * length * bend,
                              -length * 0.55f,
                              outward.z * 0.25f * length * bend};
    const Vec3 p3 = p2 + Vec3{outward.x * 0.1f * length, -length * 0.45f,
                              outward.z * 0.1f * length};
    const std::vector<SweepRing> rings = {
        {p0, width, {1.0f, 0.55f}, 0.0f},
        {p1, width * 0.85f, {1.0f, 0.55f}, 0.0f},
        {p2, width * 0.65f, {1.0f, 0.55f}, 0.0f},
        {p3, width * 0.30f, {1.0f, 0.55f}, 0.0f},
    };
    return generateSplineSweep(rings, std::max(6u, radial / 2), 3, false, false, false);
}

} // namespace

std::vector<HairPiece> generateHair(const HairParams& params,
                                    Vec3 headCenter, float headWidth,
                                    float headHeight, float headDepth,
                                    uint32_t radial) {
    std::vector<HairPiece> pieces;
    const float rx = headWidth * 0.5f;
    const float ry = headHeight * 0.5f;
    const float rz = headDepth * 0.5f;

    // Layer 1: scalp cap (slightly larger than the skull shell, shifted back).
    {
        HairPiece cap;
        cap.center = headCenter;
        cap.mesh = translateMesh(generateEllipsoid(
            Vec3{rx * 1.04f, ry * 0.92f, rz * 1.04f}, radial, radial),
            headCenter + Vec3{0.0f, ry * 0.28f, -rz * 0.10f});
        pieces.push_back(std::move(cap));
    }

    const int strandsPerLayer = std::max(6, params.strandCount);
    const float lengthByStyle[3] = {0.05f, params.backLength * 0.7f, params.backLength};
    const float bangLength = lengthByStyle[params.style] * 0.30f;

    // Layer 2: bangs across the forehead.
    for (int i = 0; i < strandsPerLayer / 2; ++i) {
        const float t = static_cast<float>(i) / std::max(1.0f, static_cast<float>(strandsPerLayer / 2 - 1));
        const float angle = (-0.5f + t) * 1.9f; // across the front arc
        const Vec3 outward{std::sin(angle), -0.15f - 0.35f * params.bend,
                           std::cos(angle)};
        const Vec3 scalp = headCenter + Vec3{std::sin(angle) * rx * 0.78f,
                                             ry * 0.80f,
                                             std::cos(angle) * rz * 0.76f};
        HairPiece piece;
        piece.center = scalp;
        piece.mesh = translateMesh(strand(scalp, outward, bangLength,
                                          params.strandWidth, params.bend, radial),
                                   Vec3{0.0f, 0.0f, 0.0f});
        pieces.push_back(std::move(piece));
    }

    // Layer 3: side strands from the temples (medium/long).
    if (params.style >= 1) {
        const float sideLength = lengthByStyle[params.style] * 0.45f;
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < 2; ++i) {
                const Vec3 outward{side * 1.0f, -0.5f - 0.3f * params.bend, 0.1f};
                const Vec3 scalp = headCenter + Vec3{side * rx * 0.92f,
                                                     ry * (0.35f - 0.2f * i),
                                                     rz * 0.10f};
                HairPiece piece;
                piece.center = scalp;
                piece.mesh = translateMesh(strand(scalp, outward, sideLength,
                                                  params.strandWidth * 1.1f,
                                                  params.bend, radial),
                                           Vec3{0.0f, 0.0f, 0.0f});
                pieces.push_back(std::move(piece));
            }
        }
    }

    // Layer 4: back strands (hang down the occiput; long style drops further).
    const int backStrands = std::max(4, strandsPerLayer / 2);
    for (int i = 0; i < backStrands; ++i) {
        const float t = static_cast<float>(i) / std::max(1.0f, static_cast<float>(backStrands - 1));
        const float angle = kPi + (-0.6f + t * 1.2f); // back arc
        const float drop = lengthByStyle[params.style] * (params.style >= 1 ? 1.6f : 0.8f);
        const Vec3 outward{std::sin(angle) * 0.35f, -0.4f - 0.4f * params.bend,
                           std::cos(angle) * 0.8f};
        const Vec3 scalp = headCenter + Vec3{std::sin(angle) * rx * 0.85f,
                                             ry * 0.45f,
                                             std::cos(angle) * rz * 0.85f};
        HairPiece piece;
        piece.center = scalp;
        piece.mesh = translateMesh(strand(scalp, outward, drop,
                                          params.strandWidth * 1.2f,
                                          params.bend, radial),
                                   Vec3{0.0f, 0.0f, 0.0f});
        pieces.push_back(std::move(piece));
    }
    return pieces;
}

} // namespace pae::hair
