// VR-1B WP3 host-only shape study (owner-authorized 2026-11-27).
//
// Maps anthropometric parameter requests to the validated MakeHuman CC0 shape
// targets, calibrates the linear response of each parameter, builds named body
// types by solving alpha per parameter, and reports measured achievement plus
// deformation quality and clay renders. NO Android code is touched.
//
// Caliber: PAE proof measurement planes (ht_measure.h), recomputed per variant
// with joints re-resolved on the deformed surface. This is a research caliber,
// NOT the final ANSUR II acceptance caliber (R7 condition 5 remains open).
//
// Usage: pae_wp3_shape_study --data <makehuman/data> --repo <makehuman root>
//                            --template <pae-human-base.paht> --out <dir>

#include "ht_common.h"
#include "ht_measure.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "ht_target.h"
#include "ht_topology.h"

#include "procedural/human_template/HumanTemplateLoader.h"
#include "procedural/human_template/PAEHumanTemplateFormat.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using namespace pae::ht;
namespace ht3 = pae::human_template;

namespace {

void say(const std::string& line) {
    std::printf("%s\n", line.c_str());
    std::fflush(stdout);
}

// Which named measurement a parameter is calibrated against (-1 = proxy only).
int measureIndexOf(const std::string& key) {
    if (key == "shoulderWidth") return 0;   // bideltoid width
    if (key == "ribcageWidth") return 1;    // chest width
    if (key == "chestDepth") return 2;      // chest depth
    if (key == "waistWidth") return 3;      // waist width
    if (key == "hipWidth") return 4;        // hip width
    if (key == "stature") return 5;         // stature
    return -1;                              // armShape / legShape / headShape
}

double measureValue(const Measures& m, int idx) {
    switch (idx) {
        case 0: return m.bideltoidWidth;
        case 1: return m.chestWidth;
        case 2: return m.chestDepth;
        case 3: return m.waistWidth;
        case 4: return m.hipWidth;
        default: return m.stature;
    }
}
const char* measureName(int idx) {
    switch (idx) {
        case 0: return "bideltoid width";
        case 1: return "chest width";
        case 2: return "chest depth";
        case 3: return "waist width";
        case 4: return "hip width";
        default: return "stature";
    }
}

struct Calibration {
    const ShapeParam* p = nullptr;
    int measure = -1;
    double base = 0.0, atPlus = 0.0, atMinus = 0.0;
    double slopePos = 0.0, slopeNeg = 0.0;   // measurement change per +1 / -1 alpha
    double proxyPlusMm = 0.0;                // mean region displacement at +1 (mm)
    long flipsPlus = 0, flipsMinus = 0;
    double edgeP99Plus = 0.0, edgeP99Minus = 0.0;
};

struct BodyType {
    std::string name;
    // (parameter key, requested measurement delta in metres) for measurable
    // parameters; proxy parameters use direct authored-range alpha.
    std::vector<std::pair<std::string, double>> requests;
    std::vector<std::pair<std::string, double>> alphas;
};

} // namespace

int main(int argc, char** argv) {
    std::string dataDir, repoRoot, templatePath, outDir;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string key = argv[i], value = argv[i + 1];
        if (key == "--data") dataDir = value;
        else if (key == "--repo") repoRoot = value;
        else if (key == "--template") templatePath = value;
        else if (key == "--out") outDir = value;
    }
    if (dataDir.empty() || repoRoot.empty() || templatePath.empty() || outDir.empty()) {
        std::fprintf(stderr, "usage: pae_wp3_shape_study --data <dir> --repo <root> --template <.paht> --out <dir>\n");
        return 2;
    }
    makeDirs(outDir);
    makeDirs(outDir + "/images");

    ProofContext c;
    std::string error;
    if (!c.load(dataDir, repoRoot, error)) {
        say("[error] load: " + error);
        return 1;
    }
    for (const std::string& l : c.log) say(l);

    // ---- 1. Template target equivalence (the .paht must be a valid shape source)
    ht3::HumanTemplate t;
    if (!ht3::loadTemplateFile(templatePath, t, error)) {
        say("[error] template load: " + error);
        return 1;
    }
    int matched = 0, mismatched = 0;
    for (const ht3::ShapeTarget& st : t.shapeTargets) {
        const ShapeParam* found = nullptr;
        bool decr = false;
        for (const ShapeParam& p : c.params) {
            if (st.id == p.key) { found = &p; decr = false; break; }
            if (st.id == p.key + "-") { found = &p; decr = true; break; }
        }
        if (!found) { ++mismatched; continue; }
        const ShapeTarget& at = decr ? found->decr : found->incr;
        bool same = at.vertex.size() == st.vertex.size() && at.delta.size() == st.delta.size();
        if (same) {
            for (size_t i = 0; i < st.vertex.size(); ++i) {
                if (at.vertex[i] != st.vertex[i] ||
                    at.delta[i].x != st.delta[i].x || at.delta[i].y != st.delta[i].y ||
                    at.delta[i].z != st.delta[i].z) { same = false; break; }
            }
        }
        if (same) ++matched; else ++mismatched;
    }
    say(fmt("[template] target equivalence: %d matched / %d mismatched (of %zu)",
            matched, mismatched, t.shapeTargets.size()));
    if (mismatched != 0 || matched != 18) {
        say("[error] template targets do not match the pinned asset targets; refusing to continue");
        return 1;
    }

    // ---- 2. Per-parameter calibration at alpha = +1 / -1
    const Measures baseM = measureBody(c.body.pos, c.regions, c.rigDeform);
    say(fmt("[base] stature %.4f bideltoid %.4f chest %.4f x %.4f waist %.4f x %.4f hip %.4f",
            baseM.stature, baseM.bideltoidWidth, baseM.chestWidth, baseM.chestDepth,
            baseM.waistWidth, baseM.waistDepth, baseM.hipWidth));

    std::vector<Calibration> cals;
    std::string calMd = "# WP3 parameter calibration (host study, generated)\n\n";
    calMd += "Linear model: measurement(alpha) ~ base + alpha * slope, calibrated at alpha = +1 and -1 (asymmetric slopes). ";
    calMd += "Planes: PAE proof caliber (ht_measure.h); joints re-resolved on each deformed surface. ";
    calMd += "armShape / legShape / headShape have no named anthropometric measure in this caliber; their effect size is the mean displacement of the primary region (proxy, mm).\n\n";
    calMd += "| Parameter | Measure | Base (m) | +1 (m) | -1 (m) | Slope + (m/alpha) | Slope - (m/alpha) | Proxy +1 (mm) | Flips +/- | Edge p99 +/- |\n";
    calMd += "|---|---|---:|---:|---:|---:|---:|---:|---|---|\n";
    for (const ShapeParam& p : c.params) {
        Calibration cal;
        cal.p = &p;
        cal.measure = measureIndexOf(p.key);
        std::vector<D3> fullP, fullM;
        applyToFull(c, {{&p.incr, 1.0}}, fullP);
        // The decr target already encodes the negative direction; apply at +1.
        applyToFull(c, {{&p.decr, 1.0}}, fullM);
        Rig rigP, rigM;
        rigOnSurface(c, fullP, false, rigP);
        rigOnSurface(c, fullM, false, rigM);
        const std::vector<D3> bodyP(fullP.begin(), fullP.begin() + static_cast<long>(c.body.pos.size()));
        const std::vector<D3> bodyM(fullM.begin(), fullM.begin() + static_cast<long>(c.body.pos.size()));
        const Measures mP = measureBody(bodyP, c.regions, rigP);
        const Measures mM = measureBody(bodyM, c.regions, rigM);
        const DeformationQuality qP = compareSurfaces(c.body, bodyP, c.topo);
        const DeformationQuality qM = compareSurfaces(c.body, bodyM, c.topo);
        cal.flipsPlus = qP.flippedFaces; cal.flipsMinus = qM.flippedFaces;
        cal.edgeP99Plus = qP.edgeRatioP99; cal.edgeP99Minus = qM.edgeRatioP99;
        if (cal.measure >= 0) {
            cal.base = measureValue(baseM, cal.measure);
            cal.atPlus = measureValue(mP, cal.measure);
            cal.atMinus = measureValue(mM, cal.measure);
            cal.slopePos = cal.atPlus - cal.base;
            cal.slopeNeg = cal.base - cal.atMinus;
        }
        // Proxy effect: mean displacement of the first expected region.
        const std::vector<double> meanP = regionMeanDisplacement(c.regions, p.incr.vertex, p.incr.delta);
        int primaryRegion = 0;
        if (p.key == "armShape") primaryRegion = static_cast<int>(Region::UpperArm);
        else if (p.key == "legShape") primaryRegion = static_cast<int>(Region::Thigh);
        else if (p.key == "headShape") primaryRegion = static_cast<int>(Region::Head);
        cal.proxyPlusMm = meanP[static_cast<size_t>(primaryRegion)] * 1000.0;
        cals.push_back(cal);
        if (cal.measure >= 0) {
            calMd += fmt("| %s | %s | %.4f | %.4f | %.4f | %+.4f | %+.4f | - | %ld / %ld | %.3f / %.3f |\n",
                         p.key.c_str(), measureName(cal.measure), cal.base, cal.atPlus, cal.atMinus,
                         cal.slopePos, cal.slopeNeg, cal.flipsPlus, cal.flipsMinus, cal.edgeP99Plus, cal.edgeP99Minus);
            say(fmt("[cal] %-13s %-16s base %.4f  +1 %.4f (%+.4f)  -1 %.4f (%+.4f)  flips %ld/%ld",
                    p.key.c_str(), measureName(cal.measure), cal.base, cal.atPlus, cal.slopePos,
                    cal.atMinus, -cal.slopeNeg, cal.flipsPlus, cal.flipsMinus));
        } else {
            calMd += fmt("| %s | proxy (%s mean disp) | - | - | - | - | - | %.2f | %ld / %ld | %.3f / %.3f |\n",
                         p.key.c_str(), regionName(static_cast<Region>(primaryRegion)), cal.proxyPlusMm,
                         cal.flipsPlus, cal.flipsMinus, cal.edgeP99Plus, cal.edgeP99Minus);
            say(fmt("[cal] %-13s proxy %s mean disp +1 = %.2f mm  flips %ld/%ld",
                    p.key.c_str(), regionName(static_cast<Region>(primaryRegion)), cal.proxyPlusMm,
                    cal.flipsPlus, cal.flipsMinus));
        }
    }
    writeFile(outDir + "/calibration.md", calMd);

    // ---- 3. Named body types: solve alpha from requested measurement deltas
    const std::vector<BodyType> types = {
        {"base", {}, {}},
        {"tall-athletic",
         {{"stature", 0.08}, {"shoulderWidth", 0.06}, {"waistWidth", -0.02}},
         {{"armShape", 0.5}, {"legShape", 0.5}}},
        {"petite",
         {{"stature", -0.10}, {"shoulderWidth", -0.04}, {"hipWidth", 0.01}},
         {}},
        {"heavy-set",
         {{"waistWidth", 0.08}, {"hipWidth", 0.06}, {"ribcageWidth", 0.04}, {"chestDepth", 0.04}},
         {{"armShape", 0.4}, {"legShape", 0.4}}},
        {"v-taper",
         {{"shoulderWidth", 0.08}, {"waistWidth", -0.05}, {"hipWidth", -0.02}, {"ribcageWidth", 0.02}},
         {}},
    };

    std::string bodyMd = "# WP3 body types (host study, generated)\n\n";
    bodyMd += "alpha solved per parameter from the calibrated asymmetric slopes, clamped to the authored [-1, +1] range (clamped requests are flagged). ";
    bodyMd += "Proxy parameters (armShape/legShape) use direct authored-range alpha. ";
    bodyMd += "stature is the standalone macro height target: anthropometric correctness UNVERIFIED by MakeHuman; quality columns show whether the combination stays clean.\n\n";
    bodyMd += "| Body type | alpha per parameter | Stature req/ach (m) | Bideltoid req/ach (m) | Waist req/ach (m) | Hip req/ach (m) | Chest W x D ach (m) | Flips | Edge p99 | Volume ratio |\n";
    bodyMd += "|---|---|---|---|---|---|---|---:|---:|---:|\n";

    std::vector<std::vector<D3>> variantBodies;
    std::vector<std::string> variantNames;
    std::vector<const std::vector<D3>*> frameSets;
    for (const BodyType& bt : types) {
        std::vector<std::pair<const ShapeTarget*, double>> weighted;
        std::string alphaStr;
        std::string clampFlags;
        auto addRequest = [&](const std::string& key, double delta, bool isAlpha) {
            const Calibration* cal = nullptr;
            for (const Calibration& cc : cals) if (cc.p->key == key) cal = &cc;
            if (!cal) return;
            double alpha = 0.0;
            if (isAlpha) {
                alpha = delta;
            } else {
                const double slope = delta >= 0 ? cal->slopePos : cal->slopeNeg;
                if (std::abs(slope) < 1e-9) { clampFlags += key + ":no-slope "; return; }
                alpha = delta / slope;
                if (alpha > 1.0) { alpha = 1.0; clampFlags += key + ":clamp+ "; }
                if (alpha < -1.0) { alpha = -1.0; clampFlags += key + ":clamp- "; }
            }
            if (std::abs(alpha) < 1e-6) return;
            const ShapeTarget* target = alpha >= 0 ? &cal->p->incr : &cal->p->decr;
            weighted.push_back({target, std::abs(alpha)});
            alphaStr += fmt("%s%+.2f ", key.c_str(), alpha);
        };
        for (const auto& r : bt.requests) addRequest(r.first, r.second, false);
        for (const auto& a : bt.alphas) addRequest(a.first, a.second, true);

        std::vector<D3> full;
        applyToFull(c, weighted, full);
        const std::vector<D3> body(full.begin(), full.begin() + static_cast<long>(c.body.pos.size()));
        Rig rig;
        rigOnSurface(c, full, false, rig);
        const Measures m = measureBody(body, c.regions, rig);
        const DeformationQuality q = compareSurfaces(c.body, body, c.topo);

        auto reqAch = [&](const std::string& key, double Measures::*field) -> std::string {
            double req = 0.0;
            bool has = false;
            for (const auto& r : bt.requests) if (r.first == key) { req = r.second; has = true; }
            const double base = baseM.*field;
            if (!has) return fmt("- / %.4f", m.*field);
            return fmt("%+.3f / %+.3f%s", req, (m.*field) - base,
                       std::abs((m.*field) - base - req) > 0.005 ? " (!)" : "");
        };
        bodyMd += fmt("| %s | %s%s | %s | %s | %s | %s | %.4f x %.4f | %ld | %.3f | %.4f |\n",
                      bt.name.c_str(), alphaStr.empty() ? "(none)" : alphaStr.c_str(),
                      clampFlags.empty() ? "" : (" [" + clampFlags + "]").c_str(),
                      reqAch("stature", &Measures::stature).c_str(),
                      reqAch("shoulderWidth", &Measures::bideltoidWidth).c_str(),
                      reqAch("waistWidth", &Measures::waistWidth).c_str(),
                      reqAch("hipWidth", &Measures::hipWidth).c_str(),
                      m.chestWidth, m.chestDepth, q.flippedFaces, q.edgeRatioP99, q.volumeRatio);
        say(fmt("[body] %-14s %s| stature %.4f bideltoid %.4f waist %.4f hip %.4f | flips %ld edgeP99 %.3f vol %.4f %s",
                bt.name.c_str(), alphaStr.c_str(), m.stature, m.bideltoidWidth, m.waistWidth, m.hipWidth,
                q.flippedFaces, q.edgeRatioP99, q.volumeRatio, clampFlags.c_str()));
        variantBodies.push_back(body);
        variantNames.push_back(bt.name);
    }
    for (const std::vector<D3>& v : variantBodies) frameSets.push_back(&v);
    writeFile(outDir + "/body-types.md", bodyMd);

    // ---- 4. Clay renders, one shared frame per view across all variants
    std::vector<Camera> cameras = protocolCameras(320, 640);
    fitSharedFrames(cameras, frameSets);
    const int views[] = {0, 1, 2};  // front, front-45-left, side-left
    int written = 0;
    for (size_t v = 0; v < variantBodies.size(); ++v) {
        RenderSurface s;
        s.mesh = &c.body;
        s.positions = &variantBodies[v];
        s.regions = &c.regions;
        s.rig = &c.rigDeform;
        for (int vi : views) {
            const Rgb img = renderSurface(s, cameras[static_cast<size_t>(vi)], ShadeMode::Clay, false);
            const std::string path = outDir + "/images/wp3-" + variantNames[v] + "-" +
                                     cameras[static_cast<size_t>(vi)].name + ".png";
            if (writePngFile(path, img)) ++written;
        }
        const Rgb wire = renderSurface(s, cameras[0], ShadeMode::Wireframe, false);
        if (writePngFile(outDir + "/images/wp3-" + variantNames[v] + "-wire-front.png", wire)) ++written;
    }
    say(fmt("[images] %d written (5 variants x 3 clay views + 5 wireframe)", written));

    // ---- 5. Caliber note
    std::string note = "# WP3 caliber note\n\n";
    note += "- Base mesh stature: " + fmt("%.4f", baseM.stature) + " m (MakeHuman neutral base).\n";
    note += "- CharacterDefinition acceptance caliber: ANSUR II male p50 stature 1.7549 m (SRC-ANTH-01); the base mesh is NOT that reference. Mapping these parameters onto the ANSUR acceptance caliber (R7 condition 5) is a WP3-Android prerequisite, not done here.\n";
    note += "- Measurement planes: PAE proof definition (bideltoid = shoulder joint height - 2.5% stature; chest 72%; waist 60%; hip at hip joint height; band half-width 1.5% stature), recomputed per variant with re-resolved joints.\n";
    note += "- NOT RUN: self-intersection after deformation, MakeHuman authored slider ranges, macro combinations beyond standalone height, ANSUR caliber alignment, Android integration.\n";
    writeFile(outDir + "/caliber-note.md", note);
    say("[done] " + outDir);
    return 0;
}