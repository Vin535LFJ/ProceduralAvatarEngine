#pragma once

#include <cstdint>
#include <vector>

namespace pae::character {

enum class RegionId : uint8_t {
    Thorax = 0,
    Abdomen,
    UpperPelvis,
    ShoulderGirdleLeft,
    ShoulderGirdleRight,
    AxillaLeft,
    AxillaRight,
    UpperArmRootLeft,
    UpperArmRootRight,
    BridgeLeft,
    BridgeRight,
    Count
};

struct RegionInfo {
    RegionId id = RegionId::Thorax;
    const char* name = "";
    uint32_t boneMask = 0;
};

const char* regionName(RegionId id);
uint32_t regionBoneMask(RegionId id);
const std::vector<RegionInfo>& torsoShoulderRegions();

} // namespace pae::character