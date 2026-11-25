#include "Anthropometry.h"

#include <algorithm>

namespace pae::character {

Anthropometry resolveAnthropometry(const shape::v2::CharacterDefinition& definition) {
    const shape::v2::ProportionProfile& p = definition.proportions;
    Anthropometry a;
    a.heightM = p.statureM;
    a.shoulderBreadthM = p.shoulderBreadthM();
    a.chestWidthM = p.chestHalfWidthM() * 2.0f;
    a.chestDepthM = p.chestDepthM();
    float waistHalfWidth = 0.0f;
    float waistHalfDepth = 0.0f;
    p.waistHalfAxes(&waistHalfWidth, &waistHalfDepth);
    a.waistWidthM = waistHalfWidth * 2.0f;
    a.waistDepthM = waistHalfDepth * 2.0f;
    a.hipWidthM = p.hipBreadthM();
    a.upperArmLengthM = p.statureM * p.upperArmLengthRatio;
    a.forearmLengthM = p.statureM * p.forearmLengthRatio;
    a.upperLegLengthM = p.statureM * p.upperLegLengthRatio;
    a.lowerLegLengthM = p.statureM * p.lowerLegLengthRatio;
    a.handLengthM = p.statureM * p.handLengthRatio;
    a.footLengthM = p.statureM * p.footLengthRatio;
    return a;
}

} // namespace pae::character