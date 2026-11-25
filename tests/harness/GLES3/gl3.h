#pragma once

#include <cstddef>

using GLuint = unsigned int;
using GLenum = unsigned int;
using GLint = int;
using GLsizei = int;
using GLsizeiptr = std::ptrdiff_t;
using GLboolean = unsigned char;

constexpr GLenum GL_NO_ERROR = 0;
constexpr GLenum GL_INVALID_VALUE = 0x0501;
constexpr GLenum GL_INVALID_OPERATION = 0x0502;
constexpr GLenum GL_OUT_OF_MEMORY = 0x0505;
constexpr GLenum GL_FLOAT = 0x1406;
constexpr GLenum GL_UNSIGNED_SHORT = 0x1403;
constexpr GLenum GL_UNSIGNED_INT = 0x1405;
constexpr GLenum GL_FALSE = 0;
constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
constexpr GLenum GL_STATIC_DRAW = 0x88e4;
constexpr GLenum GL_TRIANGLES = 0x0004;
constexpr GLenum GL_LINES = 0x0001;

extern "C" {
GLenum glGetError();
void glGenVertexArrays(GLsizei count, GLuint* arrays);
void glDeleteVertexArrays(GLsizei count, const GLuint* arrays);
void glBindVertexArray(GLuint array);
void glGenBuffers(GLsizei count, GLuint* buffers);
void glDeleteBuffers(GLsizei count, const GLuint* buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void glEnableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLsizei size, GLenum type, GLboolean normalized,
                           GLsizei stride, const void* pointer);
void glVertexAttribIPointer(GLuint index, GLsizei size, GLenum type, GLsizei stride,
                            const void* pointer);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
}
