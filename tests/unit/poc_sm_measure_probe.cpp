// PoC-SM (AUTH-08 AL-5): the measurement probe.
//
// Builds the shared skin envelope from the default contract, measures the
// shoulder-root seam with the OQ-03 fixed definition, audits the topology and
// writes the evidence: a markdown report plus the grey-clay views.
//
// The probe REPORTS; it does not judge. A number crossing the 5 deg contract
// value is printed and recorded, and the probe exits non-zero so a caller
// cannot mistake "the tool ran" for "the shape passed" (AUTH-08 FB-12).

#include "procedural/shape/CameraProtocol.h"
#include "procedural/shape/ClayRenderer.h"
#include "procedural/shape/ImageWriter.h"
#include "procedural/shape/TopologyAudit.h"
#include "procedural/shape/v2/SharedSkinEnvelope.h"
#include "procedural/shape/v2/SeamAuditV2.h"
#include "skeleton/Skeleton.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace pae;
using namespace pae::shape;
using namespace pae::shape::v2;

namespace {

std::string outDir = "evidence/poc-sm";
std::string reportPath;

bool ensureDir(const std::string& path) {
    std::string cmd = "mkdir -p \"" + path + "\"";
    return std::system(cmd.c_str()) == 0;
}

} // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outDir = argv[++i];
        } else if (std::strcmp(argv[i], "--report") == 0 && i + 1 < argc) {
            reportPath = argv[++i];
        }
    }
    if (reportPath.empty()) reportPath = outDir + "/poc-sm-shared-envelope-report.md";

    if (!ensureDir(outDir)) {
        std::fprintf(stderr, "cannot create %s\n", outDir.c_str());
        return 2;
    }

    const BodyJoints joints{};
    const CharacterDefinition definition = defaultCharacterDefinition();

    SharedSkinEnvelopeSpec spec;
    spec.rings = 26;
    spec.segments = 32;
    const SharedSkinEnvelopeResult envelope = buildSharedSkinEnvelope(definition, spec, joints);

    if (!envelope.valid) {
        std::fprintf(stderr, "envelope build failed: %s\n", envelope.error.c_str());
        return 2;
    }

    // ---- Seam measurement (OQ-03 fixed definition) -------------------------
    const SeamMeasurement seam =
        measureShoulderRootSeam(envelope.mesh, envelope.quads, envelope.cropBoundaries);

    // ---- Segment-count sweep ----------------------------------------------
    //
    // The OQ-03 statistic is the angle between the face normals of two quads
    // that are neighbours AROUND the band. On a perfect cylinder that angle
    // is exactly 360/segments, so the faceting alone crosses the 5 deg
    // contract value below 72 segments. The sweep separates the faceting
    // contribution from the shape contribution, so the report can say how
    // much of the measured angle is the shape and how much is the grid.
    const int segmentCounts[] = {32, 48, 64, 72, 96, 128};
    struct SweepRow {
        int segments = 0;
        double maxDeg = 0.0;
        double meanDeg = 0.0;
        double facetingDeg = 0.0;
        double excessDeg = 0.0;
        bool passes = false;
        bool applicable = false;
    };
    std::vector<SweepRow> sweep;
    for (int segments : segmentCounts) {
        SharedSkinEnvelopeSpec sweepSpec = spec;
        sweepSpec.segments = segments;
        const SharedSkinEnvelopeResult swept = buildSharedSkinEnvelope(definition, sweepSpec, joints);
        if (!swept.valid) continue;
        const SeamMeasurement m =
            measureShoulderRootSeam(swept.mesh, swept.quads, swept.cropBoundaries);
        SweepRow row;
        row.segments = segments;
        row.maxDeg = m.maxDeg;
        row.meanDeg = m.meanDeg;
        row.facetingDeg = 360.0 / static_cast<double>(segments);
        row.excessDeg = m.maxDeg - row.facetingDeg;
        row.passes = m.passes;
        row.applicable = (m.applicability == SeamApplicability::Applicable);
        sweep.push_back(row);
    }

    // ---- Topology audit ----------------------------------------------------
    TopologyAudit audit = auditTopology(envelope.mesh, envelope.cropBoundaries);
    // auditTopology reports the counters and the boundary loops; the face
    // quality and the edge flow are measured separately, so they are merged
    // here rather than silently left at zero.
    audit.faces = measureFaceQuality(envelope.mesh, envelope.quads);
    audit.flow = measureEdgeFlow(envelope.mesh, envelope.quads);

    // ---- Clay views --------------------------------------------------------
    const std::vector<View> views = fixedViews();
    const std::vector<const MeshData*> meshes = {&envelope.mesh};
    const float yMin = joints.hips.y - 0.02f;
    const float yMax = joints.chest.y + 0.075f;
    const std::vector<ClayFrame> frames = computeSharedClayFrames(meshes, views, yMin, yMax, 480, 480);

    std::vector<std::string> imagePaths;
    for (size_t i = 0; i < views.size() && i < frames.size(); ++i) {
        const RgbImage image = renderClayView(envelope.mesh, views[i], frames[i], yMin, yMax);
        const std::string path = outDir + "/shared-envelope-" + views[i].name + ".png";
        if (writePng(path, image)) {
            imagePaths.push_back(path);
        }
    }

    // ---- Report ------------------------------------------------------------
    std::FILE* f = std::fopen(reportPath.c_str(), "wb");
    if (f == nullptr) {
        std::fprintf(stderr, "cannot write %s\n", reportPath.c_str());
        return 2;
    }

    std::fprintf(f, "# PoC-SM — Shared Skin Envelope measurement (vertical slice)\n\n");
    std::fprintf(f, "> Generated by `poc_sm_measure_probe` on the `poc-sm/shape-model-v2` branch.\n");
    std::fprintf(f, "> This is a MEASUREMENT, not a judgement. No Gate is marked PASS.\n\n");

    std::fprintf(f, "## 1. Construction\n\n");
    std::fprintf(f, "| Item | Value |\n|---|---|\n");
    std::fprintf(f, "| Vertices | %zu |\n", envelope.mesh.vertices.size());
    std::fprintf(f, "| Quads | %zu |\n", envelope.quads.size());
    std::fprintf(f, "| Rings / segments | %d / %d |\n", spec.rings, spec.segments);
    std::fprintf(f, "| Height range | %.4f .. %.4f m |\n", yMin, yMax);
    std::fprintf(f, "| Registered crops | %zu (`%s`) |\n",
                 envelope.cropBoundaries.size(),
                 envelope.cropBoundaries.empty() ? "" : envelope.cropBoundaries[0].label.c_str());
    std::fprintf(f, "\n");

    std::fprintf(f, "## 2. Section anchors (the profile the mesh was built from)\n\n");
    std::fprintf(f, "| Height (m) | Half width (m) | Half depth (m) |\n|---:|---:|---:|\n");
    for (size_t i = 0; i < envelope.anchorHeights.size(); ++i) {
        std::fprintf(f, "| %.4f | %.4f | %.4f |\n", envelope.anchorHeights[i],
                     envelope.anchorHalfWidths[i], envelope.anchorHalfDepths[i]);
    }
    std::fprintf(f, "\n");

    std::fprintf(f, "## 3. Shoulder-root seam — OQ-03 fixed definition\n\n");
    std::fprintf(f, "| Item | Value |\n|---|---|\n");
    std::fprintf(f, "| Applicability | `%s` |\n", seamApplicabilityName(seam.applicability));
    std::fprintf(f, "| Crop label / ring | `%s` / %d |\n", seam.cropLabel.c_str(), seam.ringIndex);
    std::fprintf(f, "| Included quads | %d |\n", seam.includedQuadCount);
    std::fprintf(f, "| Angle samples | %d |\n", seam.angleSampleCount);
    std::fprintf(f, "| Cap fan triangles | %d (samples %d) |\n", seam.capTriangleCount,
                 seam.capAngleSampleCount);
    std::fprintf(f, "| Winding verified | %s (inconsistencies: %d) |\n",
                 seam.windingVerified ? "yes" : "NO", seam.windingInconsistencyCount);
    std::fprintf(f, "| **max** | **%.4f deg** |\n", seam.maxDeg);
    std::fprintf(f, "| mean | %.4f deg |\n", seam.meanDeg);
    std::fprintf(f, "| p95 | %.4f deg |\n", seam.p95Deg);
    std::fprintf(f, "| cap fan max | %.4f deg |\n", seam.capMaxDeg);
    std::fprintf(f, "| max including cap | %.4f deg |\n", seam.maxDegIncludingCap);
    std::fprintf(f, "| Contract threshold | %.1f deg |\n", seam.thresholdDeg);
    std::fprintf(f, "| Within threshold | **%s** |\n", seam.passes ? "YES" : "NO");
    std::fprintf(f, "\n");
    if (!seam.notes.empty()) {
        std::fprintf(f, "Notes:\n\n");
        for (const std::string& note : seam.notes) {
            std::fprintf(f, "- %s\n", note.c_str());
        }
        std::fprintf(f, "\n");
    }

    std::fprintf(f, "## 4. Segment-count sweep — how much of the angle is the grid?\n\n");
    std::fprintf(f, "| Segments | max (deg) | mean (deg) | faceting 360/S (deg) | excess over faceting (deg) | within 5 deg |\n");
    std::fprintf(f, "|---:|---:|---:|---:|---:|:--:|\n");
    for (const SweepRow& row : sweep) {
        std::fprintf(f, "| %d | %.4f | %.4f | %.4f | %.4f | %s |\n", row.segments, row.maxDeg,
                     row.meanDeg, row.facetingDeg, row.excessDeg,
                     row.passes ? "YES" : "no");
    }
    std::fprintf(f, "\n");
    std::fprintf(f, "> The `excess` column is the part that is NOT explained by the polygonal\n");
    std::fprintf(f, "> grid. On a perfect cylinder the excess would be 0.\n\n");

    std::fprintf(f, "## 5. Topology audit\n\n");
    std::fprintf(f, "| Item | Value |\n|---|---|\n");
    std::fprintf(f, "| Components | %d |\n", audit.base.components);
    std::fprintf(f, "| Unregistered boundary edges | %d |\n", audit.unregisteredBoundaryEdges);
    std::fprintf(f, "| Registered boundary edges | %d |\n", audit.registeredBoundaryEdges);
    std::fprintf(f, "| All crops closed | %s |\n", audit.allCropsClosed ? "yes" : "NO");
    std::fprintf(f, "| Crops match loops | %s |\n", audit.cropsMatchLoops ? "yes" : "NO");
    std::fprintf(f, "| Clean (no crack / island / non-manifold) | %s |\n", audit.clean ? "yes" : "NO");
    std::fprintf(f, "| Sliver quads | %d |\n", audit.faces.sliverCount);
    std::fprintf(f, "| Mean min interior angle | %.2f deg |\n", audit.faces.meanMinAngleDeg);
    std::fprintf(f, "| Orthogonal quad ratio | %.3f |\n", audit.flow.orthogonalQuadRatio);
    std::fprintf(f, "\n");

    std::fprintf(f, "## 6. Evidence images\n\n");
    for (const std::string& path : imagePaths) {
        std::fprintf(f, "- `%s`\n", path.c_str());
    }
    std::fprintf(f, "\n");

    std::fprintf(f, "## 7. What this does NOT say\n\n");
    std::fprintf(f, "- It does **not** mark any Gate PASS (AUTH-08 FB-12).\n");
    std::fprintf(f, "- It does **not** judge `G-P1-GEO`: DEC-05 is not approved, so the\n");
    std::fprintf(f, "  remaining SPEC 13 thresholds are still `TBD` and no metric may be\n");
    std::fprintf(f, "  used to declare the shape acceptable.\n");
    std::fprintf(f, "- It does **not** cover the upper arm, forearm, neck or head.\n");
    std::fclose(f);

    std::printf("seam: applicability=%s max=%.4f mean=%.4f p95=%.4f included=%d samples=%d\n",
                seamApplicabilityName(seam.applicability), seam.maxDeg, seam.meanDeg,
                seam.p95Deg, seam.includedQuadCount, seam.angleSampleCount);
    std::printf("topology: unregisteredBoundary=%d clean=%s slivers=%d\n",
                audit.unregisteredBoundaryEdges, audit.clean ? "yes" : "no",
                audit.faces.sliverCount);
    std::printf("report: %s\n", reportPath.c_str());
    std::printf("images: %zu\n", imagePaths.size());

    // Non-zero when the seam exceeds the contract value, so a caller cannot
    // mistake "the tool ran" for "the shape passed".
    return seam.passes ? 0 : 1;
}
