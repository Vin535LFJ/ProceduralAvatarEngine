#pragma once

#include "renderer/egl/EglContextManager.h"
#include "renderer/gl/BonePaletteUbo.h"
#include "renderer/gl/GlCapabilities.h"
#include "renderer/gl/ShaderProgram.h"
#include "renderer/gl/SkinnedGpuMesh.h"
#include "renderer/stats/RenderStats.h"
#include "motion/MotionRuntime.h"
#include "skeleton/Skeleton.h"

#include <android/native_window.h>

#include <array>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace pae {

enum class RenderCommandType : int {
    SurfaceCreated,
    SurfaceChanged,
    SurfaceDestroyed,
    SetAppearance,
    SetBackground,
    SetPoseMode,
    SetExternalPose,
    MotionReset,
    SetQuality,
    AnatomyPreview,
    CameraPreset,
    DebugView,
    Shutdown
};

struct RenderCommand {
    RenderCommandType type = RenderCommandType::Shutdown;
    ANativeWindow* window = nullptr; // owned by the command until consumed
    int width = 0;
    int height = 0;
    int value = 0;
    Vec3 vecA{};
    Vec3 vecB{};
    std::array<Vec3, 5> palette{};
    PoseFrame pose{};
    bool flag = false; // preview/debug toggles ride on the generic flag
};

class RenderThread {
public:
    explicit RenderThread(uint32_t maxQueuedCommands);
    ~RenderThread();
    RenderThread(const RenderThread&) = delete;
    RenderThread& operator=(const RenderThread&) = delete;

    bool start(std::string* error);
    void stop();

    void postSurfaceCreated(ANativeWindow* window, int width, int height);
    void postSurfaceChanged(int width, int height);
    void postSurfaceDestroyed();
    void postAppearance(const std::array<Vec3, 5>& palette);
    void postBackground(int mode, const Vec3& colorA, const Vec3& colorB);
    void postPoseMode(int mode); // -1 = automatic test sequence
    void postExternalPose(const PoseFrame& pose);
    void postMotionReset(); // clear filters/smoothing (video seek, replay, speed change)
    void postQuality(int level); // Low=0 / Medium=1 / High=2 -> geometry detail rebuild
    void postAnatomyPreview(bool on);  // M12: unified SDF upper-body preview
    void postCameraPreset(int preset); // M12: fixed evidence camera (0..7)
    void postDebugView(int view);      // M12: 0 shading / 1 wireframe / 2 normals
    std::string getStats() const;

private:
    void run();
    void processCommands();
    void drawFrame();
    bool ensureResources(std::string* error);
    void releaseGpuResources();
    void updateBonePalette(int64_t timestampNs);
    MeshData buildGroundShadow() const;
    MeshData buildAnatomyLineMesh(const MeshData& mesh) const;
    void drawOutline(const Mat4& viewProjection, const Mat4& model);
    void drawAnatomyPreview(const Mat4& viewProjection, const Mat4& model);

    EglContextManager egl_;
    ShaderProgram skinnedProgram_;
    ShaderProgram backgroundProgram_;
    ShaderProgram outlineProgram_; // inverted-hull cartoon outline (M10)
    ShaderProgram shadowProgram_;  // ground contact shadow disc (M10)
    ShaderProgram anatomyProgram_;    // M12: anatomy shading (neutral skin)
    ShaderProgram anatomyNormalProg_; // M12: normals debug view
    SkinnedGpuMesh anatomyMesh_;      // M12: unified SDF upper body
    SkinnedGpuMesh anatomyLines_;     // M12: wireframe edge buffer
    SkinnedGpuMesh avatarMesh_;
    SkinnedGpuMesh shadowMesh_;
    BonePaletteUbo boneUbo_;
    BonePaletteUboData paletteData_;
    Skeleton skeleton_;
    BodyJoints joints_;
    MotionRuntime motion_;
    GlCapabilities caps_;
    RenderStats stats_;

    std::array<Vec3, 5> appearance_{
        Vec3{0.98f, 0.79f, 0.66f}, Vec3{0.26f, 0.15f, 0.09f},
        Vec3{0.32f, 0.58f, 0.88f}, Vec3{1.00f, 1.00f, 1.00f}
    };
    int backgroundMode_ = 2;
    Vec3 backgroundA_{0.08f, 0.10f, 0.18f};
    Vec3 backgroundB_{0.28f, 0.36f, 0.52f};
    int poseMode_ = -1;
    int qualityLevel_ = 1;
    int previewMode_ = 0;  // 0 off, 1 anatomy preview
    int cameraPreset_ = 0; // M12 evidence camera index
    int debugView_ = 0;    // 0 shading / 1 wireframe / 2 normals
    uint64_t avatarRevision_ = 1;
    uint64_t sceneRevision_ = 1;

    std::vector<RenderCommand> queue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::thread thread_;
    bool running_ = false;
    bool stopRequested_ = false;
    bool eglReady_ = false;
    bool resourcesReady_ = false;
    GLint uniformViewProj_ = -1;
    GLint uniformModel_ = -1;
    GLint uniformSkin_ = -1;
    GLint uniformHair_ = -1;
    GLint uniformCloth_ = -1;
    GLint uniformEye_ = -1;
    GLint uniformPants_ = -1;
    GLint uniformBgMode_ = -1;
    GLint uniformBgA_ = -1;
    GLint uniformBgB_ = -1;
    GLint uniformResolution_ = -1;
    GLint uniformOutlineViewProj_ = -1;
    GLint uniformOutlineModel_ = -1;
    GLint uniformOutlineOffset_ = -1;
    GLint uniformOutlineColor_ = -1;
    GLint uniformShadowViewProj_ = -1;
    GLint uniformShadowModel_ = -1;
    GLint uniformShadowColor_ = -1;
    GLint uniformAnatomyViewProj_ = -1;
    GLint uniformAnatomyModel_ = -1;
    GLint uniformAnatomyNeutral_ = -1;
    GLint uniformAnatomyNormalViewProj_ = -1;
    GLint uniformAnatomyNormalModel_ = -1;
    int width_ = 0;
    int height_ = 0;
    uint32_t maxQueuedCommands_ = 64;
    std::string startupError_;
};

} // namespace pae
