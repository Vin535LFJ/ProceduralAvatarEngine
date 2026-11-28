#include "wp3_r2.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace pae::wp3r2 {
using ht::D3;
using ht::fmt;

const char* measureName(Measure m) {
    switch (m) {
        case Measure::Stature: return "stature";
        case Measure::ShoulderBreadth: return "shoulderBreadth";
        case Measure::RibcageBreadth: return "ribcageBreadth";
        case Measure::ChestDepth: return "chestDepth";
        case Measure::WaistBreadth: return "waistBreadth";
        case Measure::WaistCircumference: return "waistCircumference";
        case Measure::HipBreadth: return "hipBreadth";
        case Measure::HipCircumference: return "hipCircumference";
        default: return "?";
    }
}

const std::vector<ParamSpec>& solverParams() {
    static const std::vector<ParamSpec> p = {
        {"shoulderWidth", -1.0, 1.0}, {"ribcageWidth", -1.0, 1.0}, {"chestDepth", -1.0, 1.0},
        {"waistWidth", -1.0, 1.0}, {"hipWidth", -1.0, 1.0}, {"armShape", -1.0, 1.0},
    };
    return p;
}

// --- forward model ----------------------------------------------------------
namespace {
D3 scalePivot(const ht::ProofContext& c) {
    double xMin = 1e30, xMax = -1e30, yMin = 1e30;
    for (const D3& p : c.body.pos) { xMin = std::min(xMin, p.x); xMax = std::max(xMax, p.x); yMin = std::min(yMin, p.y); }
    const int ri = c.rigDeform.find("root");
    const double rootZ = ri >= 0 ? c.rigDeform.bones[static_cast<size_t>(ri)].head.z : 0.0;
    return D3{0.5 * (xMin + xMax), yMin, rootZ};
}
double baseStature(const ht::ProofContext& c) {
    double lo = 1e30, hi = -1e30;
    for (const D3& p : c.body.pos) { lo = std::min(lo, p.y); hi = std::max(hi, p.y); }
    return hi - lo;
}
void applyAlpha(const ht::ProofContext& c, const std::vector<double>& alpha, double targetStature,
                std::vector<D3>& full, std::vector<D3>& body, std::string& desc) {
    const auto& params = solverParams();
    std::vector<std::pair<const ht::ShapeTarget*, double>> weighted;
    desc.clear();
    for (size_t i = 0; i < params.size() && i < alpha.size(); ++i) {
        if (std::fabs(alpha[i]) < 1e-9) continue;
        const ht::ShapeParam* sp = nullptr;
        for (const ht::ShapeParam& pp : c.params) if (pp.key == params[i].key) sp = &pp;
        if (!sp) continue;
        weighted.push_back({alpha[i] >= 0 ? &sp->incr : &sp->decr, std::fabs(alpha[i])});
        desc += fmt("%s%+.3f ", params[i].key.c_str(), alpha[i]);
    }
    ht::applyToFull(c, weighted, full);
    if (targetStature > 0.0) {
        const D3 pivot = scalePivot(c);
        const double k = targetStature / baseStature(c);
        for (D3& p : full) p = pivot + (p - pivot) * k;
        desc += fmt("stature%.4f(k=%.4f) ", targetStature, k);
    }
    body.resize(c.bodyStableIds.size());
    for (size_t i = 0; i < c.bodyStableIds.size(); ++i) body[i] = full[static_cast<size_t>(c.bodyStableIds[i])];
    if (desc.empty()) desc = "(base)";
}
void mapMeasures(const wp3::Wp3Measures& m, double out[kMeasureCount]) {
    out[static_cast<int>(Measure::Stature)] = m.stature;
    out[static_cast<int>(Measure::ShoulderBreadth)] = m.biacromialProxy;
    out[static_cast<int>(Measure::RibcageBreadth)] = m.chestBreadth;
    out[static_cast<int>(Measure::ChestDepth)] = m.chestDepth;
    out[static_cast<int>(Measure::WaistBreadth)] = m.waistBreadth;
    out[static_cast<int>(Measure::WaistCircumference)] = m.waistCircumference;
    out[static_cast<int>(Measure::HipBreadth)] = m.hipBreadth;
    out[static_cast<int>(Measure::HipCircumference)] = m.hipCircumference;
}
} // namespace

void ForwardModel::measures(const std::vector<double>& alpha, double targetStature, double out[kMeasureCount]) const {
    std::vector<D3> full, body; std::string desc;
    applyAlpha(*c_, alpha, targetStature, full, body, desc);
    ht::Rig rig; ht::rigOnSurface(*c_, full, false, rig);
    const wp3::Wp3Measures m = wp3::measureAll(body, c_->regions, rig, c_->body.faces);
    mapMeasures(m, out);
}

ForwardResult ForwardModel::evaluate(const std::vector<double>& alpha, double targetStature) const {
    ForwardResult r;
    std::string desc;
    applyAlpha(*c_, alpha, targetStature, r.full, r.body, desc);
    r.alphaDesc = desc;
    ht::Rig rig; ht::rigOnSurface(*c_, r.full, false, rig);
    const wp3::Wp3Measures m = wp3::measureAll(r.body, c_->regions, rig, c_->body.faces);
    mapMeasures(m, r.measures);
    r.quality = ht::compareSurfaces(c_->body, r.body, c_->topo);
    return r;
}

// --- bounded Levenberg-Marquardt inverse solver -----------------------------
namespace {
double clampd(double x, double lo, double hi) { return std::max(lo, std::min(hi, x)); }

// relative residual vector over active measures (stature excluded)
void residual(const ForwardModel& fm, const std::vector<double>& x, const SolveRequest& req,
              std::vector<double>& r, std::vector<int>& idx, long& evals) {
    double meas[kMeasureCount];
    fm.measures(x, req.targetStature, meas);
    ++evals;
    r.clear(); idx.clear();
    for (int i = 0; i < kMeasureCount; ++i) {
        if (!req.active[i] || i == static_cast<int>(Measure::Stature)) continue;
        const double t = req.target[i];
        const double denom = std::fabs(t) > 1e-9 ? std::fabs(t) : 1.0;
        r.push_back((meas[i] - t) / denom);
        idx.push_back(i);
    }
}
double rms(const std::vector<double>& r) {
    if (r.empty()) return 0.0;
    double s = 0.0; for (double v : r) s += v * v;
    return std::sqrt(s / static_cast<double>(r.size()));
}
} // namespace

SolveResult solve(const ForwardModel& fm, const SolveRequest& req) {
    SolveResult res;
    const auto& params = solverParams();
    const int n = static_cast<int>(params.size());
    std::vector<double> x(static_cast<size_t>(n), 0.0);
    long evals = 0;
    std::vector<double> r; std::vector<int> idx;
    residual(fm, x, req, r, idx, evals);
    double cost = rms(r);
    double lambda = 1e-3;
    const int m = static_cast<int>(r.size());
    res.iterations = 0;
    for (int it = 0; it < req.maxIters; ++it) {
        res.iterations = it + 1;
        if (cost < req.relTol) break;
        // numerical Jacobian (central difference), respecting bounds
        const double h = 1e-3;
        std::vector<std::vector<double>> J(static_cast<size_t>(m), std::vector<double>(static_cast<size_t>(n), 0.0));
        for (int j = 0; j < n; ++j) {
            std::vector<double> xp = x, xm = x;
            xp[static_cast<size_t>(j)] = clampd(x[static_cast<size_t>(j)] + h, params[static_cast<size_t>(j)].minAlpha, params[static_cast<size_t>(j)].maxAlpha);
            xm[static_cast<size_t>(j)] = clampd(x[static_cast<size_t>(j)] - h, params[static_cast<size_t>(j)].minAlpha, params[static_cast<size_t>(j)].maxAlpha);
            const double dj = xp[static_cast<size_t>(j)] - xm[static_cast<size_t>(j)];
            if (std::fabs(dj) < 1e-12) continue;
            std::vector<double> rp, rm; std::vector<int> ip, im;
            residual(fm, xp, req, rp, ip, evals);
            residual(fm, xm, req, rm, im, evals);
            for (int i = 0; i < m; ++i) J[static_cast<size_t>(i)][static_cast<size_t>(j)] = (rp[static_cast<size_t>(i)] - rm[static_cast<size_t>(i)]) / dj;
        }
        // JtJ, Jtr
        std::vector<std::vector<double>> A(static_cast<size_t>(n), std::vector<double>(static_cast<size_t>(n), 0.0));
        std::vector<double> g(static_cast<size_t>(n), 0.0);
        for (int i = 0; i < m; ++i) {
            for (int a = 0; a < n; ++a) {
                g[static_cast<size_t>(a)] += J[static_cast<size_t>(i)][static_cast<size_t>(a)] * r[static_cast<size_t>(i)];
                for (int b = 0; b < n; ++b)
                    A[static_cast<size_t>(a)][static_cast<size_t>(b)] += J[static_cast<size_t>(i)][static_cast<size_t>(a)] * J[static_cast<size_t>(i)][static_cast<size_t>(b)];
            }
        }
        // Tikhonov regularization (minimum-norm bias). Parameters that do not
        // affect any targeted measurement (e.g. armShape vs the 8 semantic
        // measures) lie in the Jacobian null space; without this they drift
        // arbitrarily to a bound. reg keeps such parameters near 0 while the
        // identifiable parameters still fit the measurements. Acceptance is
        // judged on the measure-only residual, not on this penalty.
        const double reg = 1e-3;
        for (int a = 0; a < n; ++a) {
            A[static_cast<size_t>(a)][static_cast<size_t>(a)] += reg;
            g[static_cast<size_t>(a)] += reg * x[static_cast<size_t>(a)];
        }
        // LM inner loop: try damping until cost decreases
        bool accepted = false;
        for (int trial = 0; trial < 12; ++trial) {
            std::vector<std::vector<double>> M = A;
            for (int a = 0; a < n; ++a) M[static_cast<size_t>(a)][static_cast<size_t>(a)] += lambda * (A[static_cast<size_t>(a)][static_cast<size_t>(a)] + 1e-9);
            // solve M delta = -g (Gaussian elimination with partial pivot)
            std::vector<double> rhs(static_cast<size_t>(n));
            for (int a = 0; a < n; ++a) rhs[static_cast<size_t>(a)] = -g[static_cast<size_t>(a)];
            std::vector<std::vector<double>> E = M;
            std::vector<double> bb = rhs;
            bool singular = false;
            for (int col = 0; col < n; ++col) {
                int piv = col;
                for (int row = col + 1; row < n; ++row)
                    if (std::fabs(E[static_cast<size_t>(row)][static_cast<size_t>(col)]) > std::fabs(E[static_cast<size_t>(piv)][static_cast<size_t>(col)])) piv = row;
                if (std::fabs(E[static_cast<size_t>(piv)][static_cast<size_t>(col)]) < 1e-18) { singular = true; break; }
                std::swap(E[static_cast<size_t>(col)], E[static_cast<size_t>(piv)]);
                std::swap(bb[static_cast<size_t>(col)], bb[static_cast<size_t>(piv)]);
                for (int row = col + 1; row < n; ++row) {
                    const double f = E[static_cast<size_t>(row)][static_cast<size_t>(col)] / E[static_cast<size_t>(col)][static_cast<size_t>(col)];
                    for (int k = col; k < n; ++k) E[static_cast<size_t>(row)][static_cast<size_t>(k)] -= f * E[static_cast<size_t>(col)][static_cast<size_t>(k)];
                    bb[static_cast<size_t>(row)] -= f * bb[static_cast<size_t>(col)];
                }
            }
            std::vector<double> delta(static_cast<size_t>(n), 0.0);
            if (!singular) {
                for (int row = n - 1; row >= 0; --row) {
                    double s = bb[static_cast<size_t>(row)];
                    for (int k = row + 1; k < n; ++k) s -= E[static_cast<size_t>(row)][static_cast<size_t>(k)] * delta[static_cast<size_t>(k)];
                    delta[static_cast<size_t>(row)] = s / E[static_cast<size_t>(row)][static_cast<size_t>(row)];
                }
            }
            std::vector<double> xn(static_cast<size_t>(n));
            double stepNorm = 0.0;
            for (int a = 0; a < n; ++a) {
                xn[static_cast<size_t>(a)] = clampd(x[static_cast<size_t>(a)] + delta[static_cast<size_t>(a)], params[static_cast<size_t>(a)].minAlpha, params[static_cast<size_t>(a)].maxAlpha);
                stepNorm += std::fabs(xn[static_cast<size_t>(a)] - x[static_cast<size_t>(a)]);
            }
            std::vector<double> rn; std::vector<int> in;
            residual(fm, xn, req, rn, in, evals);
            const double costn = rms(rn);
            if (costn < cost) {
                x = xn; r = rn; cost = costn; lambda = std::max(lambda / 3.0, 1e-9); accepted = true;
                if (stepNorm < 1e-6) { res.iterations = it + 1; goto done; }
                break;
            } else {
                lambda *= 4.0;
            }
        }
        if (!accepted) break;  // cannot improve
    }
done:
    res.alpha = x;
    res.forwardEvals = evals;
    double meas[kMeasureCount];
    fm.measures(x, req.targetStature, meas);
    ++res.forwardEvals;
    double s = 0.0; int cnt = 0;
    for (int i = 0; i < kMeasureCount; ++i) {
        res.achieved[i] = meas[i];
        if (req.active[i] && i != static_cast<int>(Measure::Stature)) {
            const double t = req.target[i];
            const double denom = std::fabs(t) > 1e-9 ? std::fabs(t) : 1.0;
            res.relResidual[i] = (meas[i] - t) / denom;
            s += res.relResidual[i] * res.relResidual[i]; ++cnt;
        }
    }
    res.rmsRelResidual = cnt ? std::sqrt(s / static_cast<double>(cnt)) : 0.0;
    res.reachable = res.rmsRelResidual < req.relTol;
    res.status = res.reachable ? "REACHABLE" : "UNREACHABLE_NEAREST";
    // quality at solution
    const ForwardResult fr = fm.evaluate(x, req.targetStature);
    res.quality = fr.quality;
    return res;
}

// --- profile ----------------------------------------------------------------
ShapeProfile makeProfile(const ForwardModel& fm, const std::string& name,
                         const std::vector<double>& alpha, double targetStature) {
    ShapeProfile p;
    p.name = name;
    p.templateSha = fm.context().body.sourceSha256;
    p.shapeTargetHash = fm.context().rigFile.sha256;  // target/rig source identity
    p.alpha = alpha;
    p.targetStature = targetStature;
    fm.measures(alpha, targetStature, p.achieved);
    std::string blob = name + "|";
    for (double a : alpha) blob += fmt("%.6f,", a);
    blob += fmt("|%.6f", targetStature);
    std::vector<uint8_t> bytes(blob.begin(), blob.end());
    p.profileHash = ht::sha256Hex(bytes);
    return p;
}

std::string profileToText(const ShapeProfile& p) {
    std::string s = "# ShapeProfile " + p.name + "\n";
    s += "templateSha: " + p.templateSha + "\n";
    s += "shapeTargetHash: " + p.shapeTargetHash + "\n";
    s += "profileHash: " + p.profileHash + "\n";
    s += fmt("targetStature: %.6f\n", p.targetStature);
    const auto& params = solverParams();
    for (size_t i = 0; i < params.size() && i < p.alpha.size(); ++i)
        s += fmt("alpha %s: %+.6f\n", params[i].key.c_str(), p.alpha[i]);
    for (int i = 0; i < kMeasureCount; ++i)
        s += fmt("achieved %s: %.6f\n", measureName(static_cast<Measure>(i)), p.achieved[i]);
    return s;
}

} // namespace pae::wp3r2