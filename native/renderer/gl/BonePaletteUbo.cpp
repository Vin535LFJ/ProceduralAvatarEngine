#include "BonePaletteUbo.h"

namespace pae {

BonePaletteUbo::~BonePaletteUbo() {
    release();
}

bool BonePaletteUbo::create(std::string* error) {
    release();
    glGenBuffers(1, &ubo_);
    if (ubo_ == 0) {
        if (error) *error = "glGenBuffers for UBO failed";
        return false;
    }
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(BonePaletteUboData::kBytes),
                 nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return true;
}

void BonePaletteUbo::update(const BonePaletteUboData& data) {
    if (ubo_ == 0) return;
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0,
                    static_cast<GLsizeiptr>(BonePaletteUboData::kBytes), data.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void BonePaletteUbo::release() {
    if (ubo_ != 0) {
        glDeleteBuffers(1, &ubo_);
        ubo_ = 0;
    }
}

} // namespace pae
