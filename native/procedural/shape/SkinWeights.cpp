#include "procedural/shape/SkinWeights.h"

#include "procedural/shape/TopologyAudit.h"
#include "skeleton/Skinning.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pae::shape {
namespace {

constexpr double kPi = 3.14159265358979323846;

Quat axisAngle(Vec3 axis, double degrees) {
    axis = axis.normalized({0.0f, 1.0f, 0.0f});
    const double half = degrees * kPi / 360.0;
    const float s = static_cast<float>(std::sin(half));
    return Quat{axis.x * s, axis.y * s, axis.z * s, static_cast<float>(std::cos(half))}.normalized();
}

} // namespace

WeightLegality checkWeightLegality(const MeshData& mesh, double sumTolerance) {
    WeightLegality report;
    report.vertexCount = static_cast<int>(mesh.vertices.size());
    for (const Vertex& v : mesh.vertices) {
        const float weights[4] = {v.weights.x, v.weights.y, v.weights.z, v.weights.w};
        const uint16_t ids[4] = {v.joints.x, v.joints.y, v.joints.z, v.joints.w};

        int influences = 0;
        float sum = 0.0f;
        bool negative = false;
        for (int i = 0; i < 4; ++i) {
            if (weights[i] < 0.0f || !std::isfinite(weights[i])) negative = true;
            if (weights[i] > 0.0f) ++influences;
            sum += weights[i];
            if (ids[i] >= BoneCount) report.invalidBoneIdVertices = 1;
        }
        if (negative) ++report.negativeWeightVertices;
        if (influences == 0) ++report.zeroWeightVertices;
        if (influences > 4) ++report.overFourInfluenceVertices;
        report.maxInfluences = std::max(report.maxInfluences, influences);

        const double error = std::fabs(static_cast<double>(sum) - 1.0);
        report.maxSumError = std::max(report.maxSumError, error);
        if (error > sumTolerance) ++report.sumToleranceViolations;
    }

    report.valid = report.vertexCount > 0 &&
                   report.overFourInfluenceVertices == 0 &&
                   report.negativeWeightVertices == 0 &&
                   report.sumToleranceViolations == 0 &&
                   report.invalidBoneIdVertices == 0 &&
                   report.zeroWeightVertices == 0;
    return report;
}

void assignAnatomyWeights(MeshData& mesh,
                          const anatomy::HumanAnatomyParameters& params,
                          const BodyJoints& joints,
                          const Skeleton& skeleton) {
    // Mirrors HumanAnatomyGenerator.cpp:65-118 exactly. Kept here as a
    // host-side PoC implementation so the template meshes can be skinned
    // without changing the production generator. poc_weight_legality_tests
    // compares it vertex-by-vertex against the production weights on R1.
    for (Vertex& vertex : mesh.vertices) {
        const auto fields = anatomy::regionFields(params, joints, vertex.position);
        size_t owner = 0;
        float ownerField = fields[0];
        for (size_t r = 1; r < fields.size(); ++r) {
            if (fields[r] < ownerField) {
                ownerField = fields[r];
                owner = r;
            }
        }
        const int side = vertex.position.x >= 0.0f ? 1 : -1;
        const uint16_t primary = anatomy::regionBone(static_cast<anatomy::BodyRegion>(owner), side);

        float bestDistSq = std::numeric_limits<float>::max();
        uint16_t secondary = primary;
        for (uint16_t bone : {BoneHips, BoneSpine, BoneChest, BoneNeck, BoneHead,
                              BoneLeftShoulder, BoneRightShoulder,
                              BoneLeftUpperArm, BoneRightUpperArm,
                              BoneLeftLowerArm, BoneRightLowerArm}) {
            if (bone == primary) continue;
            const BoneDefinition& def = skeleton.bones[bone];
            const Vec3 ab = def.tail - def.head;
            const float l2 = ab.lengthSq();
            const float h = l2 > 1e-10f
                ? std::clamp((vertex.position - def.head).dot(ab) / l2, 0.0f, 1.0f) : 0.0f;
            const float distSq = (vertex.position - (def.head + ab * h)).lengthSq();
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                secondary = bone;
            }
        }

        const BoneDefinition& second = skeleton.bones[secondary];
        const Vec3 ab2 = second.tail - second.head;
        const float l22 = ab2.lengthSq();
        const float h2 = l22 > 1e-10f
            ? std::clamp((vertex.position - second.head).dot(ab2) / l22, 0.0f, 1.0f) : 0.0f;
        const float distSq2 = (vertex.position - (second.head + ab2 * h2)).lengthSq();

        const float w0 = 1.0f / (ownerField * ownerField + 0.0004f);
        const float w1 = 1.0f / (distSq2 + 0.0004f);
        const float sum = w0 + w1;
        vertex.joints = U16Vec4{primary, secondary, 0, 0};
        vertex.weights = Vec4{w0 / sum, w1 / sum, 0.0f, 0.0f};
    }
}

CpuDeformationReport verifyCpuDeformation(const MeshData& mesh, const BodyJoints& joints) {
    CpuDeformationReport report;
    report.vertexCount = static_cast<int>(mesh.vertices.size());
    if (mesh.vertices.empty()) return report;

    const Skeleton skeleton = buildHumanoidSkeleton(joints);
    std::array<Quat, BoneCount> rotations{};
    for (Quat& q : rotations) q = Quat{};
    // A deterministic, small test pose: 20 degrees of chest twist and a
    // 15-degree left shoulder raise. Both are rotations, so a rigidly bound
    // vertex must land exactly on its bone transform.
    rotations[BoneChest] = axisAngle(Vec3{0.0f, 1.0f, 0.0f}, 20.0);
    rotations[BoneLeftShoulder] = axisAngle(Vec3{0.0f, 0.0f, 1.0f}, 15.0);
    const PoseResult pose = computePose(skeleton, rotations);

    // ---- CPU skinner correctness, independent of the anatomy weights ----
    // The production weight formula always produces TWO non-zero influences,
    // so no real vertex is rigid. Probe each bone with a synthetic weight-1
    // vertex instead: a rigid transform must be reproduced exactly.
    const Vec3 probePoint{0.13f, 1.20f, 0.07f};
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        MeshData probe;
        Vertex v;
        v.position = probePoint;
        v.joints = U16Vec4{bone, 0, 0, 0};
        v.weights = Vec4{1.0f, 0.0f, 0.0f, 0.0f};
        probe.vertices.push_back(v);
        const std::vector<Vec3> moved = cpuSkinPositions(probe, pose.palette);
        const Vec4 expected4 = pose.palette[bone].transform(Vec4{probePoint, 1.0f});
        const Vec3 expected{expected4.x, expected4.y, expected4.z};
        if (moved.size() == 1) {
            report.maxRigidError = std::max(
                report.maxRigidError, static_cast<double>((moved[0] - expected).length()));
        }
        ++report.rigidVertexCount;
    }

    // Bind pose must be an identity deformation for every vertex.
    std::array<Quat, BoneCount> bindRotations{};
    for (Quat& q : bindRotations) q = Quat{};
    const PoseResult bindPose = computePose(skeleton, bindRotations);
    const std::vector<Vec3> bindSkinned = cpuSkinPositions(mesh, bindPose.palette);
    if (bindSkinned.size() == mesh.vertices.size()) {
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            report.bindResidual = std::max(
                report.bindResidual,
                static_cast<double>((bindSkinned[i] - mesh.vertices[i].position).length()));
        }
    }

    const std::vector<Vec3> skinned = cpuSkinPositions(mesh, pose.palette);
    if (skinned.size() != mesh.vertices.size()) return report;

    double totalDisplacement = 0.0;
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const Vec3& p = mesh.vertices[i].position;
        const Vec3& q = skinned[i];
        if (!q.finite()) {
            ++report.nonFinitePositions;
            continue;
        }
        const double displacement = static_cast<double>((q - p).length());
        totalDisplacement += displacement;
        report.maxDisplacement = std::max(report.maxDisplacement, displacement);
        if (mesh.vertices[i].joints.x == BoneRoot &&
            mesh.vertices[i].weights.y == 0.0f) {
            report.rootResidual = std::max(report.rootResidual, displacement);
        }
    }
    report.meanDisplacement = totalDisplacement / static_cast<double>(mesh.vertices.size());

    // Volume check on the posed mesh. Use a copy so the caller's mesh is not
    // modified; the topology is unchanged by skinning.
    MeshData posed = mesh;
    for (size_t i = 0; i < posed.vertices.size(); ++i) posed.vertices[i].position = skinned[i];
    report.volumeBefore = meshVolume(mesh);
    report.volumeAfter = meshVolume(posed);
    if (std::fabs(report.volumeBefore) > 1e-9) {
        report.relativeVolumeChange = std::fabs((report.volumeAfter - report.volumeBefore) /
                                                report.volumeBefore);
    }

    report.valid = report.nonFinitePositions == 0;
    return report;
}

} // namespace pae::shape
