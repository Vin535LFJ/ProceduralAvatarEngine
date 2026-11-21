#pragma once

#include "core/math/Math.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pae {

struct Vertex {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{0.0f, 0.0f};
    U16Vec4 joints{};
    Vec4 weights{1.0f, 0.0f, 0.0f, 0.0f};
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct SubMesh {
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    uint16_t materialId = 0;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> subMeshes;
    Aabb bounds;
};

struct ValidationReport {
    bool valid = true;
    std::vector<std::string> errors;

    void fail(const std::string& message) {
        valid = false;
        errors.push_back(message);
    }
};

} // namespace pae
