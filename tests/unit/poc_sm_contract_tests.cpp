// PoC-SM (AUTH-08 AL-2): the CharacterDefinition five-layer contract.
//
// These tests check the CONTRACT, not the look of a body:
//   - every layer exists and is populated;
//   - every shape parameter has exactly ONE semantic, a range that contains
//     its default, and at least one affected region (that list is what makes
//     SPEC 13 #9 measurable);
//   - every joint deformation region stays inside the frozen BoneId 0-21
//     range;
//   - the measured anthropometric values survived the translation into the
//     contract unchanged.

#include "mini_test.hpp"

#include "procedural/shape/v2/CharacterDefinition.h"

#include <cmath>
#include <set>

using namespace pae;
using namespace pae::shape::v2;

PAE_TEST_CASE(contract_has_all_five_layers) {
    const CharacterDefinition def = defaultCharacterDefinition();

    // 1. ProportionProfile
    EXPECT_TRUE(def.proportions.statureM > 0.0f);

    // 2. AnatomicalLandmarks
    EXPECT_TRUE(!def.landmarks.catalogue.empty());

    // 3. AnatomicalRegions
    EXPECT_TRUE(!def.regions.regions.empty());
    EXPECT_TRUE(!def.regions.adjacency.empty());

    // 4. ShapeParameters
    EXPECT_TRUE(!def.parameters.declarations.empty());

    // 5. JointDeformationRegions
    EXPECT_TRUE(!def.jointDeformations.empty());
}

PAE_TEST_CASE(contract_stays_inside_the_frozen_boneid_range) {
    const CharacterDefinition def = defaultCharacterDefinition();
    for (const JointDeformationRegion& region : def.jointDeformations) {
        // BoneId 0-21 is frozen (ADR-011 F-1).
        EXPECT_TRUE(region.boneId <= 21);
    }
}

PAE_TEST_CASE(every_parameter_has_one_semantic_and_affected_regions) {
    const CharacterDefinition def = defaultCharacterDefinition();
    for (const ShapeParameterDecl& decl : def.parameters.declarations) {
        EXPECT_TRUE(!decl.name.empty());
        EXPECT_TRUE(!decl.semantic.empty());
        EXPECT_TRUE(!decl.unit.empty());
        // The range must contain the default, otherwise the default is
        // outside the contract it declares.
        EXPECT_TRUE(decl.minValue <= decl.defaultValue);
        EXPECT_TRUE(decl.defaultValue <= decl.maxValue);
        // An affected-region list is what makes the cross-region impact
        // measurement (SPEC 13 #9) possible.
        EXPECT_TRUE(!decl.affectedRegions.empty());
        for (RegionId region : decl.affectedRegions) {
            EXPECT_TRUE(def.regions.contains(region));
        }
    }
}

PAE_TEST_CASE(parameter_names_are_unique) {
    const CharacterDefinition def = defaultCharacterDefinition();
    std::set<std::string> names;
    for (const ShapeParameterDecl& decl : def.parameters.declarations) {
        EXPECT_TRUE(names.insert(decl.name).second);
    }
}

PAE_TEST_CASE(parameter_values_are_clamped_to_the_declared_range) {
    CharacterDefinition def = defaultCharacterDefinition();
    const ShapeParameterDecl* decl = def.parameters.find("waist-pinch");
    EXPECT_TRUE(decl != nullptr);
    if (decl == nullptr) return;

    def.parameters.setValue("waist-pinch", 5.0f);
    EXPECT_TRUE(def.parameters.value("waist-pinch") <= decl->maxValue + 1e-6f);

    def.parameters.setValue("waist-pinch", -5.0f);
    EXPECT_TRUE(def.parameters.value("waist-pinch") >= decl->minValue - 1e-6f);

    // Reading an unknown parameter must not throw and must not invent a value.
    EXPECT_NEAR(def.parameters.value("no-such-parameter", 42.0f), 42.0f, 1e-6);
}

PAE_TEST_CASE(measured_anthropometric_values_survive_the_translation) {
    const ProportionProfile p = defaultCharacterDefinition().proportions;

    // Stature: ANSUR II male 50th percentile, 1754.9 mm.
    EXPECT_NEAR(p.statureM, 1.7549, 1e-6);

    // Bideltoid shoulder breadth 509.0 mm / 1754.9 mm stature.
    EXPECT_NEAR(p.shoulderBreadthRatio, 0.290, 0.002);
    EXPECT_NEAR(p.shoulderBreadthM(), 0.509, 0.004);

    // Hip breadth 343.9 mm -> half width 0.17195 m.
    EXPECT_NEAR(0.5 * p.hipBreadthM(), 0.17195, 0.003);

    // Chest depth 253.0 mm -> half depth 0.1265 m.
    EXPECT_NEAR(p.chestHalfDepthM(), 0.1265, 0.003);
}

PAE_TEST_CASE(waist_half_axes_reproduce_the_measured_circumference) {
    const ProportionProfile p = defaultCharacterDefinition().proportions;

    float a = 0.0f;
    float b = 0.0f;
    p.waistHalfAxes(&a, &b);

    EXPECT_TRUE(a > 0.0f);
    EXPECT_TRUE(b > 0.0f);
    EXPECT_TRUE(a > b);   // the waist is wider than it is deep

    // Ramanujan perimeter of the solved ellipse must come back to the
    // MEASURED circumference (CDC NHANES SR50, male 20-29 median: 934 mm).
    const double pi = 3.14159265358979323846;
    const double perimeter = pi * (3.0 * (a + b) - std::sqrt((3.0 * a + b) * (a + 3.0 * b)));
    EXPECT_NEAR(perimeter, p.waistCircumferenceM(), 0.002);
}

PAE_TEST_CASE(region_adjacency_is_symmetric) {
    const CharacterDefinition def = defaultCharacterDefinition();
    for (const RegionAdjacency& adj : def.regions.adjacency) {
        EXPECT_TRUE(def.regions.contains(adj.a));
        EXPECT_TRUE(def.regions.contains(adj.b));
        // The lookup must find an adjacency whichever way round it is asked.
        EXPECT_TRUE(def.regions.adjacencyBetween(adj.a, adj.b) != nullptr);
        EXPECT_TRUE(def.regions.adjacencyBetween(adj.b, adj.a) != nullptr);
    }
}

PAE_TEST_CASE(landmark_catalogue_uses_international_anatomical_names) {
    const std::vector<LandmarkDecl>& catalogue = standardLandmarkCatalogue();
    EXPECT_TRUE(catalogue.size() >= 10);
    std::set<std::string> names;
    for (const LandmarkDecl& decl : catalogue) {
        EXPECT_TRUE(names.insert(decl.name).second);
        // Lower-case, hyphenated anatomical names.
        for (char c : decl.name) {
            EXPECT_TRUE(c == '-' || (c >= 'a' && c <= 'z'));
        }
    }
}

int main() {
    return pae::test::runAllTests();
}
