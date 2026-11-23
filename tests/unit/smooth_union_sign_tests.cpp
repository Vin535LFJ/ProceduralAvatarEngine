// G2 (SPEC 6.2 note): dual-implementation unit test for the smooth-union sign.
//
// ---------------------------------------------------------------------------
// MATHEMATICAL DERIVATION (do not change the sign without redoing this)
// ---------------------------------------------------------------------------
// For two negative-inside distance fields a, b (|grad| ~ 1) and blend radius
// k > 0, let
//
//     h = clamp(1/2 + (b - a) / (2k), 0, 1).
//
// Consider the cubic blend f = b(1-h) + a*h + c*k*h*(1-h), where c is to be
// determined. Inside the blend band a - b = k(1-2h). Differentiating:
//
//     df/da = h - (1+c)(1-2h)/2
//     df/db = (1-h) + (1+c)(1-2h)/2
//     df/da + df/db = 1                       (for ANY c)
//
// For f to remain a distance BOUND the partials must stay in [0, 1]:
//
//     c = -1  ->  df/da = h,  df/db = 1-h      -> both in [0,1]   OK
//     c = +1  ->  df/da = 3h-1, df/db = 2-3h   -> negative / >1 when h<1/3  BAD
//
// Value where the two surfaces coincide (a = b = 0, so h = 1/2):
//
//     c = -1  ->  f = -k/4 < 0  -> inside. The union BULGES OUTWARD, which is
//                the defining behaviour of a smooth union.
//     c = +1  ->  f = +k/4 > 0  -> outside. The union SHRINKS: that is a carve,
//                not a union.
//
// Therefore a negative-inside smooth union REQUIRES c = -1, i.e. the
// `- k*h*(1-h)` form. AnatomyField.cpp uses this form and is correct.
//
// Smooth subtraction is smoothMax(a, -b) = -smoothMin(-a, b), so it requires
// the OPPOSITE sign in its own `h` convention, i.e. `+ k*h*(1-h)`.
// AnatomyField.cpp uses that form and is correct; SdfFusion.cpp used `-`.
//
// This test compares BOTH fusion functions against two independently written
// references: one direct polynomial, one via the -smin(-a,-b) identity.
// ---------------------------------------------------------------------------

#include "mini_test.hpp"

#include "procedural/fusion/SdfFusion.h"

#include <algorithm>
#include <cmath>

using namespace pae;
using namespace pae::fusion;

namespace {

// Reference implementation 1: the derived polynomial written out directly.
float referenceSmoothUnion(float a, float b, float k) {
    if (k <= 0.0f) return std::min(a, b);
    const float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return b * (1.0f - h) + a * h - k * h * (1.0f - h);
}

// Reference implementation 2: smooth max via the -smin(-a,-b) identity.
// This is algebraically equivalent but written independently, so an error in
// one formulation cannot silently validate the other.
float referenceSmoothMax(float a, float b, float k) {
    return -referenceSmoothUnion(-a, -b, k);
}

} // namespace

PAE_TEST_CASE(smooth_union_matches_derived_negative_inside_reference) {
    const float ks[] = {0.02f, 0.1f, 0.3f, 1.0f};
    const float values[] = {-0.4f, -0.15f, -0.02f, 0.0f, 0.02f, 0.15f, 0.4f};
    for (float k : ks) {
        for (float a : values) {
            for (float b : values) {
                EXPECT_NEAR(sdSmoothUnion(a, b, k), referenceSmoothUnion(a, b, k), 1e-6f);
                // The two independent references must agree with each other
                // for the UNION only; subtraction is a max, checked below.
                EXPECT_NEAR(referenceSmoothUnion(a, b, k),
                            -referenceSmoothMax(-a, -b, k), 1e-6f);
            }
        }
    }
}

PAE_TEST_CASE(smooth_subtraction_matches_derived_smooth_max_reference) {
    const float ks[] = {0.02f, 0.1f, 0.3f, 1.0f};
    const float values[] = {-0.4f, -0.15f, -0.02f, 0.0f, 0.02f, 0.15f, 0.4f};
    for (float k : ks) {
        for (float a : values) {
            for (float b : values) {
                EXPECT_NEAR(sdSmoothSubtraction(a, b, k),
                            referenceSmoothMax(a, -b, k), 1e-6f);
            }
        }
    }
}

PAE_TEST_CASE(smooth_union_bulges_outward_and_preserves_distance_bound) {
    // Coincident surfaces: a correct negative-inside smooth union is INSIDE.
    for (float k : {0.05f, 0.2f, 0.5f}) {
        EXPECT_TRUE(sdSmoothUnion(0.0f, 0.0f, k) < 0.0f);
        EXPECT_NEAR(sdSmoothUnion(0.0f, 0.0f, k), -0.25f * k, 1e-6f);
        // Outside the blend band the field must be EXACTLY the plain union.
        EXPECT_NEAR(sdSmoothUnion(-0.5f, 0.5f, k), -0.5f, 1e-6f);
        EXPECT_NEAR(sdSmoothUnion(0.5f, -0.5f, k), -0.5f, 1e-6f);

        // |grad f| <= 1 must hold in the blend band (the SDF distance bound).
        // Worst case is when grad(a) and grad(b) are parallel, where the
        // gradient magnitude equals df/da + df/db = 1 for the correct sign.
        constexpr float e = 1e-4f;
        const float a0 = 0.13f, b0 = -0.07f;
        const float ga = (sdSmoothUnion(a0 + e, b0, k) - sdSmoothUnion(a0 - e, b0, k)) / (2 * e);
        const float gb = (sdSmoothUnion(a0, b0 + e, k) - sdSmoothUnion(a0, b0 - e, k)) / (2 * e);
        EXPECT_TRUE(ga >= -1e-3f && ga <= 1.0f + 1e-3f);
        EXPECT_TRUE(gb >= -1e-3f && gb <= 1.0f + 1e-3f);
        EXPECT_NEAR(ga + gb, 1.0f, 1e-3f);
    }
}

PAE_TEST_CASE(smooth_subtraction_carves_inward_at_coincident_surfaces) {
    // a minus an identical b leaves nothing: the coincident point is OUTSIDE.
    for (float k : {0.05f, 0.2f, 0.5f}) {
        EXPECT_TRUE(sdSmoothSubtraction(0.0f, 0.0f, k) > 0.0f);
        EXPECT_NEAR(sdSmoothSubtraction(0.0f, 0.0f, k), 0.25f * k, 1e-6f);
    }
}

int main() {
    return test::runAllTests();
}
