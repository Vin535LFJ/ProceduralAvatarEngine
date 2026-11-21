#pragma once

#include <EGL/egl.h>
#include <android/native_window.h>

#include <string>

namespace pae {

// Owns EGL display/config/context and the current window surface.
// All methods must be called from the render thread.
class EglContextManager {
public:
    EglContextManager() = default;
    ~EglContextManager();

    EglContextManager(const EglContextManager&) = delete;
    EglContextManager& operator=(const EglContextManager&) = delete;

    bool initialize(std::string* error);
    bool createSurface(ANativeWindow* window, std::string* error);
    void destroySurface();
    bool makeCurrent(std::string* error);
    void swapBuffers();
    bool recreate(std::string* error);

    bool contextCreated() const { return context_ != EGL_NO_CONTEXT; }
    bool surfaceCreated() const { return surface_ != EGL_NO_SURFACE; }

private:
    bool chooseConfig(bool relaxed, std::string* error);
    void releaseAll();

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
};

} // namespace pae
