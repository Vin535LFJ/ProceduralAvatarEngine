#pragma once

#include "procedural/shape/v2/CharacterDefinition.h"

namespace pae::character {

// Resolved absolute measurements derived from the editable ProportionProfile.
struct Anthropometry {
    float heightM = 1.75f;
    float shoulderBreadthM = 0.50f;
    float chestWidthM = 0.36f;
    float chestDepthM = 0.24f;
    float waistWidthM = 0.30f;
    float waistDepthM = 0.22f;
    float hipWidthM = 0.34f;
    float upperArmLengthM = 0.32f;
    float forearmLengthM = 0.29f;
    float upperLegLengthM = 0.45f;
    float lowerLegLengthM = 0.44f;
    float handLengthM = 0.19f;
    float footLengthM = 0.26f;
};

Anthropometry resolveAnthropometry(const shape::v2::CharacterDefinition& definition);

} // namespace pae::character