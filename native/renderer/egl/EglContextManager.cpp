#include "EglContextManager.h"

#include "core/diagnostics/Log.h"

namespace pae {
namespace {

const char* eglErrorName(EGLint error) {
    switch (error) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        default: return "EGL_UNKNOWN";
    }
}

bool setError(std::string* error, const char* message, EGLint glError) {
    if (error) *error = std::string(message) + " (" + eglErrorName(glError) + ")";
    PAE_LOGE("PAE_EGL", "%s: %s (0x%x)", message, eglErrorName(glError), glError);
    return false;
}

} // namespace

EglContextManager::~EglContextManager() {
    releaseAll();
}

void EglContextManager::releaseAll() {
    if (display_ != EGL_NO_DISPLAY) {
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
        config_ = nullptr;
    }
}

bool EglContextManager::chooseConfig(bool relaxed, std::string* error) {
    EGLint depth = relaxed ? 16 : 24;
    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, depth,
        EGL_NONE
    };
    EGLint count = 0;
    if (!eglChooseConfig(display_, attribs, &config_, 1, &count) || count < 1) {
        return setError(error, "eglChooseConfig failed", eglGetError());
    }
    return true;
}

bool EglContextManager::initialize(std::string* error) {
    releaseAll();
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) return setError(error, "eglGetDisplay failed", eglGetError());
    EGLint major = 0, minor = 0;
    if (!eglInitialize(display_, &major, &minor)) {
        return setError(error, "eglInitialize failed", eglGetError());
    }
    PAE_LOGI("PAE_EGL", "EGL initialized version %d.%d", major, minor);
    if (!chooseConfig(false, nullptr)) {
        PAE_LOGW("PAE_EGL", "retrying EGL config with relaxed depth");
        if (!chooseConfig(true, error)) return false;
    }
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        return setError(error, "eglBindAPI failed", eglGetError());
    }
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        return setError(error, "EGL 3.0 context creation failed", eglGetError());
    }
    PAE_LOGI("PAE_EGL", "GLES 3.0 EGL context created");
    return true;
}

bool EglContextManager::createSurface(ANativeWindow* window, std::string* error) {
    if (display_ == EGL_NO_DISPLAY || context_ == EGL_NO_CONTEXT) {
        if (error) *error = "EGL not initialized";
        return false;
    }
    destroySurface();
    surface_ = eglCreateWindowSurface(display_, config_, window, nullptr);
    if (surface_ == EGL_NO_SURFACE) {
        return setError(error, "eglCreateWindowSurface failed", eglGetError());
    }
    return makeCurrent(error);
}

void EglContextManager::destroySurface() {
    if (display_ != EGL_NO_DISPLAY && surface_ != EGL_NO_SURFACE) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display_, surface_);
        surface_ = EGL_NO_SURFACE;
        PAE_LOGI("PAE_EGL", "EGLSurface destroyed");
    }
}

bool EglContextManager::makeCurrent(std::string* error) {
    if (surface_ == EGL_NO_SURFACE || context_ == EGL_NO_CONTEXT) {
        if (error) *error = "surface or context missing";
        return false;
    }
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        return setError(error, "eglMakeCurrent failed", eglGetError());
    }
    return true;
}

void EglContextManager::swapBuffers() {
    if (display_ != EGL_NO_DISPLAY && surface_ != EGL_NO_SURFACE) {
        eglSwapBuffers(display_, surface_);
    }
}

bool EglContextManager::recreate(std::string* error) {
    PAE_LOGW("PAE_EGL", "recreating EGL context after loss");
    ANativeWindow* window = nullptr;
    if (surface_ != EGL_NO_SURFACE && display_ != EGL_NO_DISPLAY) {
        // Window handle is owned by the render thread; the surface recreation path
        // passes it again through SurfaceCreated. Here we only reset GL state.
        destroySurface();
    }
    (void)window;
    return initialize(error);
}

} // namespace pae
