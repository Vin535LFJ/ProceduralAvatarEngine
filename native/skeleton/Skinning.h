#pragma once

#include "procedural/MeshData.h"
#include "skeleton/Skeleton.h"

#include <array>
#include <vector>

namespace pae {

struct PoseResult {
    std::array<Mat4, BoneCount> globals{};
    std::array<Mat4, BoneCount> palette{};
};

PoseResult computePose(const Skeleton& skeleton,
                       const std::array<Quat, BoneCount>& localRotations);

// CPU reference used to validate the GLSL linear blend skinning formula.
std::vector<Vec3> cpuSkinPositions(const MeshData& mesh,
                                   const std::array<Mat4, BoneCount>& palette);
std::vector<Vec3> cpuSkinNormals(const MeshData& mesh,
                                 const std::array<Mat4, BoneCount>& palette);

// std140 layout helper: mat4[22], 64 bytes each, column-major.
class BonePaletteUboData {
public:
    void writeBone(uint16_t bone, const Mat4& matrix);
    const float* data() const { return values_.data(); }
    static constexpr size_t kBytes = 16 * BoneCount * sizeof(float);

private:
    std::array<float, 16 * BoneCount> values_{};
};

} // namespace pae
