#pragma once

#include "core/math/Math.h"

#include <vector>

namespace pae::character {

struct SweepFrame {
    Vec3 tangent{0.0f, 1.0f, 0.0f};
    Vec3 u{1.0f, 0.0f, 0.0f};
    Vec3 v{0.0f, 0.0f, 1.0f};
};

struct SweepDiagnostics {
    bool valid = false;
    int tangentReversals = 0;
    float minOrthonormality = 1.0f;
    float minTangentDot = 1.0f;
    float minHandedness = 1.0f;
};

std::vector<SweepFrame> computeRmfFrames(const std::vector<Vec3>& points,
                                         const Vec3& initialNormal,
                                         SweepDiagnostics* diagnostics = nullptr);

// Closed periodic Catmull-Rom interpolation over normalized control values.
float periodicCatmullRom(const std::vector<float>& controls, float u);

Vec2 sampleClosedProfile(const std::vector<float>& radialControls,
                         float u, float halfWidth, float halfDepth);

} // namespace pae::character