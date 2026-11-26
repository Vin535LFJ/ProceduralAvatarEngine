#pragma once

#include "AnatomicalRegions.h"
#include "procedural/MeshData.h"
#include "procedural/MeshTopology.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pae::character {

struct PatchBoundary {
    std::string id;
    RegionId region = RegionId::Thorax;
    std::vector<uint32_t> vertexIds;
    bool closed = false;
    bool crop = false;
};

struct SurfacePatch {
    std::string id;
    RegionId region = RegionId::Thorax;
    uint8_t side = 0;
    MeshData mesh;
    std::vector<uint64_t> stableIds;
    std::vector<RegionId> vertexRegions;
    std::vector<PatchBoundary> boundaries;
    bool valid = false;
};

struct PatchAssemblyResult {
    MeshData mesh;
    TopologyReport topology;
    std::vector<uint64_t> stableIds;
    std::vector<RegionId> vertexRegions;
    std::vector<PatchBoundary> boundaries;
    bool valid = false;
    std::string error;
};

} // namespace pae::character