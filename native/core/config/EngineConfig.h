#pragma once

#include <cstdint>
#include <string>

namespace pae {

enum class QualityLevel : int { Low = 0, Medium = 1, High = 2 };

struct EngineConfig {
    QualityLevel quality = QualityLevel::Medium;
    uint32_t maxQueuedCommands = 64;
    uint32_t maxInFlightFrames = 1;
    float targetFps = 30.0f;
};

// Returns true when all fields are inside safe, documented ranges.
bool validateEngineConfig(const EngineConfig& config, std::string* reason);

const char* qualityName(QualityLevel level);

// Geometry detail level for a quality tier: Low=0, Medium=1, High=2.
// Feeds BodyParameters::detailLevel (clamped 0..3 inside the generator).
uint32_t detailLevelForQuality(QualityLevel level);

} // namespace pae
