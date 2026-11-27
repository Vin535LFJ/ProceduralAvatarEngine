#pragma once

#include "renderer/egl/EglContextManager.h"
#include "renderer/gl/BonePaletteUbo.h"
#include "renderer/gl/GlCapabilities.h"
#include "renderer/gl/ShaderProgram.h"
#include "renderer/gl/SkinnedGpuMesh.h"
#include "renderer/stats/RenderStats.h"
#include "procedural/character/BodyPatchGenerator.h"
#include "procedural/character/TorsoShoulderPatchGenerator.h"
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
    SetPreviewMode,
    CameraPreset,
    CameraOrbit, // interactive camera: orbit / zoom / pan / reset
    DebugView,
    Shutdown
};

// Interactive camera operations. The fixed evidence presets and the free
// camera share one parameterisation (target + yaw/pitch/distance), so a preset
// is a starting point for inspection rather than a dead end.
enum class CameraOp : int {
    Orbit = 0, // delta.x = yaw degrees, delta.y = pitch degrees
    Zoom = 1,  // delta.x = multiplier (>1 zooms out)
    Pan = 2,   // delta.x/delta.y = screen pixels
    Reset = 3  // snap back to the active evidence preset
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
    void postPreviewMode(int mode);    // 0 avatar / 1 anatomy / 2 P1-A / 3 canonical body / 4 static human template
    void postCameraPreset(int preset); // M12: fixed evidence camera (0..7)
    void postCameraOrbit(int op, const Vec3& delta); // interactive camera
    void postDebugView(int view);      // 0 shading / 1 wireframe / 2 normals / 3 clay / 4 material / 5 weights / 6 outline / 7 UV
    std::string getStats() const;

private:
    void run();
    void processCommands();
    void drawFrame();
    bool ensureResources(std::string* error);
    bool ensureP1aGeometry(int detailLevel);
    bool ensureCanonicalGeometry(int detailLevel);
    bool ensureTemplateGeometry();  // VR-1B WP2: static .paht human template (mode 4)
    bool validateCanonicalGpuPath(const SkinnedGpuMesh& renderMesh,
                                  const MeshData& cpuMesh,
                                  const Skeleton& skeleton,
                                  uint64_t revision);
    bool activatePreviewGpuResources(int mode);
    void releaseGpuResources();
    void updateBonePalette(int64_t timestampNs);
    MeshData buildGroundShadow() const;
    MeshData buildAnatomyLineMesh(const MeshData& mesh) const;
    MeshData buildGroundGrid(float spacing, float halfExtent, int mode) const;
    void applyCameraPreset(int preset);
    void applyCameraOp(int op, const Vec3& delta);
    Vec3 resolveCameraEye() const;
    void drawOutline(const Mat4& viewProjection, const Mat4& model);
    void drawAnatomyPreview(const Mat4& viewProjection, const Mat4& model);

    EglContextManager egl_;
    ShaderProgram skinnedProgram_;
    ShaderProgram skinnedLineProgram_;
    ShaderProgram uvProgram_;
    GLuint validationProgram_ = 0;
    ShaderProgram backgroundProgram_;
    ShaderProgram outlineProgram_; // inverted-hull cartoon outline (M10)
    ShaderProgram shadowProgram_;  // ground contact shadow disc (M10)
    ShaderProgram anatomyProgram_;    // M12: anatomy shading (neutral skin)
    ShaderProgram anatomyNormalProg_; // M12: normals debug view
    ShaderProgram anatomyClayProg_;   // M13: gray-clay smooth-lambert study view
    SkinnedGpuMesh anatomyMesh_;      // M12: unified SDF upper body
    SkinnedGpuMesh anatomyLines_;     // M12: wireframe edge buffer
    SkinnedGpuMesh p1aMesh_;           // P1-A: surface-patch torso/shoulder/arm
    SkinnedGpuMesh p1aLines_;          // P1-A: wireframe edge buffer
    SkinnedGpuMesh canonicalMesh_;     // P1-A3: continuous full-body canonical shell
    SkinnedGpuMesh canonicalLines_;    // P1-A3: skinned topology wireframe
    SkinnedGpuMesh canonicalUvMesh_;   // P1-B: split Render Mesh with parallel UV VBO
    // VR-1B WP2 (additive): static full-body human template loaded from a .paht
    // file. Rendered under an identity bone palette (mode-1 precedent); no
    // LegacyRigV1 / UBO[22] / packing change. CPU cache survives surface recreate.
    SkinnedGpuMesh templateMesh_;
    SkinnedGpuMesh templateLines_;
    MeshData cachedTemplateMesh_;
    bool hasCachedTemplate_ = false;
    std::string templateStatus_ = "template=not-loaded";
    PatchGenerationReport patchReport_;
    character::TorsoShoulderTimings p1aTimings_{};
    SkinnedGpuMesh avatarMesh_;
    SkinnedGpuMesh avatarLines_;  // avatar wireframe edge buffer
    SkinnedGpuMesh gridMesh_;     // world-space reference grid (static)
    SkinnedGpuMesh axisMesh_;     // world origin axes (static)
    SkinnedGpuMesh shadowMesh_;
    BonePaletteUbo boneUbo_;
    BonePaletteUboData paletteData_;
    Skeleton skeleton_;
    BodyJoints joints_;
    MotionRuntime motion_;
    // DEF-01 (Plan B): CPU-side geometry cache. A surface (re)creation can then
    // re-upload GPU buffers instead of regenerating geometry, which used to
    // block the render thread for ~4.1 s on device and showed a black window.
    MeshData cachedAvatarMesh_;
    MeshData cachedAnatomyMesh_;
    MeshData cachedP1aMesh_;
    Skeleton cachedP1aSkeleton_{};
    BodyJoints cachedP1aJoints_{};
    std::string cachedP1aCacheKey_;
    MeshData cachedCanonicalMesh_;
    MeshData cachedCanonicalRenderMesh_;
    Skeleton cachedCanonicalSkeleton_{};
    BodyJoints cachedCanonicalJoints_{};
    std::vector<uint32_t> canonicalRenderToCanonical_;
    bool hasCachedAvatar_ = false;
    bool avatarGeometryDirty_ = false;
    bool hasCachedAnatomy_ = false;
    bool hasCachedP1a_ = false;
    bool hasCachedCanonical_ = false;
    int canonicalDetailLevel_ = -1;
    uint64_t canonicalRevision_ = 0;
    uint64_t validatedCanonicalRevision_ = 0;
    GlCapabilities caps_;
    RenderStats stats_;

    // Natural adult palette (Asian skin, dark hair, neutral gray outfit).
    std::array<Vec3, 5> appearance_{
        Vec3{0.76f, 0.60f, 0.50f}, Vec3{0.085f, 0.075f, 0.075f},
        Vec3{0.44f, 0.43f, 0.42f}, Vec3{0.94f, 0.94f, 0.92f},
        Vec3{0.30f, 0.30f, 0.31f}
    };
    int backgroundMode_ = 0;
    Vec3 backgroundA_{0.28f, 0.29f, 0.30f};
    Vec3 backgroundB_{0.28f, 0.29f, 0.30f};
    int poseMode_ = -1;
    int qualityLevel_ = 1;
    int outfitVariant_ = 0;
    int previewMode_ = 0;  // 0 avatar / 1 anatomy / 2 P1-A / 3 canonical body / 4 static human template
    int cameraPreset_ = 0; // M12 evidence camera index
    // Interactive camera state. Shared parameterisation with the presets so a
    // preset can be orbited away from and snapped back to.
    Vec3 cameraTarget_{0.0f, 0.90f, 0.0f};
    float cameraYawDeg_ = 0.0f;   // 0 = camera on +Z looking at -Z (front)
    float cameraPitchDeg_ = 0.0f; // positive = camera above the target
    float cameraDistance_ = 2.05f;
    bool cameraFree_ = false;     // true once the user has moved off a preset
    int debugView_ = 0;    // 0 shading / 1 wireframe / 2 normals / 3 gray clay (M13)
    uint64_t avatarRevision_ = 1;
    uint64_t sceneRevision_ = 1;

    std::vector<RenderCommand> queue_;
    std::vector<RenderCommand> deferredCommands_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::thread thread_;
    bool running_ = false;
    bool stopRequested_ = false;
    bool eglReady_ = false;
    bool resourcesReady_ = false;
    GLint uniformViewProj_ = -1;
    GLint uniformModel_ = -1;
    GLint uniformNormalMatrix_ = -1;
    GLint uniformLineViewProj_ = -1;
    GLint uniformLineModel_ = -1;
    GLint uniformLineColor_ = -1;
    GLint uniformCameraPos_ = -1;
    GLint uniformDebugMode_ = -1;
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
    GLint uniformOutlineNormalMatrix_ = -1;
    GLint uniformOutlineOffset_ = -1;
    GLint uniformOutlineColor_ = -1;
    GLint uniformShadowViewProj_ = -1;
    GLint uniformShadowModel_ = -1;
    GLint uniformShadowColor_ = -1;
    GLint uniformAnatomyViewProj_ = -1;
    GLint uniformAnatomyModel_ = -1;
    GLint uniformAnatomyNormalMatrix_ = -1;
    GLint uniformAnatomyNeutral_ = -1;
    GLint uniformAnatomyNormalViewProj_ = -1;
    GLint uniformAnatomyNormalModel_ = -1;
    GLint uniformAnatomyNormalNormalMatrix_ = -1;
GLint uniformAnatomyClayViewProj_ = -1;
GLint uniformAnatomyClayModel_ = -1;
GLint uniformAnatomyClayNormalMatrix_ = -1;
GLint uniformAnatomyClayNeutral_ = -1;
GLint uniformAnatomyClayKeyDir_ = -1;
GLint uniformAnatomyClayFillDir_ = -1;
    GLint uniformUvViewProj_ = -1;
    GLint uniformUvModel_ = -1;
    GLint uniformUvNormalMatrix_ = -1;
    GLint uniformUvLightDir_ = -1;
    GLint uniformUvCheckerScale_ = -1;
    GLint uniformValidationModel_ = -1;
    GLint uniformValidationNormalMatrix_ = -1;
    int width_ = 0;
    int height_ = 0;
    uint32_t maxQueuedCommands_ = 64;
    std::string startupError_;
};

} // namespace pae
