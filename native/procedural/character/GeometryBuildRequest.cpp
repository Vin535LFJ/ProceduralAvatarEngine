#include "GeometryBuildRequest.h"

#include <cstring>
#include <sstream>

namespace pae::character {
namespace {

void hashBytes(uint64_t& hash, const void* data, size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
}

void hashFloat(uint64_t& hash, float value) {
    hashBytes(hash, &value, sizeof(value));
}

} // namespace

void hashString(uint64_t& hash, const std::string& value) {
    hashBytes(hash, value.data(), value.size());
    hashBytes(hash, "\0", 1);
}

void hashVec3(uint64_t& hash, const Vec3& value) {
    hashFloat(hash, value.x);
    hashFloat(hash, value.y);
    hashFloat(hash, value.z);
}

void hashAngularControls(uint64_t& hash, const shape::v2::TorsoAngularControls& controls) {
    hashVec3(hash, controls.front);
    hashVec3(hash, controls.frontLeft);
    hashVec3(hash, controls.left);
    hashVec3(hash, controls.backLeft);
    hashVec3(hash, controls.back);
    hashVec3(hash, controls.backRight);
    hashVec3(hash, controls.right);
    hashVec3(hash, controls.frontRight);
}

uint64_t hashCharacterDefinition(const shape::v2::CharacterDefinition& definition) {
    uint64_t hash = 1469598103934665603ull;
    hashBytes(hash, &definition.schemaVersion, sizeof(definition.schemaVersion));
    hashBytes(hash, &definition.seed, sizeof(definition.seed));
    const shape::v2::ProportionProfile& p = definition.proportions;
    hashFloat(hash, p.statureM);
    hashFloat(hash, p.shoulderBreadthRatio);
    hashFloat(hash, p.hipBreadthRatio);
    hashFloat(hash, p.chestDepthRatio);
    hashFloat(hash, p.waistCircumferenceRatio);
    hashFloat(hash, p.waistAspectRatio);
    hashFloat(hash, p.upperArmLengthRatio);
    hashFloat(hash, p.forearmLengthRatio);
    hashFloat(hash, p.upperLegLengthRatio);
    hashFloat(hash, p.lowerLegLengthRatio);
    if (definition.schemaVersion <= kR1GeneratorVersion) {
        hashFloat(hash, definition.torso.ribcageWidthScale);
        hashFloat(hash, definition.torso.chestDepthScale);
        hashFloat(hash, definition.torso.waistPinch);
        hashFloat(hash, definition.torso.upperPelvisWidthScale);
        hashFloat(hash, definition.torso.shoulderGirdleMass);
        hashFloat(hash, definition.torso.axillaDepth);
        hashFloat(hash, definition.shoulders.leftWidthScale);
        hashFloat(hash, definition.shoulders.rightWidthScale);
        hashFloat(hash, definition.shoulders.deltoidVolume);
        hashFloat(hash, definition.upperArms.leftRadiusScale);
        hashFloat(hash, definition.upperArms.rightRadiusScale);
        hashFloat(hash, definition.upperArms.elbowRadiusScale);
        hashFloat(hash, definition.asymmetry.leftShoulderWidthDelta);
        hashFloat(hash, definition.asymmetry.rightShoulderWidthDelta);
        return hash;
    }

    hashFloat(hash, p.biacromialRatio);
    hashFloat(hash, p.abdominalDepthRatio);
    hashFloat(hash, p.headHeightRatio);
    hashFloat(hash, p.handLengthRatio);
    hashFloat(hash, p.footLengthRatio);
    hashFloat(hash, definition.torso.ribcageWidthScale);
    hashFloat(hash, definition.torso.chestDepthScale);
    hashFloat(hash, definition.torso.waistPinch);
    hashFloat(hash, definition.torso.upperPelvisWidthScale);
    hashFloat(hash, definition.torso.shoulderGirdleMass);
    hashFloat(hash, definition.torso.axillaDepth);
    hashFloat(hash, definition.torso.clavicleRelief);
    hashFloat(hash, definition.torso.pectoralRelief);
    hashFloat(hash, definition.torso.ribcageRelief);
    hashFloat(hash, definition.torso.abdominalContour);
    hashFloat(hash, definition.torso.obliqueRelief);
    hashFloat(hash, definition.torso.thoracicBackContour);
    hashFloat(hash, definition.torso.scapularRelief);
    hashFloat(hash, definition.torso.trapeziusTransition);
    hashFloat(hash, definition.shoulders.leftWidthScale);
    hashFloat(hash, definition.shoulders.rightWidthScale);
    hashFloat(hash, definition.shoulders.deltoidVolume);
    hashFloat(hash, definition.shoulders.biacromialWidth);
    hashFloat(hash, definition.shoulders.yokeWidth);
    hashFloat(hash, definition.shoulders.externalWidth);
    hashFloat(hash, definition.shoulders.acromionY);
    hashFloat(hash, definition.shoulders.rootY);
    hashFloat(hash, definition.shoulders.deltoidSurfaceY);
    hashFloat(hash, definition.shoulders.deltoidAnterior);
    hashFloat(hash, definition.shoulders.deltoidLateral);
    hashFloat(hash, definition.shoulders.deltoidPosterior);
    hashFloat(hash, definition.upperArms.leftRadiusScale);
    hashFloat(hash, definition.upperArms.rightRadiusScale);
    hashFloat(hash, definition.upperArms.elbowRadiusScale);
    hashFloat(hash, definition.asymmetry.leftShoulderWidthDelta);
    hashFloat(hash, definition.asymmetry.rightShoulderWidthDelta);
    hashFloat(hash, definition.asymmetry.leftUpperArmRadiusDelta);
    hashFloat(hash, definition.asymmetry.rightUpperArmRadiusDelta);

    const uint32_t stationCount = static_cast<uint32_t>(definition.torso.controlStations.size());
    hashBytes(hash, &stationCount, sizeof(stationCount));
    for (const shape::v2::TorsoControlStation& station : definition.torso.controlStations) {
        hashString(hash, station.id);
        hashFloat(hash, station.height);
        hashAngularControls(hash, station.controls);
        hashBytes(hash, &station.confidence, sizeof(station.confidence));
    }
    const uint32_t landmarkCount = static_cast<uint32_t>(definition.landmarks.placed.size());
    hashBytes(hash, &landmarkCount, sizeof(landmarkCount));
    for (const shape::v2::LandmarkPoint& landmark : definition.landmarks.placed) {
        hashString(hash, landmark.name);
        hashVec3(hash, landmark.position);
        hashBytes(hash, &landmark.confidence, sizeof(landmark.confidence));
        hashString(hash, landmark.source);
    }
    return hash;
}

std::string GeometryBuildRequest::cacheKey() const {
    std::ostringstream stream;
    stream << static_cast<int>(topology) << ':'
           << generatorVersion << ':' << detailLevel << ':' << templateVersion << ':'
           << definitionHash << ':' << previewMode << ':' << regionMask;
    return stream.str();
}

GeometryBuildRequest makeGeometryBuildRequest(
    const shape::v2::CharacterDefinition& definition,
    uint32_t detailLevel,
    int previewMode) {
    GeometryBuildRequest request;
    request.generatorVersion = definition.schemaVersion >= kR2GeneratorVersion
        ? kR2GeneratorVersion : kR1GeneratorVersion;
    request.detailLevel = detailLevel;
    request.definitionHash = hashCharacterDefinition(definition);
    request.previewMode = previewMode;
    return request;
}

} // namespace pae::character