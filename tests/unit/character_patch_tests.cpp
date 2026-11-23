#include "mini_test.hpp"

#include "procedural/MeshTopology.h"
#include "procedural/MeshValidator.h"
#include "procedural/character/BodyPatchGenerator.h"
#include "procedural/humanoid/Humanoid.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pae;

namespace {

bool generatePatch(const CharacterDefinition& definition, MeshData& mesh,
                   PatchGenerationReport& report) {
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{}, false);
    return BodyPatchGenerator::generate(definition, humanoid.joints, humanoid.skeleton,
                                        mesh, report);
}

} // namespace

PAE_TEST_CASE(p1a_patch_mesh_is_valid_and_connected) {
    CharacterDefinition definition;
    MeshData mesh;
    PatchGenerationReport report;
    EXPECT_TRUE(generatePatch(definition, mesh, report));

    const ValidationReport validation = validateMesh(mesh);
    if (!validation.valid) {
        for (const std::string& error : validation.errors) std::fprintf(stderr, "%s\n", error.c_str());
    }
    EXPECT_TRUE(validation.valid);

    const TopologyReport topology = analyzeTopology(mesh);
    std::fprintf(stderr,
                 "P1A-TOPO verts=%zu tris=%ld components=%d boundary=%d nonmanifold=%d "
                 "degenerate=%d dup=%d badnorm=%d gen_ms=%.3f\n",
                 topology.vertices, topology.triangles, topology.components,
                 topology.boundaryEdges, topology.nonManifoldEdges,
                 topology.degenerateTriangles, topology.duplicateVertices,
                 topology.invalidNormals, report.generationMs);
    EXPECT_TRUE(topology.components == 1);
    EXPECT_TRUE(topology.nonManifoldEdges == 0);
    EXPECT_TRUE(topology.degenerateTriangles == 0);
    EXPECT_TRUE(topology.invalidNormals == 0);

    EXPECT_TRUE(report.boundaries.size() >= 2);
    for (const BoundaryReport& boundary : report.boundaries) {
        EXPECT_TRUE(boundary.valid);
        EXPECT_NEAR(boundary.maxPositionDeviation, 0.0f, 1e-6f);
        EXPECT_NEAR(boundary.maxNormalDeviationDeg, 0.0f, 1e-6f);
    }
}

PAE_TEST_CASE(p1a_parameters_change_actual_shape) {
    CharacterDefinition narrow;
    narrow.chestWidth = 0.30f;
    CharacterDefinition wide;
    wide.chestWidth = 0.46f;

    MeshData a, b;
    PatchGenerationReport ra, rb;
    EXPECT_TRUE(generatePatch(narrow, a, ra));
    EXPECT_TRUE(generatePatch(wide, b, rb));

    const HumanoidResult humanoid = generateHumanoid(BodyParameters{}, false);
    auto chestHalfWidth = [&](const MeshData& mesh) {
        float maximum = 0.0f;
        // First 5*32 vertices are the torso rings; arm vertices must not
        // dominate this measurement.
        const size_t torsoVertexCount = 5u * 32u;
        for (size_t i = 0; i < std::min(torsoVertexCount, mesh.vertices.size()); ++i) {
            const Vertex& vertex = mesh.vertices[i];
            if (std::fabs(vertex.position.y - humanoid.joints.chest.y) < 0.05f) {
                maximum = std::fmax(maximum, std::fabs(vertex.position.x));
            }
        }
        return maximum;
    };
    const float widthA = chestHalfWidth(a);
    const float widthB = chestHalfWidth(b);
    std::fprintf(stderr, "P1A-PARAM chestHalfWidth %.4f -> %.4f\n", widthA, widthB);
    EXPECT_TRUE(widthB > widthA + 0.02f);
    EXPECT_TRUE(a.vertices.size() == b.vertices.size());
}

PAE_TEST_CASE(p1a_weights_are_valid) {
    CharacterDefinition definition;
    MeshData mesh;
    PatchGenerationReport report;
    EXPECT_TRUE(generatePatch(definition, mesh, report));
    for (const Vertex& vertex : mesh.vertices) {
        const float sum = vertex.weights.x + vertex.weights.y + vertex.weights.z + vertex.weights.w;
        EXPECT_NEAR(sum, 1.0f, 1e-4f);
        EXPECT_TRUE(vertex.joints.x < BoneCount);
        EXPECT_TRUE(vertex.joints.y < BoneCount);
        EXPECT_TRUE(std::isfinite(vertex.normal.length()));
    }
}

int main() {
    return test::runAllTests();
}
