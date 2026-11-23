#pragma once

// G2 (SPEC 8.3 Layer A / 13 #1, #5, #7): the remaining Layer A metrics.
//
// Layer A is about the REFERENCE SURFACE and is independent of how the surface
// is expressed topologically. E1 (Hausdorff / RMS to the target reference)
// is already implemented in SurfaceMetrics; this module adds the three
// measurements SPEC 8.3 Layer A still asks for:
//
//   2. anatomical landmark position error      (SPEC 13 #5)
//   3. contour / cross-section deviation       (SPEC 13 #1, front/side/45 family)
//   4. normal field consistency, smooth/hard   (SPEC 13 #7)
//
// plus the seam classification that SPEC 13 #7 requires: a seam is SMOOTH when
// the dihedral deflection across it is at most 5 degrees (the contract value
// from docs/foundation/03), and HARD above that.
//
// Every metric compares the SAME (height, azimuth) cell of two surfaces, so a
// difference can only come from the geometry - never from the sampling.

#include "procedural/shape/QuadTemplate.h"
#include "procedural/shape/SurfaceMetrics.h"

#include <array>
#include <string>
#include <vector>

namespace pae::shape {

// An anatomical surface landmark.
//
//   u = 0.00  -> +X, the character's LEFT side
//   u = 0.25  -> +Z, the FRONT (sternum)
//   u = 0.50  -> -X, the character's RIGHT side
//   u = 0.75  -> -Z, the BACK (scapula)
//
// The landmark set is fixed and identical for every route (SPEC 8.1 #6).
struct AnatomicalLandmark {
    std::string name;
    float y = 0.0f;  // world height, metres
    float u = 0.0f;  // azimuth in [0,1)
};

// The PoC-A landmark set, anchored to the shared landmark skeleton so that
// every route measures the same anatomical points.
std::vector<AnatomicalLandmark> anatomicalLandmarkSet(const BodyJoints& joints,
                                                      const QuadTemplateSpec& spec);

struct LandmarkErrorEntry {
    std::string name;
    double error = 0.0;  // Euclidean distance, metres
    bool valid = false;
};

struct LandmarkError {
    std::vector<LandmarkErrorEntry> entries;
    Distribution distribution;  // over the valid entries
    bool valid = false;
};

// Landmark error of one route against the target reference.
// Both grids must come from the SAME sampling spec.
LandmarkError measureLandmarkError(const SurfaceGrid& route,
                                   const SurfaceGrid& target,
                                   const std::vector<AnatomicalLandmark>& landmarks);

// Normal field consistency (SPEC 13 #7).
struct NormalDeviation {
    Distribution angleDeg;   // angle between route normal and reference normal
    int compared = 0;
    int flipped = 0;          // cells whose normals point the opposite way
    bool valid = false;
};

NormalDeviation measureNormalDeviation(const SurfaceGrid& route, const SurfaceGrid& target);

// Contour / cross-section deviation.
//
// The section at one height is the ring of the surface grid, so the deviation
// is measured per height band as the signed difference of the horizontal
// radius at matching azimuths.
struct SectionBand {
    float y = 0.0f;
    Distribution radialDeviation;  // signed, metres
    int samples = 0;
};

struct SectionDeviation {
    std::vector<SectionBand> bands;
    Distribution absoluteDeviation;  // |radial deviation| over all cells
    bool valid = false;
};

SectionDeviation measureSectionDeviation(const SurfaceGrid& route, const SurfaceGrid& target,
                                         int bandCount = 10);

// Seam classification (SPEC 13 #7 "smooth / hard seam").
struct SeamAudit {
    // Interior seams: quad edges shared by exactly two quads.
    int interiorSeamEdges = 0;
    int smoothSeamEdges = 0;
    int hardSeamEdges = 0;
    double meanDihedralDeg = 0.0;
    double p95DihedralDeg = 0.0;
    double maxDihedralDeg = 0.0;

    // The T2 shoulder-root crop ring specifically.
    int cropRingEdges = 0;
    double cropRingMeanDihedralDeg = 0.0;
    double cropRingMaxDihedralDeg = 0.0;
    double cropRingMeanRadius = 0.0;
    double cropRingRadiusSpread = 0.0;  // std-dev of the ring radius, metres
    double cropRingPlanarityMm = 0.0;   // max |y - mean y| of the ring, millimetres

    bool cropRingSmooth = false;        // max crop dihedral <= the smooth threshold
    bool valid = false;
};

// Audits the seams of the canonical quad grid.
//
// `quads` are index quadruples in the template layout:
//   quad(i, j) = { i*S + j, i*S + jn, (i+1)*S + jn, (i+1)*S + j }
// so the quads adjacent to crop ring R are those with i == R-1.
SeamAudit auditSeams(const MeshData& mesh,
                     const std::vector<std::array<uint32_t, 4>>& quads,
                     const std::vector<CropBoundary>& crops,
                     double smoothThresholdDeg = 5.0);

} // namespace pae::shape
