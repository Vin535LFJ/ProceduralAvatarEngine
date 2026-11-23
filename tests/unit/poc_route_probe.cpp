// G1 (AUTH-07): diagnostic probe - generates the three geometry routes and
// writes the meshes (OBJ) plus a metrics table next to them, so the G1
// evidence can be inspected offline without touching the production app.
//
// Three questions are answered here, all by measurement:
//   1. Is each route geometrically feasible (does it produce a usable mesh)?
//   2. Does each route keep a FIXED topology under a shape change?
//   3. How far is each route surface from the shared target reference (E1)?

#include "procedural/shape/PocRoutes.h"
#include "procedural/shape/QuadTemplate.h"
#include "procedural/shape/ShapeModel.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace pae;
using namespace pae::shape;

namespace {

void appendRoute(std::string& text, const RouteResult& route) {
    char buffer[1024];
    std::snprintf(buffer, sizeof(buffer),
                  "| %s | %s | %d | %d | %d | %d | %d | %d | %d | %.3f | %.4f | %.4f |\n",
                  route.name.c_str(), route.feasible ? "FEASIBLE" : "KILLED",
                  route.vertexCount, route.triangleCount, route.quadCount,
                  route.topology.components, route.topology.boundaryEdges,
                  route.topology.nonManifoldEdges, route.topology.degenerateTriangles,
                  route.meanEdgeLength, route.e1Mean, route.e1Max);
    text += buffer;
}

const char* kTableHeader =
    "| Route | Status | Verts | Tris | Quads | Components | Boundary | NonManifold | Degenerate | "
    "MeanEdge(m) | E1 mean(m) | E1 max(m) |\n"
    "|---|---|---|---|---|---|---|---|---|---|---|---|\n";

QuadTemplateSpec specWith(int rings, int segments) {
    QuadTemplateSpec spec;
    spec.rings = rings;
    spec.segments = segments;
    return spec;
}

// Per-height-band E1 distribution. This separates a DISCRETISATION error
// (shrinks when the template resolution goes up) from a MODEL error (the
// route's shape model cannot represent the anatomy at all and does not
// improve with resolution).
void appendErrorBands(std::string& text, const TargetReference& target, const RouteResult& route) {
    text += "\n**" + route.name + "** - E1 by height band (metres):\n\n";
    text += "| Height band | Samples | E1 mean | E1 max |\n|---|---|---|---|\n";
    for (float lo = 0.85f; lo < 1.28f; lo += 0.10f) {
        const float hi = lo + 0.10f;
        double sum = 0.0;
        double worst = 0.0;
        int count = 0;
        for (const Vec3& p : target.samples) {
            if (p.y < lo || p.y >= hi) continue;
            float best = 3.4e38f;
            for (const Vertex& v : route.mesh.vertices) {
                const float d = (v.position - p).lengthSq();
                if (d < best) best = d;
            }
            const double dist = std::sqrt(static_cast<double>(best));
            sum += dist;
            if (dist > worst) worst = dist;
            ++count;
        }
        if (!count) continue;
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "| y=[%.2f, %.2f) | %d | %.4f | %.4f |\n",
                      lo, hi, count, sum / count, worst);
        text += buffer;
    }
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

    const ShapeModel model;

    // Two template resolutions: the coarse one used by the unit tests, and the
    // production PoC-A resolution (26 rings x 32 segments).
    const QuadTemplateSpec coarse = specWith(10, 16);
    const QuadTemplateSpec production = specWith(26, 32);

    const TargetReference target = buildTargetReference(model, production, resolution);
    std::printf("target reference: valid=%d samples=%zu\n", target.valid ? 1 : 0,
                target.samples.size());

    std::string text;
    text += "# G1 route metrics (host probe)\n\n";
    text += "Target reference: anatomy SDF isosurface, torso + shoulder-root region, ";
    text += std::to_string(target.samples.size()) + " samples.\n";
    text += "Anatomy resolution: " + std::to_string(resolution.x) + "x" +
            std::to_string(resolution.y) + "x" + std::to_string(resolution.z) + ".\n";

    // ---------------------------------------------------------------- coarse
    RouteResult r1c = runRouteR1(model, resolution);
    RouteResult r2c = runRouteR2(model, coarse);
    RouteResult r3c = runRouteR3(model, coarse);
    measureE1(target, r1c.mesh, r1c);
    measureE1(target, r2c.mesh, r2c);
    measureE1(target, r3c.mesh, r3c);

    text += "\n## Template resolution 10 rings x 16 segments (coarse, unit-test spec)\n\n";
    text += kTableHeader;
    appendRoute(text, r1c);
    appendRoute(text, r2c);
    appendRoute(text, r3c);
    text += "\n";
    for (const RouteResult* route : {&r1c, &r2c, &r3c}) {
        text += "- " + route->name + ": generation " +
                std::to_string(static_cast<int>(route->generationMs)) + " ms";
        if (!route->failureReason.empty()) text += ", failure: " + route->failureReason;
        text += "\n";
    }

    // ------------------------------------------------------------ production
    RouteResult r1p = runRouteR1(model, resolution);
    RouteResult r2p = runRouteR2(model, production);
    RouteResult r3p = runRouteR3(model, production);
    measureE1(target, r1p.mesh, r1p);
    measureE1(target, r2p.mesh, r2p);
    measureE1(target, r3p.mesh, r3p);

    text += "\n## Template resolution 26 rings x 32 segments (production PoC-A spec)\n\n";
    text += kTableHeader;
    appendRoute(text, r1p);
    appendRoute(text, r2p);
    appendRoute(text, r3p);
    text += "\n";
    for (const RouteResult* route : {&r1p, &r2p, &r3p}) {
        text += "- " + route->name + ": generation " +
                std::to_string(static_cast<int>(route->generationMs)) + " ms";
        if (!route->failureReason.empty()) text += ", failure: " + route->failureReason;
        text += "\n";
    }

    // ------------------------------------------------- topology stability
    ShapeModel mutated = model;
    writeParameter(mutated, "chestWidth", 1.15f, nullptr);
    writeParameter(mutated, "pelvisWidth", 1.12f, nullptr);

    const RouteResult r1m = runRouteR1(mutated, resolution);
    const RouteResult r2m = runRouteR2(mutated, production);
    const RouteResult r3m = runRouteR3(mutated, production);

    text += "\n## Topology stability under a shape change\n\n";
    text += "Shape change: chestWidth 1.00 -> 1.15, pelvisWidth 1.00 -> 1.12.\n\n";
    text += "| Route | Verts before | Verts after | Vertex set fixed | Index-wise delta well defined |\n";
    text += "|---|---|---|---|---|\n";
    {
        char buffer[512];
        std::snprintf(buffer, sizeof(buffer), "| R1 SDF + Surface Nets | %d | %d | %s | %s |\n",
                      r1p.vertexCount, r1m.vertexCount,
                      r1p.vertexCount == r1m.vertexCount ? "yes" : "NO",
                      maxVertexDelta(r1p.mesh, r1m.mesh) >= 0.0 ? "yes" : "NO");
        text += buffer;
        std::snprintf(buffer, sizeof(buffer), "| R2 quad template projected on SDF | %d | %d | %s | %s |\n",
                      r2p.vertexCount, r2m.vertexCount,
                      r2p.vertexCount == r2m.vertexCount ? "yes" : "NO",
                      maxVertexDelta(r2p.mesh, r2m.mesh) >= 0.0 ? "yes" : "NO");
        text += buffer;
        std::snprintf(buffer, sizeof(buffer), "| R3 quad template on B-spline control lattice | %d | %d | %s | %s |\n",
                      r3p.vertexCount, r3m.vertexCount,
                      r3p.vertexCount == r3m.vertexCount ? "yes" : "NO",
                      maxVertexDelta(r3p.mesh, r3m.mesh) >= 0.0 ? "yes" : "NO");
        text += buffer;
    }

    // ------------------------------------------------- error distribution
    text += "\n## E1 error distribution by height band (production template)\n\n";
    text += "A DISCRETISATION error shrinks as the template resolution goes up; a MODEL error ";
    text += "(the route's shape model cannot represent the anatomy) does not.\n";
    appendErrorBands(text, target, r2p);
    appendErrorBands(text, target, r3p);

    const std::string dir = outDir + "/";
    writeObj(r1p.mesh, dir + "r1-sdf-surface-nets.obj");
    writeObj(r2p.mesh, dir + "r2-quad-template-projected.obj");
    writeObj(r3p.mesh, dir + "r3-quad-template-bspline.obj");
    writeObj(r2c.mesh, dir + "r2-quad-template-projected-coarse.obj");
    writeObj(r3c.mesh, dir + "r3-quad-template-bspline-coarse.obj");

    std::FILE* file = std::fopen((dir + "route-metrics.md").c_str(), "wb");
    if (file) {
        std::fwrite(text.data(), 1, text.size(), file);
        std::fclose(file);
    }
    std::printf("%s", text.c_str());
    return (r1p.feasible && r2p.feasible && r3p.feasible) ? 0 : 1;
}
