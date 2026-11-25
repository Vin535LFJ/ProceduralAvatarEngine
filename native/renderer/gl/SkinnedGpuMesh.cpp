#include "SkinnedGpuMesh.h"

#include "core/diagnostics/Log.h"

#include <cstring>
#include <utility>

namespace pae {

SkinnedGpuMesh::~SkinnedGpuMesh() {
    release();
}

SkinnedGpuMesh::SkinnedGpuMesh(SkinnedGpuMesh&& other) noexcept {
    *this = std::move(other);
}

SkinnedGpuMesh& SkinnedGpuMesh::operator=(SkinnedGpuMesh&& other) noexcept {
    if (this == &other) return *this;
    release();
    vao_ = other.vao_;
    vbo_ = other.vbo_;
    ibo_ = other.ibo_;
    uvVao_ = other.uvVao_;
    uvVbo_ = other.uvVbo_;
    uvVersion_ = other.uvVersion_;
    lastError_ = other.lastError_;
    vertexCount_ = other.vertexCount_;
    indexCount_ = other.indexCount_;
    other.vao_ = other.vbo_ = other.ibo_ = 0;
    other.uvVao_ = other.uvVbo_ = 0;
    other.uvVersion_ = 0;
    other.lastError_ = GL_NO_ERROR;
    other.vertexCount_ = other.indexCount_ = 0;
    return *this;
}

void SkinnedGpuMesh::releaseUv() {
    if (uvVao_ != 0) glDeleteVertexArrays(1, &uvVao_);
    if (uvVbo_ != 0) glDeleteBuffers(1, &uvVbo_);
    uvVao_ = uvVbo_ = 0;
    uvVersion_ = 0;
}

void SkinnedGpuMesh::release() {
    releaseUv();
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
    if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
    if (ibo_ != 0) glDeleteBuffers(1, &ibo_);
    vao_ = vbo_ = ibo_ = 0;
    vertexCount_ = indexCount_ = 0;
}

void SkinnedGpuMesh::bindSkinnedAttributes() {
    constexpr size_t kStride = 64;
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
}

bool SkinnedGpuMesh::upload(const MeshData& mesh) {
    while (glGetError() != GL_NO_ERROR) {}
    lastError_ = GL_NO_ERROR;
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        lastError_ = GL_INVALID_VALUE;
        return false;
    }

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

    GLuint nextVao = 0, nextVbo = 0, nextIbo = 0;
    const auto fail = [&]() {
        const GLenum error = lastError_;
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (nextVao != 0) glDeleteVertexArrays(1, &nextVao);
        if (nextVbo != 0) glDeleteBuffers(1, &nextVbo);
        if (nextIbo != 0) glDeleteBuffers(1, &nextIbo);
        lastError_ = error;
        return false;
    };

    glGenVertexArrays(1, &nextVao);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR || nextVao == 0) {
        if (nextVao == 0 && lastError_ == GL_NO_ERROR) lastError_ = GL_OUT_OF_MEMORY;
        PAE_LOGE("PAE_GL", "base VAO creation failed error=0x%x vao=%u vertices=%zu",
                 static_cast<unsigned>(lastError_), nextVao, mesh.vertices.size());
        return fail();
    }
    glBindVertexArray(nextVao);
    glGenBuffers(1, &nextVbo);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR || nextVbo == 0) {
        if (nextVbo == 0 && lastError_ == GL_NO_ERROR) lastError_ = GL_OUT_OF_MEMORY;
        return fail();
    }
    glBindBuffer(GL_ARRAY_BUFFER, nextVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(packed.size()), packed.data(), GL_STATIC_DRAW);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR) return fail();

    bindSkinnedAttributes();

    glGenBuffers(1, &nextIbo);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR || nextIbo == 0) {
        if (nextIbo == 0 && lastError_ == GL_NO_ERROR) lastError_ = GL_OUT_OF_MEMORY;
        return fail();
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nextIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR) return fail();

    release();
    vao_ = nextVao;
    vbo_ = nextVbo;
    ibo_ = nextIbo;
    vertexCount_ = static_cast<uint32_t>(mesh.vertices.size());
    indexCount_ = static_cast<uint32_t>(mesh.indices.size());
    return true;
}

bool SkinnedGpuMesh::uploadUv(const MeshData& mesh, uint64_t version) {
    while (glGetError() != GL_NO_ERROR) {}
    lastError_ = GL_NO_ERROR;
    if (vao_ == 0 || vertexCount_ != mesh.vertices.size() ||
        mesh.vertices.empty() || mesh.indices.empty()) {
        lastError_ = GL_INVALID_OPERATION;
        PAE_LOGE("PAE_GL", "UV upload precondition failed vao=%u vertices=%zu expected=%u indices=%zu",
                 vao_, mesh.vertices.size(), vertexCount_, mesh.indices.size());
        return false;
    }
    GLuint nextVao = 0, nextVbo = 0;
    const auto fail = [&]() {
        const GLenum error = lastError_;
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (nextVao != 0) glDeleteVertexArrays(1, &nextVao);
        if (nextVbo != 0) glDeleteBuffers(1, &nextVbo);
        lastError_ = error;
        return false;
    };
    std::vector<Vec2> uvs;
    uvs.reserve(mesh.vertices.size());
    for (const Vertex& vertex : mesh.vertices) uvs.push_back(vertex.uv);

    glGenVertexArrays(1, &nextVao);
    glBindVertexArray(nextVao);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR || nextVao == 0) {
        if (nextVao == 0 && lastError_ == GL_NO_ERROR) lastError_ = GL_OUT_OF_MEMORY;
        PAE_LOGE("PAE_GL", "UV VAO creation/binding failed error=0x%x vao=%u",
                 static_cast<unsigned>(lastError_), nextVao);
        return fail();
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    bindSkinnedAttributes();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR) {
        PAE_LOGE("PAE_GL", "UV shared attribute binding failed error=0x%x",
                 static_cast<unsigned>(lastError_));
        return fail();
    }

    glGenBuffers(1, &nextVbo);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR || nextVbo == 0) {
        if (nextVbo == 0 && lastError_ == GL_NO_ERROR) lastError_ = GL_OUT_OF_MEMORY;
        PAE_LOGE("PAE_GL", "UV VBO creation failed error=0x%x vbo=%u",
                 static_cast<unsigned>(lastError_), nextVbo);
        return fail();
    }
    glBindBuffer(GL_ARRAY_BUFFER, nextVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(uvs.size() * sizeof(Vec2)),
                 uvs.data(), GL_STATIC_DRAW);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR) {
        PAE_LOGE("PAE_GL", "UV buffer upload failed error=0x%x bytes=%zu",
                 static_cast<unsigned>(lastError_), uvs.size() * sizeof(Vec2));
        return fail();
    }
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), nullptr);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR) {
        PAE_LOGE("PAE_GL", "UV attribute binding failed error=0x%x",
                 static_cast<unsigned>(lastError_));
        return fail();
    }

    glBindVertexArray(0);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR) {
        PAE_LOGE("PAE_GL", "UV VAO unbind failed error=0x%x", static_cast<unsigned>(lastError_));
        return fail();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    lastError_ = glGetError();
    if (lastError_ != GL_NO_ERROR) {
        PAE_LOGE("PAE_GL", "UV array buffer unbind failed error=0x%x", static_cast<unsigned>(lastError_));
        return fail();
    }
    releaseUv();
    uvVao_ = nextVao;
    uvVbo_ = nextVbo;
    uvVersion_ = version;
    return true;
}

void SkinnedGpuMesh::drawUv() const {
    if (uvVao_ == 0 || indexCount_ == 0) return;
    glBindVertexArray(uvVao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount_), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
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
