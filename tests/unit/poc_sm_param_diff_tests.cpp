// PoC-SM round 2 (AL-6): complete parameter differential test.
//
// Every declared ShapeParameter and every ProportionProfile field is perturbed
// ONE AT A TIME against a default baseline. The tests assert the ACTUAL
// geometry effect, not the declaration. A "Wired" declaration with zero vertex
// delta is a bug; an unused field must be reported honestly as zero.

#include "mini_test.hpp"

#include "poc_sm_param_diff.h"

#include <string>
#include <vector>

using namespace pae;
using namespace pae::shape;
using namespace pae::shape::v2;
using namespace pae::shape::v2::param_diff;

namespace {

const ShapeResult* findShape(const std::vector<ShapeResult>& v, const std::string& name) {
    for (const ShapeResult& r : v) {
        if (r.name == name) return &r;
    }
    return nullptr;
}

const ProportionResult* findProp(const std::vector<ProportionResult>& v,
                                 const std::string& name) {
    for (const ProportionResult& r : v) {
        if (r.field == name) return &r;
    }
    return nullptr;
}

} // namespace

PAE_TEST_CASE(baseline_is_reproducible) {
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult a = build(def);
    const SharedSkinEnvelopeResult b = build(def);
    EXPECT_TRUE(a.valid && b.valid);
    const Delta d = compare(a.mesh, b.mesh, a);
    EXPECT_TRUE(d.maxM < 1e-12);
    EXPECT_TRUE(d.comparedVertices > 100);
}

PAE_TEST_CASE(every_wired_shape_parameter_actually_moves_vertices) {
    // This is the core AL-6 requirement: a Wired declaration must be backed
    // by a measurable single-parameter mesh delta. Testing only one parameter
    // is not sufficient.
    const std::vector<ShapeResult> results = scanShapeParameters();
    EXPECT_TRUE(results.size() == 10);
    for (const ShapeResult& r : results) {
        if (r.declaredStatus != "Wired") continue;
        EXPECT_TRUE(r.actuallyMovesGeometry);
        EXPECT_TRUE(r.delta.maxM > 1e-6);
        EXPECT_TRUE(r.delta.comparedVertices == 26 * 32 + 1);
    }
}

PAE_TEST_CASE(spine_curve_is_declared_only_and_does_not_reach_geometry) {
    const std::vector<ShapeResult> results = scanShapeParameters();
    const ShapeResult* r = findShape(results, "spine-curve");
    EXPECT_TRUE(r != nullptr);
    EXPECT_TRUE(r->declaredStatus == "DeclaredOnly");
    EXPECT_TRUE(r->delta.maxM < 1e-12);
    EXPECT_FALSE(r->actuallyMovesGeometry);
}

PAE_TEST_CASE(all_ten_shape_parameters_are_enumerated) {
    const std::vector<ShapeResult> results = scanShapeParameters();
    const char* names[] = {
        "thorax-ribcage-volume", "thorax-depth-ratio", "abdomen-volume",
        "waist-pinch", "pelvis-volume", "shoulder-girdle-mass",
        "shoulder-girdle-height", "axilla-depth", "stylization", "spine-curve"
    };
    for (const char* name : names) EXPECT_TRUE(findShape(results, name) != nullptr);
}

PAE_TEST_CASE(shoulder_girdle_mass_changes_the_shoulder_band) {
    const std::vector<ShapeResult> results = scanShapeParameters();
    const ShapeResult* r = findShape(results, "shoulder-girdle-mass");
    EXPECT_TRUE(r != nullptr);
    EXPECT_TRUE(r->delta.bandMaxM[static_cast<size_t>(Band::ShoulderGirdle)] > 1e-6);
    EXPECT_TRUE(r->delta.bandVertexCount[static_cast<size_t>(Band::ShoulderGirdle)] > 0);
}

PAE_TEST_CASE(axilla_depth_changes_the_upper_band_not_the_pelvis) {
    const std::vector<ShapeResult> results = scanShapeParameters();
    const ShapeResult* r = findShape(results, "axilla-depth");
    EXPECT_TRUE(r != nullptr);
    EXPECT_TRUE(r->delta.bandMaxM[static_cast<size_t>(Band::ShoulderGirdle)] > 1e-6);
    EXPECT_NEAR(r->delta.bandMaxM[static_cast<size_t>(Band::Pelvis)], 0.0, 1e-9);
}

PAE_TEST_CASE(wired_proportion_fields_move_vertices) {
    const std::vector<ProportionResult> results = scanProportions();
    const char* wired[] = {
        "statureM", "hipBreadthRatio", "chestDepthRatio",
        "waistCircumferenceRatio", "waistAspectRatio"
    };
    for (const char* name : wired) {
        const ProportionResult* r = findProp(results, name);
        EXPECT_TRUE(r != nullptr);
        EXPECT_TRUE(r->actuallyMovesGeometry);
        EXPECT_TRUE(r->delta.maxM > 1e-6);
    }
}

PAE_TEST_CASE(measurement_fields_without_a_safe_mapping_do_not_move_vertices) {
    // shoulder breadth / biacromial breadth / abdominal depth are MEASURED in
    // the anthropometric sources but are NOT consumed by the generator.
    // Asserting zero prevents a report from claiming they are wired.
    const std::vector<ProportionResult> results = scanProportions();
    const char* unused[] = {
        "shoulderBreadthRatio", "biacromialRatio", "abdominalDepthRatio", "headHeightRatio"
    };
    for (const char* name : unused) {
        const ProportionResult* r = findProp(results, name);
        EXPECT_TRUE(r != nullptr);
        EXPECT_TRUE(r->delta.maxM < 1e-12);
        EXPECT_FALSE(r->actuallyMovesGeometry);
    }
}

PAE_TEST_CASE(single_parameter_change_does_not_mutate_other_parameters) {
    CharacterDefinition changed = defaultCharacterDefinition();
    const float expected = changed.parameters.value("pelvis-volume", -1.0f);
    changed.parameters.setValue("shoulder-girdle-mass", 1.15f);
    EXPECT_NEAR(changed.parameters.value("pelvis-volume", -1.0f), expected, 1e-12);
    EXPECT_NEAR(changed.parameters.value("shoulder-girdle-mass", -1.0f), 1.15f, 1e-6);
}

int main() {
    return pae::test::runAllTests();
}
