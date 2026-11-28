#pragma once

// VR-1B WP3-R2 (M1): Human Parameter Solver. Host-only.
// Forward model F(alpha, stature) -> Measurements + GeometryQuality, and a
// bounded nonlinear least-squares inverse solver (Levenberg-Marquardt with
// numerical Jacobian). Preserves target incr/decr asymmetry, handles non-additive
// coupling by iteration, and reports unreachable targets explicitly. Every
// evaluation starts from the fixed base mesh (never accumulates prior output).
// Reuses the WP3-HOST measurement protocol; no Android / production path touched.

#include "ht_common.h"
#include "ht_proof.h"
#include "ht_topology.h"
#include "wp3_measure.h"

#include <string>
#include <vector>

namespace pae::wp3r2 {

// The 8 semantic measurements (M1-C). Stature is achieved exactly by the PAE
// sole-anchored uniform scale; the other 7 are solved via shape targets.
enum class Measure {
    Stature = 0, ShoulderBreadth, RibcageBreadth, ChestDepth,
    WaistBreadth, WaistCircumference, HipBreadth, HipCircumference, Count
};
constexpr int kMeasureCount = static_cast<int>(Measure::Count);
const char* measureName(Measure m);

// Shape parameter keys the solver actuates (alpha in the measured effective
// range [-1, +1]; NOT extended without validation).
struct ParamSpec {
    std::string key;
    double minAlpha = -1.0, maxAlpha = 1.0;
};
const std::vector<ParamSpec>& solverParams();

struct ForwardResult {
    double measures[kMeasureCount] = {0};
    ht::DeformationQuality quality;
    std::vector<ht::D3> body, full;
    std::string alphaDesc;
};

class ForwardModel {
public:
    explicit ForwardModel(const ht::ProofContext& ctx) : c_(&ctx) {}
    // alpha aligned with solverParams(); targetStature <= 0 => keep base stature.
    ForwardResult evaluate(const std::vector<double>& alpha, double targetStature) const;
    // Lightweight measures-only evaluation for solver iterations (no quality/body).
    void measures(const std::vector<double>& alpha, double targetStature, double out[kMeasureCount]) const;
    const ht::ProofContext& context() const { return *c_; }
private:
    const ht::ProofContext* c_;
};

struct SolveRequest {
    double target[kMeasureCount] = {0};
    bool active[kMeasureCount] = {false};  // which measurements to fit
    double targetStature = 0.0;            // exact via uniform scale (0 = base)
    double relTol = 0.01;                  // 1% relative residual => reachable
    int maxIters = 60;
};

struct SolveResult {
    std::vector<double> alpha;
    double achieved[kMeasureCount] = {0};
    double relResidual[kMeasureCount] = {0};
    double rmsRelResidual = 0.0;
    bool reachable = false;
    int iterations = 0;
    long forwardEvals = 0;
    std::string status;
    ht::DeformationQuality quality;
};

// Bounded LM inverse solve. active[Stature] is ignored (stature is exact).
SolveResult solve(const ForwardModel& fm, const SolveRequest& req);

// A saveable, reproducible body-shape profile (M1-D).
struct ShapeProfile {
    std::string name;
    std::string templateSha;     // base mesh / template identity
    std::string shapeTargetHash; // target set identity
    std::vector<double> alpha;   // aligned with solverParams()
    double targetStature = 0.0;
    double achieved[kMeasureCount] = {0};
    std::string profileHash;     // hash of (name, alpha, stature)
};
ShapeProfile makeProfile(const ForwardModel& fm, const std::string& name,
                         const std::vector<double>& alpha, double targetStature);
std::string profileToText(const ShapeProfile& p);

} // namespace pae::wp3r2