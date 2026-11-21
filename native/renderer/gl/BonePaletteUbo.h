#pragma once

#include "skeleton/Skinning.h"

#include <GLES3/gl3.h>
#include <string>

namespace pae {

// std140 UBO at binding 0: mat4 uBones[22].
class BonePaletteUbo {
public:
    BonePaletteUbo() = default;
    ~BonePaletteUbo();
    BonePaletteUbo(const BonePaletteUbo&) = delete;
    BonePaletteUbo& operator=(const BonePaletteUbo&) = delete;

    bool create(std::string* error);
    void update(const BonePaletteUboData& data);
    void release();

private:
    GLuint ubo_ = 0;
};

} // namespace pae
