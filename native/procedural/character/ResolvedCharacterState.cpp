#include "ResolvedCharacterState.h"

#include "procedural/humanoid/Humanoid.h"

#include <algorithm>

namespace pae::character {

ResolvedCharacterState resolveCharacterState(
    const shape::v2::CharacterDefinition& definition,
    const BodyJoints* legacyJoints) {
    ResolvedCharacterState state;
    state.anthropometry = resolveAnthropometry(definition);
    state.regions = torsoShoulderRegions();

    BodyParameters body;
    body.height = state.anthropometry.heightM;
    body.shoulderWidth = state.anthropometry.shoulderBreadthM;
    body.chestWidth = state.anthropometry.chestWidthM;
    body.waistWidth = state.anthropometry.waistWidthM;
    body.hipWidth = state.anthropometry.hipWidthM;
    body.upperArmLength = state.anthropometry.upperArmLengthM;
    body.lowerArmLength = state.anthropometry.forearmLengthM;
    body.upperLegLength = state.anthropometry.upperLegLengthM;
    body.lowerLegLength = state.anthropometry.lowerLegLengthM;
    body.handScale = state.anthropometry.handLengthM / 0.19f;
    body.footLength = state.anthropometry.footLengthM;
    body.headHeight = definition.proportions.headHeightM();
    body.neckLength = 0.082f;
    body.detailLevel = 1;

    const BodyParameters clamped = clampBodyParameters(body);
    const float estimatedHeight = clamped.upperLegLength + clamped.lowerLegLength + 0.08f +
                                  clamped.torsoLength + clamped.neckLength + clamped.headHeight;
    const float scale = std::clamp(clamped.height / std::max(estimatedHeight, 0.1f), 0.5f, 1.8f);
    state.joints = legacyJoints ? *legacyJoints : deriveJoints(clamped, scale);
    state.skeleton = buildHumanoidSkeleton(state.joints);
    state.landmarks = resolveLandmarks(definition, state.anthropometry, state.joints,
                                       legacyJoints != nullptr, &state.landmarkReport);
    state.valid = state.landmarkReport.valid &&
                  state.skeleton.bones[BoneChest].tail.y > state.skeleton.bones[BoneSpine].tail.y;
    if (!state.valid) state.error = "resolved character state is invalid";
    return state;
}

} // namespace pae::character