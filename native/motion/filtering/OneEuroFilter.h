#pragma once

#include "core/math/Math.h"

namespace pae {

// Standard One Euro Filter: low cutoff when static, higher cutoff when moving.
class OneEuroFilter {
public:
    OneEuroFilter(float minCutoff = 1.0f, float beta = 0.05f, float dCutoff = 1.0f)
        : minCutoff_(minCutoff), beta_(beta), dCutoff_(dCutoff) {}

    void reset() { initialized_ = false; }

    float filter(float value, float dt) {
        if (!std::isfinite(value) || !std::isfinite(dt) || dt <= 0.0f) return value;
        const float dx = initialized_ ? (value - xPrev_) / dt : 0.0f;
        const float edx = alpha(dCutoff_, dt) * dx + (1.0f - alpha(dCutoff_, dt)) * dxPrev_;
        const float cutoff = minCutoff_ + beta_ * std::fabs(edx);
        const float result = initialized_
            ? alpha(cutoff, dt) * value + (1.0f - alpha(cutoff, dt)) * xPrev_
            : value;
        xPrev_ = result;
        dxPrev_ = edx;
        initialized_ = true;
        return result;
    }

private:
    static float alpha(float cutoff, float dt) {
        const float tau = 1.0f / (2.0f * 3.14159265f * cutoff);
        return 1.0f / (1.0f + tau / dt);
    }

    float minCutoff_;
    float beta_;
    float dCutoff_;
    float xPrev_ = 0.0f;
    float dxPrev_ = 0.0f;
    bool initialized_ = false;
};

} // namespace pae
