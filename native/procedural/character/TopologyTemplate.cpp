#include "TopologyTemplate.h"

namespace pae::character {
namespace {

uint64_t mix(uint64_t hash, uint64_t value) {
    hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    return hash;
}

} // namespace

uint64_t makeStableVertexId(RegionId region, uint8_t side, uint32_t localVertexId) {
    uint64_t hash = 1469598103934665603ull;
    hash = mix(hash, kTorsoShoulderTemplateUuid);
    hash = mix(hash, kTorsoShoulderTopologyVersion);
    hash = mix(hash, static_cast<uint64_t>(region));
    hash = mix(hash, side);
    hash = mix(hash, localVertexId);
    return hash;
}

uint64_t stableGridId(RegionId region, uint8_t side,
                      uint32_t ring, uint32_t segment,
                      const GridTopology& topology) {
    return makeStableVertexId(region, side, topology.index(ring, segment));
}

} // namespace pae::character