// PoC-SM (AUTH-08 AL-5): the shoulder-root seam measurement under the OQ-03
// FIXED DEFINITION.
//
// The definition is reproduced in shape/v2/SeamAuditV2.h. These tests pin the
// BEHAVIOUR of that definition.
//
// THE MEASUREMENT'S MEANING, made explicit by the tests below:
//
// The band is the ring of quads whose UPPER ROW lies on the crop ring, and
// the statistic is the angle between the FACE NORMALS of two quads that are
// neighbours AROUND the band. On a PERFECT cylinder that angle is exactly
// 360/segments - it is the polygonal faceting of the band, not a crease.
//
// Consequence, measured by `cylinder_band_reads_the_polygonal_faceting`:
// a 32-segment cylinder reads 11.25 deg, so the 5 deg contract value CANNOT
// be met by a 32-segment grid no matter how perfect the shape is. The
// faceting alone crosses the contract value below 72 segments.
//
// This is a property of the DEFINITION, not of any implementation, and it is
// why the probe reports the measurement against the 360/segments baseline.

#include "mini_test.hpp"

#include "procedural/shape/v2/SeamAuditV2.h"

#include <array>
#include <cmath>
#include <vector>

using namespace pae;
using namespace pae::shape;
using namespace pae::shape::v2;

namespace {

constexpr float kPi = 3.14159265358979323846f;

// A quad band around a cylinder: `segments` around, `rings` tall, with the
// crop ring on the top ring.
struct BandMesh {
    MeshData mesh;
    std::vector<std::array<uint32_t, 4>> quads;
    std::vector<CropBoundary> crops;
    int segments = 8;
    int rings = 3;
    float radius = 1.0f;
};

BandMesh makeCylinder(int segments = 8, int rings = 3, float radius = 1.0f) {
    BandMesh bm;
    bm.segments = segments;
    bm.rings = rings;
    bm.radius = radius;

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            const float theta = static_cast<float>(s) / static_cast<float>(segments) * 2.0f * kPi;
            Vertex v;
            v.position = Vec3{radius * std::cos(theta), static_cast<float>(r),
                              radius * std::sin(theta)};
            bm.mesh.vertices.push_back(v);
        }
    }

    for (int r = 0; r + 1 < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            const uint32_t sn = static_cast<uint32_t>((s + 1) % segments);
            const uint32_t a = static_cast<uint32_t>(r * segments + s);
            const uint32_t b = static_cast<uint32_t>(r * segments + static_cast<int>(sn));
            const uint32_t c = static_cast<uint32_t>((r + 1) * segments + static_cast<int>(sn));
            const uint32_t d = static_cast<uint32_t>((r + 1) * segments + s);
            bm.quads.push_back({a, b, c, d});
            // Outward winding: (a, c, b) and (a, d, c).
            bm.mesh.indices.push_back(a);
            bm.mesh.indices.push_back(c);
            bm.mesh.indices.push_back(b);
            bm.mesh.indices.push_back(a);
            bm.mesh.indices.push_back(d);
            bm.mesh.indices.push_back(c);
        }
    }

    CropBoundary crop;
    crop.label = "shoulder-root";
    crop.ringIndex = rings - 1;
    crop.vertexCount = segments;
    crop.edgeCount = segments;
    crop.loopCount = 1;
    crop.closed = true;
    crop.y = static_cast<float>(rings - 1);
    bm.crops.push_back(crop);
    return bm;
}

} // namespace

PAE_TEST_CASE(no_crop_ring_is_not_applicable_and_never_passes) {
    BandMesh bm = makeCylinder();
    bm.crops.clear();

    const SeamMeasurement m = measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops);
    EXPECT_TRUE(m.applicability == SeamApplicability::NotApplicable);
    EXPECT_FALSE(m.passes);
    EXPECT_NEAR(m.maxDeg, 0.0, 1e-9);
    EXPECT_TRUE(!m.notes.empty());
}

PAE_TEST_CASE(unknown_crop_label_is_not_applicable) {
    BandMesh bm = makeCylinder();
    const SeamMeasurement m =
        measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops, "pelvis-crop");
    EXPECT_TRUE(m.applicability == SeamApplicability::NotApplicable);
    EXPECT_FALSE(m.passes);
}

PAE_TEST_CASE(cylinder_band_reads_the_polygonal_faceting) {
    // A perfect cylinder has no crease: the measured angle is exactly the
    // faceting of the band, 360/segments.
    const int counts[] = {8, 16, 32};
    for (int segments : counts) {
        BandMesh bm = makeCylinder(segments, 3);
        const SeamMeasurement m = measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops);

        EXPECT_TRUE(m.applicability == SeamApplicability::Applicable);
        EXPECT_TRUE(m.windingVerified);
        EXPECT_TRUE(m.includedQuadCount == segments);
        EXPECT_TRUE(m.angleSampleCount == segments);

        const double expected = 360.0 / static_cast<double>(segments);
        EXPECT_NEAR(m.maxDeg, expected, 0.05);
        EXPECT_NEAR(m.meanDeg, expected, 0.05);
    }
}

PAE_TEST_CASE(the_five_degree_contract_needs_at_least_72_segments) {
    // The consequence of the definition, stated as a test so it cannot be
    // quietly forgotten: the faceting alone crosses the 5 deg contract value
    // below 72 segments.
    //
    // 72 segments is the theoretical boundary (360/72 = 5.0 deg exactly); the
    // assertion uses 96 so that the check is not sitting on the numerical
    // edge of acos near 1.
    {
        BandMesh bm = makeCylinder(32, 3);
        const SeamMeasurement m = measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops);
        EXPECT_TRUE(m.maxDeg > 5.0);
        EXPECT_FALSE(m.passes);
    }
    {
        BandMesh bm = makeCylinder(96, 3);
        const SeamMeasurement m = measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops);
        EXPECT_TRUE(m.maxDeg < 5.0);
        EXPECT_TRUE(m.passes);
    }
}

PAE_TEST_CASE(angle_domain_is_not_folded_to_an_acute_angle) {
    // Two quads whose normals are 120 deg apart must measure 120, not 60.
    const Vec3 n0{1.0f, 0.0f, 0.0f};
    const Vec3 n1{-0.5f, 0.0f, 0.8660254f};
    const double angle = seamAngleDeg(n0, n1);
    EXPECT_NEAR(angle, 120.0, 1e-2);

    // The full domain must be reachable.
    EXPECT_NEAR(seamAngleDeg(Vec3{1.0f, 0.0f, 0.0f}, Vec3{-1.0f, 0.0f, 0.0f}), 180.0, 1e-2);
    EXPECT_NEAR(seamAngleDeg(Vec3{1.0f, 0.0f, 0.0f}, Vec3{1.0f, 0.0f, 0.0f}), 0.0, 1e-2);
}

PAE_TEST_CASE(statistic_is_max_not_mean) {
    // Tilt one quad of the band so its neighbours see a large deflection
    // while the rest of the band stays at the faceting baseline.
    BandMesh bm = makeCylinder(32, 3);

    // Displace the two upper vertices of segment 0 inward along -X so that
    // one band quad tilts noticeably.
    const uint32_t topSeg0 = static_cast<uint32_t>(2 * bm.segments + 0);
    const uint32_t topSeg1 = static_cast<uint32_t>(2 * bm.segments + 1);
    bm.mesh.vertices[topSeg0].position.x -= 1.2f;
    bm.mesh.vertices[topSeg1].position.x -= 1.2f;

    const SeamMeasurement m = measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops);
    EXPECT_TRUE(m.applicability == SeamApplicability::Applicable);

    // The tilted quad must dominate: max > mean, and max is what gates.
    EXPECT_TRUE(m.maxDeg > m.meanDeg);
    EXPECT_TRUE(m.maxDeg > 5.0);
    EXPECT_FALSE(m.passes);
    EXPECT_NEAR(m.maxDegIncludingCap, m.maxDeg, 1e-9);
}

PAE_TEST_CASE(inverted_quad_is_detected_by_the_winding_check) {
    BandMesh bm = makeCylinder();
    // Flip one band quad by reversing its triangles in the index buffer.
    const uint32_t a = static_cast<uint32_t>(1 * bm.segments + 0);
    const uint32_t b = static_cast<uint32_t>(1 * bm.segments + 1);
    const uint32_t c = static_cast<uint32_t>(2 * bm.segments + 1);
    const uint32_t d = static_cast<uint32_t>(2 * bm.segments + 0);
    for (size_t i = 0; i + 2 < bm.mesh.indices.size(); i += 3) {
        const uint32_t ia = bm.mesh.indices[i];
        const uint32_t ib = bm.mesh.indices[i + 1];
        const uint32_t ic = bm.mesh.indices[i + 2];
        const bool touches = (ia == a || ia == b || ia == c || ia == d) &&
                             (ib == a || ib == b || ib == c || ib == d) &&
                             (ic == a || ic == b || ic == c || ic == d);
        if (!touches) continue;
        bm.mesh.indices[i + 1] = ic;
        bm.mesh.indices[i + 2] = ib;
    }

    const SeamMeasurement m = measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops);
    EXPECT_TRUE(m.applicability == SeamApplicability::Applicable);
    EXPECT_FALSE(m.windingVerified);
    EXPECT_TRUE(m.windingInconsistencyCount > 0);
    // The affected samples are dropped, so fewer samples remain.
    EXPECT_TRUE(m.angleSampleCount < m.includedQuadCount);
}

PAE_TEST_CASE(cap_fan_is_counted_and_reported_separately) {
    BandMesh bm = makeCylinder();
    // Add a FLAT bottom cap fan (the pole sits in the ring's plane), as the
    // template does. A flat fan has 0 deg between adjacent triangles.
    const uint32_t pole = static_cast<uint32_t>(bm.mesh.vertices.size());
    Vertex pv;
    pv.position = Vec3{0.0f, 0.0f, 0.0f};
    bm.mesh.vertices.push_back(pv);
    for (int j = 0; j < bm.segments; ++j) {
        const uint32_t v0 = static_cast<uint32_t>(j);
        const uint32_t v1 = static_cast<uint32_t>((j + 1) % bm.segments);
        bm.mesh.indices.push_back(pole);
        bm.mesh.indices.push_back(v0);
        bm.mesh.indices.push_back(v1);
    }

    const SeamMeasurement m = measureShoulderRootSeam(bm.mesh, bm.quads, bm.crops);
    EXPECT_TRUE(m.capTriangleCount == bm.segments);
    EXPECT_TRUE(m.capAngleSampleCount == bm.segments);
    // A flat fan has 0 deg between adjacent triangles. The tolerance is not
    // 0 because acos near 1 is ill-conditioned: a dot product of 1 - 1e-9
    // already reads as ~0.003 deg.
    EXPECT_NEAR(m.capMaxDeg, 0.0, 0.05);
    // The cap is labelled, not silently folded into the band statistic.
    EXPECT_TRUE(!m.notes.empty());
}

PAE_TEST_CASE(quad_normal_follows_the_winding) {
    BandMesh bm = makeCylinder();
    // At segment 0 (+X side) the outward radial direction is +X, so the
    // outward normal must have a positive X component.
    const Vec3 n = seamQuadNormal(bm.mesh, bm.quads[0]);
    EXPECT_TRUE(n.x > 0.0f);
    EXPECT_NEAR(n.length(), 1.0, 1e-4);
}

int main() {
    return pae::test::runAllTests();
}
