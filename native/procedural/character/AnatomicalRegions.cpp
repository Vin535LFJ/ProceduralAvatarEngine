#include "AnatomicalRegions.h"

#include "skeleton/Skeleton.h"

namespace pae::character {
namespace {

constexpr uint32_t boneBit(uint16_t bone) { return 1u << bone; }

} // namespace

const char* regionName(RegionId id) {
    switch (id) {
        case RegionId::Thorax: return "thorax";
        case RegionId::Abdomen: return "abdomen";
        case RegionId::UpperPelvis: return "upper-pelvis";
        case RegionId::ShoulderGirdleLeft: return "shoulder-girdle-left";
        case RegionId::ShoulderGirdleRight: return "shoulder-girdle-right";
        case RegionId::AxillaLeft: return "axilla-left";
        case RegionId::AxillaRight: return "axilla-right";
        case RegionId::UpperArmRootLeft: return "upper-arm-root-left";
        case RegionId::UpperArmRootRight: return "upper-arm-root-right";
        case RegionId::BridgeLeft: return "bridge-left";
        case RegionId::BridgeRight: return "bridge-right";
        case RegionId::Count: break;
    }
    return "unknown";
}

uint32_t regionBoneMask(RegionId id) {
    const uint32_t hips = boneBit(BoneHips);
    const uint32_t spine = boneBit(BoneSpine);
    const uint32_t chest = boneBit(BoneChest);
    const uint32_t leftShoulder = boneBit(BoneLeftShoulder);
    const uint32_t rightShoulder = boneBit(BoneRightShoulder);
    const uint32_t leftUpperArm = boneBit(BoneLeftUpperArm);
    const uint32_t rightUpperArm = boneBit(BoneRightUpperArm);
    switch (id) {
        case RegionId::UpperPelvis: return hips | spine;
        case RegionId::Abdomen: return hips | spine | chest;
        case RegionId::Thorax: return spine | chest | leftShoulder | rightShoulder;
        case RegionId::ShoulderGirdleLeft: return chest | leftShoulder | leftUpperArm;
        case RegionId::ShoulderGirdleRight: return chest | rightShoulder | rightUpperArm;
        case RegionId::AxillaLeft: return chest | leftShoulder | leftUpperArm;
        case RegionId::AxillaRight: return chest | rightShoulder | rightUpperArm;
        case RegionId::UpperArmRootLeft: return leftShoulder | leftUpperArm;
        case RegionId::UpperArmRootRight: return rightShoulder | rightUpperArm;
        case RegionId::BridgeLeft: return chest | leftShoulder | leftUpperArm;
        case RegionId::BridgeRight: return chest | rightShoulder | rightUpperArm;
        case RegionId::Count: break;
    }
    return chest;
}

const std::vector<RegionInfo>& torsoShoulderRegions() {
    static const std::vector<RegionInfo> regions = {
        {RegionId::Thorax, "thorax", regionBoneMask(RegionId::Thorax)},
        {RegionId::Abdomen, "abdomen", regionBoneMask(RegionId::Abdomen)},
        {RegionId::UpperPelvis, "upper-pelvis", regionBoneMask(RegionId::UpperPelvis)},
        {RegionId::ShoulderGirdleLeft, "shoulder-girdle-left", regionBoneMask(RegionId::ShoulderGirdleLeft)},
        {RegionId::ShoulderGirdleRight, "shoulder-girdle-right", regionBoneMask(RegionId::ShoulderGirdleRight)},
        {RegionId::AxillaLeft, "axilla-left", regionBoneMask(RegionId::AxillaLeft)},
        {RegionId::AxillaRight, "axilla-right", regionBoneMask(RegionId::AxillaRight)},
        {RegionId::UpperArmRootLeft, "upper-arm-root-left", regionBoneMask(RegionId::UpperArmRootLeft)},
        {RegionId::UpperArmRootRight, "upper-arm-root-right", regionBoneMask(RegionId::UpperArmRootRight)},
        {RegionId::BridgeLeft, "bridge-left", regionBoneMask(RegionId::BridgeLeft)},
        {RegionId::BridgeRight, "bridge-right", regionBoneMask(RegionId::BridgeRight)},
    };
    return regions;
}

} // namespace pae::character