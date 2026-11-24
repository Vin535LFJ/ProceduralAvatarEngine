// PoC-SM round 2 (AL-5 experiment): cross-boundary seam measurement.
//
// These tests pin the BEHAVIOUR of the new experimental tool, and contrast it
// with the signed OQ-03 ring-band tool. They are EXPERIMENTAL results only:
// no threshold decision and no Gate judgement is implied.

#include "mini_test.hpp"

#include "poc_sm_cross_seam_fixture.h"
#include "procedural/shape/v2/CrossBoundarySeamAudit.h"

#include <cmath>
#include <utility>
#include <vector>

using namespace pae;
using namespace pae::shape;
using namespace pae::shape::v2;
using namespace pae::shape::v2::cross_seam_fixture;

namespace {

const FoldedSeam make(int segments, float fold, int rows = 4) {
    return makeFoldedSeam(segments, rows, fold);
}

} // namespace

PAE_TEST_CASE(known_folds_are_recovered_from_the_real_triangle_pair) {
    const float folds[] = {0.0f, 5.0f, 15.0f, 30.0f};
    for (float fold : folds) {
        const FoldedSeam f = make(64, fold);
        const CrossBoundarySeamMeasurement m =
            measureCrossBoundarySeam(f.lower, f.upper);

        EXPECT_TRUE(m.applicability == CrossSeamApplicability::Applicable);
        EXPECT_TRUE(m.windingVerified);
        EXPECT_TRUE(m.unmatchedVertices == 0);
        EXPECT_TRUE(m.missingTrianglePairs == 0);
        EXPECT_TRUE(m.boundaryEdges == 64);
        EXPECT_TRUE(m.usableAngleSamples == 64);
        EXPECT_NEAR(m.maxDeg, static_cast<double>(fold), 0.5);
        EXPECT_NEAR(m.meanDeg, static_cast<double>(fold), 0.5);
        EXPECT_NEAR(m.p95Deg, static_cast<double>(fold), 0.5);
    }
}

PAE_TEST_CASE(fold_result_is_stable_across_32_64_96_128_segments) {
    // The legacy ring statistic is diluted by 1/S. The cross-boundary angle
    // is a local dihedral and must NOT change when the ring is subdivided.
    const int counts[] = {32, 64, 96, 128};
    std::vector<double> results;
    for (int segments : counts) {
        const FoldedSeam f = make(segments, 15.0f);
        const CrossBoundarySeamMeasurement m =
            measureCrossBoundarySeam(f.lower, f.upper);
        EXPECT_TRUE(m.applicability == CrossSeamApplicability::Applicable);
        EXPECT_NEAR(m.maxDeg, 15.0, 0.5);
        results.push_back(m.maxDeg);
    }
    double lo = results[0];
    double hi = results[0];
    for (double v : results) {
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    EXPECT_TRUE((hi - lo) < 0.5);
}

PAE_TEST_CASE(legacy_band_tool_reads_faceting_not_the_cross_boundary_fold) {
    // Same geometry, two tools. The legacy tool sees the polygonal band
    // (360/segments); the new tool sees the real 30 deg fold. This is the
    // core evidence for B4 of the task.
    const int counts[] = {32, 128};
    for (int segments : counts) {
        const FoldedSeam f = make(segments, 30.0f);
        const SeamMeasurement legacy =
            measureShoulderRootSeam(f.combined, f.quads, f.crops);
        const CrossBoundarySeamMeasurement cross =
            measureCrossBoundarySeam(f.lower, f.upper);

        EXPECT_TRUE(legacy.applicability == SeamApplicability::Applicable);
        EXPECT_TRUE(legacy.windingVerified);
        EXPECT_NEAR(legacy.maxDeg, 360.0 / static_cast<double>(segments), 0.5);
        EXPECT_NEAR(cross.maxDeg, 30.0, 0.5);
        EXPECT_TRUE(std::fabs(legacy.maxDeg - cross.maxDeg) > 10.0);
    }
}

PAE_TEST_CASE(mesh_density_does_not_change_the_fold_angle) {
    const int rows[] = {3, 5, 8};
    for (int r : rows) {
        const FoldedSeam f = makeFoldedSeam(64, r, 5.0f);
        const CrossBoundarySeamMeasurement m =
            measureCrossBoundarySeam(f.lower, f.upper);
        EXPECT_TRUE(m.applicability == CrossSeamApplicability::Applicable);
        EXPECT_NEAR(m.maxDeg, 5.0, 0.5);
    }
}

PAE_TEST_CASE(reversed_upper_winding_is_detected_and_excluded) {
    const FoldedSeam f = makeFoldedSeam(64, 4, 10.0f, true);
    const CrossBoundarySeamMeasurement m =
        measureCrossBoundarySeam(f.lower, f.upper);
    EXPECT_TRUE(m.windingInconsistentPairs > 0);
    EXPECT_FALSE(m.windingVerified);
    EXPECT_TRUE(m.usableAngleSamples == 0);
    EXPECT_TRUE(m.applicability == CrossSeamApplicability::NotApplicable);
    EXPECT_TRUE(!m.notes.empty());
}

PAE_TEST_CASE(partially_flipped_patch_cannot_be_read_as_a_pass) {
    // Only ONE upper triangle pair is reversed. The remaining edges would
    // still produce a plausible angle, so applicability and the statistic
    // must be withheld as soon as any winding check fails.
    FoldedSeam f = make(64, 10.0f);
    std::swap(f.upper.mesh.indices[1], f.upper.mesh.indices[2]);
    std::swap(f.upper.mesh.indices[4], f.upper.mesh.indices[5]);
    const CrossBoundarySeamMeasurement m =
        measureCrossBoundarySeam(f.lower, f.upper);
    EXPECT_TRUE(m.windingInconsistentPairs > 0);
    EXPECT_FALSE(m.windingVerified);
    EXPECT_TRUE(m.applicability == CrossSeamApplicability::NotApplicable);
    EXPECT_TRUE(m.usableAngleSamples == 0);
    EXPECT_NEAR(m.maxDeg, 0.0, 1e-12);
}

PAE_TEST_CASE(offset_boundary_is_not_applicable_with_position_error_reported) {
    const FoldedSeam f = makeFoldedSeam(64, 4, 10.0f, false, 0.5f);
    const CrossBoundarySeamMeasurement m =
        measureCrossBoundarySeam(f.lower, f.upper);
    EXPECT_TRUE(m.unmatchedVertices > 0);
    EXPECT_TRUE(m.positionErrorMaxM > 1e-3);
    EXPECT_TRUE(m.applicability == CrossSeamApplicability::NotApplicable);
}

PAE_TEST_CASE(missing_second_patch_is_not_applicable) {
    const FoldedSeam f = make(32, 10.0f);
    SeamPatch empty;
    const CrossBoundarySeamMeasurement m =
        measureCrossBoundarySeam(f.lower, empty);
    EXPECT_TRUE(m.applicability == CrossSeamApplicability::NotApplicable);
    EXPECT_TRUE(m.usableAngleSamples == 0);
    EXPECT_TRUE(!m.notes.empty());
}

PAE_TEST_CASE(angle_domain_keeps_folds_above_ninety_degrees) {
    EXPECT_NEAR(crossSeamAngleDeg(Vec3{1.0f, 0.0f, 0.0f}, Vec3{1.0f, 0.0f, 0.0f}), 0.0, 1e-2);
    EXPECT_NEAR(crossSeamAngleDeg(Vec3{1.0f, 0.0f, 0.0f},
                                  Vec3{-0.5f, 0.0f, 0.8660254f}), 120.0, 1e-2);
    EXPECT_NEAR(crossSeamAngleDeg(Vec3{1.0f, 0.0f, 0.0f},
                                  Vec3{-1.0f, 0.0f, 0.0f}), 180.0, 1e-2);
}

int main() {
    return pae::test::runAllTests();
}
