#include "mini_test.hpp"

#include "procedural/MeshValidator.h"
#include "procedural/humanoid/Humanoid.h"
#include "skeleton/Skeleton.h"

#include <cmath>
#include <cstring>

using namespace pae;

PAE_TEST_CASE(body_parameters_are_clamped) {
    BodyParameters params;
    params.headWidth = 99.0f;
    params.height = -1.0f;
    params.detailLevel = 99;
    const BodyParameters clamped = clampBodyParameters(params);
    EXPECT_TRUE(clamped.headWidth <= 0.40f * clamped.headScale + 1e-5f);
    EXPECT_TRUE(clamped.height >= 0.8f);
    EXPECT_TRUE(clamped.detailLevel <= 3);
}

PAE_TEST_CASE(skeleton_hierarchy_and_ibm_are_valid) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    const Skeleton& skeleton = humanoid.skeleton;
    EXPECT_TRUE(humanoid.valid);
    EXPECT_TRUE(BoneCount == 22);
    EXPECT_TRUE(std::strcmp(boneName(BoneLeftShoulder), "LeftShoulder") == 0);
    EXPECT_TRUE(std::strcmp(boneName(BoneLeftUpperArm), "LeftUpperArm") == 0);
    for (uint16_t i = 0; i < BoneCount; ++i) {
        const BoneDefinition& bone = skeleton.bones[i];
        EXPECT_TRUE(bone.id == i);
        EXPECT_TRUE(bone.parent < static_cast<int16_t>(i));
        EXPECT_TRUE(std::strcmp(bone.name, boneName(i)) == 0);
        const Mat4 product = skeleton.bindGlobal[i] * skeleton.inverseBind[i];
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                EXPECT_NEAR(product.at(c, r), c == r ? 1.0f : 0.0f, 1e-3f);
            }
        }
    }
    for (uint16_t i = 1; i < BoneCount; ++i) {
        const int16_t parent = skeleton.bones[i].parent;
        const Mat4 chain = skeleton.bindGlobal[parent] * skeleton.bindLocal[i];
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                EXPECT_NEAR(chain.at(c, r), skeleton.bindGlobal[i].at(c, r), 1e-4f);
            }
        }
    }
}

PAE_TEST_CASE(humanoid_mesh_and_weights_are_valid) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    const ValidationReport report = validateMesh(humanoid.mesh);
    EXPECT_TRUE(report.valid);
    if (!report.valid) {
        for (const std::string& error : report.errors) std::fprintf(stderr, "%s\n", error.c_str());
    }
    EXPECT_TRUE(humanoid.mesh.vertices.size() > 1000);
    EXPECT_TRUE(humanoid.mesh.subMeshes.size() >= 10);
    EXPECT_TRUE(humanoid.mesh.bounds.size().y > 1.4f);

    for (const Vertex& vertex : humanoid.mesh.vertices) {
        const float sum = vertex.weights.x + vertex.weights.y + vertex.weights.z + vertex.weights.w;
        EXPECT_NEAR(sum, 1.0f, 1e-3f);
        EXPECT_TRUE(vertex.joints.x < BoneCount);
        EXPECT_TRUE(vertex.joints.y < BoneCount);
    }
}

PAE_TEST_CASE(humanoid_generation_is_deterministic) {
    BodyParameters params;
    params.detailLevel = 2;
    const HumanoidResult a = generateHumanoid(params);
    const HumanoidResult b = generateHumanoid(params);
    EXPECT_TRUE(std::memcmp(a.mesh.vertices.data(), b.mesh.vertices.data(),
                            a.mesh.vertices.size() * sizeof(Vertex)) == 0);
    EXPECT_TRUE(std::memcmp(a.mesh.indices.data(), b.mesh.indices.data(),
                            a.mesh.indices.size() * sizeof(uint32_t)) == 0);
    for (uint16_t i = 0; i < BoneCount; ++i) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                EXPECT_NEAR(a.skeleton.bindGlobal[i].at(c, r),
                            b.skeleton.bindGlobal[i].at(c, r), 1e-6f);
            }
        }
    }
}

PAE_TEST_CASE(heatmap_colors_are_generated) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{}, true);
    EXPECT_TRUE(humanoid.valid);
    bool hasVariation = false;
    const Vec4 first = humanoid.mesh.vertices.front().color;
    for (const Vertex& vertex : humanoid.mesh.vertices) {
        if (std::fabs(vertex.color.x - first.x) > 0.05f ||
            std::fabs(vertex.color.y - first.y) > 0.05f) {
            hasVariation = true;
            break;
        }
    }
    EXPECT_TRUE(hasVariation);
}

PAE_TEST_CASE(shoulder_and_upperarm_semantics_are_distinct) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{});
    const Skeleton& skeleton = humanoid.skeleton;

    const uint16_t shoulderBones[2] = {BoneLeftShoulder, BoneRightShoulder};
    const uint16_t upperBones[2] = {BoneLeftUpperArm, BoneRightUpperArm};
    for (int side = 0; side < 2; ++side) {
        const BoneDefinition& shoulder = skeleton.bones[shoulderBones[side]];
        const BoneDefinition& upper = skeleton.bones[upperBones[side]];

        EXPECT_TRUE(shoulder.parent == BoneChest);
        EXPECT_TRUE(upper.parent == shoulderBones[side]);
        EXPECT_TRUE((shoulder.tail - upper.head).length() < 1e-5f);
        EXPECT_TRUE((shoulder.head - shoulder.tail).length() > 0.01f);
        EXPECT_TRUE((shoulder.head - shoulder.tail).length() < 0.30f);
        EXPECT_TRUE((upper.head - upper.tail).length() >
                    (shoulder.head - shoulder.tail).length());
        EXPECT_TRUE((shoulder.head - upper.head).length() > 1e-5f);
    }
}

int main() {
    return test::runAllTests();
}
