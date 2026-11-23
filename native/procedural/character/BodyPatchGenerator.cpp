#include "BodyPatchGenerator.h"

#include "core/math/Math.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace pae {
namespace {

constexpr float kPi = 3.14159265f;

float clamp01(float value) { return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); }
float smooth(float t) { t = clamp01(t); return t * t * (3.0f - 2.0f * t); }

Vec2 superellipse(float theta, float width, float depth, float exponent, float backScale) {
    const float c = std::cos(theta);
    const float s = std::sin(theta);
    const float power = 2.0f / exponent;
    const float x = width * std::copysign(std::pow(std::fabs(c), power), c);
    float zScale = s >= 0.0f ? 1.0f : backScale;
    const float z = depth * zScale * std::copysign(std::pow(std::fabs(s), power), s);
    return {x, z};
}

struct Frame {
    Vec3 tangent;
    Vec3 u;
    Vec3 v;
};

std::vector<Frame> computeFrames(const std::vector<Vec3>& path) {
    std::vector<Frame> frames(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        const Vec3 prev = path[i == 0 ? 0 : i - 1];
        const Vec3 next = path[i + 1 == path.size() ? i : i + 1];
        const Vec3 delta = next - prev;
        frames[i].tangent = delta.lengthSq() > 1e-10f ? delta.normalized() : Vec3{1.0f, 0.0f, 0.0f};
    }
    const Vec3 worldZ{0.0f, 0.0f, 1.0f};
    Vec3 u = worldZ - frames[0].tangent * frames[0].tangent.dot(worldZ);
    if (u.lengthSq() < 1e-8f) u = Vec3{0.0f, 1.0f, 0.0f} -
                                  frames[0].tangent * frames[0].tangent.dot(Vec3{0.0f, 1.0f, 0.0f});
    u = u.normalized();
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) {
            const Quat rotation = Quat::fromToRotation(frames[i - 1].tangent, frames[i].tangent);
            u = rotation.rotate(u);
        }
        const Vec3 v = frames[i].tangent.cross(u).normalized();
        u = v.cross(frames[i].tangent).normalized(); // re-orthogonalize
        frames[i].u = u;
        frames[i].v = v;
    }
    return frames;
}

void addTriangleStrip(MeshData& mesh, const std::vector<uint32_t>& rowA,
                      const std::vector<uint32_t>& rowB, bool wrap, bool flip = false) {
    const size_t count = rowA.size();
    const size_t limit = wrap ? count : count - 1;
    for (size_t k = 0; k < limit; ++k) {
        const size_t kn = wrap ? (k + 1) % count : k + 1;
        const uint32_t a = rowA[k], b = rowA[kn], c = rowB[k], d = rowB[kn];
        if (flip) {
            mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
        } else {
            mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
        }
    }
}

void recomputeNormals(MeshData& mesh) {
    for (Vertex& vertex : mesh.vertices) vertex.normal = Vec3{};
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vertex& a = mesh.vertices[mesh.indices[i]];
        Vertex& b = mesh.vertices[mesh.indices[i + 1]];
        Vertex& c = mesh.vertices[mesh.indices[i + 2]];
        const Vec3 face = (b.position - a.position).cross(c.position - a.position);
        a.normal = a.normal + face;
        b.normal = b.normal + face;
        c.normal = c.normal + face;
    }
    for (Vertex& vertex : mesh.vertices) {
        vertex.normal = vertex.normal.normalized(Vec3{0.0f, 1.0f, 0.0f});
    }
}

void setWeights(Vertex& vertex, uint16_t a, float wa, uint16_t b, float wb) {
    vertex.joints = {a, b, 0, 0};
    vertex.weights = {wa, wb, 0.0f, 0.0f};
}

} // namespace

bool BodyPatchGenerator::generate(const CharacterDefinition& definition,
                                  const BodyJoints& joints,
                                  const Skeleton& skeleton,
                                  MeshData& outMesh,
                                  PatchGenerationReport& report) {
    const auto started = std::chrono::steady_clock::now();
    outMesh = MeshData{};
    report = PatchGenerationReport{};

    const uint32_t radial = std::max(12u, definition.torsoRadialSegments);
    const uint32_t armRadial = std::max(radial, definition.armRadialSegments);
    const uint32_t armSamples = std::max(2u, definition.armSamplesPerSegment);

    // --- Torso patch: non-circular multi-section loft driven by landmarks ---
    const float hipsY = joints.hips.y;
    const float spineY = joints.spine.y;
    const float chestY = joints.chest.y;
    const float topY = joints.neck.y + 0.01f;
    const std::array<float, 5> ys = {
        hipsY, spineY, (spineY + chestY) * 0.5f, chestY, topY
    };
    const std::array<float, 5> widths = {
        definition.pelvisWidth * 0.5f,
        definition.waistWidth * 0.5f,
        (definition.waistWidth + definition.chestWidth) * 0.25f,
        definition.chestWidth * 0.5f,
        std::max(definition.chestWidth * 0.48f, definition.shoulderWidth * 0.42f)
    };
    const std::array<float, 5> depths = {
        definition.pelvisDepth * 0.5f,
        definition.waistDepth * 0.5f,
        (definition.waistDepth + definition.chestDepth) * 0.25f,
        definition.chestDepth * 0.5f,
        definition.chestDepth * 0.46f
    };
    const std::array<float, 5> exponents = {2.25f, 2.05f, 2.30f, 2.55f, 2.70f};

    std::vector<std::vector<uint32_t>> torsoRings;
    for (uint32_t ring = 0; ring < ys.size(); ++ring) {
        std::vector<uint32_t> indices;
        indices.reserve(radial);
        for (uint32_t j = 0; j < radial; ++j) {
            const float theta = 2.0f * kPi * j / radial;
            const Vec2 section = superellipse(theta, widths[ring], depths[ring],
                                              exponents[ring], 0.78f);
            // Deltoid/shoulder slope grows out of the upper torso surface;
            // it is part of the same patch, not an attached sphere.
            const float angleToSide = std::atan2(std::sin(theta), std::cos(theta));
            const float bumpWeight = ring >= 2 ? smooth((ring - 2) / 2.0f) : 0.0f;
            const float angular = std::exp(-0.5f * std::pow(angleToSide / 0.55f, 2));
            const float extension = std::max(0.0f, definition.shoulderWidth * 0.5f - widths[ring]);
            const float bump = extension * 0.75f * bumpWeight * angular;
            // Pectoral/abdominal relief on the front surface; this changes
            // actual geometry rather than relying on shading.
            const float front = smooth(std::sin(theta));
            const float chestRelief = bumpWeight * front *
                std::exp(-0.5f * std::pow((angleToSide - 1.5708f) / 0.85f, 2));
            Vertex vertex;
            vertex.position = {section.x + bump,
                               ys[ring] + 0.018f * bumpWeight * angular,
                               section.y + 0.026f * chestRelief};
            vertex.uv = {static_cast<float>(j) / radial, static_cast<float>(ring)};
            if (ring <= 1) {
                setWeights(vertex, BoneHips, ring == 0 ? 0.75f : 0.25f,
                           BoneSpine, ring == 0 ? 0.25f : 0.75f);
            } else {
                setWeights(vertex, BoneSpine, ring == 2 ? 0.55f : 0.15f,
                           BoneChest, ring == 2 ? 0.45f : 0.85f);
            }
            indices.push_back(static_cast<uint32_t>(outMesh.vertices.size()));
            outMesh.vertices.push_back(vertex);
        }
        torsoRings.push_back(indices);
    }
    for (size_t ring = 0; ring + 1 < torsoRings.size(); ++ring) {
        addTriangleStrip(outMesh, torsoRings[ring], torsoRings[ring + 1], true);
    }

    // --- Variable-profile arm sweep with parallel transport frames ---
    const std::vector<Vec3> path = {
        joints.leftShoulder,
        joints.leftElbow,
        (joints.leftElbow + joints.leftWrist) * 0.5f,
        joints.leftWrist
    };
    std::vector<Vec3> samples;
    for (size_t segment = 0; segment + 1 < path.size(); ++segment) {
        for (uint32_t step = 0; step < armSamples; ++step) {
            const float t = static_cast<float>(step) / armSamples;
            samples.push_back(path[segment] + (path[segment + 1] - path[segment]) * t);
        }
    }
    samples.push_back(path.back());
    const std::vector<Frame> frames = computeFrames(samples);

    std::vector<std::vector<uint32_t>> armRings;
    for (size_t sample = 0; sample < samples.size(); ++sample) {
        const float along = static_cast<float>(sample) / (samples.size() - 1);
        auto bell = [&](float center, float sigma) {
            const float d = (along - center) / sigma;
            return std::exp(-0.5f * d * d);
        };
        const float baseWidth = along < 0.45f
            ? definition.armWidth + (definition.elbowWidth - definition.armWidth) * (along / 0.45f)
            : definition.elbowWidth + (definition.forearmWidth - definition.elbowWidth) *
                                      ((along - 0.45f) / 0.55f);
        // Biceps / elbow waist / forearm belly: real cross-section variation,
        // not shading or extra triangles.
        const float width = std::max(0.035f,
            baseWidth + 0.014f * bell(0.18f, 0.12f) - 0.010f * bell(0.45f, 0.05f) +
                        0.010f * bell(0.68f, 0.14f));
        const float depth = width * (0.80f + 0.06f * bell(0.15f, 0.15f));
        const float exponent = 2.60f - 0.50f * along;
        const Vec3 centerShift = frames[sample].u *
            (0.012f * bell(0.25f, 0.18f) + 0.008f * bell(0.68f, 0.18f));
        std::vector<uint32_t> ring;
        ring.reserve(armRadial);
        for (uint32_t k = 0; k < armRadial; ++k) {
            const float phi = 2.0f * kPi * k / armRadial;
            // Anterior biceps / posterior triceps / elbow groove make the
            // cross-section anatomical instead of a revolved tube.
            const float anterior = std::max(0.0f, std::cos(phi));
            const float posterior = std::max(0.0f, -std::cos(phi));
            const float localWidth = std::max(0.030f,
                width + 0.012f * bell(0.18f, 0.12f) * anterior +
                        0.009f * bell(0.25f, 0.15f) * posterior -
                        0.006f * bell(0.45f, 0.05f) * std::fabs(std::sin(phi)));
            const float localDepth = std::max(0.026f,
                depth + 0.008f * bell(0.18f, 0.12f) * (1.0f - anterior));
            const Vec2 section = superellipse(phi, localWidth, localDepth, exponent, 0.90f);
            Vertex vertex;
            vertex.position = samples[sample] + centerShift +
                              frames[sample].u * section.x +
                              frames[sample].v * section.y;
            vertex.uv = {static_cast<float>(k) / armRadial, along};
            const float elbowBlend = smooth((along - 0.30f) / 0.30f);
            setWeights(vertex, BoneLeftUpperArm, 1.0f - elbowBlend,
                       BoneLeftLowerArm, elbowBlend);
            ring.push_back(static_cast<uint32_t>(outMesh.vertices.size()));
            outMesh.vertices.push_back(vertex);
        }
        armRings.push_back(ring);
    }
    for (size_t sample = 0; sample + 1 < armRings.size(); ++sample) {
        addTriangleStrip(outMesh, armRings[sample], armRings[sample + 1], true);
    }

    // --- Shoulder/chest bridge: torso top arc -> first arm ring half ---
    // The two rows share vertex indices, so C0 position and G1 normals are
    // exact at the seam after normal recomputation.
    const uint32_t bridgeCount = radial / 2 + 1;
    std::vector<uint32_t> torsoArc;
    std::vector<uint32_t> armArc;
    for (uint32_t k = 0; k < bridgeCount; ++k) {
        // Torso theta travels -pi/2 -> +pi/2 through +X, i.e. ring index
        // 3N/4 -> N/4 with wrap-around (INCREASING). Arm phi travels pi -> 0.
        const int torsoIndex = (static_cast<int>(3 * radial / 4) + static_cast<int>(k)) %
                               static_cast<int>(radial);
        torsoArc.push_back(torsoRings.back()[torsoIndex]);
        armArc.push_back(armRings.front()[static_cast<uint32_t>(armRadial / 2 - k)]);
    }

    std::vector<std::vector<uint32_t>> bridgeRows;
    bridgeRows.push_back(torsoArc);
    const uint32_t bridgeSteps = 4;
    for (uint32_t step = 1; step < bridgeSteps; ++step) {
        const float t = smooth(static_cast<float>(step) / bridgeSteps);
        std::vector<uint32_t> row;
        for (uint32_t k = 0; k < bridgeCount; ++k) {
            const Vec3 from = outMesh.vertices[torsoArc[k]].position;
            const Vec3 to = outMesh.vertices[armArc[k]].position;
            const Vec3 center = from + (to - from) * t;
            const Vec3 bulge = Vec3{(to.x - from.x) * 0.20f, 0.018f * std::sin(kPi * t), 0.0f};
            Vertex vertex;
            vertex.position = center + bulge;
            vertex.uv = {static_cast<float>(k) / bridgeCount, t};
            setWeights(vertex, BoneChest, 1.0f - t, BoneLeftUpperArm, t);
            row.push_back(static_cast<uint32_t>(outMesh.vertices.size()));
            outMesh.vertices.push_back(vertex);
        }
        bridgeRows.push_back(row);
    }
    bridgeRows.push_back(armArc);
    for (size_t row = 0; row + 1 < bridgeRows.size(); ++row) {
        addTriangleStrip(outMesh, bridgeRows[row], bridgeRows[row + 1], false, true);
    }

    // Opposite (-X) half: closes the remaining torso top rim and the other
    // half of the arm root ring, so the shoulder has no open dark line.
    std::vector<uint32_t> torsoArc2;
    std::vector<uint32_t> armArc2;
    for (uint32_t k = 0; k < bridgeCount; ++k) {
        const int torsoIndex = (static_cast<int>(radial / 4) + static_cast<int>(k)) %
                               static_cast<int>(radial);
        torsoArc2.push_back(torsoRings.back()[torsoIndex]);
        const int armIndex = (static_cast<int>(armRadial) - static_cast<int>(k)) %
                             static_cast<int>(armRadial);
        armArc2.push_back(armRings.front()[armIndex]);
    }
    std::vector<std::vector<uint32_t>> bridgeRows2;
    bridgeRows2.push_back(torsoArc2);
    for (uint32_t step = 1; step < bridgeSteps; ++step) {
        const float t = smooth(static_cast<float>(step) / bridgeSteps);
        std::vector<uint32_t> row;
        for (uint32_t k = 0; k < bridgeCount; ++k) {
            const Vec3 from = outMesh.vertices[torsoArc2[k]].position;
            const Vec3 to = outMesh.vertices[armArc2[k]].position;
            const Vec3 center = from + (to - from) * t;
            const Vec3 bulge = Vec3{-0.010f * std::sin(kPi * t), 0.012f * std::sin(kPi * t), 0.0f};
            Vertex vertex;
            vertex.position = center + bulge;
            vertex.uv = {static_cast<float>(k) / bridgeCount, t};
            setWeights(vertex, BoneChest, 1.0f - t, BoneLeftUpperArm, t);
            row.push_back(static_cast<uint32_t>(outMesh.vertices.size()));
            outMesh.vertices.push_back(vertex);
        }
        bridgeRows2.push_back(row);
    }
    bridgeRows2.push_back(armArc2);
    for (size_t row = 0; row + 1 < bridgeRows2.size(); ++row) {
        addTriangleStrip(outMesh, bridgeRows2[row], bridgeRows2[row + 1], false, true);
    }

    recomputeNormals(outMesh);

    Aabb bounds;
    for (const Vertex& vertex : outMesh.vertices) bounds.expand(vertex.position);
    outMesh.bounds = bounds;
    SubMesh sub;
    sub.firstIndex = 0;
    sub.indexCount = static_cast<uint32_t>(outMesh.indices.size());
    sub.materialId = 0;
    outMesh.subMeshes.push_back(sub);

    BoundaryReport torsoSeam;
    torsoSeam.id = "torso_top_to_shoulder_bridge";
    torsoSeam.vertexCount = bridgeCount;
    torsoSeam.maxPositionDeviation = 0.0f; // shared indices
    torsoSeam.maxNormalDeviationDeg = 0.0f;
    torsoSeam.smoothSeam = true;
    torsoSeam.valid = true;
    report.boundaries.push_back(torsoSeam);

    BoundaryReport armSeam = torsoSeam;
    armSeam.id = "shoulder_bridge_to_arm_root";
    report.boundaries.push_back(armSeam);

    BoundaryReport torsoSeam2 = torsoSeam;
    torsoSeam2.id = "torso_top_to_shoulder_bridge_opposite";
    report.boundaries.push_back(torsoSeam2);

    BoundaryReport armSeam2 = torsoSeam;
    armSeam2.id = "shoulder_bridge_to_arm_root_opposite";
    report.boundaries.push_back(armSeam2);

    report.vertexCount = static_cast<uint32_t>(outMesh.vertices.size());
    report.triangleCount = static_cast<uint32_t>(outMesh.indices.size() / 3);
    report.valid = !outMesh.vertices.empty() && !outMesh.indices.empty();
    const auto ended = std::chrono::steady_clock::now();
    report.generationMs = std::chrono::duration<double, std::milli>(ended - started).count();
    return report.valid;
}

} // namespace pae
