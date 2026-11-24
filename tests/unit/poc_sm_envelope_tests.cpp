// PoC-SM (AUTH-08 AL-3): the Shared Skin Envelope.
//
// These tests check the CONSTRUCTION, not the aesthetics:
//   - the envelope is ONE mesh with one registered crop and no crack;
//   - the profile is CONTINUOUS: no height shows a jump in the section;
//   - the surface is closed at the bottom and open only at the registered
//     shoulder-root crop;
//   - every face normal points OUTWARD (the winding is right), which is what
//     the seam measurement depends on;
//   - the section honours the measured anthropometric anchors.

#include "mini_test.hpp"

#include "procedural/shape/TopologyAudit.h"
#include "procedural/shape/v2/SharedSkinEnvelope.h"
#include "procedural/shape/v2/SeamAuditV2.h"
#include "skeleton/Skeleton.h"

#include <cmath>
#include <vector>

using namespace pae;
using namespace pae::shape;
using namespace pae::shape::v2;

namespace {

const BodyJoints& defaultJoints() {
    static const BodyJoints kJoints{};
    return kJoints;
}

SharedSkinEnvelopeResult buildDefault(const CharacterDefinition& def) {
    SharedSkinEnvelopeSpec spec;
    spec.rings = 26;
    spec.segments = 32;
    return buildSharedSkinEnvelope(def, spec, defaultJoints());
}

} // namespace

PAE_TEST_CASE(envelope_builds_and_registers_one_crop) {
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult result = buildDefault(def);

    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.error.empty());
    EXPECT_TRUE(result.cropBoundaries.size() == 1);
    EXPECT_TRUE(result.cropBoundaries[0].label == "shoulder-root");
    EXPECT_TRUE(result.cropBoundaries[0].closed);
    // The mesh must be a grid of quads.
    EXPECT_TRUE(result.quads.size() == static_cast<size_t>(25 * 32));
}

PAE_TEST_CASE(envelope_has_one_crop_and_no_unregistered_crack) {
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult result = buildDefault(def);

    const TopologyAudit audit = auditTopology(result.mesh, result.cropBoundaries);
    // The shoulder-root crop is the ONLY legitimate open boundary.
    EXPECT_TRUE(audit.unregisteredBoundaryEdges == 0);
    EXPECT_TRUE(audit.allCropsClosed);
    EXPECT_TRUE(audit.cropsMatchLoops);
    EXPECT_TRUE(audit.clean);
}

PAE_TEST_CASE(profile_is_continuous_in_height) {
    // The whole point of the shared envelope is that there is no join, so no
    // height may show a step in the section width.
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeSpec spec;

    const float yBottom = defaultJoints().hips.y + spec.yBottomOffset;
    const float yTop = defaultJoints().chest.y + spec.yTopOffset + 0.075f;

    float previous = 0.0f;
    bool first = true;
    float largestJump = 0.0f;
    for (int i = 0; i <= 400; ++i) {
        const float y = yBottom + (yTop - yBottom) * static_cast<float>(i) / 400.0f;
        const EnvelopeProfile p = evaluateProfile(def, defaultJoints(), spec, y, 0.0f);
        EXPECT_TRUE(p.halfWidth > 0.0f);
        EXPECT_TRUE(p.halfDepth > 0.0f);
        if (!first) {
            const float jump = std::fabs(p.halfWidth - previous);
            largestJump = std::max(largestJump, jump);
        }
        previous = p.halfWidth;
        first = false;
    }
    // 400 samples over ~0.6 m: a smooth profile moves at most a fraction of a
    // millimetre per sample. A join would show up as a visible step.
    EXPECT_TRUE(largestJump < 0.002f);
}

PAE_TEST_CASE(profile_matches_the_measured_anchors) {
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult result = buildDefault(def);

    const float yWaist = defaultJoints().hips.y + 0.12f;
    const float yChest = defaultJoints().chest.y;

    float waistWidth = 0.0f;
    float waistDepth = 0.0f;
    def.proportions.waistHalfAxes(&waistWidth, &waistDepth);

    // The waist section must be close to the ellipse solved from the
    // MEASURED waist circumference.
    bool foundWaist = false;
    bool foundChest = false;
    for (size_t i = 0; i < result.anchorHeights.size(); ++i) {
        if (std::fabs(result.anchorHeights[i] - yWaist) < 0.02f) {
            foundWaist = true;
            // The waist half width is within 15% of the solved value.
            EXPECT_TRUE(std::fabs(result.anchorHalfWidths[i] - waistWidth) <
                        0.15f * waistWidth);
        }
        if (std::fabs(result.anchorHeights[i] - yChest) < 0.01f) {
            foundChest = true;
            // The chest half depth is within 15% of the MEASURED chest depth.
            EXPECT_TRUE(std::fabs(result.anchorHalfDepths[i] - def.proportions.chestHalfDepthM()) <
                        0.15f * def.proportions.chestHalfDepthM());
        }
    }
    EXPECT_TRUE(foundWaist);
    EXPECT_TRUE(foundChest);
}

PAE_TEST_CASE(all_face_normals_point_outward) {
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult result = buildDefault(def);

    int inward = 0;
    int checked = 0;
    for (const std::array<uint32_t, 4>& quad : result.quads) {
        const Vec3 centroid{
            (result.mesh.vertices[quad[0]].position.x + result.mesh.vertices[quad[1]].position.x +
             result.mesh.vertices[quad[2]].position.x + result.mesh.vertices[quad[3]].position.x) / 4.0f,
            (result.mesh.vertices[quad[0]].position.y + result.mesh.vertices[quad[1]].position.y +
             result.mesh.vertices[quad[2]].position.y + result.mesh.vertices[quad[3]].position.y) / 4.0f,
            (result.mesh.vertices[quad[0]].position.z + result.mesh.vertices[quad[1]].position.z +
             result.mesh.vertices[quad[2]].position.z + result.mesh.vertices[quad[3]].position.z) / 4.0f};
        const Vec3 outward{centroid.x, 0.0f, centroid.z};
        if (outward.lengthSq() < 1e-8f) continue;
        const Vec3 n = seamQuadNormal(result.mesh, quad);
        ++checked;
        if (n.dot(outward) <= 0.0f) ++inward;
    }
    EXPECT_TRUE(checked > 0);
    EXPECT_TRUE(inward == 0);
}

PAE_TEST_CASE(bottom_cap_normals_point_down) {
    // The G2 template's bottom cap pointed INTO the body (OQ-16). The v2
    // envelope must not repeat that.
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult result = buildDefault(def);

    const int ringVertices = 32;
    const uint32_t pole = static_cast<uint32_t>(result.mesh.vertices.size() - 1);
    EXPECT_TRUE(pole > 0);

    int inward = 0;
    int checked = 0;
    for (size_t i = 0; i + 2 < result.mesh.indices.size(); i += 3) {
        const uint32_t a = result.mesh.indices[i];
        const uint32_t b = result.mesh.indices[i + 1];
        const uint32_t c = result.mesh.indices[i + 2];
        if (a != pole && b != pole && c != pole) continue;
        const Vec3& pa = result.mesh.vertices[a].position;
        const Vec3& pb = result.mesh.vertices[b].position;
        const Vec3& pc = result.mesh.vertices[c].position;
        const Vec3 n = (pb - pa).cross(pc - pa);
        ++checked;
        // The outward direction of a bottom cap is DOWN.
        if (n.y > 0.0f) ++inward;
    }
    EXPECT_TRUE(checked == ringVertices);
    EXPECT_TRUE(inward == 0);
}

PAE_TEST_CASE(seam_measurement_applies_to_the_envelope) {
    const CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult result = buildDefault(def);

    const SeamMeasurement m =
        measureShoulderRootSeam(result.mesh, result.quads, result.cropBoundaries);

    EXPECT_TRUE(m.applicability == SeamApplicability::Applicable);
    EXPECT_TRUE(m.windingVerified);
    EXPECT_TRUE(m.includedQuadCount == 32);
    // The report must carry a real number, not a placeholder.
    EXPECT_TRUE(m.maxDeg >= 0.0 && m.maxDeg <= 180.0);
    EXPECT_TRUE(m.meanDeg <= m.maxDeg + 1e-9);
    EXPECT_TRUE(m.p95Deg <= m.maxDeg + 1e-9);
}

PAE_TEST_CASE(shaping_parameter_changes_the_mesh) {
    // A parameter that is declared as Wired must actually move vertices;
    // otherwise it is DeclaredOnly in disguise (the G1 discipline).
    CharacterDefinition def = defaultCharacterDefinition();
    const SharedSkinEnvelopeResult baseline = buildDefault(def);

    CharacterDefinition wide = defaultCharacterDefinition();
    wide.parameters.setValue("shoulder-girdle-mass", 1.15f);
    const SharedSkinEnvelopeResult widened = buildDefault(wide);

    EXPECT_TRUE(baseline.valid);
    EXPECT_TRUE(widened.valid);
    EXPECT_TRUE(baseline.mesh.vertices.size() == widened.mesh.vertices.size());

    double maxDelta = 0.0;
    for (size_t i = 0; i < baseline.mesh.vertices.size(); ++i) {
        const Vec3 d = baseline.mesh.vertices[i].position - widened.mesh.vertices[i].position;
        maxDelta = std::max(maxDelta, std::sqrt(static_cast<double>(d.lengthSq())));
    }
    // Widening the shoulder girdle must move the mesh by a measurable amount.
    EXPECT_TRUE(maxDelta > 0.001);
}

int main() {
    return pae::test::runAllTests();
}
