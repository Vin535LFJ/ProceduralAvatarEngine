#pragma once

// PoC-SM (AUTH-08 AL-3): the Shared Skin Envelope.
//
// ADR-011 (ACCEPTED) defines it as:
//
//   "Shared Skin Envelope: thorax B-spline guide + abdomen/pelvis implicit
//    volume field + shoulder-girdle shared field + concave constraint -> ONE
//    final outer surface. Regions keep their RegionId but do not each emit a
//    visible mesh."
//
// WHY THIS IS SHAPED THE WAY IT IS
//
// The G2 evidence says the shoulder-root seam measured 47.66 deg (R1/R2) and
// 16.13 deg (R3) against a 5 deg contract value, and the root cause was NOT
// mesh density: the routes each carried their own implicit body and their
// surfaces had to be JOINED somewhere. A join is a crease.
//
// So this implementation never joins anything. There is exactly ONE surface,
// described by ONE continuous cross-section family:
//
//   r(u, y) = R(y) * shapeFactor(u, y)
//
//   R(y)        a single INTERPOLATING NATURAL CUBIC SPLINE over the height
//               range (not a B-spline: see the note in the .cpp). The thorax,
//               abdomen/pelvis and shoulder-girdle region splines are sampled
//               into one shared control grid and combined with an EXACT
//               partition of unity of C2 quintic weights, so the resulting
//               profile is C2 by construction and there is no surface-blending
//               step that could create a crease.
//
//   shapeFactor the azimuthal modulation: a superellipse exponent (the
//               stylisation channel) and the axilla concave constraint.
//
// The result is measured, not asserted. The signed OQ-03 ring-band statistic
// (SeamAuditV2.h) reports the polygonal faceting of the shoulder-root band.
// A separate experimental cross-boundary tool (CrossBoundarySeamAudit.h)
// exists for the contract's "two sides of a seam" semantic. Neither tool
// judges a Gate (AUTH-08 FB-12), and the OQ-03 definition is unchanged.
//
// CURRENT CAPABILITIES (actual implementation, nothing more)
//
//   - interpolating cubic spline in the HEIGHT direction only;
//   - thorax / abdomen-pelvis / shoulder-girdle section profile control;
//   - exactly normalised region weights at every sampled height;
//   - superellipse cross-sections;
//   - concave axilla constraint (quartic falloff);
//   - ONE MeshData output, with vertex normals from its own triangles;
//   - an open shoulder-root attachment boundary (top crop ring).
//
// WHAT THIS IS NOT
//
//   - Not a replacement for the production geometry path. Host-only.
//   - Not a full three-dimensional human anatomical field: there is no
//     spatial landmark system, no anatomy volume field and no limb generation.
//   - Not the full body. This slice covers the torso plus the shoulder-root
//     attachment ring; the upper arm, forearm, neck and head are out of
//     scope (SPEC 10.1 T1/T2, ADR-011 F-5).
//   - Not a Gate pass. Producing a number is not judging it (AUTH-08 FB-12).

#include "procedural/MeshData.h"
#include "procedural/shape/QuadTemplate.h"   // CropBoundary (shared vocabulary)
#include "procedural/shape/v2/CharacterDefinition.h"
#include "skeleton/Skeleton.h"

#include <array>
#include <string>
#include <vector>

namespace pae {
namespace shape {
namespace v2 {

struct SharedSkinEnvelopeSpec {
    int rings = 26;      // vertical rows (same convention as QuadTemplateSpec)
    int segments = 32;   // quads around
    float yBottomOffset = -0.02f;  // metres relative to the hip joint
    float yTopOffset = 0.00f;      // metres relative to the chest joint
    // The shoulder-root crop ring is the TOP ring and is left open; it is the
    // explicit attachment boundary for a future upper arm.
    bool capBottom = true;
    bool capTop = false;
    std::string cropLabel = "shoulder-root";
};

struct SharedSkinEnvelopeResult {
    MeshData mesh;
    std::vector<std::array<uint32_t, 4>> quads;
    std::vector<CropBoundary> cropBoundaries;

    // The measured section anchors the envelope was built from, so a report
    // can show WHERE each number came from instead of only the result.
    std::vector<float> anchorHeights;
    std::vector<float> anchorHalfWidths;
    std::vector<float> anchorHalfDepths;

    bool valid = false;
    std::string error;
};

// Builds the shared envelope from the contract.
//
// `joints` supplies the anatomical heights; `definition` supplies the
// proportions, regions and shape parameters.
SharedSkinEnvelopeResult buildSharedSkinEnvelope(const CharacterDefinition& definition,
                                                 const SharedSkinEnvelopeSpec& spec,
                                                 const BodyJoints& joints);

// The section profile the envelope uses, exposed so the seam tool and the
// tests can query the SAME profile the mesh was built from.
struct EnvelopeProfile {
    float yBottom = 0.0f;
    float yTop = 0.0f;
    float centerZ = 0.0f;
    float halfWidth = 0.0f;
    float halfDepth = 0.0f;
    float superEllipseExponent = 2.0f;
    float axillaConcavity = 0.0f;  // metres of inward carve at the armpit
};

// Evaluates the profile at a height and an azimuth (u in [0,1)).
EnvelopeProfile evaluateProfile(const CharacterDefinition& definition,
                                 const BodyJoints& joints,
                                 const SharedSkinEnvelopeSpec& spec,
                                 float y, float u);

} // namespace v2
} // namespace shape
} // namespace pae
