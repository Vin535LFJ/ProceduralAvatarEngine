#pragma once

#include "AnatomicalRegions.h"

#include <cstdint>
#include <vector>

namespace pae::character {

constexpr uint32_t kTorsoShoulderTemplateUuid = 0x50415652u; // "PAVR"
constexpr uint32_t kTorsoShoulderTopologyVersion = 1u;

uint64_t makeStableVertexId(RegionId region, uint8_t side, uint32_t localVertexId);

struct GridTopology {
    uint32_t rings = 0;
    uint32_t segments = 0;
    uint32_t vertexCount() const { return rings * segments; }
    uint32_t index(uint32_t ring, uint32_t segment) const {
        return ring * segments + (segment % segments);
    }
};

uint64_t stableGridId(RegionId region, uint8_t side,
                      uint32_t ring, uint32_t segment,
                      const GridTopology& topology);

} // namespace pae::character