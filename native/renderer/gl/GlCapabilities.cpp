#include "GlCapabilities.h"

#include "core/diagnostics/Log.h"

namespace pae {
namespace {

std::string glString(GLenum name) {
    const GLubyte* value = glGetString(name);
    return value ? reinterpret_cast<const char*>(value) : std::string("<null>");
}

} // namespace

GlCapabilities queryGlCapabilities() {
    GlCapabilities caps;
    glGetIntegerv(GL_MAJOR_VERSION, &caps.majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &caps.minorVersion);
    caps.version = glString(GL_VERSION);
    caps.renderer = glString(GL_RENDERER);
    caps.vendor = glString(GL_VENDOR);
    caps.glslVersion = glString(GL_SHADING_LANGUAGE_VERSION);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &caps.maxVertexUniformBlocks);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &caps.maxVertexAttribs);
    return caps;
}

void logGlCapabilities(const GlCapabilities& caps) {
    PAE_LOGI("PAE_GLCAPS", "GL_VERSION=%s", caps.version.c_str());
    PAE_LOGI("PAE_GLCAPS", "GL_RENDERER=%s", caps.renderer.c_str());
    PAE_LOGI("PAE_GLCAPS", "GL_VENDOR=%s", caps.vendor.c_str());
    PAE_LOGI("PAE_GLCAPS", "GLSL_VERSION=%s", caps.glslVersion.c_str());
    PAE_LOGI("PAE_GLCAPS", "MAX_VERTEX_UNIFORM_BLOCKS=%d MAX_TEXTURE_SIZE=%d MAX_VERTEX_ATTRIBS=%d",
             caps.maxVertexUniformBlocks, caps.maxTextureSize, caps.maxVertexAttribs);
    if (!caps.supportsGles3()) {
        PAE_LOGE("PAE_GLCAPS", "GLES 3.0 not supported by current context");
    }
}

} // namespace pae
