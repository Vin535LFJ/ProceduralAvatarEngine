#include "LocalShapeBasis.h"

#include <algorithm>
#include <cmath>

namespace pae::character {

float compactFalloff(float distance, float radius) {
    distance = std::fabs(distance);
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

Vec3 evaluateDirectional(const DirectionalShapeBasis& basis, float station, float angle) {
    const bool leftSide = angle < 0.5f;
    if ((leftSide && (basis.sideMask & 1u) == 0) ||
        (!leftSide && (basis.sideMask & 2u) == 0)) {
        return {};
    }
    float angleDistance = std::fabs(angle - basis.angle);
    angleDistance = std::fmin(angleDistance, 1.0f - angleDistance);
    const float weight = compactFalloff(station - basis.station, basis.stationRadius) *
                         compactFalloff(angleDistance, basis.angleRadius);
    return basis.direction.normalized(Vec3{0.0f, 0.0f, 1.0f}) *
           basis.amplitude * weight;
}

} // namespace pae::character