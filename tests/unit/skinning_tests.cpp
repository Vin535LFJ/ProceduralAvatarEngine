#include "mini_test.hpp"

#include "procedural/humanoid/Humanoid.h"
#include "skeleton/Skinning.h"

#include <cmath>
#include <cstring>

using namespace pae;

PAE_TEST_CASE(identity_pose_preserves_mesh) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    std::array<Quat, BoneCount> identity{};
    const PoseResult pose = computePose(humanoid.skeleton, identity);
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                EXPECT_NEAR(pose.palette[bone].at(c, r), c == r ? 1.0f : 0.0f, 1e-3f);
            }
        }
    }
    const std::vector<Vec3> skinned = cpuSkinPositions(humanoid.mesh, pose.palette);
    EXPECT_TRUE(skinned.size() == humanoid.mesh.vertices.size());
    for (size_t i = 0; i < skinned.size(); ++i) {
        const Vec3& original = humanoid.mesh.vertices[i].position;
        EXPECT_NEAR(skinned[i].x, original.x, 1e-3f);
        EXPECT_NEAR(skinned[i].y, original.y, 1e-3f);
        EXPECT_NEAR(skinned[i].z, original.z, 1e-3f);
    }
}

PAE_TEST_CASE(single_bone_rotation_moves_expected_vertex) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    MeshData mesh;
    Vertex vertex;
    vertex.position = {0.3f, 1.2f, 0.05f};
    vertex.joints = {BoneLeftUpperArm, 0, 0, 0};
    vertex.weights = {1.0f, 0.0f, 0.0f, 0.0f};
    mesh.vertices.push_back(vertex);
    mesh.indices = {0, 0, 0};

    const float angle = 0.5f;
    std::array<Quat, BoneCount> rotations{};
    rotations[BoneLeftUpperArm] = Quat{0.0f, 0.0f, std::sin(angle * 0.5f), std::cos(angle * 0.5f)};
    const PoseResult pose = computePose(humanoid.skeleton, rotations);
    const std::vector<Vec3> skinned = cpuSkinPositions(mesh, pose.palette);

    const Vec3 pivot = humanoid.skeleton.bones[BoneLeftUpperArm].head;
    const Vec3 relative = vertex.position - pivot;
    const Vec3 rotated = rotations[BoneLeftUpperArm].rotate(relative);
    const Vec3 expected = pivot + rotated;
    EXPECT_NEAR(skinned[0].x, expected.x, 1e-3f);
    EXPECT_NEAR(skinned[0].y, expected.y, 1e-3f);
    EXPECT_NEAR(skinned[0].z, expected.z, 1e-3f);
}

PAE_TEST_CASE(ubo_std140_layout_matches_mat4) {
    BonePaletteUboData ubo;
    Mat4 matrix;
    matrix.set(0, 0, 1.5f);
    matrix.set(3, 1, -2.25f);
    matrix.set(2, 3, 7.0f);
    ubo.writeBone(1, matrix);
    EXPECT_TRUE(std::memcmp(ubo.data() + 16, matrix.m.data(), 16 * sizeof(float)) == 0);
    EXPECT_NEAR(*(ubo.data() + 16), 1.5f, 1e-6f);
    EXPECT_NEAR(*(ubo.data() + 16 + 13), -2.25f, 1e-6f);
    EXPECT_TRUE(BonePaletteUboData::kBytes == 1408);
}

// M11-D: region weights must blend across part boundaries — vertices at the
// top of the thigh carry both Hips and LeftUpperLeg influences.
PAE_TEST_CASE(region_weights_blend_across_hip_boundary) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    const float hipsY = humanoid.joints.hips.y;
    int blended = 0, checked = 0;
    for (const Vertex& v : humanoid.mesh.vertices) {
        if (v.position.x > 0.10f && v.position.x < 0.26f &&
            v.position.y > hipsY - 0.08f && v.position.y < hipsY + 0.02f) {
            ++checked;
            const bool hasHips = (v.joints.x == BoneHips && v.weights.x > 0.05f) ||
                                 (v.joints.y == BoneHips && v.weights.y > 0.05f);
            const bool hasThigh = (v.joints.x == BoneLeftUpperLeg && v.weights.x > 0.05f) ||
                                  (v.joints.y == BoneLeftUpperLeg && v.weights.y > 0.05f);
            if (hasHips && hasThigh) ++blended;
        }
    }
    EXPECT_TRUE(checked > 0);
    EXPECT_TRUE(blended > 0);
}

// M11-D: skull vertices stay dominated by the head bone (the neck->head
// junction itself legitimately keeps neck influence).
PAE_TEST_CASE(region_weights_keep_head_dominant) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    for (const Vertex& v : humanoid.mesh.vertices) {
        if (v.position.y > humanoid.joints.head.y + 0.10f) {
            const float headWeight = (v.joints.x == BoneHead ? v.weights.x : 0.0f) +
                                     (v.joints.y == BoneHead ? v.weights.y : 0.0f);
            EXPECT_TRUE(headWeight > 0.85f);
        }
    }
}

int main() {
    return test::runAllTests();
}
