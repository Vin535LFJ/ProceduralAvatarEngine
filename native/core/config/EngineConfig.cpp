#include "EngineConfig.h"

#include <cmath>

namespace pae {

bool validateEngineConfig(const EngineConfig& config, std::string* reason) {
    auto fail = [reason](const char* message) {
        if (reason) *reason = message;
        return false;
    };
    if (static_cast<int>(config.quality) < 0 || static_cast<int>(config.quality) > 2) {
        return fail("quality must be Low(0), Medium(1) or High(2)");
    }
    if (config.maxQueuedCommands < 1 || config.maxQueuedCommands > 256) {
        return fail("maxQueuedCommands must be in [1, 256]");
    }
    if (config.maxInFlightFrames < 1 || config.maxInFlightFrames > 8) {
        return fail("maxInFlightFrames must be in [1, 8]");
    }
    if (!std::isfinite(config.targetFps) || config.targetFps <= 0.0f || config.targetFps > 240.0f) {
        return fail("targetFps must be finite and in (0, 240]");
    }
    if (reason) reason->clear();
    return true;
}

const char* qualityName(QualityLevel level) {
    switch (level) {
        case QualityLevel::Low: return "Low";
        case QualityLevel::Medium: return "Medium";
        case QualityLevel::High: return "High";
    }
    return "Unknown";
}

uint32_t detailLevelForQuality(QualityLevel level) {
    switch (level) {
        case QualityLevel::Low: return 0u;
        case QualityLevel::Medium: return 1u;
        case QualityLevel::High: return 2u;
    }
    return 1u;
}

} // namespace pae
