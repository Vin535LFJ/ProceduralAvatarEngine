#include "VariableProfileSweep.h"

#include <algorithm>
#include <cmath>

namespace pae::character {
namespace {

constexpr float kTwoPi = 6.28318530717958647692f;

} // namespace

std::vector<SweepFrame> computeRmfFrames(const std::vector<Vec3>& points,
                                         const Vec3& initialNormal,
                                         SweepDiagnostics* diagnostics) {
    SweepDiagnostics local;
    std::vector<SweepFrame> frames;
    if (points.size() < 2) {
        if (diagnostics) *diagnostics = local;
        return frames;
    }
    frames.reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        const Vec3 prev = i == 0 ? points[i] : points[i - 1];
        const Vec3 next = i + 1 < points.size() ? points[i + 1] : points[i];
        Vec3 tangent = (next - prev).normalized({0.0f, 1.0f, 0.0f});
        if (i > 0) {
            local.minTangentDot = std::fmin(local.minTangentDot,
                                            tangent.dot(frames.back().tangent));
            if (tangent.dot(frames.back().tangent) < -0.95f) ++local.tangentReversals;
        }

        Vec3 u;
        if (i == 0) {
            u = initialNormal - tangent * tangent.dot(initialNormal);
            if (u.lengthSq() < 1e-8f) {
                const Vec3 fallback = std::fabs(tangent.y) < 0.9f
                    ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
                u = fallback - tangent * tangent.dot(fallback);
            }
            u = u.normalized({1.0f, 0.0f, 0.0f});
        } else {
            const SweepFrame& previous = frames.back();
            const Quat transport = Quat::fromToRotation(previous.tangent, tangent);
            u = transport.rotate(previous.u);
            u = u - tangent * tangent.dot(u);
            if (u.lengthSq() < 1e-8f) u = previous.u;
            u = u.normalized({1.0f, 0.0f, 0.0f});
        }
        Vec3 v = tangent.cross(u).normalized({0.0f, 0.0f, 1.0f});
        if (u.cross(v).dot(tangent) < 0.0f) {
            v = v * -1.0f;
        }
        local.minOrthonormality = std::fmin(local.minOrthonormality, std::fabs(u.dot(v)));
        local.minOrthonormality = std::fmin(local.minOrthonormality, std::fabs(u.dot(tangent)));
        local.minOrthonormality = std::fmin(local.minOrthonormality, std::fabs(v.dot(tangent)));
        local.minHandedness = std::fmin(local.minHandedness, u.cross(v).dot(tangent));
        frames.push_back({tangent, u, v});
    }
    local.valid = local.tangentReversals == 0 &&
                  local.minTangentDot > -0.95f &&
                  local.minOrthonormality < 1e-3f &&
                  local.minHandedness > 0.99f;
    if (diagnostics) *diagnostics = local;
    return frames;
}

float periodicCatmullRom(const std::vector<float>& controls, float u) {
    if (controls.empty()) return 1.0f;
    const size_t count = controls.size();
    const float wrapped = u - std::floor(u);
    const float position = wrapped * static_cast<float>(count);
    const size_t index = static_cast<size_t>(std::floor(position)) % count;
    const float t = position - std::floor(position);
    const float p0 = controls[(index + count - 1) % count];
    const float p1 = controls[index];
    const float p2 = controls[(index + 1) % count];
    const float p3 = controls[(index + 2) % count];
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

Vec2 sampleClosedProfile(const std::vector<float>& radialControls,
                         float u, float halfWidth, float halfDepth) {
    const float scale = periodicCatmullRom(radialControls, u);
    const float theta = u * kTwoPi;
    return {std::cos(theta) * halfWidth * scale,
            std::sin(theta) * halfDepth * scale};
}

} // namespace pae::character