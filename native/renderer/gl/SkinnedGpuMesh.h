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
    SkinnedGpuMesh(SkinnedGpuMesh&& other) noexcept;
    SkinnedGpuMesh& operator=(SkinnedGpuMesh&& other) noexcept;

    bool upload(const MeshData& mesh);
    bool uploadUv(const MeshData& mesh, uint64_t version);
    void draw() const;
    void drawUv() const;
    void drawLines() const; // M12: GL_LINES over line-pair indices
    void release();
    bool valid() const { return vao_ != 0; }
    bool hasUv() const { return uvVao_ != 0; }
    void bindUv() const { glBindVertexArray(uvVao_); }
    uint64_t uvVersion() const { return uvVersion_; }
    GLenum lastError() const { return lastError_; }
    uint32_t vertexCount() const { return vertexCount_; }
    uint32_t indexCount() const { return indexCount_; }

private:
    static void bindSkinnedAttributes();
    void releaseUv();

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ibo_ = 0;
    GLuint uvVao_ = 0;
    GLuint uvVbo_ = 0;
    uint64_t uvVersion_ = 0;
    GLenum lastError_ = GL_NO_ERROR;
    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;
};

} // namespace pae
