#include "ShaderProgram.h"

#include "core/diagnostics/Log.h"

#include <vector>

namespace pae {

ShaderProgram::~ShaderProgram() {
    if (program_ != 0) glDeleteProgram(program_);
}

GLuint ShaderProgram::compileShader(GLenum type, const char* source, std::string* error) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        if (error) *error = "glCreateShader failed";
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) return shader;

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(static_cast<size_t>(length > 1 ? length : 1));
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    if (error) {
        *error = std::string("shader compile failed: ") + log.data() + "\n--- source ---\n" + source;
    }
    PAE_LOGE("PAE_SHADER", "compile failed:\n%s\nsource:\n%s", log.data(), source);
    glDeleteShader(shader);
    return 0;
}

bool ShaderProgram::build(const char* vertexSource, const char* fragmentSource, std::string* error) {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource, error);
    if (vs == 0) return false;
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource, error);
    if (fs == 0) {
        glDeleteShader(vs);
        return false;
    }
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint status = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) return true;

    GLint length = 0;
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(static_cast<size_t>(length > 1 ? length : 1));
    glGetProgramInfoLog(program_, static_cast<GLsizei>(log.size()), nullptr, log.data());
    if (error) *error = std::string("program link failed: ") + log.data();
    PAE_LOGE("PAE_SHADER", "link failed: %s", log.data());
    glDeleteProgram(program_);
    program_ = 0;
    return false;
}

void ShaderProgram::use() const {
    if (program_ != 0) glUseProgram(program_);
}

GLint ShaderProgram::uniform(const char* name) const {
    return program_ != 0 ? glGetUniformLocation(program_, name) : -1;
}

bool ShaderProgram::bindUniformBlock(const char* blockName, GLuint binding) const {
    if (program_ == 0) return false;
    const GLuint index = glGetUniformBlockIndex(program_, blockName);
    if (index == GL_INVALID_INDEX) return false;
    glUniformBlockBinding(program_, index, binding);
    return true;
}

} // namespace pae
