#include "EngineRuntime.h"

#include "core/diagnostics/Log.h"

namespace pae {

bool EngineRuntime::start(std::string* error) {
    renderThread_ = std::make_unique<RenderThread>(64);
    return renderThread_->start(error);
}

// Ownership of |window| transfers to the render thread, which releases it.
void EngineRuntime::surfaceCreated(ANativeWindow* window, int width, int height) {
    if (!renderThread_) {
        if (window) ANativeWindow_release(window);
        return;
    }
    renderThread_->postSurfaceCreated(window, width, height);
}

void EngineRuntime::surfaceChanged(int width, int height) {
    if (renderThread_) renderThread_->postSurfaceChanged(width, height);
}

void EngineRuntime::surfaceDestroyed() {
    if (renderThread_) renderThread_->postSurfaceDestroyed();
}

void EngineRuntime::setAppearance(const std::array<Vec3, 4>& palette) {
    if (renderThread_) renderThread_->postAppearance(palette);
}

void EngineRuntime::setBackground(int mode, const Vec3& colorA, const Vec3& colorB) {
    if (renderThread_) renderThread_->postBackground(mode, colorA, colorB);
}

void EngineRuntime::setPoseMode(int mode) {
    if (renderThread_) renderThread_->postPoseMode(mode);
}

void EngineRuntime::pushPose(const PoseFrame& pose) {
    if (renderThread_) renderThread_->postExternalPose(pose);
}

void EngineRuntime::resetMotion() {
    if (renderThread_) renderThread_->postMotionReset();
}

void EngineRuntime::setQuality(int level) {
    if (renderThread_) renderThread_->postQuality(level);
}

std::string EngineRuntime::stats() const {
    return renderThread_ ? renderThread_->getStats() : std::string("engine not started");
}

void EngineRuntime::shutdown() {
    if (!renderThread_) return;
    renderThread_->stop();
    renderThread_.reset();
    PAE_LOGI("PAE_RUNTIME", "engine shutdown complete");
}

} // namespace pae
