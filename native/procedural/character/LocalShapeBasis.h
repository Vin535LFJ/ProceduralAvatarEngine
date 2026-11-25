#pragma once

#include "AnatomicalRegions.h"
#include "core/math/Math.h"

namespace pae::character {

enum class BasisKind : uint8_t { Relief = 0, Concavity = 1 };

struct LocalShapeBasis {
    RegionId region = RegionId::Thorax;
    BasisKind kind = BasisKind::Relief;
    Vec3 center{};
    float radius = 0.10f;
    float amplitude = 0.0f;
    Vec3 normal{0.0f, 0.0f, 1.0f};

    float evaluate(const Vec3& point) const;
};

// Smooth compact-support falloff: value 0 at radius, peak 1 at center, C1 at
// the support boundary.
float compactFalloff(float distance, float radius);

} // namespace pae::character