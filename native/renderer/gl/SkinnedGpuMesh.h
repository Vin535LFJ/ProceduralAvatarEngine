#pragma once

#include "procedural/MeshData.h"

#include <GLES3/gl3.h>

#include <cstdint>

namespace pae {

// Interleaved 64-byte vertex: pos(3f) normal(3f) joints(4xuint16) weights(4f) color(4f).
class SkinnedGpuMesh {
public:
    SkinnedGpuMesh() = default;
    ~SkinnedGpuMesh();
    SkinnedGpuMesh(const SkinnedGpuMesh&) = delete;
    SkinnedGpuMesh& operator=(const SkinnedGpuMesh&) = delete;

    bool upload(const MeshData& mesh);
    void draw() const;
    void drawLines() const; // M12: GL_LINES over line-pair indices
    void release();
    bool valid() const { return vao_ != 0; }
    uint32_t indexCount() const { return indexCount_; }

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ibo_ = 0;
    uint32_t indexCount_ = 0;
};

} // namespace pae
