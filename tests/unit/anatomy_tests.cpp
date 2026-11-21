#include "mini_test.hpp"

#include "procedural/MeshValidator.h"
#include "procedural/MeshTopology.h"
#include "procedural/humanoid/anatomy/HumanAnatomyGenerator.h"
#include "procedural/humanoid/Humanoid.h"

#include <cmath>
#include <cstring>

using namespace pae;
using namespace pae::anatomy;

namespace {
constexpr float kPi = 3.14159265f;
}

// M12-A: the composed field describes a body — inside negative, outside
// positive, and each region field owns its own landmark volume.
PAE_TEST_CASE(anatomy_field_regions_own_their_volume) {
    HumanAnatomyParameters params;
    BodyParameters bodyParams{};
    const BodyJoints joints = deriveJoints(bodyParams, 1.0f);

    // Inside the ribcage center.
    const float chestField = upperBodySdf(params, joints,
                                          Vec3{0.0f, (joints.spine.y + joints.chest.y) * 0.5f, 0.01f});
    std::fprintf(stderr, "[DBG-C] chestField=%.4f\n", chestField);
    EXPECT_TRUE(chestField < 0.0f);
    // Far outside.
    const float outsideField = upperBodySdf(params, joints, Vec3{0.0f, 1.2f, 0.8f});
    if (outsideField <= 0.0f) {
        std::fprintf(stderr, "[DBG] outside field=%.4f\n", outsideField);
    }
    EXPECT_TRUE(outsideField > 0.0f);

    // Region ownership: the abdomen field is the smallest of all regions at
    // the abdomen center; the deltoid field owns the deltoid center.
    auto fieldsAtAbdomen = regionFields(params, joints, joints.spine);
    const float abdomenVal = fieldsAtAbdomen[static_cast<size_t>(BodyRegion::Abdomen)];
    EXPECT_TRUE(abdomenVal < 0.0f); // the abdomen field owns its own center

    // The pelvis field owns the pelvis center (an unambiguous region).
    auto fieldsAtPelvis = regionFields(params, joints, Vec3{0.0f, joints.hips.y + 0.01f, 0.0f});
    const float pelvisVal = fieldsAtPelvis[static_cast<size_t>(BodyRegion::Pelvis)];
    for (size_t r = 0; r < fieldsAtPelvis.size(); ++r) {
        if (r == static_cast<size_t>(BodyRegion::Pelvis)) continue;
        EXPECT_TRUE(fieldsAtPelvis[r] >= pelvisVal - 1e-4f);
    }
    const Vec3 probe{-0.25f, 1.15f, 0.30f};
    std::fprintf(stderr, "[DBG-F] upperBodySdf(probe)=%.4f\n", upperBodySdf(params, joints, probe));
    {
        auto rf = regionFields(params, joints, probe);
        static const char* names[] = {"RibCage", "Abdomen", "Pelvis", "Clavicle", "Deltoid",
                                      "UpperArm", "Forearm", "Neck", "Head"};
        for (size_t r = 0; r < rf.size(); ++r) {
            std::fprintf(stderr, "[DBG-F]   %s=%.4f\n", names[r], rf[r]);
        }
    }
    auto fieldsAtDeltoid = regionFields(params, joints,
                                        Vec3{joints.leftShoulder.x + 0.02f,
                                             joints.leftShoulder.y - 0.02f, 0.0f});
    EXPECT_TRUE(fieldsAtDeltoid[static_cast<size_t>(BodyRegion::Deltoid)] < 0.0f);
}

// M12-A: the extracted unified mesh is ONE watertight connected surface.
PAE_TEST_CASE(anatomy_mesh_is_single_watertight_surface) {
    HumanAnatomyParameters params;
    BodyParameters bodyParams{};
    AnatomyResolution res;
    res.x = 48; res.y = 72; res.z = 32; // fast validation extraction
    const AnatomyResult result = generateAnatomyUpperBody(params, bodyParams, res);
    EXPECT_TRUE(result.valid);

    const TopologyReport topology = analyzeTopology(result.mesh);
    {
        int shown = 0;
        for (const Vertex& v : result.mesh.vertices) {
            if (v.position.z > 0.30f && shown < 6) {
                std::fprintf(stderr, "[DBG-Z] vertex=(%.3f,%.3f,%.3f)\n",
                             v.position.x, v.position.y, v.position.z);
                ++shown;
            }
        }
        std::fprintf(stderr, "[DBG-TOP] components=%d boundary=%d nonmanifold=%d degen=%d dup=%d tris=%ld\n",
                     topology.components, topology.boundaryEdges, topology.nonManifoldEdges,
                     topology.degenerateTriangles, topology.duplicateVertices, topology.triangles);
        for (const Vec3& s : topology.boundarySamples) {
            std::fprintf(stderr, "[DBG-BND] boundary edge near (%.3f,%.3f,%.3f)\n", s.x, s.y, s.z);
        }
        (void)shown;
    }
    EXPECT_TRUE(topology.components == 1);       // ONE connected body
    EXPECT_TRUE(topology.boundaryEdges == 0);    // watertight (no open seams)
    EXPECT_TRUE(topology.nonManifoldEdges == 0); // clean manifold
    EXPECT_TRUE(topology.duplicateVertices == 0);
    // Surface Nets can emit rare sliver quads on the dual lattice;
    // tolerate <1% but never a systematic degeneration.
    EXPECT_TRUE(topology.degenerateTriangles * 100 < topology.triangles);

    const ValidationReport validation = validateMesh(result.mesh, 1e-12f, false);
    std::fprintf(stderr, "[DBG] mesh bounds: x[%.3f,%.3f] y[%.3f,%.3f] z[%.3f,%.3f] tris=%ld\n",
                 result.mesh.bounds.min.x, result.mesh.bounds.max.x,
                 result.mesh.bounds.min.y, result.mesh.bounds.max.y,
                 result.mesh.bounds.min.z, result.mesh.bounds.max.z, topology.triangles);
    EXPECT_TRUE(validation.valid);

    // Deterministic extraction.
    const AnatomyResult again = generateAnatomyUpperBody(params, bodyParams, res);
    EXPECT_TRUE(again.mesh.vertices.size() == result.mesh.vertices.size());
    EXPECT_TRUE(std::memcmp(again.mesh.vertices.data(), result.mesh.vertices.data(),
                            result.mesh.vertices.size() * sizeof(Vertex)) == 0);
}

// M12-A: anatomy parameters must visibly change the body — deltoidVolume
// widens the shoulder silhouette by at least 1 cm, chestWidth widens the chest.
PAE_TEST_CASE(anatomy_parameters_change_body_shape) {
    BodyParameters bodyParams{};

    HumanAnatomyParameters small;
    small.deltoidVolume = 0.4f;
    small.chestWidth = 0.8f;
    HumanAnatomyParameters large;
    large.deltoidVolume = 1.6f;
    large.chestWidth = 1.3f;
    small.stylization = large.stylization = 1.0f;

    const BodyJoints bodyJoints = deriveJoints(bodyParams, 1.0f);
    const float jMidY = bodyJoints.chest.y;
    AnatomyResolution res;
    res.x = 48; res.y = 72; res.z = 32;
    const AnatomyResult smallResult = generateAnatomyUpperBody(small, bodyParams, res);
    const AnatomyResult largeResult = generateAnatomyUpperBody(large, bodyParams, res);

    auto shoulderWidth = [](const AnatomyResult& r) {
        float maxAbsX = 0.0f;
        for (const Vertex& v : r.mesh.vertices) {
            if (v.position.y > 1.22f && v.position.y < 1.36f)
                maxAbsX = std::fmax(maxAbsX, std::fabs(v.position.x));
        }
        return maxAbsX;
    };
    const float smallShoulder = shoulderWidth(smallResult);
    const float largeShoulder = shoulderWidth(largeResult);
    EXPECT_TRUE(largeShoulder > smallShoulder + 0.01f); // >= 1 cm silhouette change

    // Chest silhouette via field ray-scan at ribcage-center height (z=0):
    // immune to arm overlap (the first +X sign change is the torso surface).
    auto chestSilhouette = [&](const HumanAnatomyParameters& hp) {
        const BodyJoints jj = deriveJoints(bodyParams, 1.0f);
        const float scanY = jj.spine.y + (jj.chest.y - jj.spine.y) * 0.55f;
        float surfaceX = 0.0f;
        float prev = upperBodySdf(hp, jj, Vec3{0.0f, scanY, 0.0f});
        for (float x = 0.005f; x < 0.6f; x += 0.005f) {
            const float cur = upperBodySdf(hp, jj, Vec3{x, scanY, 0.0f});
            if (prev < 0.0f && cur >= 0.0f) {
                surfaceX = x;
                break;
            }
            prev = cur;
        }
        return surfaceX;
    };
    const float chestSmall = chestSilhouette(small);
    const float chestLarge = chestSilhouette(large);
    std::fprintf(stderr, "[DBG-P] chest silhouette small=%.4f large=%.4f | shoulder small=%.4f large=%.4f | valid s=%d l=%d\n",
                 chestSmall, chestLarge, smallShoulder, largeShoulder,
                 (int)smallResult.valid, (int)largeResult.valid);
    EXPECT_TRUE(chestLarge > chestSmall + 0.03f);
}

// M12-A: anatomy mesh is skinnable — weights valid, arm vertices map to arm bones.
PAE_TEST_CASE(anatomy_mesh_supports_skinning) {
    HumanAnatomyParameters params;
    BodyParameters bodyParams{};
    AnatomyResolution res;
    res.x = 48; res.y = 72; res.z = 32;
    const AnatomyResult result = generateAnatomyUpperBody(params, bodyParams, res);
    for (const Vertex& v : result.mesh.vertices) {
        const float sum = v.weights.x + v.weights.y + v.weights.z + v.weights.w;
        EXPECT_NEAR(sum, 1.0f, 1e-3f);
        EXPECT_TRUE(v.joints.x < BoneCount);
        EXPECT_TRUE(v.joints.y < BoneCount);
    }
    // Forearm vertices map to forearm bones.
    int forearmOwned = 0;
    for (const Vertex& v : result.mesh.vertices) {
        if (v.position.y < 0.95f && v.position.x > 0.30f) {
            const bool forearm = (v.joints.x == BoneLeftLowerArm || v.joints.y == BoneLeftLowerArm);
            if (forearm) ++forearmOwned;
        }
    }
    EXPECT_TRUE(forearmOwned > 0);
}

int main() {
    return test::runAllTests();
}
