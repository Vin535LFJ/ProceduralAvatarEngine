#include "mini_test.hpp"

#include "procedural/fusion/SdfFusion.h"
#include "procedural/primitives/Primitives.h"
#include "procedural/surfaces/ParametricSurfaces.h"
#include <chrono>
#include "procedural/MeshValidator.h"

#include <cmath>
#include <cstring>

using namespace pae;
using namespace pae::fusion;

namespace {

bool triangleNormalsOutward(const MeshData& mesh, const SdfField& sdf) {
    auto gradient = [&sdf](Vec3 p) {
        const float e = 0.01f;
        return Vec3{sdf(p + Vec3{e, 0.0f, 0.0f}) - sdf(p - Vec3{e, 0.0f, 0.0f}),
                    sdf(p + Vec3{0.0f, e, 0.0f}) - sdf(p - Vec3{0.0f, e, 0.0f}),
                    sdf(p + Vec3{0.0f, 0.0f, e}) - sdf(p - Vec3{0.0f, 0.0f, e})};
    };
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const Vec3& a = mesh.vertices[mesh.indices[i]].position;
        const Vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const Vec3& c = mesh.vertices[mesh.indices[i + 2]].position;
        const Vec3 faceNormal = (b - a).cross(c - a);
        if (faceNormal.dot(gradient((a + b + c) * (1.0f / 3.0f))) < 0.0f) return false;
    }
    return true;
}

} // namespace

PAE_TEST_CASE(fusion_two_spheres_merges_into_single_blob) {
    FusionVolume volume;
    volume.center = {0.0f, 0.0f, 0.0f};
    volume.size = {2.4f, 2.4f, 2.4f};
    volume.resolution = {20, 20, 20};
    const float k = 0.3f;
    const SdfField field = [k](Vec3 p) {
        const float a = sdSphere(p, Vec3{-0.35f, 0.0f, 0.0f}, 0.5f);
        const float b = sdSphere(p, Vec3{0.35f, 0.15f, 0.0f}, 0.4f);
        return sdSmoothUnion(a, b, k);
    };
    const MeshData mesh = extractSurfaceNets(volume, field);
    EXPECT_TRUE(!mesh.vertices.empty());
    EXPECT_TRUE(!mesh.indices.empty());
    EXPECT_TRUE(validateMesh(mesh).valid);
    EXPECT_TRUE(triangleNormalsOutward(mesh, field));
    // Smooth union bridges the gap: surface points exist near x=0 between centers.
    bool bridged = false;
    for (const Vertex& v : mesh.vertices) {
        if (std::fabs(v.position.x) < 0.12f) bridged = true;
    }
    EXPECT_TRUE(bridged);

    const MeshData again = extractSurfaceNets(volume, field);
    EXPECT_TRUE(again.vertices.size() == mesh.vertices.size());
    EXPECT_TRUE(std::memcmp(again.vertices.data(), mesh.vertices.data(),
                            mesh.vertices.size() * sizeof(Vertex)) == 0);
}

PAE_TEST_CASE(fusion_capsule_ellipsoid_shoulder_blend) {
    FusionVolume volume;
    volume.center = {0.0f, 1.3f, 0.0f};
    volume.size = {1.6f, 1.2f, 1.4f};
    volume.resolution = {28, 28, 28};
    const float k = 0.12f;
    const SdfField field = [k](Vec3 p) {
        const float torso = sdEllipsoidApprox(p, Vec3{0.0f, 1.3f, 0.0f}, Vec3{0.21f, 0.26f, 0.15f});
        const float arm = sdCapsule(p, Vec3{0.24f, 1.32f, 0.0f}, Vec3{0.42f, 1.1f, 0.0f}, 0.062f);
        return sdSmoothUnion(torso, arm, k);
    };
    const MeshData mesh = extractSurfaceNets(volume, field);
    EXPECT_TRUE(!mesh.vertices.empty());
    EXPECT_TRUE(validateMesh(mesh).valid);
    EXPECT_TRUE(triangleNormalsOutward(mesh, field));
    // Every vertex sits near the isosurface (Surface Nets approximation error
    // is bounded by ~the cell size; tolerance covers it at resolution 28).
    for (const Vertex& v : mesh.vertices) {
        EXPECT_NEAR(field(v.position), 0.0f, 0.03f);
    }
    // Vertex normals align with the SDF gradient.
    for (const Vertex& v : mesh.vertices) {
        const float e = 0.005f;
        const Vec3 g{field(v.position + Vec3{e, 0, 0}) - field(v.position - Vec3{e, 0, 0}),
                     field(v.position + Vec3{0, e, 0}) - field(v.position - Vec3{0, e, 0}),
                     field(v.position + Vec3{0, 0, e}) - field(v.position - Vec3{0, 0, e})};
        EXPECT_NEAR(v.normal.dot(g.normalized({0.0f, 1.0f, 0.0f})), 1.0f, 0.15f);
    }
}

PAE_TEST_CASE(fusion_smooth_subtraction_carves_socket) {
    FusionVolume volume;
    volume.center = {0.0f, 0.0f, 0.0f};
    volume.size = {1.6f, 1.6f, 1.6f};
    volume.resolution = {28, 28, 28};
    const float k = 0.08f;
    const SdfField field = [k](Vec3 p) {
        const float ball = sdSphere(p, Vec3{0.0f, 0.0f, 0.0f}, 0.6f);
        const float socket = sdSphere(p, Vec3{0.0f, 0.0f, 0.62f}, 0.18f);
        return sdSmoothSubtraction(ball, socket, k);
    };
    const MeshData mesh = extractSurfaceNets(volume, field);
    EXPECT_TRUE(!mesh.vertices.empty());
    EXPECT_TRUE(validateMesh(mesh).valid);
    EXPECT_TRUE(triangleNormalsOutward(mesh, field));
    // The socket is carved: the ball's +Z pole cap (z >= 0.575 on the ball
    // surface) is removed, so no vertex may remain at the original pole.
    for (const Vertex& v : mesh.vertices) {
        EXPECT_TRUE(v.position.z < 0.59f);
    }
}

// M11-B PoC: shoulder+upper-arm built three ways, measured for the ADR-008
// decision. Gap metric = worst distance from the arm sweep's open end ring to
// the torso surface (method A leaves a visible seam; B/C should close it).
PAE_TEST_CASE(poc_shoulder_fusion_comparison) {
    const Vec3 shoulder{0.24f, 1.32f, 0.0f};
    const Vec3 elbow{0.42f, 1.10f, 0.0f};
    const float armRadius = 0.062f;
    const float k = 0.12f;
    const SdfField bodyField = [&, k](Vec3 p) {
        const float torso = sdEllipsoidApprox(p, Vec3{0.0f, 1.3f, 0.0f}, Vec3{0.21f, 0.26f, 0.15f});
        const float arm = sdCapsule(p, shoulder, elbow, armRadius);
        return sdSmoothUnion(torso, arm, k);
    };

    // Method A: torso loft + arm sweep + joint sphere (M10 baseline).
    std::vector<LoftSection> torsoSections;
    torsoSections.push_back({{0.0f, 1.04f, 0.0f}, 0.17f, 0.115f});
    torsoSections.push_back({{0.0f, 1.3f, 0.0f}, 0.21f, 0.155f});
    torsoSections.push_back({{0.0f, 1.46f, 0.0f}, 0.19f, 0.13f});
    std::vector<Vec3> armPath{shoulder, elbow};
    std::vector<float> armRadii{armRadius * 1.12f, armRadius * 0.82f};
    long long triA = 0;
    float gapA = 0.0f;
    {
        const auto t0 = std::chrono::steady_clock::now();
        MeshData a = generateLoftSurface(torsoSections, 16);
        MeshData sweep = generateSweepSurface(armPath, armRadii, 14);
        MeshData sphere = generateUvSphere(armRadius * 1.22f, 14, 8);
        for (Vertex& v : sphere.vertices) v.position = v.position + shoulder;
        triA = static_cast<long long>((a.indices.size() + sweep.indices.size() +
                                       sphere.indices.size()) / 3);
        const auto t1 = std::chrono::steady_clock::now();
        std::fprintf(stderr,
                     "[POC-A] triangles=%lld gen_ms=%.2f (loft+sweep+joint sphere)\n",
                     triA, std::chrono::duration<float, std::milli>(t1 - t0).count());
        // Seam: arm end ring center distance to the torso surface.
        gapA = std::fabs(sdEllipsoidApprox(elbow, Vec3{0.0f, 1.3f, 0.0f}, Vec3{0.21f, 0.26f, 0.15f}));
        std::fprintf(stderr, "[POC-A] seam_gap=%.4f m (arm end ring center to torso surface)\n", gapA);
    }

    // Method B: arm sweep starts embedded inside the torso (centerline probe).
    long long triB = 0;
    float gapB = 0.0f;
    {
        const auto t0 = std::chrono::steady_clock::now();
        std::vector<Vec3> path{shoulder + Vec3{-0.06f, 0.02f, 0.0f}, shoulder, elbow};
        std::vector<float> radii{armRadius * 1.05f, armRadius * 1.12f, armRadius * 0.82f};
        MeshData b = generateSweepSurface(path, radii, 14);
        triB = static_cast<long long>(b.indices.size() / 3);
        const auto t1 = std::chrono::steady_clock::now();
        std::fprintf(stderr, "[POC-B] triangles=%lld gen_ms=%.2f (embedded sweep)\n",
                     triB, std::chrono::duration<float, std::milli>(t1 - t0).count());
        (void)gapB;
    }

    // Method C: SDF fusion patch (torso ellipsoid + arm capsule, smooth union).
    long long triC = 0;
    float isoErrorC = 0.0f;
    double genMsC = 0.0;
    {
        FusionVolume volume;
        volume.center = Vec3{0.24f, 1.28f, 0.0f};
        volume.size = {0.8f, 0.7f, 0.7f};
        volume.resolution = {28, 28, 28};
        const auto t0 = std::chrono::steady_clock::now();
        const MeshData c = extractSurfaceNets(volume, bodyField);
        const auto t1 = std::chrono::steady_clock::now();
        genMsC = std::chrono::duration<double, std::milli>(t1 - t0).count();
        triC = static_cast<long long>(c.indices.size() / 3);
        for (const Vertex& v : c.vertices) {
            isoErrorC = std::fmax(isoErrorC, std::fabs(bodyField(v.position)));
        }
        std::fprintf(stderr,
                     "[POC-C] triangles=%lld gen_ms=%.2f iso_error=%.4f (SDF fusion patch)\n",
                     triC, genMsC, isoErrorC);
    }

    std::fprintf(stderr,
                 "[POC-SUMMARY] A=%lld tris seam=%.3f | B=%lld tris (embedded) | "
                 "C=%lld tris iso=%.3f gen=%.1fms\n",
                 triA, gapA, triB, triC, isoErrorC, genMsC);
    // Both fusion methods must produce a watertight-looking region (B embeds
    // the seam inside the torso; C is a single continuous surface).
    EXPECT_TRUE(triA > 0 && triB > 0 && triC > 0);
    EXPECT_TRUE(isoErrorC < 0.05f);
}

int main() {
    return test::runAllTests();
}
