#include "Skinning.h"

#include <cstring>

namespace pae {

PoseResult computePose(const Skeleton& skeleton,
                       const std::array<Quat, BoneCount>& localRotations) {
    PoseResult result;
    for (uint16_t i = 0; i < BoneCount; ++i) {
        const Mat4& bindLocal = skeleton.bindLocal[i];
        const Vec3 translation{bindLocal.at(3, 0), bindLocal.at(3, 1), bindLocal.at(3, 2)};
        const Mat4 local = Mat4::trs(translation, localRotations[i], {1.0f, 1.0f, 1.0f});
        const int16_t parent = skeleton.bones[i].parent;
        result.globals[i] = parent >= 0 ? result.globals[parent] * local : local;
        result.palette[i] = result.globals[i] * skeleton.inverseBind[i];
    }
    return result;
}

std::vector<Vec3> cpuSkinPositions(const MeshData& mesh,
                                   const std::array<Mat4, BoneCount>& palette) {
    std::vector<Vec3> result;
    result.reserve(mesh.vertices.size());
    for (const Vertex& vertex : mesh.vertices) {
        const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y, vertex.joints.z, vertex.joints.w};
        const float weights[4] = {vertex.weights.x, vertex.weights.y, vertex.weights.z, vertex.weights.w};
        Vec4 transformed{0.0f, 0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 4; ++i) {
            if (weights[i] <= 0.0f || joints[i] >= BoneCount) continue;
            const Vec4 p = palette[joints[i]].transform(Vec4{vertex.position, 1.0f});
            transformed.x += p.x * weights[i];
            transformed.y += p.y * weights[i];
            transformed.z += p.z * weights[i];
            transformed.w += p.w * weights[i];
        }
        const float inv = transformed.w > 1e-8f ? 1.0f / transformed.w : 1.0f;
        result.push_back({transformed.x * inv, transformed.y * inv, transformed.z * inv});
    }
    return result;
}

std::vector<Vec3> cpuSkinNormals(const MeshData& mesh,
                                 const std::array<Mat4, BoneCount>& palette) {
    std::vector<Vec3> result;
    result.reserve(mesh.vertices.size());
    for (const Vertex& vertex : mesh.vertices) {
        const uint16_t joints[4] = {vertex.joints.x, vertex.joints.y, vertex.joints.z, vertex.joints.w};
        const float weights[4] = {vertex.weights.x, vertex.weights.y, vertex.weights.z, vertex.weights.w};
        Vec3 normal{0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 4; ++i) {
            if (weights[i] <= 0.0f || joints[i] >= BoneCount) continue;
            const Vec3 n = palette[joints[i]].rotateVector(vertex.normal);
            normal = normal + n * weights[i];
        }
        result.push_back(normal.normalized({0.0f, 1.0f, 0.0f}));
    }
    return result;
}

void BonePaletteUboData::writeBone(uint16_t bone, const Mat4& matrix) {
    if (bone >= BoneCount) return;
    std::memcpy(values_.data() + static_cast<size_t>(bone) * 16,
                matrix.m.data(), 16 * sizeof(float));
}

} // namespace pae
