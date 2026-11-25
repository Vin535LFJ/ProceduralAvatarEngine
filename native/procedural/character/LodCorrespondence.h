#pragma once

#include "core/math/Math.h"

#include <cstdint>
#include <vector>

namespace pae::character {

struct RefinementVertex {
    uint64_t stableId = 0;
    uint32_t lodLevel = 0;
    uint64_t parentFaceStableId = 0;
    uint64_t parentEdgeStableId = 0;
    uint64_t parentVertexStableId = 0;
    Vec2 barycentric{};
    uint32_t refinementPath = 0;
};

struct LodMap {
    uint32_t fromLevel = 0;
    uint32_t toLevel = 0;
    bool identity = false;
    bool valid = false;
    std::vector<uint64_t> fromStableIds;
    std::vector<uint64_t> toStableIds;
    std::vector<uint32_t> toIndices;
};

LodMap buildLodMap(const std::vector<uint64_t>& fromStableIds,
                   const std::vector<uint64_t>& toStableIds,
                   uint32_t fromLevel, uint32_t toLevel);

} // namespace pae::character