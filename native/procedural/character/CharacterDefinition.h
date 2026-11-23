#pragma once

#include <cstdint>

namespace pae {

// Versioned P1-A PoC parameters. All dimensions are in meters.
struct CharacterDefinition {
    uint32_t version = 1;
    uint32_t detailLevel = 1;
    float pelvisWidth = 0.34f;
    float pelvisDepth = 0.22f;
    float waistWidth = 0.28f;
    float waistDepth = 0.18f;
    float chestWidth = 0.38f;
    float chestDepth = 0.24f;
    float shoulderWidth = 0.44f;
    float armWidth = 0.076f;
    float armDepth = 0.062f;
    float elbowWidth = 0.088f;
    float forearmWidth = 0.055f;
    uint32_t torsoRadialSegments = 32;
    uint32_t armRadialSegments = 32;
    uint32_t armSamplesPerSegment = 6;
    uint64_t seed = 20261123;
};

} // namespace pae
