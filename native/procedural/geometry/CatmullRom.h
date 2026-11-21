#pragma once

// Uniform Catmull-Rom spline sampling (endpoint-clamped control points).
// Pure functions: identical inputs always produce identical outputs.

#include "core/math/Math.h"

#include <cstddef>
#include <vector>

namespace pae::spline {

// Point on the Catmull-Rom segment (p1 -> p2) at t in [0, 1].
inline Vec3 catmullRom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return (p1 * 2.0f +
            (p2 - p0) * t +
            (p0 * 2.0f + p1 * -5.0f + p2 * 4.0f + p3 * -1.0f) * t2 +
            (p0 * -1.0f + p1 * 3.0f + p2 * -3.0f + p3) * t3) * 0.5f;
}

inline float catmullRom(float v0, float v1, float v2, float v3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.0f * v1) + (-v0 + v2) * t +
                   (2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 +
                   (-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3);
}

// Resample a control polyline into (n-1)*samples + 1 points that pass
// through every control point. samplesPerSegment >= 1.
inline std::vector<Vec3> resampleCatmullRom(const std::vector<Vec3>& points, int samplesPerSegment) {
    const size_t n = points.size();
    if (n < 2 || samplesPerSegment < 1) return points;
    std::vector<Vec3> out;
    out.reserve((n - 1) * static_cast<size_t>(samplesPerSegment) + 1);
    for (size_t seg = 0; seg + 1 < n; ++seg) {
        const Vec3& p0 = points[seg == 0 ? 0 : seg - 1];
        const Vec3& p1 = points[seg];
        const Vec3& p2 = points[seg + 1];
        const Vec3& p3 = points[seg + 2 < n ? seg + 2 : n - 1];
        for (int s = 0; s < samplesPerSegment; ++s) {
            out.push_back(catmullRom(p0, p1, p2, p3, static_cast<float>(s) / static_cast<float>(samplesPerSegment)));
        }
    }
    out.push_back(points[n - 1]);
    return out;
}

// Component-wise scalar variant with the same endpoint handling.
inline std::vector<float> resampleCatmullRomScalar(const std::vector<float>& values, int samplesPerSegment) {
    const size_t n = values.size();
    if (n < 2 || samplesPerSegment < 1) return values;
    std::vector<float> out;
    out.reserve((n - 1) * static_cast<size_t>(samplesPerSegment) + 1);
    for (size_t seg = 0; seg + 1 < n; ++seg) {
        const float v0 = values[seg == 0 ? 0 : seg - 1];
        const float v1 = values[seg];
        const float v2 = values[seg + 1];
        const float v3 = values[seg + 2 < n ? seg + 2 : n - 1];
        for (int s = 0; s < samplesPerSegment; ++s) {
            out.push_back(catmullRom(v0, v1, v2, v3, static_cast<float>(s) / static_cast<float>(samplesPerSegment)));
        }
    }
    out.push_back(values[n - 1]);
    return out;
}

} // namespace pae::spline
