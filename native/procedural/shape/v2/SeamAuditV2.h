#pragma once

// PoC-SM (AUTH-08 AL-5): the shoulder-root seam measurement, implemented to
// the OQ-03 FIXED DEFINITION.
//
// The definition was fixed on 2026-11-24 (see
// docs/phases/anatomy-driven-geometry-poc/10-decision-records/
// 01-OQ-03-decision-seam-hard-condition.md). It is reproduced here verbatim
// because this module IS that definition - if the two ever disagree, this
// file is wrong.
//
//   OBJECT     the shoulder-root crop ring of the CANONICAL MESH
//              (crop label "shoulder-root", ringIndex = rings-1)
//   ANGLE      the angle between the FACE NORMALS of two adjacent quads,
//              domain [0 deg, 180 deg]. It is NOT folded to an acute angle:
//              above 90 deg means the surface folds back on itself and must
//              stay visible.
//   NORMALS    OUTWARD normals, determined by the face winding. The winding
//              consistency MUST be verified before the angles are used.
//   INCLUDED   the quads whose UPPER ROW lies on the crop ring (the
//              ringIndex-1 band). If a cap exists, its fan triangles are
//              included too and labelled separately.
//   EXCLUDED   quads that do not share an edge with the ring.
//   STATISTIC  max (the contract value is a per-point requirement, not an
//              average requirement).
//   OUTPUT     max, mean, p95, included quad count, angle sample count.
//   NA         if the route has no crop ring -> NOT APPLICABLE, which must
//              never be recorded as PASS.
//
// THIS IS A SEPARATE IMPLEMENTATION ON PURPOSE. The G2 code in
// shape/LayerAMetrics.cpp is frozen evidence and is NOT modified (AUTH-08
// FB-11). Its numbers were produced BEFORE this definition existed, so they
// must be recomputed under this definition before they can gate anything.
//
// ROUND-2 NOTE (does NOT amend the signed definition above): this statistic
// compares adjacent quads WITHIN the crop band, so it necessarily contains
// the ring's 360/segments polygonal faceting. It remains the historical
// band-faceting diagnostic. The "two sides of a seam" semantic in
// docs/foundation/03 is investigated by the separate EXPERIMENTAL module
// CrossBoundarySeamAudit.h. Nothing here redefines the signed OQ-03 decision;
// that is the owner's decision, not the implementation's.

#include "procedural/MeshData.h"
#include "procedural/shape/QuadTemplate.h"

#include <array>
#include <string>
#include <vector>

namespace pae {
namespace shape {
namespace v2 {

enum class SeamApplicability : int {
    // The crop ring was found and measured.
    Applicable = 0,
    // No crop ring: NOT APPLICABLE. This must never be reported as PASS.
    NotApplicable = 1
};

const char* seamApplicabilityName(SeamApplicability a);

struct SeamMeasurement {
    SeamApplicability applicability = SeamApplicability::NotApplicable;

    // What was measured.
    std::string cropLabel;
    int ringIndex = -1;
    int ringVertexCount = 0;

    // The included quads (upper row on the crop ring).
    int includedQuadCount = 0;
    // The angle samples actually used for the statistic.
    int angleSampleCount = 0;

    // The cap fan triangles: included per the definition and labelled
    // separately, so their contribution is never hidden inside the band
    // statistic.
    int capTriangleCount = 0;
    int capAngleSampleCount = 0;

    // Winding verification (required before the angles may be used).
    bool windingVerified = false;
    int windingInconsistencyCount = 0;

    // The band statistic - this is the one that gates against 5 deg.
    double maxDeg = 0.0;
    double meanDeg = 0.0;
    double p95Deg = 0.0;

    // The cap fan, reported separately.
    double capMaxDeg = 0.0;
    double capMeanDeg = 0.0;

    // max over band AND cap together, for the alternative reading of the
    // definition. Reported, not used for the gate.
    double maxDegIncludingCap = 0.0;

    double thresholdDeg = 5.0;
    bool passes = false;  // true ONLY when applicable AND maxDeg <= threshold

    std::vector<std::string> notes;
};

// Measures the shoulder-root seam of a canonical mesh.
//
// `quads` are the canonical grid quads. `crops` are the registered crop
// boundaries; the crop whose label matches `cropLabel` is measured.
SeamMeasurement measureShoulderRootSeam(const MeshData& mesh,
                                         const std::vector<std::array<uint32_t, 4>>& quads,
                                         const std::vector<CropBoundary>& crops,
                                         const std::string& cropLabel = "shoulder-root",
                                         double thresholdDeg = 5.0);

// The face normal of a quad, from its diagonals. The direction follows the
// vertex winding, which is what makes the winding check meaningful.
Vec3 seamQuadNormal(const MeshData& mesh, const std::array<uint32_t, 4>& quad);

// The angle between two face normals, in degrees, domain [0, 180].
double seamAngleDeg(const Vec3& n0, const Vec3& n1);

} // namespace v2
} // namespace shape
} // namespace pae
