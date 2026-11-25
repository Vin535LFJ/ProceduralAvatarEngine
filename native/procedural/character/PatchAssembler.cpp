#include "PatchAssembler.h"

#include <utility>

namespace pae::character {

PatchAssemblyResult assemblePatches(const std::vector<SurfacePatch>& patches) {
    PatchAssemblyResult result;
    for (const SurfacePatch& patch : patches) {
        if (!patch.valid) {
            result.error = "invalid patch: " + patch.id;
            return result;
        }
        const uint32_t vertexOffset = static_cast<uint32_t>(result.mesh.vertices.size());
        result.mesh.vertices.insert(result.mesh.vertices.end(),
                                    patch.mesh.vertices.begin(), patch.mesh.vertices.end());
        for (uint32_t index : patch.mesh.indices) {
            result.mesh.indices.push_back(vertexOffset + index);
        }
        result.stableIds.insert(result.stableIds.end(),
                                patch.stableIds.begin(), patch.stableIds.end());
        result.vertexRegions.insert(result.vertexRegions.end(),
                                    patch.vertexRegions.begin(), patch.vertexRegions.end());
        for (PatchBoundary boundary : patch.boundaries) {
            for (uint32_t& vertex : boundary.vertexIds) vertex += vertexOffset;
            result.boundaries.push_back(std::move(boundary));
        }
    }
    for (const Vertex& vertex : result.mesh.vertices) result.mesh.bounds.expand(vertex.position);
    if (!result.mesh.vertices.empty() && result.mesh.subMeshes.empty()) {
        result.mesh.subMeshes.push_back({0, static_cast<uint32_t>(result.mesh.indices.size()), 0});
    }
    result.valid = !result.mesh.vertices.empty() && !result.mesh.indices.empty() &&
                   result.stableIds.size() == result.mesh.vertices.size() &&
                   result.vertexRegions.size() == result.mesh.vertices.size();
    return result;
}

} // namespace pae::character