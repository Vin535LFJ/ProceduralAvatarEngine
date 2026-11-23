// G1 (AUTH-07): PoC-A route feasibility + shape parameter contract tests.
//
// These tests answer two questions with measurements, not with comments:
//   1. Do R2 / R3 really keep a FIXED topology while the shape changes?
//   2. Does every shape parameter that claims to be "Wired" actually move the
//      geometry - and does every parameter labelled "DeclaredOnly" provably
//      do NOTHING?
//
// A parameter that is read by the field but geometrically masked at the
// template sampling points is reported as such (it is not silently counted as
// verified).

#include "mini_test.hpp"

#include "procedural/shape/PocRoutes.h"
#include "procedural/shape/QuadTemplate.h"
#include "procedural/shape/ShapeModel.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace pae;
using namespace pae::shape;

namespace {

QuadTemplateSpec coarseSpec() {
    QuadTemplateSpec spec;
    spec.rings = 10;
    spec.segments = 16;
    return spec;
}

anatomy::AnatomyResolution coarseResolution() {
    anatomy::AnatomyResolution resolution;
    resolution.x = 44;
    resolution.y = 66;
    resolution.z = 26;
    return resolution;
}

float perturbedValue(const ParameterMeta& meta) {
    const float span = meta.maxValue - meta.minValue;
    float value = meta.defaultValue + 0.08f * span;
    if (value > meta.maxValue) value = meta.defaultValue - 0.08f * span;
    if (value < meta.minValue) value = meta.defaultValue;
    return value;
}

// maxVertexDelta() returns -1.0 when the two meshes have a different vertex
// count: index-wise correspondence is then undefined. For R1 (SDF + Surface
// Nets) that is NOT an error, it IS the finding - R1 does not keep a fixed
// vertex set, which is exactly what R2/R3 exist to fix. The report must not
// present it as a measured distance.
std::string deltaText(double delta) {
    if (delta < 0.0) return "N/A(vertex set changed)";
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6f", delta);
    return std::string(buffer);
}

} // namespace

PAE_TEST_CASE(shape_parameter_contract_is_complete) {
    const std::vector<ParameterMeta>& table = parameterTable();
    EXPECT_TRUE(table.size() == 23);
    EXPECT_TRUE(wiredParameterCount() == 20);
    EXPECT_TRUE(declaredOnlyParameterCount() == 3);

    for (const ParameterMeta& meta : table) {
        EXPECT_TRUE(std::string(meta.name).size() > 0);
        EXPECT_TRUE(std::string(meta.unit).size() > 0);
        EXPECT_TRUE(std::string(meta.region).size() > 0);
        EXPECT_TRUE(meta.minValue < meta.maxValue);
        EXPECT_TRUE(meta.defaultValue >= meta.minValue);
        EXPECT_TRUE(meta.defaultValue <= meta.maxValue);
    }

    // Parameters that are declared but never read by the generation chain.
    EXPECT_FALSE(parameterIsWired("torsoHeight"));
    EXPECT_FALSE(parameterIsWired("upperArmLength"));
    EXPECT_FALSE(parameterIsWired("stylization"));
    // Parameters that are read by buildPrimitives / makeUpperBodyField.
    EXPECT_TRUE(parameterIsWired("chestWidth"));
    EXPECT_TRUE(parameterIsWired("waistWidth"));
    EXPECT_TRUE(parameterIsWired("pelvisWidth"));
    EXPECT_TRUE(parameterIsWired("shoulderWidth"));
    EXPECT_TRUE(parameterIsWired("smoothness"));
    EXPECT_TRUE(parameterIsWired("axillaDepth"));
}

PAE_TEST_CASE(shape_parameter_write_validates_range_and_name) {
    ShapeModel model;
    std::string error;

    EXPECT_FALSE(writeParameter(model, "does_not_exist", 1.0f, &error));
    EXPECT_TRUE(error.size() > 0);

    EXPECT_FALSE(writeParameter(model, "chestWidth", 99.0f, &error));
    EXPECT_TRUE(error.size() > 0);

    EXPECT_TRUE(writeParameter(model, "chestWidth", 1.1f, &error));
    EXPECT_NEAR(readParameter(model, "chestWidth"), 1.1f, 1e-6);

    bool found = false;
    readParameter(model, "does_not_exist", &found);
    EXPECT_FALSE(found);
}

PAE_TEST_CASE(route_r2_r3_topology_is_parameter_independent) {
    const QuadTemplateSpec spec = coarseSpec();

    ShapeModel a;
    ShapeModel b;
    EXPECT_TRUE(writeParameter(a, "chestWidth", 1.15f, nullptr));
    EXPECT_TRUE(writeParameter(a, "waistWidth", 0.88f, nullptr));
    EXPECT_TRUE(writeParameter(b, "chestWidth", 0.85f, nullptr));
    EXPECT_TRUE(writeParameter(b, "pelvisWidth", 1.12f, nullptr));

    const RouteResult r2a = runRouteR2(a, spec);
    const RouteResult r2b = runRouteR2(b, spec);
    const RouteResult r3a = runRouteR3(a, spec);
    const RouteResult r3b = runRouteR3(b, spec);

    EXPECT_TRUE(r2a.feasible);
    EXPECT_TRUE(r2b.feasible);
    EXPECT_TRUE(r3a.feasible);
    EXPECT_TRUE(r3b.feasible);

    // Fixed topology: identical vertex / quad / triangle counts.
    EXPECT_TRUE(r2a.vertexCount == r2b.vertexCount);
    EXPECT_TRUE(r2a.quadCount == r2b.quadCount);
    EXPECT_TRUE(r2a.triangleCount == r2b.triangleCount);
    EXPECT_TRUE(r3a.vertexCount == r3b.vertexCount);
    EXPECT_TRUE(r3a.quadCount == r3b.quadCount);
    EXPECT_TRUE(r3a.triangleCount == r3b.triangleCount);
    EXPECT_TRUE(r2a.vertexCount == r3a.vertexCount);
    EXPECT_TRUE(r2a.quadCount == r3a.quadCount);

    // But the SHAPE must change with the parameters.
    EXPECT_TRUE(maxVertexDelta(r2a.mesh, r2b.mesh) > 1e-4);
    EXPECT_TRUE(maxVertexDelta(r3a.mesh, r3b.mesh) > 1e-4);
}

PAE_TEST_CASE(routes_share_one_skeleton) {
    // FAIRNESS CONTRACT (SPEC 8.2): the three routes must be built on the same
    // landmark skeleton. generateAnatomyUpperBody() derives the joints from
    // the body parameters (chest y=1.254 for the 1.77 m default figure); the
    // hardcoded default BodyJoints{} is a DIFFERENT skeleton (chest y=1.350).
    // Comparing a template built on one against a reference built on the other
    // measures the skeleton offset, not the template.
    //
    // Regression guard for the defect this test was written for: R2/R3 were
    // originally built on BodyJoints{} while R1 used the derived skeleton.
    const QuadTemplateSpec spec = coarseSpec();
    const anatomy::AnatomyResolution resolution = coarseResolution();
    const ShapeModel model;

    const RouteResult r1 = runRouteR1(model, resolution);
    const RouteResult r2 = runRouteR2(model, spec);
    const RouteResult r3 = runRouteR3(model, spec);
    const BodyJoints reference = referenceJoints();

    auto sameJoint = [](const Vec3& a, const Vec3& b) {
        return std::fabs(a.x - b.x) < 1e-6f && std::fabs(a.y - b.y) < 1e-6f &&
               std::fabs(a.z - b.z) < 1e-6f;
    };
    auto sameSkeleton = [&](const BodyJoints& a, const BodyJoints& b) {
        return sameJoint(a.hips, b.hips) && sameJoint(a.spine, b.spine) &&
               sameJoint(a.chest, b.chest) && sameJoint(a.leftShoulder, b.leftShoulder) &&
               sameJoint(a.rightShoulder, b.rightShoulder) && sameJoint(a.leftElbow, b.leftElbow) &&
               sameJoint(a.leftWrist, b.leftWrist);
    };

    EXPECT_TRUE(sameSkeleton(r1.joints, reference));
    EXPECT_TRUE(sameSkeleton(r2.joints, reference));
    EXPECT_TRUE(sameSkeleton(r3.joints, reference));

    // The hardcoded default skeleton must NOT be used - that was the defect.
    const BodyJoints hardcoded{};
    EXPECT_FALSE(sameSkeleton(reference, hardcoded));

    std::fprintf(stderr, "  reference skeleton: hips y=%.3f chest y=%.3f shoulder x=%.3f elbow x=%.3f\n",
                 reference.hips.y, reference.chest.y, reference.leftShoulder.x, reference.leftElbow.x);
    std::fprintf(stderr, "  hardcoded BodyJoints{}: hips y=%.3f chest y=%.3f shoulder x=%.3f elbow x=%.3f (NOT used)\n",
                 hardcoded.hips.y, hardcoded.chest.y, hardcoded.leftShoulder.x, hardcoded.leftElbow.x);
}

PAE_TEST_CASE(route_r1_topology_is_not_fixed) {
    // This is the central G1 contrast. R1 (SDF + Surface Nets) re-derives its
    // vertex set from the isosurface every time, so a shape change alters the
    // vertex count and breaks any index-wise correspondence. R2/R3 keep a
    // template, so their vertex set is invariant under shape change.
    const QuadTemplateSpec spec = coarseSpec();
    const anatomy::AnatomyResolution resolution = coarseResolution();
    const ShapeModel baseline;

    ShapeModel mutated = baseline;
    EXPECT_TRUE(writeParameter(mutated, "chestWidth", 1.15f, nullptr));
    EXPECT_TRUE(writeParameter(mutated, "pelvisWidth", 1.12f, nullptr));

    const RouteResult r1a = runRouteR1(baseline, resolution);
    const RouteResult r1b = runRouteR1(mutated, resolution);
    const RouteResult r2a = runRouteR2(baseline, spec);
    const RouteResult r2b = runRouteR2(mutated, spec);
    const RouteResult r3a = runRouteR3(baseline, spec);
    const RouteResult r3b = runRouteR3(mutated, spec);

    // R1: the vertex set changes -> index-wise delta is undefined.
    EXPECT_FALSE(r1a.vertexCount == r1b.vertexCount);
    EXPECT_TRUE(maxVertexDelta(r1a.mesh, r1b.mesh) < 0.0);

    // R2/R3: the vertex set is invariant -> index-wise delta is well defined.
    EXPECT_TRUE(r2a.vertexCount == r2b.vertexCount);
    EXPECT_TRUE(r3a.vertexCount == r3b.vertexCount);
    EXPECT_TRUE(maxVertexDelta(r2a.mesh, r2b.mesh) >= 0.0);
    EXPECT_TRUE(maxVertexDelta(r3a.mesh, r3b.mesh) >= 0.0);

    std::fprintf(stderr, "  R1 verts %d -> %d (topology NOT fixed)\n", r1a.vertexCount,
                 r1b.vertexCount);
    std::fprintf(stderr, "  R2 verts %d -> %d, R3 verts %d -> %d (topology fixed)\n",
                 r2a.vertexCount, r2b.vertexCount, r3a.vertexCount, r3b.vertexCount);
}

PAE_TEST_CASE(route_topology_is_clean) {
    const QuadTemplateSpec spec = coarseSpec();
    const ShapeModel model;

    const RouteResult r2 = runRouteR2(model, spec);
    const RouteResult r3 = runRouteR3(model, spec);

    for (const RouteResult* route : {&r2, &r3}) {
        std::fprintf(stderr,
                     "  [topology] %s: boundaryEdges=%d registered=%d unregistered=%d "
                     "loops=%zu crops=%zu match=%d closed=%d\n",
                     route->name.c_str(), route->topology.boundaryEdges,
                     route->audit.registeredBoundaryEdges,
                     route->audit.unregisteredBoundaryEdges,
                     route->audit.boundaryLoops.size(), route->cropBoundaries.size(),
                     route->audit.cropsMatchLoops ? 1 : 0,
                     route->audit.allCropsClosed ? 1 : 0);
        EXPECT_TRUE(route->feasible);
        EXPECT_TRUE(route->topology.components == 1);
        EXPECT_TRUE(route->topology.nonManifoldEdges == 0);
        EXPECT_TRUE(route->topology.degenerateTriangles == 0);
        EXPECT_TRUE(route->topology.invalidNormals == 0);
        EXPECT_TRUE(route->quadCount == (spec.rings - 1) * spec.segments);

        // SPEC 13 #10: boundary edges are allowed ONLY where a crop boundary is
        // registered (the T2 shoulder root). An unregistered crack is a defect.
        EXPECT_TRUE(route->audit.unregisteredBoundaryEdges == 0);
        EXPECT_TRUE(route->audit.registeredBoundaryEdges == route->topology.boundaryEdges);
        EXPECT_TRUE(route->audit.cropsMatchLoops);
        EXPECT_TRUE(route->audit.allCropsClosed);
        EXPECT_TRUE(route->audit.clean);
    }
}

PAE_TEST_CASE(t2_shoulder_root_boundary_is_explicit) {
    // SPEC 10.1 T2: the shoulder root must carry an EXPLICIT crop boundary -
    // the junction topology between the shoulder root and the torso must be
    // continuous and free of separate islands. A separate shoulder ball or a
    // simple tube substitute is forbidden.
    const QuadTemplateSpec spec = coarseSpec();
    const ShapeModel model;

    const RouteResult r2 = runRouteR2(model, spec);
    const RouteResult r3 = runRouteR3(model, spec);

    for (const RouteResult* route : {&r2, &r3}) {
        // Exactly one registered crop, and it is the shoulder root.
        EXPECT_TRUE(route->cropBoundaries.size() == 1);
        EXPECT_TRUE(route->cropBoundaries[0].label == "shoulder-root");
        EXPECT_TRUE(route->cropBoundaries[0].ringIndex == spec.rings - 1);
        EXPECT_TRUE(route->cropBoundaries[0].vertexCount == spec.segments);
        EXPECT_TRUE(route->cropBoundaries[0].closed);

        // It maps 1:1 onto exactly one boundary loop, and that loop is a
        // single closed chain (every boundary vertex has degree 2).
        EXPECT_TRUE(route->audit.boundaryLoops.size() == 1);
        const BoundaryLoop& loop = route->audit.boundaryLoops[0];
        EXPECT_TRUE(loop.closed);
        EXPECT_TRUE(loop.vertexCount == spec.segments);
        EXPECT_TRUE(loop.edgeCount == spec.segments);

        // The boundary sits at the shoulder-root height (the chest landmark).
        const BodyJoints& joints = route->joints;
        EXPECT_TRUE(std::fabs(loop.centroid.y - joints.chest.y) < 0.05f);

        // No separate island: the whole template is one connected component.
        EXPECT_TRUE(route->topology.components == 1);

        // The boundary is a ring of the quad grid, not a fan cap: the loop
        // vertices are exactly the top template ring.
        EXPECT_TRUE(route->audit.registeredBoundaryEdges == spec.segments);

        std::fprintf(stderr,
                     "  T2 %s: crop y=%.3f loop verts=%d edges=%d closed=%d "
                     "radius=[%.3f..%.3f]\n",
                     route->name.c_str(), route->cropBoundaries[0].y, loop.vertexCount,
                     loop.edgeCount, loop.closed ? 1 : 0, loop.minRadius, loop.maxRadius);
    }
}

PAE_TEST_CASE(declared_only_parameters_have_zero_effect) {
    const QuadTemplateSpec spec = coarseSpec();
    const ShapeModel baseline;

    for (const ParameterMeta& meta : parameterTable()) {
        if (meta.status != ParameterStatus::DeclaredOnly) continue;
        ShapeModel mutated = baseline;
        EXPECT_TRUE(writeParameter(mutated, meta.name, perturbedValue(meta), nullptr));

        const RouteResult r2a = runRouteR2(baseline, spec);
        const RouteResult r2b = runRouteR2(mutated, spec);
        const RouteResult r3a = runRouteR3(baseline, spec);
        const RouteResult r3b = runRouteR3(mutated, spec);

        EXPECT_TRUE(r2a.feasible);
        EXPECT_TRUE(r2b.feasible);
        EXPECT_TRUE(r3a.feasible);
        EXPECT_TRUE(r3b.feasible);
        EXPECT_NEAR(maxVertexDelta(r2a.mesh, r2b.mesh), 0.0, 1e-9);
        EXPECT_NEAR(maxVertexDelta(r3a.mesh, r3b.mesh), 0.0, 1e-9);
        std::fprintf(stderr, "  [declared-only] %-18s R2 delta=%.9g R3 delta=%.9g\n",
                     meta.name, maxVertexDelta(r2a.mesh, r2b.mesh),
                     maxVertexDelta(r3a.mesh, r3b.mesh));
    }
}

PAE_TEST_CASE(wired_parameters_effect_is_measured) {
    const QuadTemplateSpec spec = coarseSpec();
    const anatomy::AnatomyResolution resolution = coarseResolution();
    const ShapeModel baseline;

    const RouteResult r1Base = runRouteR1(baseline, resolution);
    const RouteResult r2Base = runRouteR2(baseline, spec);
    const RouteResult r3Base = runRouteR3(baseline, spec);
    EXPECT_TRUE(r1Base.feasible);

    int wiredWithEffect = 0;
    int wiredMasked = 0;
    for (const ParameterMeta& meta : parameterTable()) {
        if (meta.status != ParameterStatus::Wired) continue;
        ShapeModel mutated = baseline;
        EXPECT_TRUE(writeParameter(mutated, meta.name, perturbedValue(meta), nullptr));

        const RouteResult r1 = runRouteR1(mutated, resolution);
        const RouteResult r2 = runRouteR2(mutated, spec);
        const RouteResult r3 = runRouteR3(mutated, spec);

        const double d1 = maxVertexDelta(r1Base.mesh, r1.mesh);
        const double d2 = maxVertexDelta(r2Base.mesh, r2.mesh);
        const double d3 = maxVertexDelta(r3Base.mesh, r3.mesh);
        const bool effective = d1 > 1e-5 || d2 > 1e-5 || d3 > 1e-5;
        if (effective) {
            ++wiredWithEffect;
        } else {
            ++wiredMasked;
        }
        std::fprintf(stderr, "  [wired] %-18s R1=%-24s R2=%-10s R3=%-10s %s\n", meta.name,
                     deltaText(d1).c_str(), deltaText(d2).c_str(), deltaText(d3).c_str(),
                     effective ? "EFFECTIVE" : "MASKED(read but not visible)");
    }
    std::fprintf(stderr, "  wired parameters with measurable effect: %d, masked: %d\n",
                 wiredWithEffect, wiredMasked);
    // The core proportion parameters must be effective on the reference route.
    EXPECT_TRUE(wiredWithEffect >= 10);
}

PAE_TEST_CASE(e1_is_measured_per_route) {
    const QuadTemplateSpec spec = coarseSpec();
    const anatomy::AnatomyResolution resolution = coarseResolution();
    const ShapeModel model;

    const TargetReference target = buildTargetReference(model, spec, resolution);
    EXPECT_TRUE(target.valid);
    EXPECT_TRUE(target.samples.size() >= 32);

    RouteResult r2 = runRouteR2(model, spec);
    RouteResult r3 = runRouteR3(model, spec);
    measureE1(target, r2.mesh, r2);
    measureE1(target, r3.mesh, r3);

    EXPECT_TRUE(r2.targetSampleCount > 0);
    EXPECT_TRUE(r3.targetSampleCount > 0);
    EXPECT_TRUE(std::isfinite(r2.e1Mean) && r2.e1Mean > 0.0);
    EXPECT_TRUE(std::isfinite(r3.e1Mean) && r3.e1Mean > 0.0);
    std::fprintf(stderr, "  E1 target samples=%d  R2 mean=%.5f max=%.5f  R3 mean=%.5f max=%.5f\n",
                 r2.targetSampleCount, r2.e1Mean, r2.e1Max, r3.e1Mean, r3.e1Max);
}

// ---------------------------------------------------------------------------
// G2 (GATE-03): Layer A metrics, §11.1 skinning contract, seam audit.
// ---------------------------------------------------------------------------

PAE_TEST_CASE(g2_layer_a_metrics_are_measured_not_assigned) {
    const QuadTemplateSpec spec = coarseSpec();
    const ShapeModel model;
    const TargetReference target = buildTargetReference(model, spec, coarseResolution());
    EXPECT_TRUE(target.valid);

    RouteResult r2 = runRouteR2(model, spec);
    RouteResult r3 = runRouteR3(model, spec);
    measureRouteErrors(target, model, spec, r2);
    measureRouteErrors(target, model, spec, r3);

    EXPECT_TRUE(r2.e1.valid && r2.e2.valid && r2.e3.valid);
    EXPECT_TRUE(r3.e1.valid && r3.e2.valid && r3.e3.valid);
    EXPECT_TRUE(r2.landmarks.entries.size() == 15);
    EXPECT_TRUE(r2.landmarks.distribution.count > 0);
    EXPECT_TRUE(r2.normals.compared > 0);
    EXPECT_TRUE(r2.sections.bands.size() == 10);
    EXPECT_TRUE(r2.sections.valid);
    // The attribution must be visible at the coarse resolution too: R3's
    // projection cost is low but its final shape error is much larger.
    EXPECT_TRUE(r3.e2.mean < r2.e2.mean);
    EXPECT_TRUE(r3.e3.mean > r2.e3.mean);
    std::fprintf(stderr, "  Layer A: landmarks=%d normals=%d sections=%zu | "
                 "R2 E2=%.5f E3=%.5f | R3 E2=%.5f E3=%.5f\n",
                 r2.landmarks.distribution.count, r2.normals.compared,
                 r2.sections.bands.size(), r2.e2.mean, r2.e3.mean,
                 r3.e2.mean, r3.e3.mean);
}

PAE_TEST_CASE(g2_weight_legality_and_cpu_deformation) {
    const QuadTemplateSpec spec = coarseSpec();
    const ShapeModel model;
    const TargetReference target = buildTargetReference(model, spec, coarseResolution());
    EXPECT_TRUE(target.valid);

    RouteResult r2 = runRouteR2(model, spec);
    RouteResult r3 = runRouteR3(model, spec);
    for (RouteResult* route : {&r2, &r3}) {
        measureRouteErrors(target, model, spec, *route);
        const WeightLegality& w = route->weights;
        EXPECT_TRUE(w.valid);
        EXPECT_TRUE(w.maxInfluences <= 4);
        EXPECT_TRUE(w.negativeWeightVertices == 0);
        EXPECT_TRUE(w.sumToleranceViolations == 0);
        EXPECT_TRUE(w.invalidBoneIdVertices == 0);
        EXPECT_TRUE(w.maxSumError <= 1e-3);

        const CpuDeformationReport& d = route->deformation;
        EXPECT_TRUE(d.valid);
        EXPECT_TRUE(d.nonFinitePositions == 0);
        EXPECT_TRUE(d.rigidVertexCount == BoneCount);
        EXPECT_TRUE(d.maxRigidError < 1e-6);
        EXPECT_TRUE(d.bindResidual < 1e-5);
        std::fprintf(stderr, "  %s: legal=%s rigid=%d maxRigid=%.2e bind=%.2e\n",
                     route->name.c_str(), w.valid ? "yes" : "NO",
                     d.rigidVertexCount, d.maxRigidError, d.bindResidual);
    }
}

PAE_TEST_CASE(g2_poc_weight_assigner_matches_production) {
    const QuadTemplateSpec spec = coarseSpec();
    const ShapeModel model;
    const RouteResult r1 = runRouteR1(model, coarseResolution());
    EXPECT_TRUE(r1.feasible);

    MeshData copy = r1.mesh;
    assignAnatomyWeights(copy, model.anatomy, r1.joints, buildHumanoidSkeleton(r1.joints));
    EXPECT_TRUE(copy.vertices.size() == r1.mesh.vertices.size());
    int mismatches = 0;
    for (size_t i = 0; i < copy.vertices.size(); ++i) {
        const Vertex& a = copy.vertices[i];
        const Vertex& b = r1.mesh.vertices[i];
        const U16Vec4& ja = a.joints;
        const U16Vec4& jb = b.joints;
        if (ja.x != jb.x || ja.y != jb.y || ja.z != jb.z || ja.w != jb.w) {
            ++mismatches;
            continue;
        }
        if (std::fabs(a.weights.x - b.weights.x) > 1e-5f ||
            std::fabs(a.weights.y - b.weights.y) > 1e-5f) {
            ++mismatches;
        }
    }
    EXPECT_TRUE(mismatches == 0);
    std::fprintf(stderr, "  PoC weight assigner vs production: %d mismatches / %zu vertices\n",
                 mismatches, copy.vertices.size());
}

PAE_TEST_CASE(g2_seam_audit_finds_registered_shoulder_root_ring) {
    const QuadTemplateSpec spec = coarseSpec();
    const ShapeModel model;
    const TargetReference target = buildTargetReference(model, spec, coarseResolution());
    EXPECT_TRUE(target.valid);
    RouteResult r2 = runRouteR2(model, spec);
    measureRouteErrors(target, model, spec, r2);
    EXPECT_TRUE(r2.feasible);
    EXPECT_TRUE(r2.seams.valid);
    EXPECT_TRUE(r2.seams.cropRingEdges == spec.segments);
    EXPECT_TRUE(r2.seams.interiorSeamEdges > 0);
    // The crop ring is measured, never silently assumed smooth.
    EXPECT_TRUE(r2.seams.cropRingMaxDihedralDeg >= 0.0);
    EXPECT_TRUE(r2.audit.clean);
    std::fprintf(stderr, "  Seam audit: interior=%d crop=%d crop max=%.2f deg smooth=%s\n",
                 r2.seams.interiorSeamEdges, r2.seams.cropRingEdges,
                 r2.seams.cropRingMaxDihedralDeg,
                 r2.seams.cropRingSmooth ? "yes" : "NO");
}

int main() {
    return pae::test::runAllTests();
}
