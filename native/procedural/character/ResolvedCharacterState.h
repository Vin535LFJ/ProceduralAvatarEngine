#pragma once

#include "AnatomicalLandmarks.h"
#include "AnatomicalRegions.h"
#include "Anthropometry.h"
#include "procedural/shape/v2/CharacterDefinition.h"
#include "skeleton/Skeleton.h"

#include <string>
#include <vector>

namespace pae::character {

struct ResolvedCharacterState {
    Anthropometry anthropometry;
    AnatomicalLandmarkSet landmarks;
    std::vector<RegionInfo> regions;
    BodyJoints joints;
    Skeleton skeleton;
    LandmarkResolverReport landmarkReport;
    bool valid = false;
    std::string error;
};

ResolvedCharacterState resolveCharacterState(
    const shape::v2::CharacterDefinition& definition,
    const BodyJoints* legacyJoints = nullptr);

} // namespace pae::character