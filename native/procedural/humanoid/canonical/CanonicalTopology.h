#pragma once

#include "procedural/MeshData.h"

#include <array>
#include <cstdint>

namespace pae::canonical {

struct SelfIntersectionReport {
    size_t candidatePairs = 0;
    size_t intersectingPairs = 0;
    std::vector<std::array<uint32_t, 2>> samples;
};

SelfIntersectionReport detectSelfIntersections(const MeshData& mesh, size_t maxSamples = 16);

} // namespace pae::canonical
