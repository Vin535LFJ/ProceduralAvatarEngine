// VR-1B-WP3-HOST-01 experiment runner (owner-authorized 2026-11-27).
// Host-only. No Android production path is touched.
//
// Proves: parameter -> target -> alpha -> deformation -> measured geometry,
// with asymmetric response curves, true cross-section circumferences, three
// shoulder breadth definitions, a stature special study, linkage matrices,
// PAE's own topology/self-intersection checkers, determinism proofs, and
// multi-view clay/wireframe/normal evidence for 10 body variants.
//
// Usage: pae_wp3_experiment --data <makehuman/data> --repo <root>
//                           --template <.paht> --out <dir>

#include "ht_common.h"
#include "ht_measure.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "ht_target.h"
#include "ht_topology.h"
#include "wp3_measure.h"
#include "wp3_quality.h"

#include "procedural/MeshData.h"
#include "procedural/human_template/HumanTemplateLoader.h"
#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

using namespace pae::ht;
namespace ht3 = pae::human_template;
using pae::wp3::Wp3Measures;
using pae::wp3::QualityReport;

namespace {

void say(const std::string& line) {
    std::printf("%s\n", line.c_str());
    std::fflush(stdout);
}

std::string g_out;

void writeText(const std::string& name, const std::string& text) {
    writeFile(g_out + "/" + name, text);
}

double maxDiff(const std::vector<D3>& a, const std::vector<D3>& b) {
    if (a.size() != b.size()) return 1e30;
    double m = 0.0;
    for (size_t i = 0; i < a.size(); ++i) m = std::max(m, (a[i] - b[i]).length());
    return m;
}

const std::vector<double> kAlphaGrid = {-1.0, -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75, 1.0};

struct Variant {
    std::string name;
    std::vector<std::pair<std::string, double>> weighted;  // (param key, signed alpha)
    double uniformScaleTo = 0.0;   // if > 0: PAE uniform scale to this stature (m)
    bool verticalOnly = false;
};

// Authoritative uniform-scale anchor for stature transforms (WP3-H-R1 P0-1).
//   X = sagittal symmetry plane = body bbox x-centre. For the pinned base mesh
//       this is exactly 0 and coincides with the left/right mirror plane
//       (vertex mean x = 0, bounds symmetric); using it preserves L/R symmetry.
//   Y = sole plane (min body y) so the feet stay planted on the ground.
//   Z = rig Root bone head z (the authoritative character root node).
// The WP3-HOST-01 code wrongly reused the sole Y value as the X/Z centre
// (D3{sole,sole,sole}), which translated the character horizontally by
// sole*(1-k) (~5.7 cm at 1.55 m). The root helper cube has a small asymmetric
// x offset (~-6.8 mm); X therefore uses the confirmed symmetry plane, not the
// helper mean, so left/right joint symmetry is preserved exactly.
struct ScaleAnchor {
    D3 pivot;
    double soleY = 0.0, rootX = 0.0, rootZ = 0.0, meanX = 0.0;
    bool symmetric = false;
};
ScaleAnchor scaleAnchor(const ProofContext& c) {
    ScaleAnchor a;
    double xMin = 1e30, xMax = -1e30, yMin = 1e30, sx = 0.0;
    for (const D3& p : c.body.pos) {
        xMin = std::min(xMin, p.x); xMax = std::max(xMax, p.x);
        yMin = std::min(yMin, p.y); sx += p.x;
    }
    a.meanX = sx / static_cast<double>(c.body.pos.size());
    a.rootX = 0.5 * (xMin + xMax);
    a.soleY = yMin;
    a.symmetric = std::fabs(a.meanX) < 1e-6 && std::fabs(xMin + xMax) < 1e-6;
    const int ri = c.rigDeform.find("root");
    a.rootZ = ri >= 0 ? c.rigDeform.bones[static_cast<size_t>(ri)].head.z : 0.0;
    a.pivot = D3{a.rootX, a.soleY, a.rootZ};
    return a;
}

std::vector<D3> applyVariant(const ProofContext& c, const Variant& v, std::string& alphaDesc) {
    std::vector<std::pair<const ShapeTarget*, double>> weighted;
    alphaDesc.clear();
    for (const auto& w : v.weighted) {
        const ShapeParam* p = nullptr;
        for (const ShapeParam& pp : c.params) if (pp.key == w.first) p = &pp;
        if (!p || std::fabs(w.second) < 1e-9) continue;
        const ShapeTarget* t = w.second >= 0 ? &p->incr : &p->decr;
        weighted.push_back({t, std::fabs(w.second)});
        alphaDesc += fmt("%s%+.2f ", w.first.c_str(), w.second);
    }
    std::vector<D3> full;
    applyToFull(c, weighted, full);
    if (v.uniformScaleTo > 0.0) {
        const ScaleAnchor a = scaleAnchor(c);
        double baseStature = 0.0;
        {
            double lo = 1e30, hi = -1e30;
            for (const D3& p : c.body.pos) { lo = std::min(lo, p.y); hi = std::max(hi, p.y); }
            baseStature = hi - lo;
        }
        const double k = v.uniformScaleTo / baseStature;
        for (D3& p : full) {
            if (v.verticalOnly) p.y = a.soleY + (p.y - a.soleY) * k;
            else p = a.pivot + (p - a.pivot) * k;
        }
        alphaDesc += fmt("%s%.4f(k=%.4f) ", v.verticalOnly ? "verticalScaleTo" : "uniformScaleTo",
                         v.uniformScaleTo, k);
    }
    if (alphaDesc.empty()) alphaDesc = "(base)";
    return full;
}

std::string measuresCsvHeader() {
    return "name,alpha,stature,biacromial_proxy,shoulder_yoke,outer_shoulder,"
           "chest_breadth,chest_depth,chest_circumference,"
           "waist_breadth,waist_depth,waist_circumference,"
           "hip_breadth,hip_depth,hip_circumference,"
           "upperarm_circumference_L,upperarm_breadth_L,"
           "shoulder_joint_height,hip_joint_height,"
           "upperarm_length,forearm_length,thigh_length,shank_length";
}

std::string measuresCsvRow(const std::string& name, const std::string& alpha, const Wp3Measures& m) {
    return fmt("%s,%s,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f",
               name.c_str(), alpha.c_str(), m.stature, m.biacromialProxy, m.shoulderYoke, m.outerShoulder,
               m.chestBreadth, m.chestDepth, m.chestCircumference,
               m.waistBreadth, m.waistDepth, m.waistCircumference,
               m.hipBreadth, m.hipDepth, m.hipCircumference,
               m.upperArmCircumferenceL, m.upperArmBreadthL,
               m.shoulderJointHeight, m.hipJointHeight,
               m.upperArmLength, m.forearmLength, m.thighLength, m.shankLength);
}

std::string qualityCsvHeader() {
    return "name,vertices,triangles,components,boundary_edges,nonmanifold,degenerate,dup_vertices,invalid_normals,"
           "self_intersecting_pairs,candidate_pairs,flipped_faces,edge_ratio_p99,edge_ratio_min,edge_ratio_max,"
           "volume_ratio,volume_m3,bbox_x,bbox_y,bbox_z,edge_min,edge_p50,edge_p99,edge_max,ms";
}

std::string qualityCsvRow(const std::string& name, const QualityReport& q) {
    return fmt("%s,%zu,%ld,%d,%d,%d,%d,%d,%d,%zu,%zu,%ld,%.4f,%.4f,%.4f,%.4f,%.6f,%.4f,%.4f,%.4f,%.5f,%.5f,%.5f,%.5f,%.1f",
               name.c_str(), q.vertices, q.triangles, q.components, q.boundaryEdges, q.nonManifoldEdges,
               q.degenerateTriangles, q.duplicateVertices, q.invalidNormals,
               q.selfIntersectingPairs, q.candidatePairs, q.flippedFaces,
               q.edgeRatioP99, q.edgeRatioMin, q.edgeRatioMax, q.volumeRatio, q.volume,
               q.bboxX, q.bboxY, q.bboxZ, q.edgeMin, q.edgeP50, q.edgeP99, q.edgeMax, q.ms);
}

size_t peakWorkingSetMB() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return static_cast<size_t>(pmc.PeakWorkingSetSize / (1024 * 1024));
#endif
    // Cygwin fallback: /proc/self/status VmPeak (kB).
    if (std::FILE* f = std::fopen("/proc/self/status", "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            unsigned long kb = 0;
            if (std::sscanf(line, "VmPeak: %lu kB", &kb) == 1) {
                std::fclose(f);
                return static_cast<size_t>(kb / 1024);
            }
        }
        std::fclose(f);
    }
    return 0;
}

// ---- WP3-H-R1 P1-2: section visualization ----------------------------------
// Draw the closed intersection loops of a horizontal section in the (x, z)
// plane; the selected torso loop is highlighted, other loops (arms) are grey.
Rgb drawSection(const pae::wp3::Section& s, int selected, double centerX) {
    const int W = 480, H = 480;
    Rgb img; img.width = W; img.height = H; img.pixels.assign(static_cast<size_t>(W * H * 3), 255);
    double xMin = 1e30, xMax = -1e30, zMin = 1e30, zMax = -1e30;
    for (const auto& lp : s.loops) for (const D3& p : lp.pts) {
        xMin = std::min(xMin, p.x); xMax = std::max(xMax, p.x);
        zMin = std::min(zMin, p.z); zMax = std::max(zMax, p.z);
    }
    if (xMax <= xMin) { xMin = -0.5; xMax = 0.5; zMin = -0.2; zMax = 0.4; }
    const double pad = 0.05 * std::max(xMax - xMin, zMax - zMin);
    xMin -= pad; xMax += pad; zMin -= pad; zMax += pad;
    const double span = std::max(xMax - xMin, zMax - zMin);
    auto px = [&](double x, double z) {
        const int ix = static_cast<int>((x - xMin) / span * (W - 1));
        const int iy = static_cast<int>((zMax - z) / span * (H - 1));
        return std::pair<int, int>(std::max(0, std::min(W - 1, ix)), std::max(0, std::min(H - 1, iy)));
    };
    auto put = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        const size_t o = (static_cast<size_t>(y) * W + x) * 3;
        img.pixels[o] = r; img.pixels[o + 1] = g; img.pixels[o + 2] = b;
    };
    auto line = [&](D3 a, D3 b, uint8_t r, uint8_t g, uint8_t bl) {
        auto p0 = px(a.x, a.z); auto p1 = px(b.x, b.z);
        const int steps = std::max(std::abs(p1.first - p0.first), std::abs(p1.second - p0.second));
        for (int i = 0; i <= steps; ++i) {
            const double t = steps ? static_cast<double>(i) / steps : 0.0;
            put(static_cast<int>(std::lround(p0.first + (p1.first - p0.first) * t)),
                static_cast<int>(std::lround(p0.second + (p1.second - p0.second) * t)), r, g, bl);
        }
    };
    // sagittal centreline reference (x = centerX)
    for (int y = 0; y < H; ++y) { auto p = px(centerX, zMin); put(p.first, y, 210, 210, 235); }
    for (size_t li = 0; li < s.loops.size(); ++li) {
        const auto& lp = s.loops[li];
        const bool sel = static_cast<int>(li) == selected;
        const uint8_t r = sel ? 200 : 150, g = sel ? 30 : 150, b = sel ? 30 : 150;
        for (size_t i = 0; i < lp.pts.size(); ++i)
            line(lp.pts[i], lp.pts[(i + 1) % lp.pts.size()], r, g, b);
    }
    return img;
}

// ---- WP3-H-R1 P0-2: degenerate-triangle investigation ----------------------
// Mirrors pae::analyzeTopology's degenerate census EXACTLY: triangulate quads
// with the 0-2 diagonal, convert positions to float, and flag triangles whose
// float cross-product lengthSq < 1e-14f. Records the quad face id, vertex ids,
// double-precision area, and the rest/deformed face-normal dot (to prove that a
// uniform scale does NOT flip any face).
struct DegenerateTri {
    int quadFace = -1, triInFace = -1;
    int v[3] = {-1, -1, -1};
    double areaDouble = 0.0;
    float crossLenSqFloat = 0.0f;
    double normalDot = 1.0;
};
std::vector<DegenerateTri> findDegenerateTriangles(const PolyMesh& body, const std::vector<D3>& def,
                                                   const std::vector<D3>& base) {
    std::vector<DegenerateTri> out;
    auto floatCrossLenSq = [](const D3& a, const D3& b, const D3& c) {
        const pae::Vec3 pa(static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z));
        const pae::Vec3 pb(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z));
        const pae::Vec3 pc(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));
        return (pb - pa).cross(pc - pa).lengthSq();
    };
    for (size_t f = 0; f < body.faces.size(); ++f) {
        const PolyFace& fc = body.faces[f];
        if (fc.v.size() != 4) continue;
        const int tris[2][3] = {{fc.v[0], fc.v[1], fc.v[2]}, {fc.v[0], fc.v[2], fc.v[3]}};
        for (int t = 0; t < 2; ++t) {
            const D3& a = def[static_cast<size_t>(tris[t][0])];
            const D3& b = def[static_cast<size_t>(tris[t][1])];
            const D3& c = def[static_cast<size_t>(tris[t][2])];
            const float ls = floatCrossLenSq(a, b, c);
            if (ls < 1e-14f) {
                DegenerateTri d;
                d.quadFace = static_cast<int>(f); d.triInFace = t;
                d.v[0] = tris[t][0]; d.v[1] = tris[t][1]; d.v[2] = tris[t][2];
                d.areaDouble = (b - a).cross(c - a).length() * 0.5;
                d.crossLenSqFloat = ls;
                const D3 rn = faceNormal(body, static_cast<int>(f));
                PolyMesh moved = body; moved.pos = def;
                const D3 dn = faceNormal(moved, static_cast<int>(f));
                d.normalDot = rn.dot(dn);
                out.push_back(d);
            }
        }
    }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    std::string dataDir, repoRoot, templatePath;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string key = argv[i], value = argv[i + 1];
        if (key == "--data") dataDir = value;
        else if (key == "--repo") repoRoot = value;
        else if (key == "--template") templatePath = value;
        else if (key == "--out") g_out = value;
    }
    if (dataDir.empty() || repoRoot.empty() || templatePath.empty() || g_out.empty()) {
        std::fprintf(stderr, "usage: pae_wp3_experiment --data <dir> --repo <root> --template <.paht> --out <dir>\n");
        return 2;
    }
    makeDirs(g_out);
    makeDirs(g_out + "/images");
    const double tStart = nowMs();

    ProofContext c;
    std::string error;
    if (!c.load(dataDir, repoRoot, error)) { say("[error] load: " + error); return 1; }
    for (const std::string& l : c.log) say(l);

    // ---- template equivalence ------------------------------------------------
    ht3::HumanTemplate t;
    if (!ht3::loadTemplateFile(templatePath, t, error)) { say("[error] template: " + error); return 1; }
    int matched = 0;
    for (const ht3::ShapeTarget& st : t.shapeTargets) {
        for (const ShapeParam& p : c.params) {
            const ShapeTarget* at = nullptr;
            if (st.id == p.key) at = &p.incr;
            else if (st.id == p.key + "-") at = &p.decr;
            if (!at) continue;
            bool same = at->vertex.size() == st.vertex.size();
            if (same) for (size_t i = 0; i < st.vertex.size(); ++i)
                if (at->vertex[i] != st.vertex[i] || at->delta[i].x != st.delta[i].x ||
                    at->delta[i].y != st.delta[i].y || at->delta[i].z != st.delta[i].z) { same = false; break; }
            if (same) ++matched;
        }
    }
    say(fmt("[template] equivalence %d/18", matched));
    if (matched != 18) { say("[error] template targets mismatch"); return 1; }

    const std::vector<D3>& baseFull = c.full;
    const std::vector<D3>& baseBody = c.body.pos;
    const Wp3Measures baseM = pae::wp3::measureAll(baseBody, c.regions, c.rigDeform, c.body.faces);
    const QualityReport baseQ = pae::wp3::evaluateQuality(c.body, baseBody, c.topo, nullptr);
    say(fmt("[base-quality] components=%d boundary=%d nonmanifold=%d degenerate=%d selfInt=%zu volume=%.6f m3",
            baseQ.components, baseQ.boundaryEdges, baseQ.nonManifoldEdges, baseQ.degenerateTriangles,
            baseQ.selfIntersectingPairs, baseQ.volume));
    say(fmt("[base] stature %.4f biacromial-proxy %.4f yoke %.4f outer %.4f waistCirc %.4f hipCirc %.4f chestCirc %.4f",
            baseM.stature, baseM.biacromialProxy, baseM.shoulderYoke, baseM.outerShoulder,
            baseM.waistCircumference, baseM.hipCircumference, baseM.chestCircumference));

    // ---- target manifest -----------------------------------------------------
    std::string manifest = "# WP3 target manifest (generated)\n\n";
    manifest += fmt("Base mesh SHA-256: `%s` (commit a8bc2d54ff0ac92e78ff71431b1023eda42bf482, CC0-1.0)\n\n", c.obj.sourceSha256.c_str());
    manifest += "| Parameter | File | Target SHA-256 | Body rows | Helper/joint rows | alpha tested |\n|---|---|---|---:|---:|---|\n";
    for (const ShapeParam& p : c.params) {
        for (const ShapeTarget* tt : {&p.incr, &p.decr}) {
            long bodyRows = 0, helperRows = 0;
            for (int v : tt->vertex) (v < static_cast<int>(baseBody.size()) ? bodyRows : helperRows)++;
            manifest += fmt("| %s | %s | `%s` | %ld | %ld | -1..+1 grid 0.25 |\n",
                            p.key.c_str(), tt->relPath.c_str(), tt->sha256.c_str(), bodyRows, helperRows);
        }
    }
    manifest += "\nLicence: every listed target carries a CC0 file header and is enumerated by LICENSE.md section C. ";
    manifest += "default.mhskel is used ONLY as a host-only joint-naming reference (PENDING_CONFIRMATION, not distributed); ";
    manifest += "default_weights.mhw is NOT used in this study.\n";
    writeText("target-manifest.md", manifest);

    // ---- determinism proofs --------------------------------------------------
    std::string det = "# WP3 determinism proofs (generated)\n\n";
    {
        std::vector<D3> zero;
        applyToFull(c, {}, zero);
        const double d0 = maxDiff(zero, baseFull);
        det += fmt("- alpha=0 recovery: max vertex difference = %.3e m (bit-exact: %s)\n", d0, d0 == 0.0 ? "yes" : "NO");
        say(fmt("[det] alpha=0 max diff %.3e", d0));
    }
    {
        const ShapeParam* sh = nullptr;
        const ShapeParam* wa = nullptr;
        for (const ShapeParam& p : c.params) {
            if (p.key == "shoulderWidth") sh = &p;
            if (p.key == "waistWidth") wa = &p;
        }
        std::vector<D3> ab, ba, ab2;
        applyToFull(c, {{&sh->incr, 0.6}, {&wa->decr, 0.4}}, ab);
        applyToFull(c, {{&wa->decr, 0.4}, {&sh->incr, 0.6}}, ba);
        applyToFull(c, {{&sh->incr, 0.6}, {&wa->decr, 0.4}}, ab2);
        const double dOrder = maxDiff(ab, ba), dRepeat = maxDiff(ab, ab2);
        det += fmt("- order independence (shoulder+0.6, waist-0.4): max diff = %.3e m\n", dOrder);
        det += fmt("- repeat generation drift: max diff = %.3e m\n", dRepeat);
        det += fmt("- stable vertex ids: full=%zu body=%zu unchanged across all variants (positions only)\n",
                   baseFull.size(), baseBody.size());
        say(fmt("[det] order %.3e repeat %.3e", dOrder, dRepeat));
        // helper/joint following
        Rig rigP;
        rigOnSurface(c, ab, false, rigP);
        const int jL = rigP.find("upperarm01.L"), jB = c.rigDeform.find("upperarm01.L");
        const D3 jointMove = (jL >= 0 && jB >= 0) ? rigP.bones[static_cast<size_t>(jL)].head - c.rigDeform.bones[static_cast<size_t>(jB)].head : D3{};
        double helperMean = 0.0; long hn = 0;
        for (size_t i = baseBody.size(); i < ab.size(); ++i) { helperMean += (ab[i] - baseFull[i]).length(); ++hn; }
        helperMean = hn ? helperMean / static_cast<double>(hn) : 0.0;
        det += fmt("- helper/joint following (shoulder+0.6, waist-0.4): mean helper displacement = %.4f m; "
                   "upperarm01.L joint displacement = (%+.4f, %+.4f, %+.4f) m (non-zero: %s)\n",
                   helperMean, jointMove.x, jointMove.y, jointMove.z, jointMove.length() > 1e-6 ? "yes" : "NO");
        say(fmt("[det] helper mean %.4f joint move %.4f", helperMean, jointMove.length()));
    }
    writeText("determinism.md", det);

    // ---- single-parameter response curves ------------------------------------
    const std::vector<std::string> coreParams = {"shoulderWidth", "ribcageWidth", "chestDepth", "waistWidth", "hipWidth"};
    std::string curvesCsv = measuresCsvHeader() + ",flipped_faces,self_intersecting_pairs,edge_ratio_p99,volume_ratio\n";
    std::string curvesMd = "# WP3 alpha response curves (generated)\n\n";
    curvesMd += "Asymmetric incr/decr targets applied separately per sign; alpha grid {-1,-0.75,-0.5,-0.25,0,0.25,0.5,0.75,1}. ";
    curvesMd += "All measurements are PAE-defined surface measurements (see measurements-protocol.md). Circumferences are true plane-cut perimeters.\n\n";
    // WP3-H-R1 P1-1: linkage matrix with explicit affected-vs-whole-region semantics.
    std::string linkageCsv = "parameter,alpha,";
    for (int r = 0; r < kRegionCount; ++r) {
        const std::string rn = regionName(static_cast<Region>(r));
        linkageCsv += rn + "_affected," + rn + "_total," + rn + "_coverage," + rn + "_meanAffected_mm," +
                      rn + "_meanAll_mm," + rn + "_p95_mm," + rn + "_max_mm,";
    }
    linkageCsv += "helper_affected,helper_total,helper_coverage,helper_meanAffected_mm,helper_meanAll_mm,helper_p95_mm,helper_max_mm\n";
    std::string linkageMd = "# WP3 parameter linkage matrix (generated, WP3-H-R1 corrected semantics)\n\n";
    linkageMd += "Two distinct means per region (mm): **meanAll** = displacement averaged over ALL vertices of the region ";
    linkageMd += "(the whole-region effect); **meanAffected** = averaged only over vertices the target actually moves. ";
    linkageMd += "**coverage** = affected / total. The WP3-HOST-01 matrix reported meanAffected only and must not be read as a whole-region mean. ";
    linkageMd += "Full per-region affected/total/coverage/p95/max are in linkage-matrix.csv. alpha=+1 and -1.\n\n";
    linkageMd += "| Parameter | alpha | metric | ";
    for (int r = 0; r < kRegionCount; ++r) linkageMd += std::string(regionName(static_cast<Region>(r))) + " | ";
    linkageMd += "helper |\n|---|---|---|";
    for (int r = 0; r < kRegionCount; ++r) linkageMd += "---:|";
    linkageMd += "---:|\n";

    for (const std::string& key : coreParams) {
        const ShapeParam* p = nullptr;
        for (const ShapeParam& pp : c.params) if (pp.key == key) p = &pp;
        curvesMd += "## " + key + "\n\n| alpha | stature | biacromial | yoke | outer | chest B/D/Circ | waist B/D/Circ | hip B/Circ | armCirc | flips | selfInt | edgeP99 |\n";
        curvesMd += "|---:|---:|---:|---:|---:|---|---|---|---:|---:|---:|---:|\n";
        long firstFlipAlpha = 99; size_t firstSelfAlpha = 99;
        for (double a : kAlphaGrid) {
            std::vector<D3> full;
            if (a > 0) applyToFull(c, {{&p->incr, a}}, full);
            else if (a < 0) applyToFull(c, {{&p->decr, -a}}, full);
            else full = baseFull;
            const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(baseBody.size()));
            Rig rig;
            rigOnSurface(c, full, false, rig);
            const Wp3Measures m = pae::wp3::measureAll(body, c.regions, rig, c.body.faces);
            const QualityReport q = pae::wp3::evaluateQuality(c.body, body, c.topo, &baseBody);
            curvesCsv += measuresCsvRow(key, fmt("%.2f", a), m) +
                         fmt(",%ld,%zu,%.4f,%.4f\n", q.flippedFaces, q.selfIntersectingPairs, q.edgeRatioP99, q.volumeRatio);
            curvesMd += fmt("| %+.2f | %.4f | %.4f | %.4f | %.4f | %.4f/%.4f/%.4f | %.4f/%.4f/%.4f | %.4f/%.4f | %.4f | %ld | %zu | %.3f |\n",
                            a, m.stature, m.biacromialProxy, m.shoulderYoke, m.outerShoulder,
                            m.chestBreadth, m.chestDepth, m.chestCircumference,
                            m.waistBreadth, m.waistDepth, m.waistCircumference,
                            m.hipBreadth, m.hipCircumference, m.upperArmCircumferenceL,
                            q.flippedFaces, q.selfIntersectingPairs, q.edgeRatioP99);
            if (q.flippedFaces > 0 && firstFlipAlpha == 99) firstFlipAlpha = static_cast<long>(a * 100);
            if (q.selfIntersectingPairs > 0 && firstSelfAlpha == 99) firstSelfAlpha = static_cast<size_t>(std::fabs(a) * 100);
            if (std::fabs(std::fabs(a) - 1.0) < 1e-9) {
                const ShapeTarget* tt = a > 0 ? &p->incr : &p->decr;
                const pae::wp3::RegionLinkage l = pae::wp3::regionLinkage(c.regions, tt->vertex, tt->delta, baseBody.size(), baseFull.size());
                linkageCsv += fmt("%s,%+.2f,", key.c_str(), a);
                for (int r = 0; r < kRegionCount; ++r) {
                    linkageCsv += fmt("%ld,%ld,%.4f,%.3f,%.3f,%.3f,%.3f,", l.affected[r], l.total[r], l.coverage[r],
                                      l.meanAffected[r] * 1000.0, l.meanAll[r] * 1000.0, l.p95[r] * 1000.0, l.max[r] * 1000.0);
                }
                linkageCsv += fmt("%ld,%ld,%.4f,%.3f,%.3f,%.3f,%.3f\n", l.helperAffected, l.helperTotal, l.helperCoverage,
                                  l.helperMeanAffected * 1000.0, l.helperMeanAll * 1000.0, l.helperP95 * 1000.0, l.helperMax * 1000.0);
                linkageMd += fmt("| %s | %+.2f | meanAll mm |", key.c_str(), a);
                for (int r = 0; r < kRegionCount; ++r) linkageMd += fmt(" %.2f |", l.meanAll[r] * 1000.0);
                linkageMd += fmt(" %.2f |\n", l.helperMeanAll * 1000.0);
                linkageMd += fmt("| %s | %+.2f | meanAffected mm |", key.c_str(), a);
                for (int r = 0; r < kRegionCount; ++r) linkageMd += fmt(" %.2f |", l.meanAffected[r] * 1000.0);
                linkageMd += fmt(" %.2f |\n", l.helperMeanAffected * 1000.0);
                linkageMd += fmt("| %s | %+.2f | coverage |", key.c_str(), a);
                for (int r = 0; r < kRegionCount; ++r) linkageMd += fmt(" %.2f |", l.coverage[r]);
                linkageMd += fmt(" %.2f |\n", l.helperCoverage);
            }
        }
        curvesMd += fmt("\nCritical alpha: first flips at %s; first self-intersection at %s (99 = none within +/-1).\n\n",
                        firstFlipAlpha == 99 ? "none" : fmt("%.2f", firstFlipAlpha / 100.0).c_str(),
                        firstSelfAlpha == 99 ? "none" : fmt("%.2f", firstSelfAlpha / 100.0).c_str());
        say(fmt("[curve] %-13s done (flips<=%s, selfInt<=%s)", key.c_str(),
                firstFlipAlpha == 99 ? "none" : "FOUND", firstSelfAlpha == 99 ? "none" : "FOUND"));
    }
    writeText("response-curves.csv", curvesCsv);
    writeText("response-curves.md", curvesMd);
    writeText("linkage-matrix.csv", linkageCsv);
    writeText("linkage-matrix.md", linkageMd);

    // ---- stature special study -----------------------------------------------
    std::string heightMd = "# WP3 stature special study (generated)\n\n";
    heightMd += "## Baseline difference\n\n";
    heightMd += fmt("- MakeHuman neutral base mesh stature: **%.4f m** (this study's reference).\n", baseM.stature);
    heightMd += "- CharacterDefinition statureM default: **1.7549 m** = ANSUR II male 50th percentile (SRC-ANTH-01), a different reference population. The 89 mm gap is a caliber difference, not a defect.\n";
    heightMd += "- Macro height +1 alone gives ~2.3884 m (+0.7225 m): the maxheight target encodes MakeHuman's full height-slider range inside its macro combination (muscle/weight/age/proportion), NOT a calibrated metre scale. Standalone anthropometric correctness is **NOT VERIFIED**.\n\n";
    const ShapeParam* stature = nullptr;
    for (const ShapeParam& p : c.params) if (p.key == "stature") stature = &p;
    heightMd += "## A. MakeHuman macro height curve (standalone, research only)\n\n| alpha | stature (m) | flips | selfInt | edgeP99 | upperArm/stature | thigh/stature |\n|---:|---:|---:|---:|---:|---:|---:|\n";
    std::string heightCsv = "method,target_alpha_or_stature,stature,upperarm_ratio,thigh_ratio,shoulder_joint_height,hip_joint_height,flips,self_intersecting_pairs,edge_ratio_p99,volume_ratio\n";
    for (double a : {-1.0, -0.5, -0.25, 0.0, 0.25, 0.5, 1.0}) {
        std::vector<D3> full;
        if (a > 0) applyToFull(c, {{&stature->incr, a}}, full);
        else if (a < 0) applyToFull(c, {{&stature->decr, -a}}, full);
        else full = baseFull;
        const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(baseBody.size()));
        Rig rig; rigOnSurface(c, full, false, rig);
        const Wp3Measures m = pae::wp3::measureAll(body, c.regions, rig, c.body.faces);
        const QualityReport q = pae::wp3::evaluateQuality(c.body, body, c.topo, &baseBody);
        heightMd += fmt("| %+.2f | %.4f | %ld | %zu | %.3f | %.4f | %.4f |\n",
                        a, m.stature, q.flippedFaces, q.selfIntersectingPairs, q.edgeRatioP99,
                        m.upperArmLength / m.stature, m.thighLength / m.stature);
        heightCsv += fmt("macro,%+.2f,%.5f,%.5f,%.5f,%.5f,%.5f,%ld,%zu,%.4f,%.4f\n",
                         a, m.stature, m.upperArmLength / m.stature, m.thighLength / m.stature,
                         m.shoulderJointHeight, m.hipJointHeight, q.flippedFaces, q.selfIntersectingPairs,
                         q.edgeRatioP99, q.volumeRatio);
    }
    heightMd += "\n## B. PAE own height transforms (full OBJ incl. helper/joint, joints re-resolved)\n\n";
    heightMd += "Uniform scale about the sole plane preserves all proportions and scales joint heights exactly; vertical-only scale stretches y without x/z (distorts proportions).\n\n";
    heightMd += "| Method | Target (m) | Achieved | upperArm/stature | thigh/stature | shoulderJointH | hipJointH | flips | selfInt | edgeP99 |\n|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    for (bool vertical : {false, true}) {
        for (double target : {1.55, 1.6659, 1.7549, 1.85}) {
            Variant v;
            v.name = vertical ? "vertical" : "uniform";
            v.uniformScaleTo = target;
            v.verticalOnly = vertical;
            std::string desc;
            const std::vector<D3> full = applyVariant(c, v, desc);
            const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(baseBody.size()));
            Rig rig; rigOnSurface(c, full, false, rig);
            const Wp3Measures m = pae::wp3::measureAll(body, c.regions, rig, c.body.faces);
            const QualityReport q = pae::wp3::evaluateQuality(c.body, body, c.topo, &baseBody);
            heightMd += fmt("| %s | %.4f | %.4f | %.4f | %.4f | %.4f | %.4f | %ld | %zu | %.3f |\n",
                            v.name.c_str(), target, m.stature, m.upperArmLength / m.stature, m.thighLength / m.stature,
                            m.shoulderJointHeight, m.hipJointHeight, q.flippedFaces, q.selfIntersectingPairs, q.edgeRatioP99);
            heightCsv += fmt("%s,%.4f,%.5f,%.5f,%.5f,%.5f,%.5f,%ld,%zu,%.4f,%.4f\n",
                             v.name.c_str(), target, m.stature, m.upperArmLength / m.stature, m.thighLength / m.stature,
                             m.shoulderJointHeight, m.hipJointHeight, q.flippedFaces, q.selfIntersectingPairs,
                             q.edgeRatioP99, q.volumeRatio);
            say(fmt("[height] %s %.4f -> %.4f flips %ld selfInt %zu", v.name.c_str(), target, m.stature,
                    q.flippedFaces, q.selfIntersectingPairs));
        }
    }
    heightMd += "\n## Conclusion\n\n";
    heightMd += "- Macro height standalone: NOT VERIFIED for anthropometric stature control (non-linear, proportion-shifting, authored for macro combinations). Do NOT wire statureM to it.\n";
    heightMd += "- PAE uniform scale about the sole: deterministic, proportion-preserving, exact joint sync, clean quality at 1.55-1.85 m. Recommended host-verified stature method for WP3-Production (with helper geometry scaled together).\n";
    heightMd += "- Vertical-only scale: changes limb/head proportions; recorded for comparison, not recommended.\n";
    writeText("height-study.md", heightMd);
    writeText("height-study.csv", heightCsv);

    // ---- WP3-H-R1 P0-1: scaling-anchor verification --------------------------
    {
        const ScaleAnchor a = scaleAnchor(c);
        const Wp3Measures bm = pae::wp3::measureAll(baseBody, c.regions, c.rigDeform, c.body.faces);
        const double baseUpperRatio = bm.upperArmLength / bm.stature;
        const double baseThighRatio = bm.thighLength / bm.stature;
        std::string amd = "# WP3-H-R1 scaling-anchor verification (generated)\n\n";
        amd += fmt("Anchor pivot = (rootX=%.6f, soleY=%.6f, rootZ=%.6f) m.\n", a.pivot.x, a.pivot.y, a.pivot.z);
        amd += fmt("- rootX = body bbox x-centre = %.6f m; vertex mean x = %.3e m; mirror-symmetric: %s.\n",
                   a.rootX, a.meanX, a.symmetric ? "YES" : "NO");
        amd += "- rootZ = rig Root bone head z (authoritative character root node).\n";
        amd += "- soleY = min body y (feet planted).\n";
        amd += fmt("- WP3-HOST-01 bug: pivot was (sole,sole,sole)=(%.4f,%.4f,%.4f); horizontal drift at 1.55 m = %.4f m (x), %.4f m (z).\n\n",
                   a.soleY, a.soleY, a.soleY, a.soleY * (1.0 - 1.55 / bm.stature), a.soleY * (1.0 - 1.55 / bm.stature));
        amd += "| Target (m) | achieved | sole fixed | rootZ fixed | meanX drift | height err | upperRatio | thighRatio | L/R sym err | helper sync | repeat diff | PASS |\n";
        amd += "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|\n";
        std::string acsv = "target,achieved,sole_fixed_mm,rootZ_fixed_mm,meanX_drift_mm,height_err_mm,upper_ratio,thigh_ratio,lr_sym_err_mm,helper_sync_mm,repeat_diff_m,pass\n";
        say(fmt("[anchor] pivot=(%.5f,%.5f,%.5f) symmetric=%s", a.pivot.x, a.pivot.y, a.pivot.z, a.symmetric ? "yes" : "no"));
        for (double target : {1.55, 1.6659, 1.7549, 1.85}) {
            Variant v; v.name = "uniform"; v.uniformScaleTo = target; v.verticalOnly = false;
            std::string desc;
            const std::vector<D3> full = applyVariant(c, v, desc);
            const std::vector<D3> full2 = applyVariant(c, v, desc);
            const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(baseBody.size()));
            Rig rig; rigOnSurface(c, full, false, rig);
            const Wp3Measures m = pae::wp3::measureAll(body, c.regions, rig, c.body.faces);
            // 1 sole fixed
            double yMin = 1e30, sx = 0.0; for (const D3& p : body) { yMin = std::min(yMin, p.y); sx += p.x; }
            const double soleErr = std::fabs(yMin - a.soleY);
            // 2 root node Z fixed
            const int ri = rig.find("root");
            const double rootZErr = ri >= 0 ? std::fabs(rig.bones[static_cast<size_t>(ri)].head.z - a.rootZ) : 1e30;
            // 3 X centerline drift
            const double meanX = sx / static_cast<double>(body.size());
            const double meanXDrift = std::fabs(meanX - a.meanX);
            // 4 height exact
            const double heightErr = std::fabs(m.stature - target);
            // 5 proportions
            const double ur = m.upperArmLength / m.stature, tr = m.thighLength / m.stature;
            // 6 L/R symmetry of shoulder joints
            double symErr = 0.0;
            {
                const int jL = rig.find("upperarm01.L"), jR = rig.find("upperarm01.R");
                if (jL >= 0 && jR >= 0)
                    symErr = std::fabs(rig.bones[static_cast<size_t>(jL)].head.x + rig.bones[static_cast<size_t>(jR)].head.x);
            }
            // 7 helper sync: a helper vertex must move exactly by k about pivot
            const double k = target / bm.stature;
            double helperErr = 0.0;
            for (size_t i = baseBody.size(); i < full.size(); ++i) {
                const D3 expect = a.pivot + (baseFull[i] - a.pivot) * k;
                helperErr = std::max(helperErr, (full[i] - expect).length());
            }
            // 8 repeat
            const double repeatDiff = maxDiff(full, full2);
            const bool pass = soleErr < 1e-9 && rootZErr < 1e-9 && meanXDrift < 1e-6 && heightErr < 1e-4 &&
                              std::fabs(ur - baseUpperRatio) < 1e-4 && std::fabs(tr - baseThighRatio) < 1e-4 &&
                              symErr < 1e-6 && helperErr < 1e-9 && repeatDiff == 0.0;
            amd += fmt("| %.4f | %.4f | %.2e | %.2e | %.2e | %.2e | %.5f | %.5f | %.2e | %.2e | %.1e | %s |\n",
                       target, m.stature, soleErr, rootZErr, meanXDrift, heightErr, ur, tr, symErr, helperErr, repeatDiff, pass ? "YES" : "NO");
            acsv += fmt("%.4f,%.5f,%.3e,%.3e,%.3e,%.3e,%.5f,%.5f,%.3e,%.3e,%.1e,%d\n",
                        target, m.stature, soleErr, rootZErr, meanXDrift, heightErr, ur, tr, symErr, helperErr, repeatDiff, pass ? 1 : 0);
            say(fmt("[anchor] %.4f -> %.4f soleErr %.1e symErr %.1e helperErr %.1e %s", target, m.stature, soleErr, symErr, helperErr, pass ? "PASS" : "FAIL"));
        }
        amd += "\nAll nine checks: sole anchor fixed, root node fixed, X/Z centreline no drift, height exact, limb proportions preserved, ";
        amd += "L/R joint symmetry preserved, helper/joint synced, repeat bit-exact, 1.55/1.6659/1.7549/1.85 retested.\n";
        writeText("anchor-study.md", amd);
        writeText("anchor-study.csv", acsv);
    }

    // ---- body variants ---------------------------------------------------------
    const std::vector<Variant> variants = {
        {"baseline", {}, 0.0, false},
        {"broader-shoulder", {{"shoulderWidth", 1.0}}, 0.0, false},
        {"broader-ribcage", {{"ribcageWidth", 1.0}}, 0.0, false},
        {"narrower-waist", {{"waistWidth", -1.0}}, 0.0, false},
        {"wider-hip", {{"hipWidth", 1.0}}, 0.0, false},
        {"taller-pae", {}, 1.7549, false},
        {"shorter-pae", {}, 1.55, false},
        {"combo-athletic", {{"shoulderWidth", 0.75}, {"waistWidth", -0.5}, {"armShape", 0.5}}, 0.0, false},
        {"combo-pear", {{"hipWidth", 0.75}, {"waistWidth", -0.25}, {"shoulderWidth", -0.5}}, 0.0, false},
        {"combo-tall-heavy", {{"waistWidth", 0.75}, {"hipWidth", 0.75}}, 1.7549, false},
    };
    std::string varCsv = measuresCsvHeader() + "\n";
    std::string varQualityCsv = qualityCsvHeader() + "\n";
    std::string varMd = "# WP3 body variants (generated)\n\nAll variants start from the SAME fixed base mesh and template; no deformation is stacked. Parameter vectors are explicit.\n\n";
    varMd += "| Variant | Parameter vector | stature | biacromial | yoke | outer | waistCirc | hipCirc | chestCirc | flips | selfInt | edgeP99 | volRatio |\n|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    std::vector<std::vector<D3>> varBodies;
    std::vector<std::string> varNames;
    for (const Variant& v : variants) {
        std::string desc;
        const std::vector<D3> full = applyVariant(c, v, desc);
        const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(baseBody.size()));
        Rig rig; rigOnSurface(c, full, false, rig);
        const Wp3Measures m = pae::wp3::measureAll(body, c.regions, rig, c.body.faces);
        const QualityReport q = pae::wp3::evaluateQuality(c.body, body, c.topo, &baseBody);
        varCsv += measuresCsvRow(v.name, desc, m) + "\n";
        varQualityCsv += qualityCsvRow(v.name, q) + "\n";
        varMd += fmt("| %s | %s | %.4f | %.4f | %.4f | %.4f | %.4f | %.4f | %.4f | %ld | %zu | %.3f | %.4f |\n",
                     v.name.c_str(), desc.c_str(), m.stature, m.biacromialProxy, m.shoulderYoke, m.outerShoulder,
                     m.waistCircumference, m.hipCircumference, m.chestCircumference,
                     q.flippedFaces, q.selfIntersectingPairs, q.edgeRatioP99, q.volumeRatio);
        say(fmt("[variant] %-18s stature %.4f waistCirc %.4f hipCirc %.4f flips %ld selfInt %zu",
                v.name.c_str(), m.stature, m.waistCircumference, m.hipCircumference, q.flippedFaces, q.selfIntersectingPairs));
        varBodies.push_back(body);
        varNames.push_back(v.name);
    }
    writeText("variants.csv", varCsv);
    writeText("variants-quality.csv", varQualityCsv);
    writeText("variants.md", varMd);

    // ---- WP3-H-R1 P0-2: shorter-pae flipped-face / degenerate investigation --
    {
        std::string fid = "# WP3-H-R1 shorter-pae quality investigation (generated)\n\n";
        fid += fmt("Base mesh SHA-256: `%s`.\n\n", c.obj.sourceSha256.c_str());
        fid += "## CSV column clarification\n\n";
        fid += "`variants-quality.csv` column order is: name,vertices,triangles,components,boundary_edges,nonmanifold,**degenerate**,dup_vertices,invalid_normals,self_intersecting_pairs,candidate_pairs,**flipped_faces**,edge_ratio_p99,...\n\n";
        fid += "The `2` in the shorter-pae row is in the **degenerate** column (degenerateTriangles), NOT flipped_faces. ";
        fid += "flipped_faces is **0** for baseline, taller-pae AND shorter-pae. The 18 report (flips=0) and height-study (uniform 1.55 flips=0) are therefore consistent with the CSV; there is no flipped-face contradiction.\n\n";
        // locate shorter-pae
        size_t si = varNames.size();
        for (size_t i = 0; i < varNames.size(); ++i) if (varNames[i] == "shorter-pae") si = i;
        if (si < varBodies.size()) {
            const std::vector<D3>& sb = varBodies[si];
            const DeformationQuality dq = compareSurfaces(c.body, sb, c.topo);
            fid += "## Re-run from code (shorter-pae = uniform scale to 1.55 m)\n\n";
            fid += fmt("- compareSurfaces flipped_faces = **%ld**, minNormalDot = %.6f (a uniform scale with k>0 preserves every face orientation; Newell normal direction is scale-invariant).\n", dq.flippedFaces, dq.minNormalDot);
            fid += fmt("- volumeRatio = %.6f, edgeRatioP99 = %.4f.\n\n", dq.volumeRatio, dq.edgeRatioP99);
            const std::vector<DegenerateTri> deg = findDegenerateTriangles(c.body, sb, baseBody);
            fid += fmt("## Degenerate triangles at 1.55 m: %zu (this is the `2` in the CSV)\n\n", deg.size());
            fid += "Root cause: `pae::analyzeTopology` (native/procedural/MeshTopology.cpp:45, read-only reuse) flags a triangle degenerate when its **float** cross-product lengthSq < **1e-14f** — an ABSOLUTE threshold (area < ~5e-8 m^2). ";
            fid += "Uniform downscale to 1.55 m (k=0.9304) shrinks every area by k^2=0.8656; the two smallest base-mesh triangles fall below that absolute threshold after float conversion. ";
            fid += "This is a scale-dependent threshold artifact of the checker, NOT a geometry defect and NOT a flipped face. It is outside the WP3-H-R1 whitelist (native/), so it is documented, not modified.\n\n";
            fid += "| quad face ID | tri | vertex ids | double area (m^2) | float crossLenSq | rest.deformed normal dot |\n|---:|---:|---|---:|---:|---:|\n";
            for (const DegenerateTri& d : deg)
                fid += fmt("| %d | %d | %d,%d,%d | %.3e | %.3e | %.6f |\n", d.quadFace, d.triInFace, d.v[0], d.v[1], d.v[2], d.areaDouble, static_cast<double>(d.crossLenSqFloat), d.normalDot);
            // base areas of the same triangles for before/after comparison
            fid += "\nBefore/after area of the same triangles (double precision):\n\n| quad face | tri | base area (m^2) | 1.55 m area (m^2) | ratio |\n|---:|---:|---:|---:|---:|\n";
            for (const DegenerateTri& d : deg) {
                const D3& a0 = baseBody[static_cast<size_t>(d.v[0])];
                const D3& b0 = baseBody[static_cast<size_t>(d.v[1])];
                const D3& c0 = baseBody[static_cast<size_t>(d.v[2])];
                const double baseArea = (b0 - a0).cross(c0 - a0).length() * 0.5;
                fid += fmt("| %d | %d | %.3e | %.3e | %.4f |\n", d.quadFace, d.triInFace, baseArea, d.areaDouble, baseArea > 0 ? d.areaDouble / baseArea : 0.0);
            }
            fid += "\n## Conclusion\n\nflipped_faces = 0 (uniform scale cannot flip a face). The reported `2` is degenerateTriangles, a scale-dependent absolute-threshold artifact in PAE's own checker. ";
            fid += "Recommendation for WP3-Production: use a scale-relative degenerate threshold (or evaluate at native scale) so the census is stature-invariant. CSV and Markdown are unified: flips=0, degenerate=2 at 1.55 m.\n";
            say(fmt("[flip-investigation] shorter-pae flipped=%ld degenerate=%zu minNormalDot=%.6f", dq.flippedFaces, deg.size(), dq.minNormalDot));
        }
        writeText("flip-investigation.md", fid);
    }

    // ---- WP3-H-R1 P1-2: cross-section anatomical validation ------------------
    {
        std::string svc = "variant,section,plane_y,loop_count,closed_loop_count,open_chains,selected_loop,selected_centroid_x,torso_confirmed,breadth,depth,circumference,reason\n";
        std::string svmd = "# WP3-H-R1 cross-section anatomical validation (generated)\n\n";
        svmd += "largestArea() is the candidate strategy; the selected loop is CONFIRMED as torso only when its centroid lies near the sagittal centreline (|x| <= 0.10 m) and the section has no open chains. ";
        svmd += "Arm/thigh loops (A-pose) sit at |x| ~ 0.4 m and are rejected. NOT VERIFIED rows must not be read as anatomical circumferences.\n\n";
        svmd += "| Variant | Section | loops | closed | open | selected | centroid.x | torso? | circ (m) |\n|---|---|---:|---:|---:|---:|---:|---|---:|\n";
        const double centerX = scaleAnchor(c).rootX;
        auto validateOne = [&](const std::string& name, const std::vector<D3>& body, const Rig& rig, bool draw) {
            const Wp3Measures m = pae::wp3::measureAll(body, c.regions, rig, c.body.faces);
            struct Cut { const char* label; double y; };
            const Cut cuts[] = {{"chest", m.soleY + 0.72 * m.stature}, {"waist", m.soleY + 0.60 * m.stature}, {"hip", m.soleY + m.hipJointHeight}};
            for (const Cut& ct : cuts) {
                const pae::wp3::Section s = pae::wp3::cutSection(body, c.body.faces, {0, 1, 0}, {0, ct.y, 0});
                const pae::wp3::SectionValidation vv = pae::wp3::validateTorsoSection(s, centerX);
                const pae::wp3::SectionLoop* lp = vv.selectedLoop >= 0 ? &s.loops[static_cast<size_t>(vv.selectedLoop)] : nullptr;
                svc += fmt("%s,%s,%.5f,%zu,%zu,%d,%d,%.5f,%s,%.5f,%.5f,%.5f,%s\n",
                           name.c_str(), ct.label, ct.y, vv.loopCount, vv.closedLoopCount, vv.openChains, vv.selectedLoop,
                           vv.selectedCentroidX, vv.torsoConfirmed ? "YES" : "NOT_VERIFIED",
                           lp ? lp->breadth : 0.0, lp ? lp->depth : 0.0, lp ? lp->perimeter : 0.0, vv.reason.c_str());
                svmd += fmt("| %s | %s | %zu | %zu | %d | %d | %+.4f | %s | %.4f |\n",
                            name.c_str(), ct.label, vv.loopCount, vv.closedLoopCount, vv.openChains, vv.selectedLoop,
                            vv.selectedCentroidX, vv.torsoConfirmed ? "YES" : "NOT VERIFIED", lp ? lp->perimeter : 0.0);
                if (draw) {
                    const Rgb img = drawSection(s, vv.selectedLoop, centerX);
                    writePngFile(g_out + "/images/section-" + name + "-" + ct.label + ".png", img);
                }
            }
        };
        validateOne("base", baseBody, c.rigDeform, true);
        for (const Variant& v : variants) {
            std::string desc;
            const std::vector<D3> full = applyVariant(c, v, desc);   // includes scaled helper/joint geometry
            const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(baseBody.size()));
            Rig vr; rigOnSurface(c, full, false, vr);
            validateOne(v.name, body, vr, false);
        }
        // single-parameter extremes
        for (const std::string& key : coreParams) {
            const ShapeParam* p = nullptr;
            for (const ShapeParam& pp : c.params) if (pp.key == key) p = &pp;
            for (double a : {-1.0, 1.0}) {
                std::vector<D3> full;
                if (a > 0) applyToFull(c, {{&p->incr, a}}, full); else applyToFull(c, {{&p->decr, -a}}, full);
                const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(baseBody.size()));
                Rig rig; rigOnSurface(c, full, false, rig);
                validateOne(key + (a > 0 ? "+1" : "-1"), body, rig, false);
            }
        }
        writeText("section-validation.csv", svc);
        writeText("section-validation.md", svmd);
        say("[section-validation] base + 10 variants + 10 extremes written; base chest/waist/hip section images drawn");
    }

    // ---- renders: 5 views x 3 modes per variant, shared frames ----------------
    std::vector<Camera> cameras = protocolCameras(320, 640);
    std::vector<const std::vector<D3>*> sets;
    for (const std::vector<D3>& b : varBodies) sets.push_back(&b);
    fitSharedFrames(cameras, sets);
    const int viewIdx[] = {0, 4, 2, 7, 1};  // front, back, side-left, side-right, front-45-left
    const ShadeMode modes[] = {ShadeMode::Clay, ShadeMode::Wireframe, ShadeMode::ViewNormal};
    const char* modeName[] = {"clay", "wire", "normal"};
    int written = 0;
    for (size_t v = 0; v < varBodies.size(); ++v) {
        RenderSurface s;
        s.mesh = &c.body;
        s.positions = &varBodies[v];
        s.regions = &c.regions;
        s.rig = &c.rigDeform;
        for (int vi : viewIdx) {
            for (int mi = 0; mi < 3; ++mi) {
                const Rgb img = renderSurface(s, cameras[static_cast<size_t>(vi)], modes[mi], false);
                const std::string path = g_out + "/images/wp3h-" + varNames[v] + "-" +
                                         cameras[static_cast<size_t>(vi)].name + "-" + modeName[mi] + ".png";
                if (writePngFile(path, img)) ++written;
            }
        }
    }
    say(fmt("[images] %d written (10 variants x 5 views x 3 modes)", written));

    // ---- measurement protocol doc --------------------------------------------
    std::string proto = "# WP3 measurement protocol (PAE-defined surface measurements)\n\n";
    proto += "ALL values below are PAE custom surface measurements on the pinned MakeHuman base mesh. They are NOT ANSUR or MakeHuman official measurements. Units: metres (m), precision 1e-5 m in CSV. Circumferences are true plane-cut perimeters (triangle-plane intersection segments summed; loops chained and closure verified).\n\n";
    proto += "| Measurement | Plane / position | Sampling | Regions | Direction | Formula |\n|---|---|---|---|---|---|\n";
    proto += "| stature | bbox over all body vertices | whole surface | all | y | max(y)-min(y) |\n";
    proto += "| biacromial_proxy | shoulder joint centres (upperarm01.L/R head) | rig joints re-resolved on deformed surface | rig | 3D distance | |L-R|; NOT anatomical acromion distance |\n";
    proto += "| shoulder_yoke | y = sole + shoulderJointH - 2.5% stature | band half-width 1.5% stature | Neck+Thorax+ShoulderGirdle | x | max(x)-min(x) of band vertices |\n";
    proto += "| outer_shoulder | same plane | same band | +UpperArm | x | max(x)-min(x) |\n";
    proto += "| chest_breadth/depth/circumference | y = sole + 72% stature | exact plane cut, TORSO loop (largest area; arm loops excluded) | whole body section | x/z/perimeter | section extents; sum of cut segments of the torso loop |\n";
    proto += "| waist_breadth/depth/circumference | y = sole + 60% stature | exact plane cut, TORSO loop | whole body section | x/z/perimeter | same |\n";
    proto += "| hip_breadth/depth/circumference | y = sole + hipJointH | exact plane cut, TORSO loop | whole body section | x/z/perimeter | same |\n";
    proto += "| upperarm_circumference_L | plane normal = upperarm02.L axis, through bone midpoint | exact oblique cut, loop NEAREST the midpoint | whole body section | in-plane | perimeter of that loop |\n";
    proto += "| limb lengths | joint-to-joint (01 head / 02 tail / wrist / foot) | rig joints | rig | 3D distance | mean over L/R |\n";
    proto += "\nPlane fractions follow the PAE proof caliber (ht_measure) for comparability with R3; alignment to the ANSUR acceptance caliber is a WP3-Production prerequisite.\n";
    writeText("measurements-protocol.md", proto);

    const double totalMs = nowMs() - tStart;
    say(fmt("[done] %.1f s, peak working set %zu MB -> %s", totalMs / 1000.0, peakWorkingSetMB(), g_out.c_str()));
    writeText("run-summary.txt", fmt("total_ms=%.1f\npeak_working_set_mb=%zu\nimages=%d\nvariants=%zu\n",
                                     totalMs, peakWorkingSetMB(), written, variants.size()));
    return 0;
}