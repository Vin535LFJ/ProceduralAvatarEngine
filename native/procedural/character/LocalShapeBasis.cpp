#include "LocalShapeBasis.h"

#include <algorithm>
#include <cmath>

namespace pae::character {

float compactFalloff(float distance, float radius) {
    if (!(radius > 1e-6f) || distance >= radius) return 0.0f;
    const float t = 1.0f - distance / radius;
    return t * t * (3.0f - 2.0f * t);
}

float LocalShapeBasis::evaluate(const Vec3& point) const {
    const float weight = compactFalloff((point - center).length(), radius);
    if (weight <= 0.0f) return 0.0f;
    const float signedAmplitude = kind == BasisKind::Concavity ? -std::fabs(amplitude) : amplitude;
    return signedAmplitude * weight;
}

} // namespace pae::character