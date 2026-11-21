#include "SkinnedGpuMesh.h"

#include <cstring>

namespace pae {

SkinnedGpuMesh::~SkinnedGpuMesh() {
    release();
}

void SkinnedGpuMesh::release() {
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
    if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
    if (ibo_ != 0) glDeleteBuffers(1, &ibo_);
    vao_ = vbo_ = ibo_ = 0;
    indexCount_ = 0;
}

bool SkinnedGpuMesh::upload(const MeshData& mesh) {
    release();
    if (mesh.vertices.empty() || mesh.indices.empty()) return false;

    constexpr size_t kStride = 64;
    std::vector<uint8_t> packed(mesh.vertices.size() * kStride, 0);
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const Vertex& v = mesh.vertices[i];
        uint8_t* base = packed.data() + i * kStride;
        std::memcpy(base, &v.position, sizeof(Vec3));
        std::memcpy(base + 12, &v.normal, sizeof(Vec3));
        std::memcpy(base + 24, &v.joints, sizeof(U16Vec4));
        std::memcpy(base + 32, &v.weights, sizeof(Vec4));
        std::memcpy(base + 48, &v.color, sizeof(Vec4));
    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(packed.size()), packed.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(12));
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 4, GL_UNSIGNED_SHORT, kStride, reinterpret_cast<void*>(24));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(32));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(48));

    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);
    indexCount_ = static_cast<uint32_t>(mesh.indices.size());

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void SkinnedGpuMesh::drawLines() const {
    if (vao_ == 0 || indexCount_ == 0) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_LINES, static_cast<GLsizei>(indexCount_), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void SkinnedGpuMesh::draw() const {
    if (vao_ == 0 || indexCount_ == 0) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount_), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace pae
