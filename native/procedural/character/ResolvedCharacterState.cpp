#include "ResolvedCharacterState.h"

#include "procedural/humanoid/Humanoid.h"

#include <algorithm>

namespace pae::character {
namespace {

std::vector<shape::v2::TorsoControlStation> resolveTorsoStations(
        const shape::v2::CharacterDefinition& definition,
        const Anthropometry& anthropometry,
        const BodyJoints& joints,
        const AnatomicalLandmarkSet& landmarks) {
    std::vector<shape::v2::TorsoControlStation> stations = definition.torso.controlStations;
    const float scale = anthropometry.heightM / 1.7549f;
    for (shape::v2::TorsoControlStation& station : stations) {
        station.controls.front.x *= scale;
        station.controls.front.z *= scale;
        station.controls.back.x *= scale;
        station.controls.back.z *= scale;
        station.controls.left.x *= scale;
        station.controls.left.z *= scale;
        station.controls.right.x *= scale;
        station.controls.right.z *= scale;
        station.controls.frontLeft.x *= scale;
        station.controls.frontLeft.z *= scale;
        station.controls.frontRight.x *= scale;
        station.controls.frontRight.z *= scale;
        station.controls.backLeft.x *= scale;
        station.controls.backLeft.z *= scale;
        station.controls.backRight.x *= scale;
        station.controls.backRight.z *= scale;
    }

    const float yBottom = joints.hips.y + 0.010f;
    const float yNeck = joints.neck.y;
    const std::pair<const char*, LandmarkId> bindings[] = {
        {"iliac-crest", LandmarkId::IliacCrestCenter},
        {"umbilicus", LandmarkId::Umbilicus},
        {"costal-margin", LandmarkId::CostalMargin},
        {"lower-rib-cage", LandmarkId::LowerRibCageCenter},
        {"chest-center", LandmarkId::ChestCenter},
        {"upper-rib-cage", LandmarkId::UpperRibCageCenter},
        {"neck-base-interface", LandmarkId::NeckBaseInterface},
    };
    for (const auto& binding : bindings) {
        const LandmarkPoint* landmark = landmarks.find(binding.second);
        if (!landmark || landmark->position.y < yBottom || landmark->position.y > yNeck) continue;
        for (shape::v2::TorsoControlStation& station : stations) {
            if (station.id != binding.first) continue;
            station.height = (landmark->position.y - yBottom) / std::max(yNeck - yBottom, 1e-4f);
            const float deltaZ = landmark->position.z - station.controls.front.z;
            station.controls.front.z += deltaZ;
            station.controls.back.z += deltaZ;
            station.controls.left.z += deltaZ;
            station.controls.right.z += deltaZ;
            station.controls.frontLeft.z += deltaZ;
            station.controls.frontRight.z += deltaZ;
            station.controls.backLeft.z += deltaZ;
            station.controls.backRight.z += deltaZ;
            station.confidence = landmark->confidence;
            break;
        }
    }
    return stations;
}

} // namespace

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
    state.torsoStations = resolveTorsoStations(definition, state.anthropometry,
                                               state.joints, state.landmarks);
    bool stationsValid = true;
    for (size_t i = 1; i < state.torsoStations.size(); ++i) {
        if (state.torsoStations[i].height <= state.torsoStations[i - 1].height) {
            stationsValid = false;
            break;
        }
    }
    state.valid = state.landmarkReport.valid && stationsValid &&
                  state.skeleton.bones[BoneChest].tail.y > state.skeleton.bones[BoneSpine].tail.y;
    if (!state.valid) state.error = "resolved character state is invalid";
    return state;
}

} // namespace pae::character