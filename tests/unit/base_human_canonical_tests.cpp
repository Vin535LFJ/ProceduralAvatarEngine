// WP2 unit tests: static Render Mesh builder (template quads -> engine MeshData).

#include "mini_test.hpp"

#include "procedural/human_template/BaseHumanRenderMeshBuilder.h"
#include "procedural/human_template/HumanTemplateLoader.h"
#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <cmath>
#include <string>

using namespace pae::human_template;
using pae::MeshData;
using pae::Vertex;

namespace {

HumanTemplate makeTwoQuadStrip() {
    HumanTemplate t;
    // Two quads sharing edge (1,2): a 2x1 strip in XY.
    t.topology.pos = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {2, 0, 0}, {2, 1, 0}};
    t.topology.uv = {{0, 0}, {0.5f, 0}, {0.5f, 1}, {0, 1}, {1, 0}, {1, 1}};
    PolyFace a;
    a.v = {0, 1, 2, 3};
    a.vt = {0, 1, 2, 3};
    PolyFace b;
    b.v = {1, 4, 5, 2};
    b.vt = {1, 4, 5, 2};
    t.topology.faces = {a, b};
    t.regions.assign(6, Region::Thorax);
    t.weights.vertexCount = 6;
    t.weights.bone.assign(6, {0, 0, 0, 0});
    t.weights.w.assign(6, {1.0f, 0, 0, 0});
    t.weights.count.assign(6, 1);
    t.weights.fallback.assign(6, 0);
    return t;
}

} // namespace

PAE_TEST_CASE(static_mesh_two_quads_become_four_triangles) {
    const HumanTemplate t = makeTwoQuadStrip();
    MeshData mesh;
    StaticRenderMeshReport report;
    std::string error;
    EXPECT_TRUE(buildStaticRenderMesh(t, mesh, report, error));
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(mesh.vertices.size() == 6);
    EXPECT_TRUE(mesh.indices.size() == 12);
    EXPECT_TRUE(report.triangleCount == 4);
    EXPECT_TRUE(report.quadCount == 2);
    EXPECT_TRUE(report.degenerateTriangles == 0);
    EXPECT_TRUE(mesh.subMeshes.size() == 1);
    EXPECT_TRUE(mesh.subMeshes[0].indexCount == 12);
}

PAE_TEST_CASE(static_mesh_normals_unit_and_outward) {
    const HumanTemplate t = makeTwoQuadStrip();
    MeshData mesh;
    std::string error;
    EXPECT_TRUE(buildStaticRenderMesh(t, mesh, error));
    for (const Vertex& v : mesh.vertices) {
        const float len = v.normal.length();
        EXPECT_NEAR(len, 1.0f, 1e-5f);
        // CCW winding in XY -> +Z normal.
        EXPECT_NEAR(v.normal.z, 1.0f, 1e-5f);
    }
}

PAE_TEST_CASE(static_mesh_static_skinning_fields) {
    const HumanTemplate t = makeTwoQuadStrip();
    MeshData mesh;
    std::string error;
    EXPECT_TRUE(buildStaticRenderMesh(t, mesh, error));
    for (const Vertex& v : mesh.vertices) {
        EXPECT_TRUE(v.joints.x == 0 && v.joints.y == 0 && v.joints.z == 0 && v.joints.w == 0);
        EXPECT_NEAR(v.weights.x, 1.0f, 1e-6f);
        EXPECT_NEAR(v.weights.y, 0.0f, 1e-6f);
    }
}

PAE_TEST_CASE(static_mesh_uv_and_bounds) {
    const HumanTemplate t = makeTwoQuadStrip();
    MeshData mesh;
    std::string error;
    EXPECT_TRUE(buildStaticRenderMesh(t, mesh, error));
    EXPECT_NEAR(mesh.vertices[4].uv.x, 1.0f, 1e-6f);
    EXPECT_NEAR(mesh.vertices[2].uv.y, 1.0f, 1e-6f);
    EXPECT_NEAR(mesh.bounds.max.x, 2.0f, 1e-6f);
    EXPECT_NEAR(mesh.bounds.max.y, 1.0f, 1e-6f);
    EXPECT_NEAR(mesh.bounds.min.x, 0.0f, 1e-6f);
}

PAE_TEST_CASE(static_mesh_deterministic) {
    const HumanTemplate t = makeTwoQuadStrip();
    MeshData a;
    MeshData b;
    std::string error;
    EXPECT_TRUE(buildStaticRenderMesh(t, a, error));
    EXPECT_TRUE(buildStaticRenderMesh(t, b, error));
    EXPECT_TRUE(a.indices == b.indices);
    EXPECT_TRUE(a.vertices.size() == b.vertices.size());
    for (size_t i = 0; i < a.vertices.size(); ++i) {
        EXPECT_TRUE(a.vertices[i].position.x == b.vertices[i].position.x);
        EXPECT_TRUE(a.vertices[i].normal.z == b.vertices[i].normal.z);
    }
}

PAE_TEST_CASE(static_mesh_rejects_empty) {
    HumanTemplate t;
    MeshData mesh;
    std::string error;
    EXPECT_FALSE(buildStaticRenderMesh(t, mesh, error));
    EXPECT_FALSE(error.empty());
}

#ifdef PAE_TEMPLATE_ASSET_PATH
PAE_TEST_CASE(static_mesh_real_asset_full_body) {
    HumanTemplate t;
    std::string error;
    EXPECT_TRUE(loadTemplateFile(PAE_TEMPLATE_ASSET_PATH, t, error));
    MeshData mesh;
    StaticRenderMeshReport report;
    EXPECT_TRUE(buildStaticRenderMesh(t, mesh, report, error));
    EXPECT_TRUE(mesh.vertices.size() == 13380);
    EXPECT_TRUE(report.triangleCount == 26756);  // 2 per quad, 13378 quads
    EXPECT_TRUE(report.quadCount == 13378);
    // Every normal finite and unit length.
    size_t badNormals = 0;
    for (const Vertex& v : mesh.vertices) {
        if (!v.normal.finite()) { ++badNormals; continue; }
        const float len = v.normal.length();
        if (std::fabs(len - 1.0f) > 1e-3f) ++badNormals;
    }
    EXPECT_TRUE(badNormals == 0);
    // Stature ~1.66 m. The source base mesh is centred on the origin (feet at
    // negative y); grounding to y=0 is a scene transform applied by the loader.
    EXPECT_NEAR(mesh.bounds.max.y - mesh.bounds.min.y, 1.6659, 0.02);
    EXPECT_NEAR(mesh.bounds.min.y, -0.8168, 0.01);
    EXPECT_NEAR(mesh.bounds.max.y, 0.8491, 0.01);
    std::fprintf(stderr, "[info] real asset mesh: verts=%zu tris=%u degenerate=%u boundsY=[%.4f, %.4f]\n",
                 mesh.vertices.size(), report.triangleCount, report.degenerateTriangles,
                 mesh.bounds.min.y, mesh.bounds.max.y);
}
#endif

int main() { return pae::test::runAllTests(); }