#include "FaceGenerator.h"

#include "procedural/primitives/Primitives.h"
#include "procedural/surfaces/ParametricSurfaces.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pae::face {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.2831853f;

float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

float smooth(float a, float b, float x) {
    const float t = clamp01((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}

float lerpF(float a, float b, float t) { return a + (b - a) * t; }

MeshData translateMesh(const MeshData& source, const Vec3& offset) {
    MeshData result = source;
    for (Vertex& vertex : result.vertices) vertex.position = vertex.position + offset;
    Aabb bounds;
    for (const Vertex& vertex : result.vertices) bounds.expand(vertex.position);
    result.bounds = bounds;
    return result;
}

MeshData finalize(MeshData& mesh) {
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

// Shape factor per direction: jaw taper, chin, cheek, forehead, temple and
// back-head modulations. y = cos(polar) in [-1, 1] (up +); fx = cos(azimuth),
// fz = sin(azimuth) with fz > 0 = face front.
float shapeFactor(const FaceParams& fp, float y, float fx, float fz) {
    float s = 1.0f;
    const float squareness = std::strcmp(fp.headShape, "square") == 0 ? 1.0f : 0.0f;
    const float roundness = std::strcmp(fp.headShape, "round") == 0 ? 1.0f : 0.0f;

    const float jawStart = -0.05f + roundness * 0.15f;
    const float jawT = smooth(jawStart, -0.85f, y);
    const float jawX = fp.jawWidth * (1.0f - squareness * 0.15f);
    s *= 1.0f - jawT * (1.0f - jawX);

    const float chinZone = smooth(-0.55f, -0.95f, y) * smooth(0.0f, 0.55f, fz);
    s *= 1.0f + fp.chinShape * 0.18f * chinZone;

    const float cheekZone = smooth(-0.35f, -0.05f, y) * (1.0f - smooth(0.15f, 0.45f, y));
    s *= 1.0f + fp.cheekVolume * 0.10f * cheekZone * std::fabs(fx);

    const float foreheadZone = smooth(0.25f, 0.62f, y) * smooth(-0.1f, 0.5f, fz);
    s *= 1.0f + 0.05f * foreheadZone;

    const float templeZone = smooth(0.35f, 0.65f, y) * std::fabs(fx);
    s *= 1.0f - 0.05f * templeZone * (1.0f - squareness);

    s *= 1.0f + 0.06f * smooth(-0.1f, -0.6f, fz) * (1.0f - squareness * 0.6f);
    return s;
}

} // namespace

MeshData generateHeadMesh(const FaceParams& fp, Vec3 headCenter,
                          float headWidth, float headHeight,
                          uint32_t radialSegments, uint32_t rings) {
    MeshData mesh;
    const uint32_t S = std::max(radialSegments, 6u);
    const uint32_t R = std::max(rings, 6u);
    const float rx = headWidth * 0.5f;
    const float ry = headHeight * 0.5f;
    const float rz = headWidth * 0.42f;

    // Pass 1: shaped positions on the (R+1) x (S+1) grid (face front = +Z).
    std::vector<Vec3> grid((R + 1) * (S + 1));
    auto gridAt = [&](uint32_t i, uint32_t j) -> Vec3& { return grid[i * (S + 1) + j]; };
    for (uint32_t i = 0; i <= R; ++i) {
        const float theta = kPi * static_cast<float>(i) / static_cast<float>(R);
        const float y = std::cos(theta);
        const float sinT = std::sin(theta);
        for (uint32_t j = 0; j <= S; ++j) {
            const float phi = kTwoPi * static_cast<float>(j) / static_cast<float>(S);
            const float fx = std::cos(phi);
            const float fz = std::sin(phi);
            const float shape = shapeFactor(fp, y, fx, fz);
            gridAt(i, j) = headCenter + Vec3{sinT * fx * rx * shape,
                                             y * ry * shape,
                                             sinT * fz * rz * shape};
        }
    }

    // Pass 2: normals from grid neighbours (tangent cross product), seam-safe.
    mesh.vertices.reserve(grid.size());
    for (uint32_t i = 0; i <= R; ++i) {
        for (uint32_t j = 0; j <= S; ++j) {
            const Vec3& p = gridAt(i, j);
            const Vec3& right = gridAt(i, (j + 1) % (S + 1));
            const Vec3& left = gridAt(i, (j + S) % (S + 1));
            const Vec3& up = gridAt(i > 0 ? i - 1 : 0, j);
            const Vec3& down = gridAt(i < R ? i + 1 : R, j);
            Vec3 normal = (right - left).cross(down - up);
            // Orient outward: away from the head center.
            const Vec3 outward = p - headCenter;
            if (normal.dot(outward) < 0.0f) normal = normal * -1.0f;
            Vertex vertex;
            vertex.position = p;
            vertex.normal = normal.normalized({0.0f, 1.0f, 0.0f});
            vertex.uv = {static_cast<float>(j) / static_cast<float>(S),
                         static_cast<float>(i) / static_cast<float>(R)};
            mesh.vertices.push_back(vertex);
        }
    }
    for (uint32_t i = 0; i < R; ++i) {
        for (uint32_t j = 0; j < S; ++j) {
            const uint32_t a = i * (S + 1) + j;
            const uint32_t b = i * (S + 1) + j + 1;
            const uint32_t c = (i + 1) * (S + 1) + j;
            const uint32_t d = (i + 1) * (S + 1) + j + 1;
            // Winding: outward relative to the head center.
            const Vec3& pa = mesh.vertices[a].position;
            const Vec3& pb = mesh.vertices[b].position;
            const Vec3& pc = mesh.vertices[c].position;
            const Vec3 faceNormal = (pb - pa).cross(pc - pa);
            const Vec3 outward = pa - headCenter;
            if (faceNormal.dot(outward) >= 0.0f) {
                mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
            } else {
                mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
            }
        }
    }
    return finalize(mesh);
}

std::vector<FacePiece> generateFaceFeatures(const FaceParams& fp,
                                            Vec3 headCenter, float headWidth,
                                            float headHeight, uint32_t radial) {
    std::vector<FacePiece> pieces;
    const float rx = headWidth * 0.5f;
    const float ry = headHeight * 0.5f;
    const float rz = headWidth * 0.42f;
    const uint32_t rings = radial;
    auto add = [&pieces](MeshData m, Vec3 center, uint16_t materialId) {
        pieces.push_back({std::move(m), center, materialId});
    };

    // Eyes: sclera + ink iris disc + highlight, driven by spacing/size.
    const float eyeRadius = headWidth * 0.095f * fp.eyeSize;
    const float eyeX = rx * 0.42f * fp.eyeSpacing;
    const float eyeY = headCenter.y + ry * 0.04f;
    for (int side = -1; side <= 1; side += 2) {
        const Vec3 eyeCenter{side * eyeX, eyeY, rz * 0.86f};
        add(translateMesh(generateUvSphere(eyeRadius, radial, rings / 2), eyeCenter), eyeCenter, 3);
        add(translateMesh(generateEllipsoid(
                Vec3{eyeRadius * 0.58f, eyeRadius * 0.58f, eyeRadius * 0.35f}, radial, rings / 2),
            eyeCenter + Vec3{0.0f, 0.0f, eyeRadius * 0.80f}), eyeCenter, 4);
        add(translateMesh(generateUvSphere(eyeRadius * 0.18f, 10, 6),
            eyeCenter + Vec3{-side * eyeRadius * 0.28f, eyeRadius * 0.38f, eyeRadius * 0.97f}),
            eyeCenter, 3);

        // Brows in hair colour at browHeight between the eyes and hairline.
        const float browY = headCenter.y + lerpF(ry * 0.04f, ry * 0.52f, clamp01(fp.browHeight));
        add(translateMesh(generateEllipsoid(
                Vec3{eyeRadius * 0.90f, eyeRadius * 0.22f, eyeRadius * 0.28f}, radial, rings / 2),
            Vec3{side * eyeX, browY, rz * 0.82f}), headCenter, 1);
    }

    // Nose: bridge sweep + tip + wings, driven by width/bridge factors.
    const float noseW = rx * fp.noseWidth * 0.55f;
    const float noseTop = headCenter.y + ry * (0.05f + 0.20f * fp.noseBridge);
    const float noseTipY = headCenter.y - ry * 0.10f;
    add(translateMesh(generateSweepSurface(
            std::vector<Vec3>{{0.0f, noseTop, rz * 0.86f}, {0.0f, noseTipY, rz * 0.97f}},
            std::vector<float>{noseW * 0.55f, noseW * 0.85f}, radial / 2 + 2),
        headCenter), headCenter, 0);
    add(translateMesh(generateEllipsoid(
            Vec3{noseW, noseW * 1.15f, noseW * 0.9f}, radial / 2 + 2, rings / 2),
        Vec3{0.0f, noseTipY - noseW * 0.15f, rz * 0.99f}), headCenter, 0);
    for (int side = -1; side <= 1; side += 2) {
        add(translateMesh(generateEllipsoid(
                Vec3{noseW * 0.35f, noseW * 0.45f, noseW * 0.45f}, radial / 2 + 2, rings / 2),
            Vec3{side * noseW * 0.95f, noseTipY - noseW * 0.05f, rz * 0.93f}), headCenter, 0);
    }

    // Mouth: upper + lower lip pieces in ink, driven by mouth width.
    const float mouthHalfW = rx * fp.mouthWidth * 0.38f;
    const float mouthY = headCenter.y - ry * 0.26f;
    add(translateMesh(generateEllipsoid(
            Vec3{mouthHalfW, mouthHalfW * 0.10f, mouthHalfW * 0.14f}, radial / 2 + 2, rings / 2),
        Vec3{0.0f, mouthY + mouthHalfW * 0.05f, rz * 0.90f}), headCenter, 4);
    add(translateMesh(generateEllipsoid(
            Vec3{mouthHalfW * 0.92f, mouthHalfW * 0.12f, mouthHalfW * 0.12f}, radial / 2 + 2, rings / 2),
        Vec3{0.0f, mouthY - mouthHalfW * 0.10f, rz * 0.90f}), headCenter, 4);

    // Ears: flattened ellipsoids on the sides at eye height.
    for (int side = -1; side <= 1; side += 2) {
        add(translateMesh(generateEllipsoid(
                Vec3{rx * 0.10f, ry * 0.24f, rz * 0.20f}, radial / 2 + 2, rings / 2),
            Vec3{side * rx * 0.98f, headCenter.y + ry * 0.02f, -rz * 0.05f}), headCenter, 0);
    }
    return pieces;
}

} // namespace pae::face
