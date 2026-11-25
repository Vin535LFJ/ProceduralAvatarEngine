#include "mini_test.hpp"

#include "renderer/gl/SkinnedGpuMesh.h"

#include <set>
#include <vector>

using namespace pae;

namespace {

std::set<GLuint> liveVaos;
std::set<GLuint> liveBuffers;
std::vector<GLenum> errors;
GLuint nextId = 1;
GLuint boundVao = 0;
GLuint drawnVao = 0;
size_t deletedVaos = 0;
size_t deletedBuffers = 0;
bool failBufferUpload = false;

void resetGl() {
    liveVaos.clear();
    liveBuffers.clear();
    errors.clear();
    nextId = 1;
    boundVao = drawnVao = 0;
    deletedVaos = deletedBuffers = 0;
    failBufferUpload = false;
}

MeshData triangleMesh() {
    MeshData mesh;
    mesh.vertices = {
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {}, {}},
        {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {}, {}},
        {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {}, {}},
    };
    mesh.indices = {0, 1, 2};
    return mesh;
}

} // namespace

extern "C" {

GLenum glGetError() {
    if (errors.empty()) return GL_NO_ERROR;
    const GLenum error = errors.front();
    errors.erase(errors.begin());
    return error;
}

void glGenVertexArrays(GLsizei count, GLuint* arrays) {
    for (GLsizei i = 0; i < count; ++i) {
        arrays[i] = nextId++;
        liveVaos.insert(arrays[i]);
    }
}

void glDeleteVertexArrays(GLsizei count, const GLuint* arrays) {
    for (GLsizei i = 0; i < count; ++i) {
        liveVaos.erase(arrays[i]);
        ++deletedVaos;
    }
}

void glBindVertexArray(GLuint array) { boundVao = array; }

void glGenBuffers(GLsizei count, GLuint* buffers) {
    for (GLsizei i = 0; i < count; ++i) {
        buffers[i] = nextId++;
        liveBuffers.insert(buffers[i]);
    }
}

void glDeleteBuffers(GLsizei count, const GLuint* buffers) {
    for (GLsizei i = 0; i < count; ++i) {
        liveBuffers.erase(buffers[i]);
        ++deletedBuffers;
    }
}

void glBindBuffer(GLenum, GLuint) {}
void glBufferData(GLenum, GLsizeiptr, const void*, GLenum) {
    if (failBufferUpload) errors.push_back(GL_OUT_OF_MEMORY);
}
void glEnableVertexAttribArray(GLuint) {}
void glVertexAttribPointer(GLuint, GLsizei, GLenum, GLboolean, GLsizei, const void*) {}
void glVertexAttribIPointer(GLuint, GLsizei, GLenum, GLsizei, const void*) {}
void glDrawElements(GLenum, GLsizei, GLenum, const void*) { drawnVao = boundVao; }

}

PAE_TEST_CASE(p1_render_invalid_uploads_preserve_live_resources) {
    resetGl();
    SkinnedGpuMesh mesh;
    const MeshData source = triangleMesh();
    EXPECT_TRUE(mesh.upload(source));
    EXPECT_TRUE(mesh.uploadUv(source, 7));
    const size_t vaoCount = liveVaos.size();
    const size_t bufferCount = liveBuffers.size();
    const size_t vaoDeletes = deletedVaos;
    const size_t bufferDeletes = deletedBuffers;

    EXPECT_TRUE(!mesh.upload(MeshData{}));
    EXPECT_TRUE(mesh.lastError() == GL_INVALID_VALUE);
    EXPECT_TRUE(mesh.valid());
    EXPECT_TRUE(mesh.hasUv());
    EXPECT_TRUE(mesh.uvVersion() == 7);
    mesh.draw();
    mesh.drawUv();
    EXPECT_TRUE(drawnVao != 0);

    MeshData mismatch;
    mismatch.vertices = source.vertices;
    mismatch.indices = source.indices;
    mismatch.vertices.push_back(source.vertices[0]);
    EXPECT_TRUE(!mesh.uploadUv(mismatch, 8));
    EXPECT_TRUE(mesh.lastError() == GL_INVALID_OPERATION);
    EXPECT_TRUE(mesh.hasUv());
    EXPECT_TRUE(mesh.uvVersion() == 7);
    EXPECT_TRUE(liveVaos.size() == vaoCount);
    EXPECT_TRUE(liveBuffers.size() == bufferCount);
    EXPECT_TRUE(deletedVaos == vaoDeletes);
    EXPECT_TRUE(deletedBuffers == bufferDeletes);
}

PAE_TEST_CASE(p1_render_failed_buffer_uploads_preserve_live_resources) {
    resetGl();
    SkinnedGpuMesh mesh;
    const MeshData source = triangleMesh();
    EXPECT_TRUE(mesh.upload(source));
    EXPECT_TRUE(mesh.uploadUv(source, 11));
    const size_t vaoCount = liveVaos.size();
    const size_t bufferCount = liveBuffers.size();
    const size_t vaoDeletes = deletedVaos;
    const size_t bufferDeletes = deletedBuffers;

    failBufferUpload = true;
    EXPECT_TRUE(!mesh.upload(source));
    EXPECT_TRUE(mesh.lastError() == GL_OUT_OF_MEMORY);
    EXPECT_TRUE(mesh.valid());
    EXPECT_TRUE(mesh.hasUv());
    EXPECT_TRUE(mesh.uvVersion() == 11);
    EXPECT_TRUE(liveVaos.size() == vaoCount);
    EXPECT_TRUE(liveBuffers.size() == bufferCount);
    EXPECT_TRUE(deletedVaos == vaoDeletes + 1);
    EXPECT_TRUE(deletedBuffers == bufferDeletes + 1);

    EXPECT_TRUE(!mesh.uploadUv(source, 12));
    EXPECT_TRUE(mesh.lastError() == GL_OUT_OF_MEMORY);
    EXPECT_TRUE(mesh.valid());
    EXPECT_TRUE(mesh.hasUv());
    EXPECT_TRUE(mesh.uvVersion() == 11);
    EXPECT_TRUE(liveVaos.size() == vaoCount);
    EXPECT_TRUE(liveBuffers.size() == bufferCount);
    EXPECT_TRUE(deletedVaos == vaoDeletes + 2);
    EXPECT_TRUE(deletedBuffers == bufferDeletes + 2);

    failBufferUpload = false;
    EXPECT_TRUE(mesh.upload(source));
    EXPECT_TRUE(mesh.valid());
    EXPECT_TRUE(!mesh.hasUv());
    EXPECT_TRUE(liveVaos.size() == 1);
    EXPECT_TRUE(liveBuffers.size() == 2);
}

int main() { return test::runAllTests(); }
