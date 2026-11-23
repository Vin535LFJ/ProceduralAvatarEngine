#pragma once

// G1 (AUTH-07): the three geometry routes under comparison.
//
//   R1 - existing SDF + Surface Nets (reference route, NO fixed topology)
//   R2 - fixed quad template projected onto the anatomy SDF isosurface
//   R3 - fixed quad template evaluated on a cubic B-spline control lattice
//
// All three consume the SAME ShapeModel contract, so a parameter change must
// move every route in a comparable way (SPEC 8.2 comparison protocol).
//
// FAIRNESS CONTRACT (SPEC 8.2 "same landmarks / same reference surface"):
// the three routes MUST be built on the SAME landmark skeleton and MUST be
// measured against the SAME reference samples. The route entry points
// therefore take NO joints argument - they all derive the reference skeleton
// internally via referenceJoints(), so it is impossible to compare two routes
// that sit on different skeletons.

#include "procedural/MeshTopology.h"
#include "procedural/humanoid/anatomy/HumanAnatomyGenerator.h"
#include "procedural/shape/LayerAMetrics.h"
#include "procedural/shape/QuadTemplate.h"
#include "procedural/shape/ShapeModel.h"
#include "procedural/shape/SkinWeights.h"
#include "procedural/shape/SurfaceMetrics.h"
#include "procedural/shape/TopologyAudit.h"

#include <array>
#include <string>
#include <vector>

namespace pae::shape {

enum class RouteId : int {
    R1 = 1,
    R2 = 2,
    R3 = 3
};

// The landmark skeleton shared by every route.
//
// This mirrors the derivation inside generateAnatomyUpperBody() - the
// reference surface is generated with THAT skeleton, so the template routes
// have to use it as well. routes_share_one_skeleton() asserts the mirror
// stays exact, so a change to the production derivation cannot silently
// desynchronise the PoC.
BodyJoints referenceJoints();

struct RouteResult {
    RouteId id = RouteId::R1;
    std::string name;
    bool feasible = false;
    std::string failureReason;
    BodyJoints joints;          // the skeleton this route was built on
    MeshData mesh;
    std::vector<std::array<uint32_t, 4>> quads;
    std::vector<CropBoundary> cropBoundaries;  // registered crop boundaries (T2)
    std::vector<CropBoundary> canonicalCrops;   // crops of the shared canonical mesh
    TopologyReport topology;
    TopologyAudit audit;         // Layer B topology audit (loops, faces, flow)

    // ---- G2: the three error layers (SPEC 8.2) ----
    // The route's own REFERENCE SURFACE, sampled with the common rule.
    std::vector<Vec3> referenceSamples;
    // The route resampled onto the SHARED fixed quad template.
    MeshData canonicalMesh;
    std::vector<std::array<uint32_t, 4>> canonicalQuads;
    SurfaceDistance e1;   // reference surface <-> target reference
    MeshDistance e2;      // reference surface -> canonical mesh
    MeshDistance e3;      // canonical mesh -> target reference

    // ---- G2: remaining Layer A / Layer B evidence ----
    SurfaceGrid referenceGrid;    // this route's reference surface, common grid
    SurfaceGrid targetGrid;       // the shared target reference, common grid
    SurfaceGrid canonicalGrid;    // the SHIPPED mesh, on the common grid
    LandmarkError landmarks;      // SPEC 13 #5
    NormalDeviation normals;      // SPEC 13 #7
    SectionDeviation sections;    // SPEC 13 #1, cross-section family
    SeamAudit seams;              // SPEC 13 #7 smooth/hard seams + T2 crop ring
    WeightLegality weights;       // SPEC 11.1
    CpuDeformationReport deformation;  // SPEC 11.1

    double generationMs = 0.0;
    double e1Mean = 0.0;   // target reference -> route surface (SPEC E1)
    double e1Max = 0.0;
    double e1Rms = 0.0;
    int targetSampleCount = 0;
    int vertexCount = 0;
    int triangleCount = 0;
    int quadCount = 0;
    double meanEdgeLength = 0.0;
};

// Target reference shared by every route: the anatomy SDF isosurface,
// restricted to the PoC-A torso region so that all routes are measured
// against exactly the same samples.
//
// The region is defined by the TORSO LOBE only: the upper arm is a separate
// lobe further out, and the PoC-A template covers the torso, so including the
// arm would measure the arm gap instead of template fidelity.
struct TargetReference {
    MeshData mesh;
    std::vector<Vec3> samples;   // coarse reference-mesh vertices in the torso lobe
    SurfaceGrid grid;            // the continuous field on the COMMON grid
    BodyJoints joints;
    std::function<float(Vec3)> field;  // the continuous SDF the samples come from
    bool valid = false;
    std::string error;
};

TargetReference buildTargetReference(const ShapeModel& model,
                                     const QuadTemplateSpec& spec,
                                     const anatomy::AnatomyResolution& resolution);

RouteResult runRouteR1(const ShapeModel& model,
                        const anatomy::AnatomyResolution& resolution);

RouteResult runRouteR2(const ShapeModel& model, const QuadTemplateSpec& spec);

RouteResult runRouteR3(const ShapeModel& model, const QuadTemplateSpec& spec);

// Layer A metric: distance from the shared target samples to the nearest
// vertex of the route surface. Measured per route, never shared.
void measureE1(const TargetReference& target, const MeshData& route, RouteResult& out);

// G2 (SPEC 8.2): measures the three error layers for ONE route against the
// shared target reference.
//
//   E1  route's reference surface  <-> target reference   (Layer A)
//   E2  route's reference surface  -> canonical mesh      (Layer B prereq)
//   E3  canonical mesh             -> target reference    (Layer B)
//
// The three layers are computed separately and must NOT be summed: E2 is the
// cost of the fixed-topology projection alone.
void measureRouteErrors(const TargetReference& target, const ShapeModel& model,
                        const QuadTemplateSpec& spec, RouteResult& route);

// The common surface sampling rule used by every route (SPEC 8.1 #2/#3).
SurfaceSamplingSpec commonSamplingSpec();

double maxVertexDelta(const MeshData& a, const MeshData& b);
double meanVertexDelta(const MeshData& a, const MeshData& b);

} // namespace pae::shape
