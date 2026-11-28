// VR-1B WP3-R2 (M1) experiment runner: forward model + bounded inverse solver.
// Host-only; no Android, no APK, no commit. Produces measured data + images.

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_render.h"
#include "ht_rig.h"
#include "wp3_r2.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace pae;
using ht::D3;
using ht::fmt;

namespace {
std::string arg(int argc, char** argv, const std::string& k, const std::string& d) {
    for (int i = 1; i + 1 < argc; ++i) if (k == argv[i]) return argv[i + 1];
    return d;
}
std::vector<double> alphaOf(const std::vector<std::pair<std::string, double>>& kv) {
    const auto& params = wp3r2::solverParams();
    std::vector<double> a(params.size(), 0.0);
    for (const auto& w : kv)
        for (size_t i = 0; i < params.size(); ++i)
            if (params[i].key == w.first) a[i] = w.second;
    return a;
}
void fitOne(ht::Camera& cam, const std::vector<D3>& pos) {
    std::vector<ht::Camera> v{cam};
    std::vector<const std::vector<D3>*> sets{&pos};
    ht::fitSharedFrames(v, sets);
    cam = v[0];
}
} // namespace

int main(int argc, char** argv) {
    const std::string dataDir = arg(argc, argv, "--data", "");
    const std::string repoRoot = arg(argc, argv, "--repo", "");
    const std::string outDir = arg(argc, argv, "--out", "wp3r2-out");
    if (dataDir.empty() || repoRoot.empty()) { std::printf("usage: --data --repo --out\n"); return 2; }
    ht::makeDirs(outDir); ht::makeDirs(outDir + "/images"); ht::makeDirs(outDir + "/profiles");
    std::string log;
    auto say = [&](const std::string& s) { log += s + "\n"; std::printf("%s\n", s.c_str()); };

    ht::ProofContext ctx; std::string err;
    if (!ctx.load(dataDir, repoRoot, err)) { std::printf("load failed: %s\n", err.c_str()); return 1; }
    wp3r2::ForwardModel fm(ctx);
    say(fmt("[load] body=%zu params=%zu", ctx.body.pos.size(), ctx.params.size()));

    // base measures
    double base[wp3r2::kMeasureCount];
    fm.measures(std::vector<double>(wp3r2::solverParams().size(), 0.0), 0.0, base);
    std::string baseMd = "# WP3-R2 base measures (generated)\n\n| measure | value(m) |\n|---|---|\n";
    for (int i = 0; i < wp3r2::kMeasureCount; ++i) {
        baseMd += fmt("| %s | %.6f |\n", wp3r2::measureName(static_cast<wp3r2::Measure>(i)), base[i]);
        say(fmt("[base] %-20s %.6f", wp3r2::measureName(static_cast<wp3r2::Measure>(i)), base[i]));
    }
    ht::writeFile(outDir + "/base-measures.md", baseMd);

    auto active7 = [](wp3r2::SolveRequest& r) {
        for (int i = 1; i < wp3r2::kMeasureCount; ++i) r.active[i] = true;  // skip stature
    };

    std::string csv = "case,kind,reachable,rmsRelResidual,iters,forwardEvals,flipped,minNormalDot,edgeP99,volumeRatio,";
    for (int i = 0; i < wp3r2::kMeasureCount; ++i) csv += std::string(wp3r2::measureName(static_cast<wp3r2::Measure>(i))) + "Target,";
    for (int i = 0; i < wp3r2::kMeasureCount; ++i) csv += std::string(wp3r2::measureName(static_cast<wp3r2::Measure>(i))) + "Achieved,";
    for (size_t i = 0; i < wp3r2::solverParams().size(); ++i) csv += wp3r2::solverParams()[i].key + "Alpha,";
    csv += "\n";

    auto record = [&](const std::string& name, const std::string& kind, const wp3r2::SolveRequest& req,
                      const wp3r2::SolveResult& res) {
        csv += name + "," + kind + "," + (res.reachable ? "1" : "0") + "," + fmt("%.6f,%d,%ld,%ld,%.4f,%.4f,%.4f,",
               res.rmsRelResidual, res.iterations, res.forwardEvals, res.quality.flippedFaces,
               res.quality.minNormalDot, res.quality.edgeRatioP99, res.quality.volumeRatio);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) csv += fmt("%.6f,", req.target[i]);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) csv += fmt("%.6f,", res.achieved[i]);
        for (double a : res.alpha) csv += fmt("%+.6f,", a);
        csv += "\n";
        say(fmt("[solve] %-22s %-12s reach=%d rms=%.4f iters=%d evals=%ld flips=%ld", name.c_str(), kind.c_str(),
                res.reachable, res.rmsRelResidual, res.iterations, res.forwardEvals, res.quality.flippedFaces));
    };

    // ---- known-solution recovery ------------------------------------------
    const std::vector<std::pair<std::string, std::vector<std::pair<std::string, double>>>> known = {
        {"ks-shoulder-waist", {{"shoulderWidth", 0.6}, {"waistWidth", -0.4}}},
        {"ks-hip-ribcage", {{"hipWidth", 0.7}, {"ribcageWidth", 0.3}}},
        {"ks-combo", {{"shoulderWidth", 0.5}, {"ribcageWidth", 0.5}, {"waistWidth", -0.5}, {"hipWidth", 0.5}}},
    };
    std::string ksMd = "# WP3-R2 known-solution recovery (generated)\n\n| case | rmsRelResidual | reachable | maxAlphaErr |\n|---|---|---|---|\n";
    for (const auto& k : known) {
        const auto truth = alphaOf(k.second);
        double meas[wp3r2::kMeasureCount];
        fm.measures(truth, 0.0, meas);
        wp3r2::SolveRequest req; active7(req);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) req.target[i] = meas[i];
        const auto res = wp3r2::solve(fm, req);
        double maxErr = 0.0;
        for (size_t i = 0; i < truth.size(); ++i) maxErr = std::max(maxErr, std::fabs(res.alpha[i] - truth[i]));
        record(k.first, "known", req, res);
        ksMd += fmt("| %s | %.6f | %d | %.4f |\n", k.first.c_str(), res.rmsRelResidual, res.reachable, maxErr);
    }
    ht::writeFile(outDir + "/known-solution.md", ksMd);

    // ---- holdout combinations ---------------------------------------------
    const std::vector<std::pair<std::string, std::vector<std::pair<std::string, double>>>> holdout = {
        {"ho-chestdepth", {{"chestDepth", 0.8}}},
        {"ho-waist-up", {{"waistWidth", 0.9}}},
        {"ho-hip-narrow-shoulder", {{"hipWidth", -0.6}, {"shoulderWidth", -0.3}}},
    };
    for (const auto& h : holdout) {
        const auto truth = alphaOf(h.second);
        double meas[wp3r2::kMeasureCount];
        fm.measures(truth, 0.0, meas);
        wp3r2::SolveRequest req; active7(req);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) req.target[i] = meas[i];
        const auto res = wp3r2::solve(fm, req);
        record(h.first, "holdout", req, res);
    }

    // ---- unreachable targets ----------------------------------------------
    {
        wp3r2::SolveRequest req; active7(req);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) req.target[i] = base[i];
        req.target[static_cast<int>(wp3r2::Measure::WaistCircumference)] = base[static_cast<int>(wp3r2::Measure::WaistCircumference)] * 0.5;
        const auto res = wp3r2::solve(fm, req);
        record("unreach-waist-half", "unreachable", req, res);
    }
    {
        wp3r2::SolveRequest req; active7(req);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) req.target[i] = base[i];
        req.target[static_cast<int>(wp3r2::Measure::ShoulderBreadth)] = base[static_cast<int>(wp3r2::Measure::ShoulderBreadth)] * 1.8;
        const auto res = wp3r2::solve(fm, req);
        record("unreach-shoulder-1p8", "unreachable", req, res);
    }

    // ---- non-additivity ----------------------------------------------------
    std::string naMd = "# WP3-R2 multi-target non-additivity (generated)\n\n";
    {
        // ribcageWidth and shoulderWidth BOTH move the shoulder girdle/arms
        // (WP3 linkage: ribcage -> arms 34mm), so they are coupled, not additive.
        const auto aS = alphaOf({{"shoulderWidth", 0.7}});
        const auto aR = alphaOf({{"ribcageWidth", 0.7}});
        const auto aB = alphaOf({{"shoulderWidth", 0.7}, {"ribcageWidth", 0.7}});
        double mS[wp3r2::kMeasureCount], mR[wp3r2::kMeasureCount], mB[wp3r2::kMeasureCount];
        fm.measures(aS, 0.0, mS); fm.measures(aR, 0.0, mR); fm.measures(aB, 0.0, mB);
        naMd += "| measure | base | shoulder-only | ribcage-only | linear-sum | combined(actual) | nonAdditiveDelta |\n|---|---|---|---|---|---|---|\n";
        for (int i : {static_cast<int>(wp3r2::Measure::ShoulderBreadth), static_cast<int>(wp3r2::Measure::RibcageBreadth), static_cast<int>(wp3r2::Measure::WaistBreadth)}) {
            const double lin = base[i] + (mS[i] - base[i]) + (mR[i] - base[i]);
            naMd += fmt("| %s | %.4f | %.4f | %.4f | %.4f | %.4f | %+.4f |\n", wp3r2::measureName(static_cast<wp3r2::Measure>(i)),
                        base[i], mS[i], mR[i], lin, mB[i], mB[i] - lin);
        }
        // solver hits the combined target despite non-additivity
        wp3r2::SolveRequest req; active7(req);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) req.target[i] = mB[i];
        const auto res = wp3r2::solve(fm, req);
        record("nonadditive-combo", "nonadditive", req, res);
        naMd += fmt("\nSolver recovery of combined target: rmsRelResidual=%.6f reachable=%d (iteration handles coupling).\n", res.rmsRelResidual, res.reachable);
    }
    ht::writeFile(outDir + "/non-additivity.md", naMd);

    // ---- determinism -------------------------------------------------------
    {
        const auto truth = alphaOf({{"shoulderWidth", 0.5}, {"hipWidth", 0.5}});
        double meas[wp3r2::kMeasureCount]; fm.measures(truth, 0.0, meas);
        wp3r2::SolveRequest req; active7(req);
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) req.target[i] = meas[i];
        const auto r1 = wp3r2::solve(fm, req);
        const auto r2 = wp3r2::solve(fm, req);
        bool same = r1.alpha.size() == r2.alpha.size();
        for (size_t i = 0; same && i < r1.alpha.size(); ++i) same = r1.alpha[i] == r2.alpha[i];
        say(fmt("[determinism] identical alpha across two solves: %d", same));
        ht::writeFile(outDir + "/determinism.md", fmt("# WP3-R2 determinism\n\nidentical alpha across two solves: %d\nrms1=%.8f rms2=%.8f\n", same, r1.rmsRelResidual, r2.rmsRelResidual));
    }

    // ---- profiles + images -------------------------------------------------
    const int W = 360, H = 720;
    ht::Camera front = ht::makeCamera("front", 0, 0, W, H);
    fitOne(front, ctx.body.pos);
    const std::vector<ht::Region> regions0 = ht::labelRegions(ctx.body, ctx.rigDeform);
    auto renderBody = [&](const std::string& tag, const std::vector<D3>& body) {
        ht::Camera cam = ht::makeCamera("front", 0, 0, W, H);
        fitOne(cam, body);
        ht::RenderSurface s{&ctx.body, &body, &regions0, nullptr, &ctx.rigDeform};
        ht::writePngFile(outDir + "/images/" + tag + "-clay.png", ht::renderSurface(s, cam, ht::ShadeMode::Clay, false));
        ht::writePngFile(outDir + "/images/" + tag + "-wire.png", ht::renderSurface(s, cam, ht::ShadeMode::Wireframe, false));
    };
    renderBody("profile-baseline", ctx.body.pos);

    struct ProfDef { std::string name; std::vector<std::pair<std::string, double>> kv; double stature; };
    const std::vector<ProfDef> profs = {
        {"broad-shoulder", {{"shoulderWidth", 1.0}}, 0.0},
        {"broad-ribcage", {{"ribcageWidth", 1.0}}, 0.0},
        {"narrow-waist", {{"waistWidth", -1.0}}, 0.0},
        {"wide-hip", {{"hipWidth", 1.0}}, 0.0},
        {"tall", {}, 1.80},
        {"short", {}, 1.60},
        {"combo-athletic", {{"shoulderWidth", 0.75}, {"waistWidth", -0.5}, {"armShape", 0.5}}, 0.0},
        {"combo-pear", {{"hipWidth", 0.75}, {"waistWidth", -0.25}, {"shoulderWidth", -0.5}}, 0.0},
    };
    std::string profCsv = "profile,profileHash,";
    for (int i = 0; i < wp3r2::kMeasureCount; ++i) profCsv += std::string(wp3r2::measureName(static_cast<wp3r2::Measure>(i))) + ",";
    profCsv += "\n";
    for (const auto& pd : profs) {
        const auto a = alphaOf(pd.kv);
        const auto prof = wp3r2::makeProfile(fm, pd.name, a, pd.stature);
        ht::writeFile(outDir + "/profiles/" + pd.name + ".txt", wp3r2::profileToText(prof));
        profCsv += pd.name + "," + prof.profileHash + ",";
        for (int i = 0; i < wp3r2::kMeasureCount; ++i) profCsv += fmt("%.6f,", prof.achieved[i]);
        profCsv += "\n";
        const auto fr = fm.evaluate(a, pd.stature);
        renderBody("profile-" + pd.name, fr.body);
        say(fmt("[profile] %-18s hash=%s flips=%ld", pd.name.c_str(), prof.profileHash.substr(0, 12).c_str(), fr.quality.flippedFaces));
    }
    ht::writeFile(outDir + "/profiles.csv", profCsv);

    ht::writeFile(outDir + "/solve-cases.csv", csv);
    ht::writeFile(outDir + "/run.log", log);
    say("[done] WP3-R2 experiment complete");
    ht::writeFile(outDir + "/run.log", log);
    return 0;
}