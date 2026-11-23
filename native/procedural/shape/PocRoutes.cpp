#include "procedural/shape/PocRoutes.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace pae::shape {
namespace {

double nowMs() {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

double meanEdgeLength(const MeshData& mesh) {
    if (mesh.indices.size() < 3) return 0.0;
    double total = 0.0;
    size_t edges = 0;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t idx[3] = {mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]};
        for (int e = 0; e < 3; ++e) {
            const Vec3& p = mesh.vertices[idx[e]].position;
            const Vec3& q = mesh.vertices[idx[(e + 1) % 3]].position;
            total += (q - p).length();
            ++edges;
        }
    }
    return edges ? total / static_cast<double>(edges) : 0.0;
}

constexpr float kPi = 3.14159265358979323846f;

// Tolerance for "is this mesh vertex inside the torso lobe". The reference
// mesh is a coarse discretisation of the continuous field, so its vertices sit
// roughly one cell (about 16 mm) inside or outside the true isosurface.
constexpr float kTorsoTolerance = 0.016f;

} // namespace

BodyJoints referenceJoints() {
    // Mirrors the landmark derivation inside generateAnatomyUpperBody().
    // Keep in sync - routes_share_one_skeleton() verifies the mirror against
    // the skeleton the reference generator actually used.
    const BodyParameters bodyParams{};
    const float estimatedHeight = bodyParams.upperLegLength + bodyParams.lowerLegLength +
                                  0.08f + bodyParams.torsoLength + bodyParams.neckLength +
                                  bodyParams.headHeight;
    const float scale = std::clamp(bodyParams.height / std::fmax(estimatedHeight, 0.1f), 0.5f, 1.8f);
    return deriveJoints(bodyParams, scale);
}

TargetReference buildTargetReference(const ShapeModel& model, const QuadTemplateSpec& spec,
                                     const anatomy::AnatomyResolution& resolution) {
    TargetReference out;
    const auto start = nowMs();
    const anatomy::AnatomyResult result =
        anatomy::generateAnatomyUpperBody(model.anatomy, BodyParameters{}, resolution);
    const double ms = nowMs() - start;
    if (!result.valid) {
        out.error = "target reference generation failed";
        return out;
    }
    out.mesh = result.mesh;
    out.joints = result.joints;

    const std::function<float(Vec3)> field =
        anatomy::makeUpperBodyField(model.anatomy, result.joints);
    out.field = field;

    // The common grid is built from the CONTINUOUS field, so it is free of
    // the coarse mesh's discretisation error. It is the reference for the
    // landmark / normal / section metrics; `samples` above remains the
    // coarse-mesh reference used for the E1 fidelity number.
    out.grid = sampleImplicitSurfaceGrid(field, result.joints, commonSamplingSpec());

    const float yBottom = result.joints.hips.y + spec.yBottomOffset;
    const float yTop = result.joints.chest.y + spec.yTopOffset;

    for (const Vertex& v : out.mesh.vertices) {
        const Vec3& p = v.position;
        if (p.y < yBottom - 0.01f || p.y > yTop + 0.01f) continue;

        // Keep only the torso lobe. The upper arm is a separate lobe further
        // out; the PoC-A template does not cover it, so arm samples would
        // measure the arm gap rather than the template fidelity.
        const float radius = std::hypot(p.x, p.z);
        if (radius < 1e-6f) {
            out.samples.push_back(p);
            continue;
        }
        float u = std::atan2(p.z, p.x) / (2.0f * kPi);
        if (u < 0.0f) u += 1.0f;
        const float torsoRadius = torsoRadiusAt(field, p.y, u);
        if (torsoRadius < 0.0f) continue;
        if (radius > torsoRadius + kTorsoTolerance) continue;

        out.samples.push_back(p);
    }
    if (out.samples.size() < 32) {
        out.error = "target reference region is empty (" +
                    std::to_string(out.samples.size()) + " samples)";
        return out;
    }
    out.valid = true;
    (void)ms;
    return out;
}

RouteResult runRouteR1(const ShapeModel& model,
                        const anatomy::AnatomyResolution& resolution) {
    RouteResult out;
    out.id = RouteId::R1;
    out.name = "R1 SDF + Surface Nets";
    const auto start = nowMs();
    const anatomy::AnatomyResult result =
        anatomy::generateAnatomyUpperBody(model.anatomy, BodyParameters{}, resolution);
    out.generationMs = nowMs() - start;
    if (!result.valid) {
        out.failureReason = "anatomy generator returned an invalid mesh";
        return out;
    }
    out.mesh = result.mesh;
    out.quads = result.surfaceQuads;
    out.joints = result.joints;
    out.topology = analyzeTopology(out.mesh);
    out.audit = auditTopology(out.mesh, out.cropBoundaries);
    out.audit.faces = measureFaceQuality(out.mesh, out.quads);
    out.audit.flow = measureEdgeFlow(out.mesh, out.quads);
    out.vertexCount = static_cast<int>(out.mesh.vertices.size());
    out.triangleCount = static_cast<int>(out.mesh.indices.size() / 3);
    out.quadCount = static_cast<int>(out.quads.size());
    out.meanEdgeLength = meanEdgeLength(out.mesh);
    out.feasible = out.vertexCount > 0 && out.triangleCount > 0;
    if (!out.feasible) out.failureReason = "R1 produced an empty mesh";
    return out;
}

RouteResult runRouteR2(const ShapeModel& model, const QuadTemplateSpec& spec) {
    RouteResult out;
    out.id = RouteId::R2;
    out.name = "R2 quad template projected on SDF";
    out.joints = referenceJoints();
    const auto start = nowMs();
    const TemplateMesh built =
        projectTemplateOntoField(spec, model.anatomy, out.joints);
    out.generationMs = nowMs() - start;
    if (!built.valid) {
        out.failureReason = built.error.empty() ? "R2 template projection failed" : built.error;
        return out;
    }
    out.mesh = built.mesh;
    out.quads = built.quads;
    out.cropBoundaries = built.cropBoundaries;
    // The template carries geometry only until here. Give it the SAME
    // semantic weights the production anatomy generator assigns, so the
    // canonical mesh can be checked for skinning-data legality (SPEC 11.1).
    assignAnatomyWeights(out.mesh, model.anatomy, out.joints,
                         buildHumanoidSkeleton(out.joints));
    out.topology = analyzeTopology(out.mesh);
    out.audit = auditTopology(out.mesh, out.cropBoundaries);
    out.audit.faces = measureFaceQuality(out.mesh, out.quads);
    out.audit.flow = measureEdgeFlow(out.mesh, out.quads);
    out.vertexCount = static_cast<int>(out.mesh.vertices.size());
    out.triangleCount = static_cast<int>(out.mesh.indices.size() / 3);
    out.quadCount = static_cast<int>(out.quads.size());
    out.meanEdgeLength = meanEdgeLength(out.mesh);
    out.feasible = out.vertexCount > 0 && out.triangleCount > 0;
    if (!out.feasible) out.failureReason = "R2 produced an empty mesh";
    return out;
}

RouteResult runRouteR3(const ShapeModel& model, const QuadTemplateSpec& spec) {
    RouteResult out;
    out.id = RouteId::R3;
    out.name = "R3 quad template on B-spline control lattice";
    out.joints = referenceJoints();
    const auto start = nowMs();
    const TemplateMesh built =
        buildControlSurfaceTemplate(spec, model.anatomy, out.joints);
    out.generationMs = nowMs() - start;
    if (!built.valid) {
        out.failureReason = built.error.empty() ? "R3 control surface failed" : built.error;
        return out;
    }
    out.mesh = built.mesh;
    out.quads = built.quads;
    out.cropBoundaries = built.cropBoundaries;
    // The template carries geometry only until here. Give it the SAME
    // semantic weights the production anatomy generator assigns, so the
    // canonical mesh can be checked for skinning-data legality (SPEC 11.1).
    assignAnatomyWeights(out.mesh, model.anatomy, out.joints,
                         buildHumanoidSkeleton(out.joints));
    out.topology = analyzeTopology(out.mesh);
    out.audit = auditTopology(out.mesh, out.cropBoundaries);
    out.audit.faces = measureFaceQuality(out.mesh, out.quads);
    out.audit.flow = measureEdgeFlow(out.mesh, out.quads);
    out.vertexCount = static_cast<int>(out.mesh.vertices.size());
    out.triangleCount = static_cast<int>(out.mesh.indices.size() / 3);
    out.quadCount = static_cast<int>(out.quads.size());
    out.meanEdgeLength = meanEdgeLength(out.mesh);
    out.feasible = out.vertexCount > 0 && out.triangleCount > 0;
    if (!out.feasible) out.failureReason = "R3 produced an empty mesh";
    return out;
}

void measureE1(const TargetReference& target, const MeshData& route, RouteResult& out) {
    if (!target.valid || route.vertices.empty()) return;
    const size_t maxSamples = 2000;
    const size_t stride = target.samples.size() > maxSamples
                              ? target.samples.size() / maxSamples
                              : 1;
    double sum = 0.0;
    double sumSq = 0.0;
    double worst = 0.0;
    size_t count = 0;
    for (size_t i = 0; i < target.samples.size(); i += stride) {
        const Vec3& p = target.samples[i];
        float best = std::numeric_limits<float>::max();
        for (const Vertex& v : route.vertices) {
            const float d = (v.position - p).lengthSq();
            if (d < best) best = d;
        }
        const float dist = std::sqrt(best);
        sum += dist;
        sumSq += static_cast<double>(dist) * static_cast<double>(dist);
        if (dist > worst) worst = dist;
        ++count;
    }
    if (!count) return;
    out.targetSampleCount = static_cast<int>(count);
    out.e1Mean = sum / static_cast<double>(count);
    out.e1Max = worst;
    out.e1Rms = std::sqrt(sumSq / static_cast<double>(count));
}

SurfaceSamplingSpec commonSamplingSpec() {
    SurfaceSamplingSpec spec;
    spec.rings = 52;
    spec.segments = 104;
    spec.yBottomOffset = -0.02f;
    spec.yTopOffset = 0.00f;
    return spec;
}

namespace {

// Samples a template mesh on the COMMON (height, azimuth) grid.
//
// The template stores vertices at ring*segments + segment, so a common-grid
// cell maps directly onto a template vertex. This lets the normal-field
// metric be measured on the mesh that would actually be SHIPPED, rather than
// on an idealised surface.
SurfaceGrid canonicalGridFromTemplate(const MeshData& mesh, const QuadTemplateSpec& spec,
                                      const BodyJoints& joints,
                                      const SurfaceSamplingSpec& sampling) {
    SurfaceGrid grid;
    grid.rings = sampling.rings;
    grid.segments = sampling.segments;
    grid.yBottom = joints.hips.y + sampling.yBottomOffset;
    grid.yTop = joints.chest.y + sampling.yTopOffset;
    const size_t cells = static_cast<size_t>(std::max(0, sampling.rings)) *
                         static_cast<size_t>(std::max(0, sampling.segments));
    grid.positions.assign(cells, Vec3{});
    grid.normals.assign(cells, Vec3{0.0f, 1.0f, 0.0f});
    grid.valid.assign(cells, 0);

    const int templateRings = spec.rings;
    const int templateSegments = spec.segments;
    if (templateRings <= 0 || templateSegments <= 0) return grid;

    for (int i = 0; i < sampling.rings; ++i) {
        const float v = sampling.rings > 1
                            ? static_cast<float>(i) / static_cast<float>(sampling.rings - 1)
                            : 0.0f;
        for (int j = 0; j < sampling.segments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(sampling.segments);
            const int tr = std::clamp(static_cast<int>(
                std::lround(v * static_cast<float>(templateRings - 1))), 0, templateRings - 1);
            int ts = static_cast<int>(std::lround(u * static_cast<float>(templateSegments)));
            ts %= templateSegments;
            if (ts < 0) ts += templateSegments;
            const size_t vertexIndex = static_cast<size_t>(tr * templateSegments + ts);
            if (vertexIndex >= mesh.vertices.size()) continue;

            const Vertex& vertex = mesh.vertices[vertexIndex];
            const size_t cell = static_cast<size_t>(grid.index(i, j));
            grid.positions[cell] = vertex.position;
            grid.normals[cell] = vertex.normal;
            grid.valid[cell] = 1;
        }
    }
    return grid;
}

std::vector<Vec3> flattenGrid(const SurfaceGrid& grid) {
    std::vector<Vec3> samples;
    samples.reserve(static_cast<size_t>(grid.rings) * static_cast<size_t>(grid.segments));
    for (int r = 0; r < grid.rings; ++r) {
        for (int s = 0; s < grid.segments; ++s) {
            if (!grid.isValid(r, s)) continue;
            samples.push_back(grid.positions[static_cast<size_t>(grid.index(r, s))]);
        }
    }
    return samples;
}

} // namespace

void measureRouteErrors(const TargetReference& target, const ShapeModel& model,
                        const QuadTemplateSpec& spec, RouteResult& route) {
    if (!target.valid) return;
    const SurfaceSamplingSpec sampling = commonSamplingSpec();

    // ---- The route's own reference surface, sampled with the common rule ----
    switch (route.id) {
    case RouteId::R1:
    case RouteId::R2: {
        // Both represent the same continuous SDF isosurface. R1 approximates
        // it with a discretised mesh, R2 projects a template onto it.
        route.referenceGrid = sampleImplicitSurfaceGrid(target.field, target.joints, sampling);
        break;
    }
    case RouteId::R3: {
        // R3's reference surface is its analytic B-spline. No explicit normal
        // function is supplied: central differences of the surface function
        // give the normal, which is then oriented away from the body axis.
        const ControlSurface surface = buildControlSurface(spec, model.anatomy, target.joints);
        route.referenceGrid = sampleParametricSurfaceGrid(
            [&surface](float v, float u) { return evaluateControlSurface(surface, v, u); },
            {}, target.joints, sampling);
        break;
    }
    }
    route.referenceSamples = flattenGrid(route.referenceGrid);
    route.targetGrid = target.grid;

    // ---- The route resampled onto the shared fixed quad template ----
    if (route.id == RouteId::R3) {
        // R3's canonical mesh IS its own template.
        route.canonicalMesh = route.mesh;
        route.canonicalQuads = route.quads;
        route.canonicalCrops = route.cropBoundaries;
    } else {
        // R1 and R2 share the same canonical mesh: the SDF projected onto the
        // template. R1 has no fixed topology of its own, so Layer B requires
        // it to be resampled - and resampling it lands exactly on R2.
        const TemplateMesh canonical = projectTemplateOntoField(spec, model.anatomy, target.joints);
        route.canonicalMesh = canonical.mesh;
        route.canonicalQuads = canonical.quads;
        route.canonicalCrops = canonical.cropBoundaries;
    }
    route.canonicalGrid = canonicalGridFromTemplate(route.canonicalMesh, spec,
                                                    target.joints, sampling);

    // ---- E1: reference surface <-> target reference (Layer A) ----
    route.e1 = measureSurfaceDistance(route.referenceSamples, target.samples);

    // ---- E2: reference surface -> canonical mesh (cost of the projection) ----
    route.e2 = measureMeshDistance(route.canonicalMesh, route.referenceSamples);

    // ---- E3: canonical mesh -> target reference (final shape error) ----
    route.e3 = measureMeshDistance(route.canonicalMesh, target.samples);

    // ---- Remaining Layer A metrics (SPEC 8.3 item 2/3/4) ----
    route.landmarks = measureLandmarkError(
        route.referenceGrid, target.grid,
        anatomicalLandmarkSet(target.joints, spec));
    route.normals = measureNormalDeviation(route.referenceGrid, target.grid);
    route.sections = measureSectionDeviation(route.referenceGrid, target.grid, 10);

    // ---- Layer B seam smoothness and the T2 shoulder-root crop ring ----
    route.seams = auditSeams(route.canonicalMesh, route.canonicalQuads, route.canonicalCrops);

    // ---- SPEC 11.1: legal weights + correct CPU-side deformation ----
    // The route's own mesh is the one that would be shipped; for R2/R3 it is
    // identical to the canonical mesh.
    route.weights = checkWeightLegality(route.mesh);
    route.deformation = verifyCpuDeformation(route.mesh, target.joints);
}

double maxVertexDelta(const MeshData& a, const MeshData& b) {
    if (a.vertices.size() != b.vertices.size() || a.vertices.empty()) return -1.0;
    double worst = 0.0;
    for (size_t i = 0; i < a.vertices.size(); ++i) {
        const double d = (a.vertices[i].position - b.vertices[i].position).length();
        if (d > worst) worst = d;
    }
    return worst;
}

double meanVertexDelta(const MeshData& a, const MeshData& b) {
    if (a.vertices.size() != b.vertices.size() || a.vertices.empty()) return -1.0;
    double total = 0.0;
    for (size_t i = 0; i < a.vertices.size(); ++i) {
        total += (a.vertices[i].position - b.vertices[i].position).length();
    }
    return total / static_cast<double>(a.vertices.size());
}

} // namespace pae::shape
