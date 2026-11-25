#include "RegionWeightGenerator.h"

#include "skeleton/weights/WeightGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace pae::character {
namespace {

float segmentDistanceSq(const Vec3& point, const BoneDefinition& bone) {
    const Vec3 segment = bone.tail - bone.head;
    const float lengthSq = segment.lengthSq();
    const float t = lengthSq > 1e-10f
        ? std::clamp((point - bone.head).dot(segment) / lengthSq, 0.0f, 1.0f)
        : 0.0f;
    return (point - (bone.head + segment * t)).lengthSq();
}

bool isLeftLimbBone(uint16_t bone) {
    return bone == BoneLeftShoulder || bone == BoneLeftUpperArm ||
           bone == BoneLeftLowerArm || bone == BoneLeftHand;
}

bool isRightLimbBone(uint16_t bone) {
    return bone == BoneRightShoulder || bone == BoneRightUpperArm ||
           bone == BoneRightLowerArm || bone == BoneRightHand;
}

} // namespace

RegionWeightReport assignRegionWeights(MeshData& mesh,
                                       const std::vector<RegionId>& vertexRegions,
                                       const Skeleton& skeleton,
                                       bool heatmap) {
    RegionWeightReport report;
    report.vertices = mesh.vertices.size();
    if (vertexRegions.size() != mesh.vertices.size()) {
        report.invalidVertices = mesh.vertices.size();
        return report;
    }
    for (size_t vertexIndex = 0; vertexIndex < mesh.vertices.size(); ++vertexIndex) {
        Vertex& vertex = mesh.vertices[vertexIndex];
        const RegionId region = vertexRegions[vertexIndex];
        uint32_t mask = regionBoneMask(region);
        if (region == RegionId::Thorax) {
            if (vertex.position.x > 0.02f) mask &= ~(1u << BoneRightShoulder);
            if (vertex.position.x < -0.02f) mask &= ~(1u << BoneLeftShoulder);
        }

        std::array<float, BoneCount> scores{};
        for (uint16_t bone = 0; bone < BoneCount; ++bone) {
            if ((mask & (1u << bone)) == 0) continue;
            if (isLeftLimbBone(bone) && vertex.position.x < -0.015f) continue;
            if (isRightLimbBone(bone) && vertex.position.x > 0.015f) continue;
            const float distanceSq = segmentDistanceSq(vertex.position, skeleton.bones[bone]);
            scores[bone] = 1.0f / (distanceSq + 4e-4f);
        }

        std::array<uint16_t, 4> bones{};
        std::array<float, 4> values{};
        for (uint16_t bone = 0; bone < BoneCount; ++bone) {
            const float value = scores[bone];
            if (value <= 0.0f) continue;
            for (size_t slot = 0; slot < values.size(); ++slot) {
                if (value > values[slot]) {
                    for (size_t shift = values.size() - 1; shift > slot; --shift) {
                        values[shift] = values[shift - 1];
                        bones[shift] = bones[shift - 1];
                    }
                    values[slot] = value;
                    bones[slot] = bone;
                    break;
                }
            }
        }
        const float sum = values[0] + values[1] + values[2] + values[3];
        if (!(sum > 1e-8f)) {
            vertex.joints = {BoneChest, 0, 0, 0};
            vertex.weights = {1.0f, 0.0f, 0.0f, 0.0f};
            ++report.invalidVertices;
        } else {
            vertex.joints = {bones[0], bones[1], bones[2], bones[3]};
            vertex.weights = {values[0] / sum, values[1] / sum,
                              values[2] / sum, values[3] / sum};
            report.maxNormalizationError = std::fmax(
                report.maxNormalizationError,
                std::fabs(vertex.weights.x + vertex.weights.y +
                          vertex.weights.z + vertex.weights.w - 1.0f));
        }

        const uint16_t assigned[4] = {vertex.joints.x, vertex.joints.y,
                                      vertex.joints.z, vertex.joints.w};
        for (uint16_t bone : assigned) {
            if ((isLeftLimbBone(bone) && vertex.position.x < -0.015f) ||
                (isRightLimbBone(bone) && vertex.position.x > 0.015f)) {
                ++report.crossSideViolations;
            }
        }

        if (heatmap) {
            Vec3 color{};
            const float assignedWeights[4] = {vertex.weights.x, vertex.weights.y,
                                              vertex.weights.z, vertex.weights.w};
            for (size_t slot = 0; slot < 4; ++slot) {
                if (assignedWeights[slot] <= 0.0f) continue;
                color = color + weights::boneColor(assigned[slot]) * assignedWeights[slot];
            }
            vertex.color = Vec4{color.x, color.y, color.z, vertex.color.w};
        } else {
            vertex.color = Vec4{0.72f, 0.72f, 0.72f, vertex.color.w};
        }
    }
    report.valid = report.invalidVertices == 0 && report.crossSideViolations == 0;
    return report;
}

} // namespace pae::character