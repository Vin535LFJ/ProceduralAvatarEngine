#pragma once

#include "Anthropometry.h"
#include "skeleton/Skeleton.h"

#include <string>
#include <vector>

namespace pae::character {

enum class LandmarkId : uint16_t {
    HipCenter = 0,
    WaistCenter,
    CostalMargin,
    ChestCenter,
    NeckBase,
    LeftAcromion,
    RightAcromion,
    LeftShoulderJoint,
    RightShoulderJoint,
    LeftAxilla,
    RightAxilla,
    LeftElbow,
    RightElbow,
    Count
};

enum class LandmarkSide : uint8_t { Center = 0, Left = 1, Right = 2 };

struct LandmarkPoint {
    LandmarkId id = LandmarkId::HipCenter;
    const char* name = "";
    Vec3 position{};
    LandmarkSide side = LandmarkSide::Center;
    shape::v2::ValueConfidence confidence = shape::v2::ValueConfidence::Derived;
    uint16_t boneId = BoneCount;
};

struct AnatomicalLandmarkSet {
    std::vector<LandmarkPoint> points;
    const LandmarkPoint* find(LandmarkId id) const;
};

struct LandmarkResolverReport {
    bool valid = false;
    bool usedLegacyJoints = false;
    std::vector<std::string> notes;
};

AnatomicalLandmarkSet resolveLandmarks(const shape::v2::CharacterDefinition& definition,
                                       const Anthropometry& anthropometry,
                                       const BodyJoints& joints,
                                       bool usedLegacyJoints,
                                       LandmarkResolverReport* report);

} // namespace pae::character