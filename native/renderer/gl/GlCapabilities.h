#pragma once

#include <GLES3/gl3.h>

#include <string>

namespace pae {

struct GlCapabilities {
    int majorVersion = 0;
    int minorVersion = 0;
    std::string version;
    std::string renderer;
    std::string vendor;
    std::string glslVersion;
    int maxVertexUniformBlocks = 0;
    int maxTextureSize = 0;
    int maxVertexAttribs = 0;

    bool supportsGles3() const { return majorVersion >= 3; }
};

// Queries capabilities on the render thread after a context is current.
GlCapabilities queryGlCapabilities();
void logGlCapabilities(const GlCapabilities& caps);

} // namespace pae
