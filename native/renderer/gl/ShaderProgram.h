#pragma once

#include <GLES3/gl3.h>

#include <string>

namespace pae {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    bool build(const char* vertexSource, const char* fragmentSource, std::string* error);
    void use() const;
    GLint uniform(const char* name) const;
    bool bindUniformBlock(const char* blockName, GLuint binding) const;
    bool valid() const { return program_ != 0; }

private:
    GLuint compileShader(GLenum type, const char* source, std::string* error);

    GLuint program_ = 0;
};

} // namespace pae
