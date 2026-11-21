#pragma once

#include <GLES3/gl3.h>

#include <cstdint>
#include <vector>

namespace pae {

// RAII wrapper for one VAO + VBO + IBO. Stride is 6 floats: position(3) + color(3).
class GpuMesh {
public:
    GpuMesh() = default;
    ~GpuMesh();
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;

    bool upload(const std::vector<float>& interleavedVertices,
                const std::vector<uint32_t>& indices);
    void draw() const;
    void release();
    bool valid() const { return vao_ != 0; }

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ibo_ = 0;
    GLsizei indexCount_ = 0;
};

} // namespace pae
