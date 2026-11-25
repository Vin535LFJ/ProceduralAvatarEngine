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
    request.generatorVersion = definition.schemaVersion;
    request.detailLevel = detailLevel;
    request.definitionHash = hashCharacterDefinition(definition);
    request.previewMode = previewMode;
    return request;
}

} // namespace pae::character