#pragma once

#include "renderer/RenderThread.h"

#include <android/native_window.h>

#include <memory>
#include <string>

namespace pae {

// Thin facade used by the JNI boundary. Owns the render thread and command queue.
class EngineRuntime {
public:
    bool start(std::string* error);
    void surfaceCreated(ANativeWindow* window, int width, int height);
    void surfaceChanged(int width, int height);
    void surfaceDestroyed();
    void setAppearance(const std::array<Vec3, 4>& palette);
    void setBackground(int mode, const Vec3& colorA, const Vec3& colorB);
    void setPoseMode(int mode);
    void pushPose(const PoseFrame& pose);
    void resetMotion();
    void setQuality(int level);
    std::string stats() const;
    void shutdown();

private:
    std::unique_ptr<RenderThread> renderThread_;
};

} // namespace pae
