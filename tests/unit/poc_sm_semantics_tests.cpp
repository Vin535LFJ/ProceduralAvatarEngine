// PoC-SM round 2 (AL-4 / task D): Shape Model semantic contract checks.
//
// These tests pin the ACTUAL state of the five layers. In particular they
// assert that no spatial landmark positions exist yet: a declared catalogue
// entry with a confidence label is not a placed anatomical point.

#include "mini_test.hpp"

#include "procedural/shape/v2/CharacterDefinition.h"
#include "procedural/shape/v2/SharedSkinEnvelope.h"
#include "skeleton/Skeleton.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace pae;
using namespace pae::shape::v2;

namespace {

bool hasLandmark(const CharacterDefinition& def, const std::string& name) {
    for (const LandmarkDecl& d : def.landmarks.catalogue) {
        if (d.name == name) return true;
    }
    return false;
}

double meshDeltaMax(const MeshData& a, const MeshData& b) {
    const size_t n = std::min(a.vertices.size(), b.vertices.size());
    double max = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const Vec3 d = b.vertices[i].position - a.vertices[i].position;
        max = std::max(max, std::sqrt(static_cast<double>(d.lengthSq())));
    }
    return max;
}

SharedSkinEnvelopeResult buildWith(const BodyJoints& joints) {
    SharedSkinEnvelopeSpec spec;
    spec.rings = 26;
    spec.segments = 32;
    return buildSharedSkinEnvelope(defaultCharacterDefinition(), spec, joints);
}

} // namespace

// Mapping plan (NOT implemented here, does not touch BoneId ABI):
//   BodyJoints.hips  -> RegionId::Pelvis        / "iliac-crest"      (DERIVED, generator uses hips.y)
//   BodyJoints.spine -> RegionId::Abdomen       / "costal-margin"    (DERIVED, generator uses spine.y)
//   BodyJoints.chest -> RegionId::Thorax        / "sternal-angle"    (DERIVED, chest.y - 0.08)
//   BodyJoints.neck  -> RegionId::Neck          / "jugular-notch"    (UNDETERMINED)
//   left/rightShoulder -> ShoulderGirdle        / "acromion"         (UNDETERMINED)
// This table would live in host code and consume the existing RegionId and
// LandmarkDecl names. No BoneId 0-21, UBO[22] or 64B packing change is needed.
// Only the DERIVED rows above are backed by the current generator.

PAE_TEST_CASE(five_layers_are_present_in_the_default_definition) {
    const CharacterDefinition def = defaultCharacterDefinition();
    EXPECT_TRUE(def.proportions.statureM > 1.0f);
    EXPECT_TRUE(def.landmarks.catalogue.size() >= 9);
    EXPECT_TRUE(def.regions.regions.size() == 5);
    EXPECT_TRUE(def.parameters.declarations.size() == 10);
    EXPECT_TRUE(def.jointDeformations.size() == 4);
}

PAE_TEST_CASE(no_spatial_landmark_positions_are_placed) {
    // The current system is a declared CATALOGUE, not a placed Landmark system.
    const CharacterDefinition def = defaultCharacterDefinition();
    EXPECT_TRUE(def.landmarks.placed.empty());
    for (const LandmarkDecl& d : def.landmarks.catalogue) {
        EXPECT_FALSE(d.positioned);
    }
}

PAE_TEST_CASE(confidence_labels_do_not_create_an_anatomical_coordinate) {
    // Several declarations carry a Measured confidence label, but none has a
    // measured coordinate in this project. Today every spatial position is
    // UNDETERMINED. This test prevents a report from upgrading a label into a
    // placement.
    const CharacterDefinition def = defaultCharacterDefinition();
    for (const LandmarkDecl& d : def.landmarks.catalogue) {
        if (d.confidence == ValueConfidence::Measured) {
            EXPECT_FALSE(d.positioned);
        }
    }
}

PAE_TEST_CASE(first_batch_torso_landmarks_are_declared_by_name) {
    const CharacterDefinition def = defaultCharacterDefinition();
    const char* names[] = {
        "jugular-notch", "sternal-angle", "xiphoid-process", "costal-margin",
        "clavicle", "acromion", "iliac-crest",
        "anterior-superior-iliac-spine", "umbilicus"
    };
    for (const char* name : names) EXPECT_TRUE(hasLandmark(def, name));
}

PAE_TEST_CASE(regions_and_adjacency_are_consistent) {
    const CharacterDefinition def = defaultCharacterDefinition();
    EXPECT_TRUE(def.regions.contains(RegionId::Thorax));
    EXPECT_TRUE(def.regions.contains(RegionId::Abdomen));
    EXPECT_TRUE(def.regions.contains(RegionId::Pelvis));
    EXPECT_TRUE(def.regions.contains(RegionId::ShoulderGirdle));
    EXPECT_TRUE(def.regions.contains(RegionId::Axilla));
    for (const RegionAdjacency& a : def.regions.adjacency) {
        EXPECT_TRUE(a.sharedWeight >= 0.0f && a.sharedWeight <= 1.0f);
        EXPECT_TRUE(def.regions.adjacencyBetween(a.a, a.b) != nullptr);
        EXPECT_TRUE(def.regions.adjacencyBetween(a.b, a.a) != nullptr);
    }
}

PAE_TEST_CASE(joint_deformations_remain_an_interface_declaration) {
    const CharacterDefinition def = defaultCharacterDefinition();
    for (const JointDeformationRegion& j : def.jointDeformations) {
        EXPECT_TRUE(j.boneId <= 21);
        EXPECT_FALSE(j.hasCorrective);
        EXPECT_TRUE(j.volumePreserving);
        EXPECT_TRUE(j.influenceBandM > 0.0f);
    }
}

PAE_TEST_CASE(body_joints_is_a_separate_input_semantic_source) {
    // CharacterDefinition carries no joint coordinates: heights reach the
    // generator through BodyJoints, a separate input. Changing BodyJoints
    // moves the mesh without changing the definition.
    const CharacterDefinition defBefore = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult a = buildWith(BodyJoints{});
    BodyJoints moved;
    moved.chest.y += 0.08f;
    const SharedSkinEnvelopeResult b = buildWith(moved);
    EXPECT_TRUE(a.valid && b.valid);
    EXPECT_TRUE(meshDeltaMax(a.mesh, b.mesh) > 1e-4);

    const CharacterDefinition defAfter = defaultCharacterDefinition();
    EXPECT_TRUE(defAfter.landmarks.placed.empty());
    EXPECT_TRUE(defBefore.proportions.statureM == defAfter.proportions.statureM);
}

int main() {
    return pae::test::runAllTests();
}
