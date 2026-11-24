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
    void setAppearance(const std::array<Vec3, 5>& palette);
    void setBackground(int mode, const Vec3& colorA, const Vec3& colorB);
    void setPoseMode(int mode);
    void pushPose(const PoseFrame& pose);
    void resetMotion();
    void setQuality(int level);
    void setAnatomyPreview(bool on);
    void setPreviewMode(int mode);
    void setCameraPreset(int preset);
    void setDebugView(int view);
    // Interactive camera: op is a CameraOp value, delta carries the gesture.
    void cameraCommand(int op, const Vec3& delta);
    std::string stats() const;
    void shutdown();

private:
    std::unique_ptr<RenderThread> renderThread_;
};

} // namespace pae
