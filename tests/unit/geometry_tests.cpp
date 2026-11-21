#include "mini_test.hpp"

#include "procedural/MeshValidator.h"
#include "procedural/primitives/Primitives.h"
#include "procedural/surfaces/ParametricSurfaces.h"

#include <cmath>
#include <cstring>

using namespace pae;

PAE_TEST_CASE(uv_sphere_is_valid_and_deterministic) {
    const MeshData a = generateUvSphere(0.5f, 16, 12);
    const MeshData b = generateUvSphere(0.5f, 16, 12);
    const ValidationReport report = validateMesh(a);
    EXPECT_TRUE(report.valid);
    if (!report.valid) {
        for (const std::string& error : report.errors) std::fprintf(stderr, "%s\n", error.c_str());
    }
    EXPECT_TRUE(a.vertices.size() > 100);
    EXPECT_TRUE(a.indices.size() > 500);
    EXPECT_NEAR(a.bounds.size().x, 1.0f, 1e-4);
    EXPECT_NEAR(a.bounds.size().y, 1.0f, 1e-4);
    EXPECT_TRUE(std::memcmp(a.vertices.data(), b.vertices.data(),
                            a.vertices.size() * sizeof(Vertex)) == 0);
    EXPECT_TRUE(std::memcmp(a.indices.data(), b.indices.data(),
                            a.indices.size() * sizeof(uint32_t)) == 0);
}

PAE_TEST_CASE(ellipsoid_matches_parameter_extents) {
    const MeshData mesh = generateEllipsoid({0.2f, 0.4f, 0.3f}, 12, 8);
    EXPECT_TRUE(validateMesh(mesh).valid);
    EXPECT_NEAR(mesh.bounds.size().x, 0.4f, 1e-4);
    EXPECT_NEAR(mesh.bounds.size().y, 0.8f, 1e-4);
    EXPECT_NEAR(mesh.bounds.size().z, 0.6f, 1e-4);
}

PAE_TEST_CASE(capsule_and_rounded_box_are_valid) {
    const MeshData capsule = generateCapsule(0.1f, 0.6f, 12, 4, 3);
    EXPECT_TRUE(validateMesh(capsule).valid);
    EXPECT_NEAR(capsule.bounds.size().y, 0.8f, 1e-4);

    const MeshData box = generateRoundedBox({0.4f, 0.3f, 0.2f}, 0.05f, 3);
    EXPECT_TRUE(validateMesh(box).valid);
    EXPECT_NEAR(box.bounds.size().x, 0.4f, 1e-3);
}

PAE_TEST_CASE(parallel_transport_frames_avoid_flipping) {
    std::vector<Vec3> path;
    for (int i = 0; i <= 20; ++i) {
        const float t = static_cast<float>(i) / 20.0f;
        path.push_back({std::sin(t * 3.14159f) * 0.5f, t, std::cos(t * 3.14159f) * 0.25f});
    }
    const std::vector<SurfaceFrame> frames = computeParallelTransportFrames(path);
    EXPECT_TRUE(frames.size() == path.size());
    for (size_t i = 0; i < frames.size(); ++i) {
        EXPECT_NEAR(frames[i].tangent.length(), 1.0f, 1e-4);
        EXPECT_NEAR(frames[i].normal.length(), 1.0f, 1e-4);
        EXPECT_NEAR(frames[i].binormal.length(), 1.0f, 1e-4);
        EXPECT_NEAR(frames[i].normal.dot(frames[i].binormal), 0.0f, 1e-4);
        if (i > 0) EXPECT_TRUE(frames[i].normal.dot(frames[i - 1].normal) > 0.5f);
    }
}

PAE_TEST_CASE(sweep_and_loft_generate_valid_surfaces) {
    std::vector<Vec3> path;
    std::vector<float> radii;
    for (int i = 0; i <= 8; ++i) {
        path.push_back({0.0f, static_cast<float>(i) * 0.1f, 0.0f});
        radii.push_back(0.05f + 0.01f * static_cast<float>(i));
    }
    const MeshData sweep = generateSweepSurface(path, radii, 12);
    EXPECT_TRUE(validateMesh(sweep).valid);
    EXPECT_TRUE(sweep.bounds.size().y > 0.7f);

    std::vector<LoftSection> sections;
    sections.push_back({{0.0f, 0.0f, 0.0f}, 0.12f, 0.10f});
    sections.push_back({{0.0f, 0.2f, 0.0f}, 0.16f, 0.12f});
    sections.push_back({{0.0f, 0.5f, 0.0f}, 0.10f, 0.09f});
    const MeshData loft = generateLoftSurface(sections, 12);
    EXPECT_TRUE(validateMesh(loft).valid);
    EXPECT_NEAR(loft.bounds.size().y, 0.5f, 1e-4);
}

PAE_TEST_CASE(validator_rejects_invalid_mesh) {
    MeshData mesh = generateUvSphere(0.5f, 8, 6);
    mesh.indices[0] = 99999;
    EXPECT_FALSE(validateMesh(mesh).valid);

    MeshData nanMesh = generateUvSphere(0.5f, 8, 6);
    nanMesh.vertices[3].position.x = std::nanf("");
    EXPECT_FALSE(validateMesh(nanMesh).valid);

    MeshData weightsMesh = generateUvSphere(0.5f, 8, 6);
    weightsMesh.vertices[2].weights = {0.5f, 0.0f, 0.0f, 0.0f};
    EXPECT_FALSE(validateMesh(weightsMesh).valid);
}

// M11: spline sweep with elliptical sections, taper normals, caps, UV seam.
PAE_TEST_CASE(spline_sweep_passes_through_rings_and_supports_ellipse) {
    std::vector<SweepRing> rings;
    rings.push_back({{0.0f, 0.0f, 0.0f}, 0.06f, {1.0f, 1.0f}, 0.0f});
    rings.push_back({{0.0f, 0.25f, 0.0f}, 0.09f, {1.0f, 1.0f}, 0.0f});
    rings.push_back({{0.0f, 0.55f, 0.0f}, 0.05f, {1.0f, 1.0f}, 0.0f});
    const MeshData mesh = generateSplineSweep(rings, 12, 6, true, true, false);
    const ValidationReport report = validateMesh(mesh);
    EXPECT_TRUE(report.valid);
    if (!report.valid) {
        for (const std::string& error : report.errors) std::fprintf(stderr, "%s\n", error.c_str());
    }
    // Spline passes through every control ring: vertices exist in each
    // control ring's plane at roughly the control radius (horizontal distance).
    bool foundBottom = false, foundMiddle = false, foundTop = false;
    for (const Vertex& v : mesh.vertices) {
        const float radial = std::sqrt(v.position.x * v.position.x + v.position.z * v.position.z);
        if (std::fabs(v.position.y - 0.0f) < 0.02f &&
            std::fabs(radial - 0.06f) < 0.02f) foundBottom = true;
        if (std::fabs(v.position.y - 0.25f) < 0.02f &&
            std::fabs(radial - 0.09f) < 0.02f) foundMiddle = true;
        if (std::fabs(v.position.y - 0.55f) < 0.02f &&
            std::fabs(radial - 0.05f) < 0.02f) foundTop = true;
    }
    EXPECT_TRUE(foundBottom && foundMiddle && foundTop);
    // UV seam: duplicated column reaches u = 1.0.
    bool uReachedOne = false;
    for (const Vertex& v : mesh.vertices) {
        if (v.uv.x > 0.999f) uReachedOne = true;
    }
    EXPECT_TRUE(uReachedOne);
}

PAE_TEST_CASE(spline_sweep_taper_normals_tilt_against_growth) {
    std::vector<SweepRing> rings;
    rings.push_back({{0.0f, 0.0f, 0.0f}, 0.04f, {1.0f, 1.0f}, 0.0f});
    rings.push_back({{0.0f, 0.3f, 0.0f}, 0.16f, {1.0f, 1.0f}, 0.0f});
    rings.push_back({{0.0f, 0.6f, 0.0f}, 0.16f, {1.0f, 1.0f}, 0.0f});
    const MeshData mesh = generateSplineSweep(rings, 12, 8, false, false, false);
    EXPECT_TRUE(validateMesh(mesh).valid);
    // On the strongly widening lower half, normals must lean toward -Y.
    bool foundTilted = false;
    for (const Vertex& v : mesh.vertices) {
        if (v.position.y > 0.05f && v.position.y < 0.25f && v.position.z > 0.0f) {
            if (v.normal.y < -0.05f) foundTilted = true;
        }
    }
    EXPECT_TRUE(foundTilted);
}

PAE_TEST_CASE(spline_sweep_ellipse_section_scales_axes) {
    std::vector<SweepRing> rings;
    rings.push_back({{0.0f, 0.0f, 0.0f}, 0.10f, {2.0f, 1.0f}, 0.0f});
    rings.push_back({{0.0f, 0.4f, 0.0f}, 0.10f, {2.0f, 1.0f}, 0.0f});
    const MeshData mesh = generateSplineSweep(rings, 12, 6, true, true, false);
    const float extentX = mesh.bounds.size().x;
    const float extentZ = mesh.bounds.size().z;
    EXPECT_NEAR(extentX / extentZ, 2.0f, 0.05f);
}

PAE_TEST_CASE(spline_loft_normalizes_winding_for_downward_sections) {
    std::vector<LoftSection> down;
    down.push_back({{0.0f, 0.5f, 0.0f}, 0.12f, 0.10f});
    down.push_back({{0.0f, 0.2f, 0.0f}, 0.16f, 0.12f});
    const MeshData meshDown = generateSplineLoft(down, 12, 4);
    EXPECT_TRUE(validateMesh(meshDown).valid);
    // Outward-facing: face normal must agree with the radial direction.
    size_t checked = 0, outward = 0;
    for (size_t i = 0; i + 2 < meshDown.indices.size() && checked < 64; i += 3, ++checked) {
        const Vec3& a = meshDown.vertices[meshDown.indices[i]].position;
        const Vec3& b = meshDown.vertices[meshDown.indices[i + 1]].position;
        const Vec3& c = meshDown.vertices[meshDown.indices[i + 2]].position;
        const Vec3 faceNormal = (b - a).cross(c - a);
        const Vec3 radial{a.x, 0.0f, a.z};
        if (radial.lengthSq() < 1e-8f) continue;
        if (faceNormal.dot(radial) > 0.0f) ++outward;
    }
    EXPECT_TRUE(outward == checked);
}

int main() {
    return test::runAllTests();
}
