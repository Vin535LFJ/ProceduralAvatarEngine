#include "LodCorrespondence.h"

#include <unordered_map>

namespace pae::character {

LodMap buildLodMap(const std::vector<uint64_t>& fromStableIds,
                   const std::vector<uint64_t>& toStableIds,
                   uint32_t fromLevel, uint32_t toLevel) {
    LodMap map;
    map.fromLevel = fromLevel;
    map.toLevel = toLevel;
    map.fromStableIds = fromStableIds;
    map.toStableIds = toStableIds;
    map.toIndices.assign(fromStableIds.size(), UINT32_MAX);
    std::unordered_map<uint64_t, uint32_t> indexById;
    indexById.reserve(toStableIds.size());
    for (uint32_t i = 0; i < toStableIds.size(); ++i) {
        indexById[toStableIds[i]] = i;
    }
    bool identity = fromStableIds.size() == toStableIds.size();
    size_t mapped = 0;
    for (size_t i = 0; i < fromStableIds.size(); ++i) {
        const auto it = indexById.find(fromStableIds[i]);
        if (it == indexById.end()) {
            identity = false;
            continue;
        }
        map.toIndices[i] = it->second;
        if (it->second != i) identity = false;
        ++mapped;
    }
    map.identity = identity && mapped == fromStableIds.size();
    map.valid = mapped == fromStableIds.size();
    return map;
}

} // namespace pae::character