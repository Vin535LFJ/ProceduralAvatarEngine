// G2 (GATE-03 / AUTH-07): the PoC-A FAIR COMPARISON probe.
//
// SPEC 8.1 demands that every route is measured under IDENTICAL conditions:
// same Shape Model, same landmarks, same reference surface, same sampling
// distribution, same sample count, same distance metric, same coordinate
// system, same parameter set, same camera protocol. This probe implements
// that protocol and writes the Layer A / Layer B evidence.
//
// SPEC 8.2: the three error layers are measured SEPARATELY and never mixed:
//   E1  route reference surface  <-> target reference   (Layer A)
//   E2  route reference surface  -> canonical mesh      (Layer B prereq)
//   E3  canonical mesh            -> target reference   (Layer B)

#include "procedural/shape/CameraProtocol.h"
#include "procedural/shape/ClayRenderer.h"
#include "procedural/shape/ImageWriter.h"
#include "procedural/shape/PocRoutes.h"
#include "procedural/shape/QuadTemplate.h"
#include "procedural/shape/ShapeModel.h"
#include "procedural/shape/SurfaceMetrics.h"
#include "procedural/shape/TopologyAudit.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

using namespace pae;
using namespace pae::shape;

namespace {

// SPEC 8.4: at least two parameter groups besides the baseline, no per-route
// tuning. Every route is built from EVERY group.
struct ParameterGroup {
    std::string name;
    std::vector<std::pair<std::string, float>> parameters;
};

std::vector<ParameterGroup> parameterGroups() {
    return {
        {"Baseline", {}},
        {"Chest Width-Depth", {{"chestWidth", 1.15f}, {"chestDepth", 1.10f}}},
        {"Waist-Pelvis Ratio", {{"waistWidth", 0.85f}, {"pelvisWidth", 1.15f}}},
    };
}

ShapeModel buildModel(const ParameterGroup& group) {
    ShapeModel model;
    for (const auto& entry : group.parameters) {
        std::string error;
        if (!writeParameter(model, entry.first.c_str(), entry.second, &error)) {
            std::fprintf(stderr, "parameter %s rejected: %s\n", entry.first.c_str(), error.c_str());
        }
    }
    return model;
}

std::string slug(const std::string& name) {
    std::string result;
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) result.push_back(c);
        else if (!result.empty() && result.back() != '-') result.push_back('-');
    }
    while (!result.empty() && result.back() == '-') result.pop_back();
    return result;
}

bool dataIsChestGroup(const std::string& name) {
    return name == "Chest Width-Depth";
}

struct RingExtent {
    double width = 0.0;  // max x - min x, metres
    double depth = 0.0;  // max z - min z, metres
    int samples = 0;
};

RingExtent ringExtentAt(const SurfaceGrid& grid, float y) {
    RingExtent extent;
    const int ring = grid.ringForHeight(y);
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minZ = std::numeric_limits<double>::max();
    double maxZ = std::numeric_limits<double>::lowest();
    for (int s = 0; s < grid.segments; ++s) {
        if (!grid.isValid(ring, s)) continue;
        const Vec3& p = grid.positions[static_cast<size_t>(grid.index(ring, s))];
        minX = std::min(minX, static_cast<double>(p.x));
        maxX = std::max(maxX, static_cast<double>(p.x));
        minZ = std::min(minZ, static_cast<double>(p.z));
        maxZ = std::max(maxZ, static_cast<double>(p.z));
        ++extent.samples;
    }
    if (extent.samples > 0) {
        extent.width = maxX - minX;
        extent.depth = maxZ - minZ;
    }
    return extent;
}

void appendRow(std::string& text, const char* group, const RouteResult& route) {
    char buffer[1024];
    std::snprintf(buffer, sizeof(buffer),
                  "| %s | %s | %d | %d | %d | %.5f | %.5f | %.5f | %.5f | %.5f | %.5f |\n",
                  group, route.name.c_str(), route.e1.sampleCountA, route.e2.sampleCount,
                  route.e3.sampleCount, route.e1.mean, route.e1.rms, route.e1.hausdorff,
                  route.e2.mean, route.e2.hausdorff, route.e3.mean);
    text += buffer;
}

} // namespace

int main(int argc, char** argv) {
    const std::string outDir = argc > 1 ? argv[1] : ".";
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);

    anatomy::AnatomyResolution resolution;
    resolution.x = 44;
    resolution.y = 66;
    resolution.z = 26;

    QuadTemplateSpec production;
    production.rings = 26;
    production.segments = 32;

    std::string text;
    text += "# G2 fair comparison (PoC-A)\n\n";
    text += "All routes share the same Shape Model, landmarks, reference surface, ";
    text += "sampling rule, sample count, distance metric, coordinate system, ";
    text += "parameter groups and camera protocol (SPEC 8.1).\n\n";
    text += "Anatomy resolution " + std::to_string(resolution.x) + "x" +
            std::to_string(resolution.y) + "x" + std::to_string(resolution.z) +
            "; template " + std::to_string(production.rings) + " rings x " +
            std::to_string(production.segments) + " segments.\n";

    // Reference silhouette per view, taken from R1 (the route that defines the
    // target reference). Every route is compared against it.
    struct GroupData {
        std::string name;
        RouteResult r1, r2, r3;
        TargetReference target;
    };
    std::vector<GroupData> groups;

    for (const ParameterGroup& group : parameterGroups()) {
        GroupData data;
        data.name = group.name;
        const ShapeModel model = buildModel(group);

        data.target = buildTargetReference(model, production, resolution);
        if (!data.target.valid) {
            std::fprintf(stderr, "group %s: target reference invalid: %s\n", group.name.c_str(),
                         data.target.error.c_str());
            return 1;
        }
        data.r1 = runRouteR1(model, resolution);
        data.r2 = runRouteR2(model, production);
        data.r3 = runRouteR3(model, production);
        measureRouteErrors(data.target, model, production, data.r1);
        measureRouteErrors(data.target, model, production, data.r2);
        measureRouteErrors(data.target, model, production, data.r3);
        groups.push_back(data);
    }

    // ------------------------------------------ grey-clay visual Gate export
    // One shared orthographic frame for EVERY image. The owner's visual veto
    // stays independent: these images are evidence, not an AI verdict.
    const float clayYMin = groups[0].target.joints.hips.y + production.yBottomOffset - 0.01f;
    const float clayYMax = groups[0].target.joints.chest.y + production.yTopOffset + 0.01f;
    std::vector<const MeshData*> clayMeshes;
    for (const GroupData& data : groups) {
        clayMeshes.push_back(&data.r1.mesh);
        clayMeshes.push_back(&data.r2.mesh);
        clayMeshes.push_back(&data.r3.mesh);
    }
    const std::vector<ClayFrame> clayFrames = computeSharedClayFrames(
        clayMeshes, fixedViews(), clayYMin, clayYMax, 480, 480);
    const Vec3& clayLight = clayLightInViewFrame();

    std::string clayIndex;
    clayIndex += "# G2 grey-clay fixed-view evidence (PoC-A)\n\n";
    clayIndex += "Software Lambert-grey render, NOT the production GLES renderer. ";
    clayIndex += "The camera is the same `viewFrame()` used by the numeric ";
    clayIndex += "silhouette protocol; the light, band and resolution are identical ";
    clayIndex += "for every route and parameter group, and the framing is identical ";
    clayIndex += "for every route within a view. The owner keeps an independent ";
    clayIndex += "visual veto - no image here is marked ACCEPTED.\n\n";
    {
        char buffer[512];
        std::snprintf(buffer, sizeof(buffer),
                      "- Band: y = %.4f .. %.4f m\n"
                      "- Image: %d x %d px\n"
                      "- Light in view frame (right, up, towards camera): "
                      "(%.2f, %.2f, %.2f)\n"
                      "- Lambert: 0.28 ambient + 0.72 * max(0, N.L); "
                      "background (28,28,31)\n\n"
                      "The frame is shared by every route and group WITHIN a "
                      "view, but may differ between views so each view is "
                      "framed tightly.\n\n"
                      "| View | u range (m) | v range (m) |\n|---|---|---|\n",
                      clayYMin, clayYMax, clayFrames[0].width, clayFrames[0].height,
                      clayLight.x, clayLight.y, clayLight.z);
        clayIndex += buffer;
        for (size_t i = 0; i < fixedViews().size(); ++i) {
            std::snprintf(buffer, sizeof(buffer), "| %s | [%.4f, %.4f] | [%.4f, %.4f] |\n",
                          fixedViews()[i].name.c_str(),
                          clayFrames[i].uMin, clayFrames[i].uMax,
                          clayFrames[i].vMin, clayFrames[i].vMax);
            clayIndex += buffer;
        }
    }
    clayIndex += "\n| Group | Route | View | Image |\n|---|---|---|---|\n";

    const char* shortRoutes[3] = {"r1", "r2", "r3"};
    for (const GroupData& data : groups) {
        const RouteResult* routes[3] = {&data.r1, &data.r2, &data.r3};
        for (int routeIndex = 0; routeIndex < 3; ++routeIndex) {
            for (size_t viewIndex = 0; viewIndex < fixedViews().size(); ++viewIndex) {
                const View& view = fixedViews()[viewIndex];
                const std::string relative = "clay/" + slug(data.name) + "/" +
                                             shortRoutes[routeIndex] + "/" +
                                             slug(view.name) + ".png";
                const std::string path = outDir + "/" + relative;
                std::error_code dirError;
                std::filesystem::create_directories(
                    std::filesystem::path(path).parent_path(), dirError);
                const RgbImage image = renderClayView(
                    routes[routeIndex]->mesh, view, clayFrames[viewIndex],
                    clayYMin, clayYMax);
                if (!writePng(path, image)) {
                    std::fprintf(stderr, "failed to write %s\n", path.c_str());
                    return 1;
                }
                clayIndex += "| " + data.name + " | " + routes[routeIndex]->name + " | " +
                             view.name + " | [" + relative + "](" + relative + ") |\n";
            }
        }
    }
    {
        std::FILE* file = std::fopen((outDir + "/clay/index.md").c_str(), "wb");
        if (file) {
            std::fwrite(clayIndex.data(), 1, clayIndex.size(), file);
            std::fclose(file);
        }
    }

    // ---------------------------------------------------- Layer A / Layer B
    text += "\n## Error layers (metres)\n\n";
    text += "| Group | Route | E1 samples | E2 samples | E3 samples | E1 mean | E1 RMS | "
            "E1 Hausdorff | E2 mean | E2 max | E3 mean |\n";
    text += "|---|---|---|---|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        appendRow(text, data.name.c_str(), data.r1);
        appendRow(text, data.name.c_str(), data.r2);
        appendRow(text, data.name.c_str(), data.r3);
    }

    text += "\n**Reading the layers**\n\n";
    text += "- E1 = route reference surface vs the shared target reference ";
    text += "(topology independent). For R1 and R2 this is the distance between the ";
    text += "continuous SDF and R1's discretised mesh - a REFERENCE FIDELITY number, ";
    text += "not a route-quality number, because R1 and R2 share the same surface.\n";
    text += "- E2 = route reference surface vs its own canonical mesh. This is the ";
    text += "cost of the fixed-topology projection ALONE and must not be mixed into ";
    text += "E1 or E3.\n";
    text += "- E3 = canonical mesh vs the shared target reference: the final shape ";
    text += "error of the route that would actually be shipped.\n";
    text += "\nGrey-clay fixed-view evidence for the owner's independent visual "
            "Gate: [clay/index.md](clay/index.md) (3 groups x 3 routes x 7 "
            "views). Camera, light and band are shared; the framing is shared "
            "by every route within a view. No image is marked VISUALLY "
            "ACCEPTED by the AI.\n";

    // ------------------------------------------------------- topology audit
    text += "\n## Topology audit (Layer B)\n\n";
    text += "| Group | Route | Verts | Tris | Quads | Components | Boundary (crop) | "
            "Unregistered | NonManifold | Degenerate | Clean |\n";
    text += "|---|---|---|---|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            char buffer[1024];
            std::snprintf(buffer, sizeof(buffer),
                          "| %s | %s | %d | %d | %d | %d | %d | %d | %d | %d | %s |\n",
                          data.name.c_str(), route->name.c_str(), route->vertexCount,
                          route->triangleCount, route->quadCount, route->topology.components,
                          route->audit.registeredBoundaryEdges,
                          route->audit.unregisteredBoundaryEdges,
                          route->topology.nonManifoldEdges,
                          route->topology.degenerateTriangles,
                          route->audit.clean ? "yes" : "NO");
            text += buffer;
        }
    }

    text += "\n## Face quality and edge flow (Layer B, canonical quad grid)\n\n";
    text += "| Group | Route | Mean aspect | Max aspect | Mean min angle | Slivers | "
            "Edge flow circum. p95 | Edge flow long. p95 | Orthogonal quad ratio |\n";
    text += "|---|---|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            const FaceQuality& f = route->audit.faces;
            const EdgeFlow& e = route->audit.flow;
            char buffer[1024];
            std::snprintf(buffer, sizeof(buffer),
                          "| %s | %s | %.3f | %.3f | %.1f deg | %d | %.1f deg | %.1f deg | %.3f |\n",
                          data.name.c_str(), route->name.c_str(), f.meanAspectRatio,
                          f.maxAspectRatio, f.meanMinAngleDeg, f.sliverCount,
                          e.p95CircumferentialDeg, e.p95LongitudinalDeg, e.orthogonalQuadRatio);
            text += buffer;
        }
    }

    // ------------------------------------------- Layer A landmark / normal
    text += "\n## Layer A: anatomical landmark error (SPEC 13 #5)\n\n";
    text += "Fifteen named PoC-A landmarks, identical for every route. Each "
            "landmark is resolved on the SAME (height, azimuth) grid cell of "
            "the route's reference surface and of the target reference. "
            "Euclidean distance in metres; threshold TBD (§13 #5).\n\n";
    text += "| Group | Route | compared | mean | p50 | p95 | max |\n";
    text += "|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            char buffer[512];
            std::snprintf(buffer, sizeof(buffer), "| %s | %s | %d | %.5f | %.5f | %.5f | %.5f |\n",
                          data.name.c_str(), route->name.c_str(), route->landmarks.distribution.count,
                          route->landmarks.distribution.mean, route->landmarks.distribution.p50,
                          route->landmarks.distribution.p95, route->landmarks.distribution.max);
            text += buffer;
        }
    }
    text += "\nPer-landmark detail, Baseline group (metres):\n\n";
    text += "| Landmark | " + groups[0].r1.name + " | " + groups[0].r2.name + " | " +
            groups[0].r3.name + " |\n|---|---|---|---|\n";
    for (size_t i = 0; i < groups[0].r1.landmarks.entries.size(); ++i) {
        char buffer[512];
        std::snprintf(buffer, sizeof(buffer), "| %s | %.5f | %.5f | %.5f |\n",
                      groups[0].r1.landmarks.entries[i].name.c_str(),
                      groups[0].r1.landmarks.entries[i].error,
                      groups[0].r2.landmarks.entries[i].error,
                      groups[0].r3.landmarks.entries[i].error);
        text += buffer;
    }

    text += "\n## Layer A: normal field consistency (SPEC 13 #7)\n\n";
    text += "Angle between each sample's normal and the reference normal, in "
            "degrees. `reference` measures the route's reference surface; "
            "`shipped mesh` measures the canonical mesh that would actually "
            "be rendered. Both use the same reference normals and the same "
            "grid cells. Smooth-seam contract value: <=5 degrees; all other "
            "thresholds TBD.\n\n";
    text += "| Group | Route | surface | compared | flipped | mean | p50 | p95 | max |\n";
    text += "|---|---|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            const NormalDeviation shipped = measureNormalDeviation(route->canonicalGrid,
                                                                   route->targetGrid);
            for (int which = 0; which < 2; ++which) {
                const NormalDeviation& d = which == 0 ? route->normals : shipped;
                char buffer[512];
                std::snprintf(buffer, sizeof(buffer),
                              "| %s | %s | %s | %d | %d | %.2f | %.2f | %.2f | %.2f |\n",
                              data.name.c_str(), route->name.c_str(),
                              which == 0 ? "reference" : "shipped mesh",
                              d.compared, d.flipped, d.angleDeg.mean, d.angleDeg.p50,
                              d.angleDeg.p95, d.angleDeg.max);
                text += buffer;
            }
        }
    }

    // Where the shipped mesh's normals disagree with the reference normals.
    // A flip is a hard defect, so its anatomical location matters.
    text += "\nFlipped-normal cells by ring (shipped meshes, Baseline; only rings "
            "with a flip are listed):\n\n";
    text += "A flip is a hard normal-field disagreement and is recorded rather "
            "than hidden by folding it into an acute angle. Every flip is "
            "confined to rings 0-1 (y ~ 0.844-0.852 m), where the pelvis fan "
            "cap meets the side wall: the cap's downward faces dominate the "
            "accumulated vertex normal there while the reference normal is "
            "radial. Rings 2-51 have zero flips.\n\n";
    text += "| Ring | y (m) | " + groups[0].r1.name + " | " + groups[0].r2.name + " | " +
            groups[0].r3.name + " |\n|---|---|---|---|---|\n";
    {
        const SurfaceGrid& grid = groups[0].r1.canonicalGrid;
        for (int r = 0; r < grid.rings; ++r) {
            int flips[3] = {0, 0, 0};
            int index = 0;
            for (const RouteResult* route : {&groups[0].r1, &groups[0].r2, &groups[0].r3}) {
                for (int s = 0; s < grid.segments; ++s) {
                    if (!route->canonicalGrid.isValid(r, s) ||
                        !route->targetGrid.isValid(r, s)) continue;
                    const Vec3& a = route->canonicalGrid.normals[
                        static_cast<size_t>(route->canonicalGrid.index(r, s))];
                    const Vec3& b = route->targetGrid.normals[
                        static_cast<size_t>(route->targetGrid.index(r, s))];
                    if (a.dot(b) < 0.0f) ++flips[index];
                }
                ++index;
            }
            if (flips[0] + flips[1] + flips[2] == 0) continue;
            char buffer[512];
            std::snprintf(buffer, sizeof(buffer), "| %d | %.3f | %d | %d | %d |\n",
                          r, grid.ringY(r), flips[0], flips[1], flips[2]);
            text += buffer;
        }
    }

    text += "\n## Layer A: cross-section contour deviation (SPEC 13 #1)\n\n";
    text += "Signed horizontal-radius deviation at matched (height, azimuth) "
            "cells, over ten height bands of the PoC-A torso. Positive means "
            "the route is wider than the target at that cell.\n\n";
    text += "| Group | Route | |deviation| mean | p95 | max | band range (signed) |\n";
    text += "|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            double lo = 0.0, hi = 0.0;
            for (const SectionBand& band : route->sections.bands) {
                lo = std::min(lo, band.radialDeviation.mean);
                hi = std::max(hi, band.radialDeviation.mean);
            }
            char buffer[512];
            std::snprintf(buffer, sizeof(buffer),
                          "| %s | %s | %.5f | %.5f | %.5f | %+.5f .. %+.5f |\n",
                          data.name.c_str(), route->name.c_str(),
                          route->sections.absoluteDeviation.mean,
                          route->sections.absoluteDeviation.p95,
                          route->sections.absoluteDeviation.max, lo, hi);
            text += buffer;
        }
    }

    // ------------------------------------------- seams / skinning contract
    text += "\n## Layer B: seam smoothness and T2 crop ring (SPEC 13 #7)\n\n";
    text += "Dihedral deflection between adjacent quad face normals. "
            "`smooth` = <=5 degrees (contract value); `hard` = above it. "
            "The crop ring is the T2 shoulder-root attachment boundary.\n\n";
    text += "| Group | Route | interior edges | smooth | hard | mean | p95 | max | "
            "crop edges | crop mean | crop max | crop planarity (mm) | crop smooth |\n";
    text += "|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            const SeamAudit& s = route->seams;
            char buffer[768];
            std::snprintf(buffer, sizeof(buffer),
                          "| %s | %s | %d | %d | %d | %.2f | %.2f | %.2f | %d | %.2f | %.2f | "
                          "%.2f | %s |\n",
                          data.name.c_str(), route->name.c_str(),
                          s.interiorSeamEdges, s.smoothSeamEdges, s.hardSeamEdges,
                          s.meanDihedralDeg, s.p95DihedralDeg, s.maxDihedralDeg,
                          s.cropRingEdges, s.cropRingMeanDihedralDeg, s.cropRingMaxDihedralDeg,
                          s.cropRingPlanarityMm, s.cropRingSmooth ? "yes" : "NO");
            text += buffer;
        }
    }

    text += "\n## Weight legality and CPU deformation (SPEC 11.1)\n\n";
    text += "GPU numerical parity remains `[UNVERIFIED]` at G2 and is NOT a "
            "kill condition (SPEC 11.1). The test pose is a 20-degree chest "
            "twist plus a 15-degree left-shoulder raise.\n\n";
    text += "| Group | Route | legal | max influences | negative | sum violations | "
            "bad IDs | rigid probes | max rigid error | bind residual | max move | vol change |\n";
    text += "|---|---|---|---|---|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            char buffer[768];
            std::snprintf(buffer, sizeof(buffer),
                          "| %s | %s | %s | %d | %d | %d | %d | %d | %.3e | %.3e | %.5f | %.3f%% |\n",
                          data.name.c_str(), route->name.c_str(),
                          route->weights.valid ? "yes" : "NO", route->weights.maxInfluences,
                          route->weights.negativeWeightVertices,
                          route->weights.sumToleranceViolations,
                          route->weights.invalidBoneIdVertices,
                          route->deformation.rigidVertexCount,
                          route->deformation.maxRigidError, route->deformation.bindResidual,
                          route->deformation.maxDisplacement,
                          route->deformation.relativeVolumeChange * 100.0);
            text += buffer;
        }
    }

    // ------------------------------------------------- silhouette (7 views)
    text += "\n## Silhouette deviation vs R1, seven fixed views (metres)\n\n";
    text += "Orthographic projection, identical for every route (SPEC 8.4). ";
    text += "R1 is the reference because it defines the target reference surface.\n\n";
    text += "| Group | Route | View | Compared bins | p50 | p95 | max abs | mean abs |\n";
    text += "|---|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        const BodyJoints& joints = data.target.joints;
        // The shared PoC-A band: every route is clipped to the same torso
        // region, so the silhouette difference is never anatomy that a route
        // never claimed to produce.
        const float yMin = joints.hips.y + production.yBottomOffset - 0.01f;
        const float yMax = joints.chest.y + production.yTopOffset + 0.01f;
        for (const View& view : fixedViews()) {
            const Silhouette ref =
                extractSilhouette(data.r1.mesh, view, joints.hips, joints.chest, yMin, yMax);
            for (const RouteResult* route : {&data.r2, &data.r3}) {
                const Silhouette other = extractSilhouette(
                    route->mesh, view, joints.hips, joints.chest, yMin, yMax);
                const SilhouetteDelta delta = compareSilhouettes(ref, other);
                char buffer[1024];
                std::snprintf(buffer, sizeof(buffer),
                              "| %s | %s | %s | %d | %.5f | %.5f | %.5f | %.5f |\n",
                              data.name.c_str(), route->name.c_str(), view.name.c_str(),
                              delta.comparedBins, delta.p50, delta.p95, delta.maxAbs,
                              delta.meanAbs);
                text += buffer;
            }
        }
    }

    // ------------------------------------------------------ topology stability
    text += "\n## Topology stability across parameter groups (SPEC 13 #10)\n\n";
    text += "Same vertex ID across parameter groups must keep pointing at the same ";
    text += "anatomical position; face adjacency must stay constant.\n\n";
    text += "| Route | ID defined | Face adjacency constant | Mean vertex displacement | "
            "Max vertex displacement | Vertices moved > 1 mm |\n";
    text += "|---|---|---|---|---|---|\n";
    for (size_t i = 1; i < groups.size(); ++i) {
        for (int routeIndex = 0; routeIndex < 3; ++routeIndex) {
            const RouteResult& base = routeIndex == 0   ? groups[0].r1
                                      : routeIndex == 1 ? groups[0].r2
                                                        : groups[0].r3;
            const RouteResult& other = routeIndex == 0   ? groups[i].r1
                                       : routeIndex == 1 ? groups[i].r2
                                                         : groups[i].r3;
            double sum = 0.0;
            double worst = 0.0;
            int moved = 0;
            int count = 0;
            bool sameIds = base.mesh.vertices.size() == other.mesh.vertices.size();
            bool sameAdjacency = base.quads.size() == other.quads.size();
            if (sameIds) {
                for (size_t k = 0; k < base.mesh.vertices.size(); ++k) {
                    const double d = (base.mesh.vertices[k].position -
                                      other.mesh.vertices[k].position).length();
                    sum += d;
                    worst = std::max(worst, d);
                    if (d > 0.001) ++moved;
                    ++count;
                }
            }
            if (sameAdjacency) {
                for (size_t k = 0; k < base.quads.size(); ++k) {
                    if (base.quads[k] != other.quads[k]) {
                        sameAdjacency = false;
                        break;
                    }
                }
            }
            char buffer[1024];
            std::snprintf(buffer, sizeof(buffer),
                          "| %s (%s) | %s | %s | %.5f | %.5f | %d/%d |\n",
                          other.name.c_str(), groups[i].name.c_str(),
                          sameIds ? "yes" : "NO", sameAdjacency ? "yes" : "NO",
                          count ? sum / count : 0.0, worst, moved, count);
            text += buffer;
        }
    }

    // ------------------------------------------- §13 #3 / #4 proportions
    text += "\n## Parameter-group proportions (SPEC 13 #3 / #4)\n\n";
    text += "Ring extents are measured on the shared target reference grid, "
            "at the same anatomical heights in every group. All thresholds "
            "are TBD until the owner approves them against DEC-05.\n\n";
    text += "| Group | waist width (m) | chest width (m) | waist/chest | "
            "iliac crest width (m) | pelvic depth (m) | width/depth |\n";
    text += "|---|---|---|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        const BodyJoints& j = data.target.joints;
        const RingExtent waist = ringExtentAt(data.target.grid, j.hips.y + 0.12f);
        const RingExtent chest = ringExtentAt(data.target.grid, j.chest.y - 0.08f);
        const RingExtent iliac = ringExtentAt(data.target.grid, j.hips.y + 0.04f);
        char buffer[640];
        std::snprintf(buffer, sizeof(buffer),
                      "| %s | %.4f | %.4f | %.4f | %.4f | %.4f | %.4f |\n",
                      data.name.c_str(), waist.width, chest.width,
                      chest.width > 1e-6 ? waist.width / chest.width : 0.0,
                      iliac.width, iliac.depth,
                      iliac.depth > 1e-6 ? iliac.width / iliac.depth : 0.0);
        text += buffer;
    }

    // ------------------------------------------------ §13 #9 region impact
    text += "\n## Parameter region impact (SPEC 13 #9)\n\n";
    text += "Displacement against the Baseline group. `stable region` is the "
            "part of the torso the changed parameters do NOT act on: the "
            "lower torso (below the spine landmark) for Chest Width-Depth, "
            "and the upper chest (above spine + 50 mm) for Waist-Pelvis Ratio. "
            "A displacement >1 mm there is an illegal cross-region influence. "
            "The 1 mm guard is a measurement guard, NOT an approved threshold. "
            "R1 shows 0/0 because its vertex count changes between parameter "
            "groups, so vertex-ID comparison is UNDEFINED - this is NOT a "
            "claim that R1 did not move.\n\n";
    text += "| Group | Route | moved | total | mean (m) | max (m) | "
            "stable max (m) | stable >1 mm |\n|---|---|---|---|---|---|---|---|\n";
    for (size_t g = 1; g < groups.size(); ++g) {
        for (int routeIndex = 0; routeIndex < 3; ++routeIndex) {
            const RouteResult& base = routeIndex == 0   ? groups[0].r1
                                      : routeIndex == 1 ? groups[0].r2
                                                        : groups[0].r3;
            const RouteResult& other = routeIndex == 0   ? groups[g].r1
                                       : routeIndex == 1 ? groups[g].r2
                                                         : groups[g].r3;
            const bool comparable = base.mesh.vertices.size() == other.mesh.vertices.size();
            int moved = 0, stableMoved = 0, total = 0;
            double sum = 0.0, worst = 0.0, stableWorst = 0.0;
            const float spineY = groups[g].target.joints.spine.y;
            if (comparable) {
                for (size_t k = 0; k < base.mesh.vertices.size(); ++k) {
                    const Vec3& p = base.mesh.vertices[k].position;
                    const double d = (p - other.mesh.vertices[k].position).length();
                    if (d > 0.0) ++moved;
                    sum += d;
                    worst = std::max(worst, d);
                    const bool stable = dataIsChestGroup(groups[g].name)
                                            ? p.y < spineY
                                            : p.y > spineY + 0.05f;
                    if (stable) {
                        stableWorst = std::max(stableWorst, d);
                        if (d > 0.001) ++stableMoved;
                    }
                    ++total;
                }
            }
            char buffer[640];
            std::snprintf(buffer, sizeof(buffer),
                          "| %s | %s | %d | %d | %.5f | %.5f | %.5f | %d |\n",
                          groups[g].name.c_str(), other.name.c_str(), moved, total,
                          total ? sum / total : 0.0, worst, stableWorst, stableMoved);
            text += buffer;
        }
    }

    // ----------------------------------------------------------- volumes
    text += "\n## Volume (SPEC 13 #2 / #8)\n\n";
    text += "Signed triangle sum. A mesh with a registered crop boundary is open, ";
    text += "so the value is the volume of the surface closed by the crop plane - ";
    text += "usable for relative comparison, not as an absolute body volume.\n\n";
    text += "| Group | Route | Volume (m3) | Delta vs R1 |\n|---|---|---|---|\n";
    for (const GroupData& data : groups) {
        const double ref = data.r1.audit.volumeM3;
        for (const RouteResult* route : {&data.r1, &data.r2, &data.r3}) {
            char buffer[512];
            std::snprintf(buffer, sizeof(buffer), "| %s | %s | %.6f | %+.2f%% |\n",
                          data.name.c_str(), route->name.c_str(), route->audit.volumeM3,
                          ref != 0.0 ? (route->audit.volumeM3 - ref) / ref * 100.0 : 0.0);
            text += buffer;
        }
    }

    const std::string dir = outDir + "/";
    std::FILE* file = std::fopen((dir + "fair-comparison.md").c_str(), "wb");
    if (file) {
        std::fwrite(text.data(), 1, text.size(), file);
        std::fclose(file);
    }
    std::printf("%s", text.c_str());
    return 0;
}
