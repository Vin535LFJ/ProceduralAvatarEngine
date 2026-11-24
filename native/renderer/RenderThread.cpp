#include "RenderThread.h"

#include "core/config/EngineConfig.h"
#include "core/diagnostics/Log.h"
#include "core/math/Math.h"
#include "procedural/MeshTopology.h"
#include "procedural/humanoid/Humanoid.h"
#include "procedural/humanoid/anatomy/HumanAnatomyGenerator.h"
#include "skeleton/Skinning.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <cmath>
#include <cstdio>
#include <utility>

namespace pae {
namespace {

constexpr const char* kSkinnedVertex = R"GLSL(#version 300 es
layout(std140) uniform BoneBlock { mat4 uBones[22]; };
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in uvec4 aJoints;
layout(location = 3) in vec4 aWeights;
layout(location = 4) in vec4 aColor;
uniform mat4 uViewProj;
uniform mat4 uModel;
out vec3 vNormal;
out vec4 vColor;
out vec3 vWorldPos;
flat out uvec4 vJoints;
flat out vec4 vWeights;
void main() {
    mat4 skin = uBones[aJoints.x] * aWeights.x
              + uBones[aJoints.y] * aWeights.y
              + uBones[aJoints.z] * aWeights.z
              + uBones[aJoints.w] * aWeights.w;
    vec4 world = uModel * skin * vec4(aPosition, 1.0);
    vNormal = normalize(mat3(uModel) * mat3(skin) * aNormal);
    vColor = aColor;
    vWorldPos = world.xyz;
    vJoints = aJoints;
    vWeights = aWeights;
    gl_Position = uViewProj * world;
}
)GLSL";

// Realistic character material shading. No toon quantisation: continuous
// Lambert + Blinn-Phong specular, material-dependent roughness, subtle
// procedural micro variation and a hemispheric ambient term. Texture detail
// is derived analytically from world position so the frozen 64-byte vertex
// layout (which carries no UV) does not need to change.
constexpr const char* kSkinnedFragment = R"GLSL(#version 300 es
precision highp float;
layout(std140) uniform BoneBlock { mat4 uBones[22]; };
in vec3 vNormal;
in vec4 vColor;
in vec3 vWorldPos;
flat in uvec4 vJoints;
flat in vec4 vWeights;
uniform vec3 uSkinColor;
uniform vec3 uHairColor;
uniform vec3 uClothColor;
uniform vec3 uEyeColor;
uniform vec3 uPantsColor;
uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform float uAmbient;
uniform float uToonSteps;
uniform float uRim;
uniform int uDebugMode; // 0 material / 2 normals / 3 clay / 4 material id / 5 weights
out vec4 fragColor;

float hash31(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}
float noise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = hash31(i);
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));
    return mix(mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y),
               mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y), f.z);
}

// Per-bone tint used by the skin-weight inspection view. Derived from the
// bone index only, so it never depends on the bone palette contents.
vec3 boneTint(uint boneIndex) {
    float h = fract(float(boneIndex) * 0.6180339887 + 0.13);
    vec3 k = 0.5 + 0.5 * cos(6.2831853 * (h + vec3(0.0, 0.33, 0.67)));
    return mix(vec3(0.22), k, 0.9);
}

void main() {
    const float mid = 5.0;
    float materialId = vColor.w;
    vec3 n = normalize(vNormal);

    // ---- Inspection views that bypass the material model entirely. ----
    if (uDebugMode == 2) {            // world normals
        fragColor = vec4(n * 0.5 + 0.5, 1.0);
        return;
    }
    if (uDebugMode == 4) {            // material id
        vec3 idColor = vec3(0.34, 0.34, 0.38);
        if (materialId < 0.5)      idColor = vec3(0.86, 0.70, 0.60);
        else if (materialId < 1.5) idColor = vec3(0.18, 0.16, 0.18);
        else if (materialId < 2.5) idColor = vec3(0.24, 0.46, 0.76);
        else if (materialId < 3.5) idColor = vec3(0.95, 0.95, 0.90);
        else if (materialId < 4.5) idColor = vec3(0.05, 0.05, 0.07);
        float lambert = 0.35 + 0.65 * max(dot(n, normalize(uLightDir)), 0.0);
        fragColor = vec4(idColor * lambert, 1.0);
        return;
    }
    if (uDebugMode == 5) {            // skin weight heatmap
        vec3 heat = vec3(0.0);
        heat += boneTint(vJoints.x) * vWeights.x;
        heat += boneTint(vJoints.y) * vWeights.y;
        heat += boneTint(vJoints.z) * vWeights.z;
        heat += boneTint(vJoints.w) * vWeights.w;
        float lambert = 0.55 + 0.45 * max(dot(n, normalize(uLightDir)), 0.0);
        fragColor = vec4(heat * lambert, 1.0);
        return;
    }
    if (uDebugMode == 3) {            // neutral clay, form only
        float key = max(dot(n, normalize(vec3(0.45, 0.75, 0.48))), 0.0);
        float fill = max(dot(n, normalize(vec3(-0.55, 0.15, 0.35))), 0.0);
        fragColor = vec4(vec3(0.62) * (0.22 + 0.75 * key + 0.22 * fill), 1.0);
        return;
    }

    vec3 base = materialId < 0.5 ? uSkinColor
              : (materialId < 1.5 ? uHairColor
              : (materialId < 2.5 ? uClothColor
              : (materialId < 3.5 ? uEyeColor
              : (materialId < 4.5 ? uPantsColor : vec3(0.035, 0.032, 0.038)))));

    // Real camera position: the view direction must follow the camera, while
    // the lights stay fixed in world space.
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 lightDir = normalize(uLightDir);
    vec3 halfDir = normalize(lightDir + viewDir);
    float ndl = max(dot(n, lightDir), 0.0);
    float ndh = max(dot(n, halfDir), 0.0);

    float rough = 0.55;
    float specStrength = 0.12;
    if (materialId < 0.5) {
        rough = 0.42; specStrength = 0.18;
        float pores = noise3(vWorldPos * 320.0);
        float tone = noise3(vWorldPos * 18.0);
        base *= 0.92 + 0.10 * pores + 0.07 * tone;
        float wrap = max(dot(n, lightDir) * 0.5 + 0.5, 0.0);
        ndl = mix(ndl, wrap, 0.28);
    } else if (materialId < 1.5) {
        rough = 0.22; specStrength = 0.42;
        float strands = noise3(vWorldPos * vec3(180.0, 26.0, 180.0));
        base *= 0.78 + 0.34 * strands;
    } else if (materialId < 2.5 || (materialId >= 4.5 && materialId < mid)) {
        rough = 0.72; specStrength = 0.05;
        float weave = noise3(vWorldPos * 240.0);
        base *= 0.90 + 0.14 * weave;
    } else if (materialId < 3.5) {
        rough = 0.12; specStrength = 0.65;
    }
    base *= 0.88 + 0.12 * clamp(vColor.rgb, 0.0, 1.0);

    float gloss = pow(ndh, max(4.0, 2.0 / max(rough * rough, 0.02)));
    float hemi = 0.5 + 0.5 * n.y;
    vec3 ambient = base * mix(uAmbient * 0.75, uAmbient * 1.30, hemi);
    vec3 fill = base * max(dot(n, normalize(vec3(-0.6, 0.3, -0.4))), 0.0) * 0.22;
    vec3 diffuse = base * ndl * 1.10;
    vec3 specular = vec3(specStrength) * gloss * (0.35 + 0.65 * ndl);
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0) * uRim;
    vec3 color = ambient + fill + diffuse + specular + vec3(rim) * base;
    color = pow(color, vec3(0.92));
    fragColor = vec4(color, 1.0);
}
)GLSL";

constexpr const char* kAnatomyNeutralFragment = R"GLSL(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec4 vColor;
uniform vec3 uNeutral;
uniform vec3 uLightDir;
uniform float uAmbient;
uniform float uToonSteps;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNormal);
    float ndl = max(dot(n, normalize(uLightDir)), 0.0);
    float toon = floor(ndl * uToonSteps) / max(uToonSteps, 1.0);
    float shade = mix(uAmbient, 1.0, toon);
    vec3 base = uNeutral; // neutral matte study material
    fragColor = vec4(base * shade, 1.0);
}
)GLSL";

constexpr const char* kAnatomyNormalsFragment = R"GLSL(#version 300 es
precision mediump float;
in vec3 vNormal;
out vec4 fragColor;
void main() { fragColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0); }
)GLSL";

// M13: gray-clay study view. Continuous smooth Lambert only — no toon
// quantisation, no rim, no palette. Toon banding in the old neutral view
// quantised the shading into 6 flat rings, which hid (or invented) surface
// detail and made morphology acceptance unreliable. Neutral gray + a key and
// a dim fill light so form is read purely from curvature.
constexpr const char* kAnatomyClayFragment = R"GLSL(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec4 vColor;
uniform vec3 uNeutral;
uniform vec3 uKeyDir;
uniform vec3 uFillDir;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNormal);
    float key = max(dot(n, normalize(uKeyDir)), 0.0);
    float fill = max(dot(n, normalize(uFillDir)), 0.0);
    float lambert = 0.22 + 0.75 * key + 0.22 * fill;
    fragColor = vec4(uNeutral * lambert, 1.0);
}
)GLSL";

constexpr const char* kBackgroundVertex = R"GLSL(#version 300 es
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

constexpr const char* kOutlineVertex = R"GLSL(#version 300 es
layout(std140) uniform BoneBlock { mat4 uBones[22]; };
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in uvec4 aJoints;
layout(location = 3) in vec4 aWeights;
layout(location = 4) in vec4 aColor;
uniform mat4 uViewProj;
uniform mat4 uModel;
uniform float uOffset;
out float vMaterial;
void main() {
    mat4 skin = uBones[aJoints.x] * aWeights.x
              + uBones[aJoints.y] * aWeights.y
              + uBones[aJoints.z] * aWeights.z
              + uBones[aJoints.w] * aWeights.w;
    vec4 world = uModel * skin * vec4(aPosition, 1.0);
    vec3 worldNormal = mat3(uModel) * mat3(skin) * aNormal;
    world.xyz += normalize(worldNormal) * uOffset;
    vMaterial = aColor.w;
    gl_Position = uViewProj * world;
}
)GLSL";

constexpr const char* kFlatColorVertex = R"GLSL(#version 300 es
layout(location = 0) in vec3 aPosition;
uniform mat4 uViewProj;
uniform mat4 uModel;
void main() {
    gl_Position = uViewProj * uModel * vec4(aPosition, 1.0);
}
)GLSL";

constexpr const char* kFlatColorFragment = R"GLSL(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; }
)GLSL";

constexpr const char* kOutlineFragment = R"GLSL(#version 300 es
precision mediump float;
uniform vec4 uColor;
in float vMaterial;
out vec4 fragColor;
void main() {
    // Ink features (eyes/mouth) must not grow outline halos.
    if (vMaterial >= 3.5) discard;
    fragColor = uColor;
}
)GLSL";

constexpr const char* kBackgroundFragment = R"GLSL(#version 300 es
precision mediump float;
uniform int uMode;
uniform vec3 uColorA;
uniform vec3 uColorB;
uniform vec2 uResolution;
out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec3 color;
    if (uMode == 0) {
        color = uColorA;
    } else if (uMode == 1) {
        color = mix(uColorA, uColorB, uv.y);
    } else {
        float floorLine = smoothstep(0.42, 0.44, uv.y);
        color = mix(uColorB, uColorA, floorLine);
        float column = step(0.985, fract(uv.x * 12.0)) * step(0.02, uv.y);
        color += vec3(column * 0.06);
    }
    fragColor = vec4(color, 1.0);
}
)GLSL";

// Fixed evidence cameras. These are the reproducible capture rig; the
// interactive camera is expressed in the same parameterisation so a preset
// can be orbited away from and snapped back to.
constexpr int kPresetCount = 8;
const Vec3 kPresetEye[kPresetCount] = {
    {0.0f, 0.98f, 2.05f},    // 0 FRONT (full body)
    {0.0f, 0.98f, -2.05f},   // 1 BACK
    {2.05f, 0.98f, 0.0f},    // 2 LEFT (+X side)
    {-2.05f, 0.98f, 0.0f},   // 3 RIGHT
    {1.45f, 0.98f, 1.45f},   // 4 45 LEFT
    {-1.45f, 0.98f, 1.45f},  // 5 45 RIGHT
    {0.42f, 1.55f, 0.78f},   // 6 SHOULDER CLOSEUP
    {0.0f, 1.62f, 0.42f},    // 7 FACE CLOSEUP
};
const Vec3 kPresetTarget[kPresetCount] = {
    {0.0f, 0.90f, 0.0f}, {0.0f, 0.90f, 0.0f}, {0.0f, 0.90f, 0.0f},
    {0.0f, 0.90f, 0.0f}, {0.0f, 0.90f, 0.0f}, {0.0f, 0.90f, 0.0f},
    {0.16f, 1.34f, 0.0f}, {0.0f, 1.61f, 0.0f},
};

constexpr float kRadToDeg = 57.2957795f;
constexpr float kDegToRad = 0.0174532925f;

} // namespace

RenderThread::RenderThread(uint32_t maxQueuedCommands)
    : maxQueuedCommands_(maxQueuedCommands) {
    // Start on evidence preset 0 (front, full body) so the first frame is
    // reproducible before any interactive input arrives.
    applyCameraPreset(0);
}

RenderThread::~RenderThread() {
    stop();
}

bool RenderThread::start(std::string* error) {
    if (running_) return true;
    stopRequested_ = false;
    running_ = true;
    thread_ = std::thread(&RenderThread::run, this);
    (void)error;
    return true;
}

void RenderThread::stop() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (!running_) return;
        stopRequested_ = true;
        queueCv_.notify_all();
    }
    if (thread_.joinable()) thread_.join();
    running_ = false;
}

void RenderThread::postSurfaceCreated(ANativeWindow* window, int width, int height) {
    RenderCommand command;
    command.type = RenderCommandType::SurfaceCreated;
    command.window = window;
    command.width = width;
    command.height = height;
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (queue_.size() >= maxQueuedCommands_) {
        RenderCommand dropped = queue_.front();
        queue_.erase(queue_.begin());
        if (dropped.window) ANativeWindow_release(dropped.window);
        PAE_LOGW("PAE_RENDER", "command queue full, dropped oldest command");
    }
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postSurfaceChanged(int width, int height) {
    RenderCommand command;
    command.type = RenderCommandType::SurfaceChanged;
    command.width = width;
    command.height = height;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postSurfaceDestroyed() {
    RenderCommand command;
    command.type = RenderCommandType::SurfaceDestroyed;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postAppearance(const std::array<Vec3, 5>& palette) {
    RenderCommand command;
    command.type = RenderCommandType::SetAppearance;
    command.palette = palette;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postBackground(int mode, const Vec3& colorA, const Vec3& colorB) {
    RenderCommand command;
    command.type = RenderCommandType::SetBackground;
    command.value = mode;
    command.vecA = colorA;
    command.vecB = colorB;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postPoseMode(int mode) {
    RenderCommand command;
    command.type = RenderCommandType::SetPoseMode;
    command.value = mode;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postExternalPose(const PoseFrame& pose) {
    RenderCommand command;
    command.type = RenderCommandType::SetExternalPose;
    command.pose = pose;
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (queue_.size() >= maxQueuedCommands_) {
        queue_.erase(queue_.begin()); // pose frames are stale; drop oldest
    }
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postMotionReset() {
    RenderCommand command;
    command.type = RenderCommandType::MotionReset;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postAnatomyPreview(bool on) {
    RenderCommand command;
    command.type = RenderCommandType::AnatomyPreview;
    command.flag = on;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postPreviewMode(int mode) {
    RenderCommand command;
    command.type = RenderCommandType::SetPreviewMode;
    command.value = std::clamp(mode, 0, 2);
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postCameraPreset(int preset) {
    RenderCommand command;
    command.type = RenderCommandType::CameraPreset;
    command.value = preset;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postCameraOrbit(int op, const Vec3& delta) {
    RenderCommand command;
    command.type = RenderCommandType::CameraOrbit;
    command.value = op;
    command.vecA = delta;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postDebugView(int view) {
    RenderCommand command;
    command.type = RenderCommandType::DebugView;
    command.value = view;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

void RenderThread::postQuality(int level) {
    RenderCommand command;
    command.type = RenderCommandType::SetQuality;
    command.value = level;
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(command);
    queueCv_.notify_one();
}

std::string RenderThread::getStats() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    char camera[160];
    std::snprintf(camera, sizeof(camera),
                  " cam=%.1f/%.1f/%.2f yaw=%.1f pitch=%.1f dist=%.2f %s",
                  cameraTarget_.x, cameraTarget_.y, cameraTarget_.z,
                  cameraYawDeg_, cameraPitchDeg_, cameraDistance_,
                  cameraFree_ ? "free" : "preset");
    return stats_.toString() + " avatar_rev=" + std::to_string(avatarRevision_) +
           " scene_rev=" + std::to_string(sceneRevision_) + camera + " " + startupError_;
}

void RenderThread::run() {
    std::string error;
    eglReady_ = egl_.initialize(&error);
    if (!eglReady_) {
        startupError_ = error;
        PAE_LOGE("PAE_RENDER", "EGL init failed: %s", error.c_str());
        return;
    }
    while (!stopRequested_) {
        processCommands();
        if (eglReady_ && egl_.surfaceCreated() && resourcesReady_) {
            drawFrame();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        } else {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait_for(lock, std::chrono::milliseconds(8),
                              [this] { return stopRequested_ || !queue_.empty(); });
        }
    }
    releaseGpuResources();
    egl_.destroySurface();
}

void RenderThread::processCommands() {
    std::vector<RenderCommand> commands;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        commands.swap(queue_);
    }
    for (const RenderCommand& command : commands) {
        switch (command.type) {
            case RenderCommandType::SurfaceCreated: {
                std::string error;
                if (!command.window) break;
                width_ = command.width;
                height_ = command.height;
                const bool ok = egl_.createSurface(command.window, &error);
                ANativeWindow_release(command.window);
                if (!ok) {
                    startupError_ = error;
                    PAE_LOGE("PAE_RENDER", "surface create failed: %s", error.c_str());
                    break;
                }
                if (!ensureResources(&error)) {
                    startupError_ = error;
                    PAE_LOGE("PAE_RENDER", "resource init failed: %s", error.c_str());
                } else {
                    PAE_LOGI("PAE_RENDER", "surface ready %dx%d", width_, height_);
                }
                break;
            }
            case RenderCommandType::SurfaceChanged:
                width_ = command.width;
                height_ = command.height;
                break;
            case RenderCommandType::SurfaceDestroyed:
                releaseGpuResources();
                egl_.destroySurface();
                break;
            case RenderCommandType::SetAppearance: {
                const int previousVariant = outfitVariant_;
                appearance_ = command.palette;
                // Outfit variant is derived from the cloth palette so the
                // frozen JNI ABI can switch a full clothing configuration
                // through the existing appearance command.
                outfitVariant_ = appearance_[2].x > appearance_[2].z ? 1 : 0;
                if (outfitVariant_ != previousVariant && hasCachedAvatar_) {
                    BodyParameters params{};
                    params.detailLevel = detailLevelForQuality(static_cast<QualityLevel>(qualityLevel_));
                    if (outfitVariant_ == 1) {
                        params.clothing.thickness = 0.018f;
                        params.clothing.shirtHem = 0.86f;
                        params.clothing.sleeveLength = 0.40f;
                        params.clothing.pantsLength = 0.82f;
                    }
                    const HumanoidResult humanoid = generateHumanoid(params, false);
                    if (humanoid.valid) {
                        cachedAvatarMesh_ = humanoid.mesh;
                        skeleton_ = humanoid.skeleton;
                        joints_ = humanoid.joints;
                        avatarMesh_.upload(cachedAvatarMesh_);
                        ++avatarRevision_;
                    }
                }
                ++sceneRevision_;
                break;
            }
            case RenderCommandType::SetBackground:
                backgroundMode_ = command.value;
                backgroundA_ = command.vecA;
                backgroundB_ = command.vecB;
                ++sceneRevision_;
                break;
            case RenderCommandType::SetPoseMode:
                poseMode_ = command.value;
                // Explicit synthetic selection takes over from any external
                // source (e.g. a one-shot static-image feed) — last source wins.
                motion_.setExternalEnabled(false);
                break;
            case RenderCommandType::SetExternalPose:
                motion_.setExternalEnabled(true);
                motion_.pushExternalFrame(command.pose);
                break;
            case RenderCommandType::MotionReset:
                motion_.resetFilters();
                break;
            case RenderCommandType::AnatomyPreview:
                previewMode_ = command.flag ? 1 : 0;
                PAE_LOGI("PAE_RENDER", "applied anatomyPreview=%d", previewMode_);
                break;
            case RenderCommandType::SetPreviewMode:
                previewMode_ = command.value;
                PAE_LOGI("PAE_RENDER", "applied previewMode=%d", previewMode_);
                break;
            case RenderCommandType::CameraPreset:
                cameraPreset_ = command.value;
                applyCameraPreset(cameraPreset_);
                PAE_LOGI("PAE_RENDER", "applied cameraPreset=%d", cameraPreset_);
                break;
            case RenderCommandType::CameraOrbit:
                applyCameraOp(command.value, command.vecA);
                break;
            case RenderCommandType::DebugView:
                debugView_ = command.value;
                PAE_LOGI("PAE_RENDER", "applied debugView=%d", debugView_);
                break;
            case RenderCommandType::SetQuality: {
                const int level = command.value;
                if (level < 0 || level > 2) break;
                qualityLevel_ = level;
                const QualityLevel quality = static_cast<QualityLevel>(level);
                BodyParameters params{};
                params.detailLevel = detailLevelForQuality(quality);
                const HumanoidResult humanoid = generateHumanoid(params, false);
                if (humanoid.valid && avatarMesh_.upload(humanoid.mesh)) {
                    skeleton_ = humanoid.skeleton;
                    joints_ = humanoid.joints;
                    // DEF-01 (Plan B): keep the geometry cache in sync with the
                    // rebuilt avatar so a later surface re-creation re-uploads
                    // instead of regenerating.
                    cachedAvatarMesh_ = humanoid.mesh;
                    hasCachedAvatar_ = true;
                    ++avatarRevision_; // geometry rebuilt: full avatar revision bump
                    stats_.setBufferBytes(static_cast<uint64_t>(
                        humanoid.mesh.vertices.size() * 64 +
                        humanoid.mesh.indices.size() * sizeof(uint32_t) +
                        BonePaletteUboData::kBytes));
                    PAE_LOGI("PAE_RENDER", "quality=%s triangles=%u vertices=%zu",
                             qualityName(quality),
                             static_cast<uint32_t>(humanoid.mesh.indices.size() / 3),
                             humanoid.mesh.vertices.size());
                } else {
                    PAE_LOGE("PAE_RENDER", "quality rebuild failed for level %d", level);
                }
                break;
            }
            case RenderCommandType::Shutdown:
                stopRequested_ = true;
                break;
        }
    }
}

bool RenderThread::ensureResources(std::string* error) {
    if (resourcesReady_) return true;
    caps_ = queryGlCapabilities();
    logGlCapabilities(caps_);
    if (!caps_.supportsGles3()) {
        if (error) *error = "GLES 3.0 unsupported";
        return false;
    }
    if (!skinnedProgram_.build(kSkinnedVertex, kSkinnedFragment, error)) return false;
    if (!skinnedProgram_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "BoneBlock binding failed";
        return false;
    }
    if (!backgroundProgram_.build(kBackgroundVertex, kBackgroundFragment, error)) return false;
    if (!outlineProgram_.build(kOutlineVertex, kOutlineFragment, error)) return false;
    if (!outlineProgram_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "outline BoneBlock binding failed";
        return false;
    }
    if (!shadowProgram_.build(kFlatColorVertex, kFlatColorFragment, error)) return false;
    uniformOutlineViewProj_ = outlineProgram_.uniform("uViewProj");
    uniformOutlineModel_ = outlineProgram_.uniform("uModel");
    uniformOutlineOffset_ = outlineProgram_.uniform("uOffset");
    uniformOutlineColor_ = outlineProgram_.uniform("uColor");
    uniformShadowViewProj_ = shadowProgram_.uniform("uViewProj");
    uniformShadowModel_ = shadowProgram_.uniform("uModel");
    uniformShadowColor_ = shadowProgram_.uniform("uColor");
    if (!shadowMesh_.upload(buildGroundShadow())) {
        if (error) *error = "ground shadow upload failed";
        return false;
    }
    // World-space reference grid + origin axes. Static geometry, no skinning.
    if (!gridMesh_.upload(buildGroundGrid(0.25f, 2.5f, 0))) {
        if (error) *error = "ground grid upload failed";
        return false;
    }
    if (!axisMesh_.upload(buildGroundGrid(0.25f, 2.5f, 1))) {
        if (error) *error = "ground axes upload failed";
        return false;
    }
    if (!anatomyProgram_.build(kSkinnedVertex, kAnatomyNeutralFragment, error)) return false;
    if (!anatomyProgram_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "anatomy BoneBlock binding failed";
        return false;
    }
    if (!anatomyNormalProg_.build(kSkinnedVertex, kAnatomyNormalsFragment, error)) return false;
    if (!anatomyNormalProg_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "anatomy normals BoneBlock binding failed";
        return false;
    }
    if (!anatomyClayProg_.build(kSkinnedVertex, kAnatomyClayFragment, error)) return false;
    if (!anatomyClayProg_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "anatomy clay BoneBlock binding failed";
        return false;
    }
    uniformAnatomyViewProj_ = anatomyProgram_.uniform("uViewProj");
    uniformAnatomyModel_ = anatomyProgram_.uniform("uModel");
    uniformAnatomyNeutral_ = anatomyProgram_.uniform("uNeutral");
    uniformAnatomyNormalViewProj_ = anatomyNormalProg_.uniform("uViewProj");
    uniformAnatomyNormalModel_ = anatomyNormalProg_.uniform("uModel");
    uniformAnatomyClayViewProj_ = anatomyClayProg_.uniform("uViewProj");
    uniformAnatomyClayModel_ = anatomyClayProg_.uniform("uModel");
    uniformAnatomyClayNeutral_ = anatomyClayProg_.uniform("uNeutral");
    uniformAnatomyClayKeyDir_ = anatomyClayProg_.uniform("uKeyDir");
    uniformAnatomyClayFillDir_ = anatomyClayProg_.uniform("uFillDir");
    uniformViewProj_ = skinnedProgram_.uniform("uViewProj");
    uniformModel_ = skinnedProgram_.uniform("uModel");
    uniformCameraPos_ = skinnedProgram_.uniform("uCameraPos");
    uniformDebugMode_ = skinnedProgram_.uniform("uDebugMode");
    uniformSkin_ = skinnedProgram_.uniform("uSkinColor");
    uniformHair_ = skinnedProgram_.uniform("uHairColor");
    uniformCloth_ = skinnedProgram_.uniform("uClothColor");
    uniformEye_ = skinnedProgram_.uniform("uEyeColor");
    uniformPants_ = skinnedProgram_.uniform("uPantsColor");
    uniformBgMode_ = backgroundProgram_.uniform("uMode");
    uniformBgA_ = backgroundProgram_.uniform("uColorA");
    uniformBgB_ = backgroundProgram_.uniform("uColorB");
    uniformResolution_ = backgroundProgram_.uniform("uResolution");

    if (!boneUbo_.create(error)) return false;
    if (!hasCachedAvatar_) {
        const HumanoidResult humanoid = generateHumanoid(BodyParameters{}, false);
        if (!humanoid.valid) {
            if (error) *error = "skinned humanoid generation failed";
            return false;
        }
        cachedAvatarMesh_ = humanoid.mesh;
        skeleton_ = humanoid.skeleton;
        joints_ = humanoid.joints;
        hasCachedAvatar_ = true;
        if (!motion_.initialize(skeleton_, joints_)) {
            if (error) *error = "motion runtime init failed";
            return false;
        }
        ++avatarRevision_;
        PAE_LOGI("PAE_RENDER", "geometry generated: avatar");
    } else {
        PAE_LOGI("PAE_RENDER", "geometry cache hit: avatar");
    }
    if (!avatarMesh_.upload(cachedAvatarMesh_)) {
        if (error) *error = "skinned humanoid upload failed";
        return false;
    }
    // Avatar wireframe edge buffer for the topology inspection view.
    if (!avatarLines_.upload(buildAnatomyLineMesh(cachedAvatarMesh_))) {
        if (error) *error = "avatar wireframe upload failed";
        return false;
    }
    stats_.setBufferBytes(static_cast<uint64_t>(
        cachedAvatarMesh_.vertices.size() * 64 +
        cachedAvatarMesh_.indices.size() * sizeof(uint32_t) +
        BonePaletteUboData::kBytes));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // M12-A: unified SDF upper body for the Anatomy Preview Mode.
    {
        if (!hasCachedAnatomy_) {
            anatomy::HumanAnatomyParameters anatomyParams;
            const auto anatomyStart = std::chrono::steady_clock::now();
            const BodyParameters anatomyBodyParams{};
            auto anatomy = anatomy::generateAnatomyUpperBody(
                anatomyParams, anatomyBodyParams, anatomy::AnatomyResolution{});
            const auto anatomyEnd = std::chrono::steady_clock::now();
            const double ms = std::chrono::duration<double, std::milli>(
                anatomyEnd - anatomyStart).count();
            if (!anatomy.valid) {
                PAE_LOGE("PAE_RENDER", "anatomy generation failed");
            } else {
                cachedAnatomyMesh_ = anatomy.mesh;
                hasCachedAnatomy_ = true;
                PAE_LOGI("PAE_RENDER", "anatomy preview ready: %u tris, %.0f ms",
                         static_cast<unsigned>(cachedAnatomyMesh_.indices.size() / 3), ms);
            }
        } else {
            PAE_LOGI("PAE_RENDER", "geometry cache hit: anatomy (%u tris)",
                     static_cast<unsigned>(cachedAnatomyMesh_.indices.size() / 3));
        }
        if (hasCachedAnatomy_ && !anatomyMesh_.upload(cachedAnatomyMesh_)) {
            PAE_LOGE("PAE_RENDER", "anatomy mesh upload failed");
        }
        if (hasCachedAnatomy_ &&
            !anatomyLines_.upload(buildAnatomyLineMesh(cachedAnatomyMesh_))) {
            PAE_LOGE("PAE_RENDER", "anatomy wireframe upload failed");
        }
    }

    // P1-A: surface-patch torso + shoulder bridge + arm PoC. SDF anatomy
    // preview above is preserved as a research/contrast path.
    {
        bool p1aRegenerated = false;
        if (!hasCachedP1a_) {
            CharacterDefinition definition;
            MeshData patchMesh;
            if (!BodyPatchGenerator::generate(definition, joints_, skeleton_,
                                              patchMesh, patchReport_)) {
                PAE_LOGE("PAE_RENDER", "P1-A patch generation failed");
            } else {
                cachedP1aMesh_ = patchMesh;
                hasCachedP1a_ = true;
                p1aRegenerated = true;
            }
        } else {
            PAE_LOGI("PAE_RENDER", "geometry cache hit: p1a");
        }
        if (hasCachedP1a_ && !p1aMesh_.upload(cachedP1aMesh_)) {
            PAE_LOGE("PAE_RENDER", "P1-A patch mesh upload failed");
        } else if (hasCachedP1a_) {
            const TopologyReport topology = analyzeTopology(cachedP1aMesh_);
            PAE_LOGI("PAE_RENDER",
                     "P1A-REPORT%s verts=%u tris=%u components=%d boundary=%d nonmanifold=%d "
                     "degenerate=%d badnorm=%d gen_ms=%.3f boundaries=%zu",
                     p1aRegenerated ? "" : " (cached)",
                     patchReport_.vertexCount, patchReport_.triangleCount,
                     topology.components, topology.boundaryEdges, topology.nonManifoldEdges,
                     topology.degenerateTriangles, topology.invalidNormals,
                     patchReport_.generationMs, patchReport_.boundaries.size());
        }
        if (hasCachedP1a_ && !p1aLines_.upload(buildAnatomyLineMesh(cachedP1aMesh_))) {
            PAE_LOGE("PAE_RENDER", "P1-A wireframe upload failed");
        }
    }

    resourcesReady_ = true;
    return true;
}

void RenderThread::releaseGpuResources() {
    avatarMesh_.release();
    avatarLines_.release();
    gridMesh_.release();
    axisMesh_.release();
    shadowMesh_.release();
    p1aMesh_.release();
    p1aLines_.release();
    // DEF-01 (Plan B): the anatomy buffers used to stay alive across a surface
    // teardown. They are GPU objects and belong to the released set; the
    // CPU-side mesh stays in the geometry cache for a fast re-upload.
    anatomyMesh_.release();
    anatomyLines_.release();
    boneUbo_.release();
    resourcesReady_ = false;
}

MeshData RenderThread::buildAnatomyLineMesh(const MeshData& mesh) const {
    // Edge list (deduplicated) from the triangle indices -> GL_LINES pairs.
    MeshData lines;
    lines.vertices = mesh.vertices;
    std::set<std::pair<uint32_t, uint32_t>> edges;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t a = mesh.indices[i], b = mesh.indices[i + 1], c = mesh.indices[i + 2];
        edges.insert({std::min(a, b), std::max(a, b)});
        edges.insert({std::min(b, c), std::max(b, c)});
        edges.insert({std::min(c, a), std::max(c, a)});
    }
    for (const auto& edge : edges) {
        lines.indices.insert(lines.indices.end(), {edge.first, edge.second});
    }
    Aabb bounds;
    for (const Vertex& v : lines.vertices) bounds.expand(v.position);
    lines.bounds = bounds;
    SubMesh sub;
    sub.indexCount = static_cast<uint32_t>(lines.indices.size());
    lines.subMeshes.push_back(sub);
    return lines;
}

MeshData RenderThread::buildGroundShadow() const {
    MeshData mesh;
    const uint32_t segments = 40;
    Vertex center;
    center.position = {0.0f, 0.0f, 0.0f};
    center.normal = {0.0f, 1.0f, 0.0f};
    center.joints = {0, 0, 0, 0};
    center.weights = {1.0f, 0.0f, 0.0f, 0.0f};
    center.color = {0.0f, 0.0f, 0.0f, 0.0f};
    mesh.vertices.push_back(center);
    for (uint32_t i = 0; i <= segments; ++i) {
        const float angle = 6.2831853f * static_cast<float>(i) / static_cast<float>(segments);
        Vertex ring;
        ring.position = {std::cos(angle) * 0.34f, 0.0f, std::sin(angle) * 0.34f};
        ring.normal = {0.0f, 1.0f, 0.0f};
        ring.joints = {0, 0, 0, 0};
        ring.weights = {1.0f, 0.0f, 0.0f, 0.0f};
        ring.color = {0.0f, 0.0f, 0.0f, 0.0f};
        mesh.vertices.push_back(ring);
    }
    for (uint32_t i = 1; i <= segments; ++i) {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i);
    }
    Aabb bounds;
    for (const Vertex& vertex : mesh.vertices) bounds.expand(vertex.position);
    mesh.bounds = bounds;
    return mesh;
}

void RenderThread::applyCameraPreset(int preset) {
    const int index = std::clamp(preset, 0, kPresetCount - 1);
    const Vec3 eye = kPresetEye[index];
    const Vec3 target = kPresetTarget[index];
    cameraTarget_ = target;
    const Vec3 delta = eye - target;
    cameraDistance_ = std::max(delta.length(), 0.05f);
    cameraYawDeg_ = std::atan2(delta.x, delta.z) * kRadToDeg;
    cameraPitchDeg_ = std::asin(std::clamp(delta.y / cameraDistance_, -1.0f, 1.0f)) * kRadToDeg;
    cameraFree_ = false;
}

Vec3 RenderThread::resolveCameraEye() const {
    const float yaw = cameraYawDeg_ * kDegToRad;
    const float pitch = cameraPitchDeg_ * kDegToRad;
    const Vec3 dir{std::sin(yaw) * std::cos(pitch),
                   std::sin(pitch),
                   std::cos(yaw) * std::cos(pitch)};
    return cameraTarget_ + dir * cameraDistance_;
}

void RenderThread::applyCameraOp(int op, const Vec3& delta) {
    switch (static_cast<CameraOp>(op)) {
        case CameraOp::Orbit:
            cameraYawDeg_ += delta.x;
            cameraPitchDeg_ = std::clamp(cameraPitchDeg_ + delta.y, -85.0f, 85.0f);
            cameraFree_ = true;
            break;
        case CameraOp::Zoom: {
            const float multiplier = delta.x > 0.0f ? delta.x : 1.0f;
            cameraDistance_ = std::clamp(cameraDistance_ / multiplier, 0.22f, 14.0f);
            cameraFree_ = true;
            break;
        }
        case CameraOp::Pan: {
            const Vec3 eye = resolveCameraEye();
            const Vec3 forward = (cameraTarget_ - eye).normalized({0.0f, 0.0f, -1.0f});
            const Vec3 right = forward.cross(Vec3{0.0f, 1.0f, 0.0f}).normalized({1.0f, 0.0f, 0.0f});
            const Vec3 up = right.cross(forward).normalized({0.0f, 1.0f, 0.0f});
            // Screen pixels -> world units. The scale follows the orbit
            // distance so panning feels the same at every zoom level.
            const float scale = cameraDistance_ * 0.0019f;
            cameraTarget_ = cameraTarget_ - right * (delta.x * scale) + up * (delta.y * scale);
            cameraTarget_.y = std::clamp(cameraTarget_.y, -1.0f, 3.0f);
            cameraFree_ = true;
            break;
        }
        case CameraOp::Reset:
            applyCameraPreset(cameraPreset_);
            break;
    }
}

MeshData RenderThread::buildGroundGrid(float spacing, float halfExtent, int mode) const {
    // World-space reference grid: gives the character a readable ground plane
    // and a metric sense of scale under the free camera. Drawn as GL_LINES.
    // mode 0 = minor grid (skips the centre axes), mode 1 = the two axes.
    MeshData lines;
    const float y = -0.052f; // just below the avatar's model offset floor
    auto pushVertex = [&](float x, float z) {
        Vertex v;
        v.position = {x, y, z};
        v.normal = {0.0f, 1.0f, 0.0f};
        v.joints = {0, 0, 0, 0};
        v.weights = {1.0f, 0.0f, 0.0f, 0.0f};
        v.color = {1.0f, 1.0f, 1.0f, 0.0f};
        lines.vertices.push_back(v);
    };
    if (mode == 1) {
        pushVertex(0.0f, -halfExtent);
        pushVertex(0.0f, halfExtent);
        pushVertex(-halfExtent, 0.0f);
        pushVertex(halfExtent, 0.0f);
    } else {
        const int steps = static_cast<int>(std::floor(halfExtent / spacing));
        for (int i = -steps; i <= steps; ++i) {
            const float coordinate = static_cast<float>(i) * spacing;
            if (std::fabs(coordinate) < 0.001f) continue; // axes drawn separately
            pushVertex(coordinate, -halfExtent);
            pushVertex(coordinate, halfExtent);
            pushVertex(-halfExtent, coordinate);
            pushVertex(halfExtent, coordinate);
        }
    }
    for (size_t i = 0; i + 1 < lines.vertices.size(); i += 2) {
        lines.indices.push_back(static_cast<uint32_t>(i));
        lines.indices.push_back(static_cast<uint32_t>(i + 1));
    }
    Aabb bounds;
    for (const Vertex& vertex : lines.vertices) bounds.expand(vertex.position);
    lines.bounds = bounds;
    SubMesh sub;
    sub.indexCount = static_cast<uint32_t>(lines.indices.size());
    lines.subMeshes.push_back(sub);
    return lines;
}

void RenderThread::drawOutline(const Mat4& viewProjection, const Mat4& model) {
    glCullFace(GL_FRONT);
    outlineProgram_.use();
    glUniformMatrix4fv(uniformOutlineViewProj_, 1, GL_FALSE, viewProjection.m.data());
    glUniformMatrix4fv(uniformOutlineModel_, 1, GL_FALSE, model.m.data());
    glUniform1f(uniformOutlineOffset_, 0.010f);
    glUniform4f(uniformOutlineColor_, 0.05f, 0.06f, 0.10f, 1.0f);
    avatarMesh_.draw();
    glCullFace(GL_BACK);
}

void RenderThread::updateBonePalette(int64_t timestampNs) {
    const PoseResult pose = motion_.update(timestampNs, poseMode_);
    for (uint16_t bone = 0; bone < BoneCount; ++bone) {
        paletteData_.writeBone(bone, pose.palette[bone]);
    }
    boneUbo_.update(paletteData_);
}

void RenderThread::drawFrame() {
    stats_.beginFrame();
    const float aspect = height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
    const Mat4 projection = Mat4::perspective(1.05f, aspect, 0.1f, 100.0f);
    // The camera is state, not a lookup table: the evidence presets write into
    // the same parameterisation the interactive camera uses, so a fixed
    // capture rig and a free inspection rig are the same rig.
    const Vec3 cameraEye = resolveCameraEye();
    const Mat4 view = Mat4::lookAt(cameraEye, cameraTarget_, {0.0f, 1.0f, 0.0f});
    const Mat4 viewProjection = projection * view;
    const Mat4 model = Mat4::translation({0.0f, -0.05f, 0.0f});

    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    backgroundProgram_.use();
    glUniform1i(uniformBgMode_, backgroundMode_);
    glUniform3f(uniformBgA_, backgroundA_.x, backgroundA_.y, backgroundA_.z);
    glUniform3f(uniformBgB_, backgroundB_.x, backgroundB_.y, backgroundB_.z);
    glUniform2f(uniformResolution_, static_cast<float>(width_), static_cast<float>(height_));
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    const int64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (previewMode_ == 1) {
        // SDF anatomy preview shows raw bind-pose geometry: identity palette.
        for (uint16_t bone = 0; bone < BoneCount; ++bone) {
            paletteData_.writeBone(bone, Mat4{});
        }
        boneUbo_.update(paletteData_);
    } else {
        // P1-A patches and the production avatar both exercise real bone palettes.
        updateBonePalette(nowNs);
    }


    // Ground contact shadow (M10): blended disc under the feet, drawn before
    // the character so the avatar occludes it.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    shadowProgram_.use();
    glUniformMatrix4fv(uniformShadowViewProj_, 1, GL_FALSE, viewProjection.m.data());
    glUniformMatrix4fv(uniformShadowModel_, 1, GL_FALSE,
                       Mat4::translation({0.0f, -0.046f, 0.02f}).m.data());
    glUniform4f(uniformShadowColor_, 0.0f, 0.0f, 0.05f, 0.35f);
    shadowMesh_.draw();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // World-space reference grid. Drawn before the character so the avatar
    // occludes it, and in world space so the free camera reads the scene in
    // metres instead of against an empty backdrop.
    if (previewMode_ == 0) {
        const Mat4 identity = Mat4{};
        shadowProgram_.use();
        glUniformMatrix4fv(uniformShadowViewProj_, 1, GL_FALSE, viewProjection.m.data());
        glUniformMatrix4fv(uniformShadowModel_, 1, GL_FALSE, identity.m.data());
        glUniform4f(uniformShadowColor_, 0.32f, 0.36f, 0.42f, 1.0f);
        gridMesh_.drawLines();
        glUniform4f(uniformShadowColor_, 0.52f, 0.58f, 0.66f, 1.0f);
        axisMesh_.drawLines();
    }

    if (previewMode_ == 0) {
        skinnedProgram_.use();
        glUniformMatrix4fv(uniformViewProj_, 1, GL_FALSE, viewProjection.m.data());
        glUniformMatrix4fv(uniformModel_, 1, GL_FALSE, model.m.data());
        glUniform3f(uniformCameraPos_, cameraEye.x, cameraEye.y, cameraEye.z);
        glUniform3f(uniformSkin_, appearance_[0].x, appearance_[0].y, appearance_[0].z);
        glUniform3f(uniformHair_, appearance_[1].x, appearance_[1].y, appearance_[1].z);
        glUniform3f(uniformCloth_, appearance_[2].x, appearance_[2].y, appearance_[2].z);
        glUniform3f(uniformEye_, appearance_[3].x, appearance_[3].y, appearance_[3].z);
        glUniform3f(uniformPants_, appearance_[4].x, appearance_[4].y, appearance_[4].z);
        glUniform3f(skinnedProgram_.uniform("uLightDir"), 0.4f, 0.8f, 0.6f);
        glUniform1f(skinnedProgram_.uniform("uAmbient"), 0.42f);
        glUniform1f(skinnedProgram_.uniform("uToonSteps"), 3.0f);
        glUniform1f(skinnedProgram_.uniform("uRim"), 0.12f);
        // Inspection views. The inverted-hull outline is a study mode, not the
        // production look: the black rim reinforced the toy read of the skin.
        const int debugMode = (debugView_ == 1) ? 3
                            : (debugView_ == 2) ? 2
                            : (debugView_ == 3) ? 3
                            : (debugView_ == 4) ? 4
                            : (debugView_ == 5) ? 5
                            : 0;
        glUniform1i(uniformDebugMode_, debugMode);
        avatarMesh_.draw();
        if (debugView_ == 1) {
            // Wireframe over the clay shading: the triangle topology is the
            // point of this view, so the lines are pulled to the front.
            glDepthRangef(0.0f, 0.0f);
            shadowProgram_.use();
            glUniformMatrix4fv(uniformShadowViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformShadowModel_, 1, GL_FALSE, model.m.data());
            glUniform4f(uniformShadowColor_, 0.10f, 0.42f, 0.16f, 1.0f);
            avatarLines_.drawLines();
            glDepthRangef(0.0f, 1.0f);
        } else if (debugView_ == 6) {
            drawOutline(viewProjection, model);
        }
    } else if (previewMode_ == 2) {
        // P1-A patch PoC: continuous gray Lambert by default; normals and
        // wireframe are the only additional debug views. No toon, no material.
        if (debugView_ == 2) {
            anatomyNormalProg_.use();
            glUniformMatrix4fv(uniformAnatomyNormalViewProj_, 1, GL_FALSE,
                               viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyNormalModel_, 1, GL_FALSE, model.m.data());
            p1aMesh_.draw();
        } else {
            anatomyClayProg_.use();
            glUniformMatrix4fv(uniformAnatomyClayViewProj_, 1, GL_FALSE,
                               viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyClayModel_, 1, GL_FALSE, model.m.data());
            glUniform3f(uniformAnatomyClayNeutral_, 0.62f, 0.62f, 0.64f);
            glUniform3f(uniformAnatomyClayKeyDir_, 0.45f, 0.75f, 0.48f);
            glUniform3f(uniformAnatomyClayFillDir_, -0.55f, 0.15f, 0.35f);
            p1aMesh_.draw();
            if (debugView_ == 1) {
                glDepthRangef(0.0f, 0.0f);
                shadowProgram_.use();
                glUniformMatrix4fv(uniformShadowViewProj_, 1, GL_FALSE,
                                   viewProjection.m.data());
                glUniformMatrix4fv(uniformShadowModel_, 1, GL_FALSE, model.m.data());
                glUniform4f(uniformShadowColor_, 0.05f, 0.35f, 0.08f, 1.0f);
                p1aLines_.drawLines();
                glDepthRangef(0.0f, 1.0f);
            }
        }
    } else {
        // M12-A anatomy preview: neutral study material + optional wireframe
        // / normals debug views; bind pose; no outline, no clothing.
        const Mat4 identity = Mat4{};
        if (debugView_ == 2) {
            anatomyNormalProg_.use();
            glUniformMatrix4fv(uniformAnatomyNormalViewProj_, 1, GL_FALSE,
                               viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyNormalModel_, 1, GL_FALSE, model.m.data());
            anatomyMesh_.draw();
        } else if (debugView_ == 3) {
            // M13 gray clay: continuous Lambert, no toon banding — the only
            // view morphology should be accepted in.
            anatomyClayProg_.use();
            glUniformMatrix4fv(uniformAnatomyClayViewProj_, 1, GL_FALSE,
                               viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyClayModel_, 1, GL_FALSE, model.m.data());
            glUniform3f(uniformAnatomyClayNeutral_, 0.62f, 0.62f, 0.64f);
            glUniform3f(uniformAnatomyClayKeyDir_, 0.45f, 0.75f, 0.48f);
            glUniform3f(uniformAnatomyClayFillDir_, -0.55f, 0.15f, 0.35f);
            anatomyMesh_.draw();
        } else {
            anatomyProgram_.use();
            glUniformMatrix4fv(uniformAnatomyViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyModel_, 1, GL_FALSE, model.m.data());
            glUniform3f(uniformAnatomyNeutral_, 0.72f, 0.66f, 0.60f);
            // Neutral study lighting: without these the shared fragment shader
            // defaults to ambient=0/steps=0 and the whole mesh renders black.
            glUniform3f(anatomyProgram_.uniform("uLightDir"), 0.42f, 0.78f, 0.55f);
            glUniform1f(anatomyProgram_.uniform("uAmbient"), 0.36f);
            glUniform1f(anatomyProgram_.uniform("uToonSteps"), 6.0f);
            anatomyMesh_.draw();
            if (debugView_ == 1 || debugView_ == 3) {
                glDepthRangef(0.0f, 0.0f); // pull lines to the front
                shadowProgram_.use();
                glUniformMatrix4fv(uniformShadowViewProj_, 1, GL_FALSE,
                                   viewProjection.m.data());
                glUniformMatrix4fv(uniformShadowModel_, 1, GL_FALSE, model.m.data());
                glUniform4f(uniformShadowColor_, 0.05f, 0.35f, 0.08f, 1.0f);
                anatomyLines_.drawLines();
                glDepthRangef(0.0f, 1.0f);
            }
        }
    }

    egl_.swapBuffers();
    const uint32_t frameDraws = previewMode_ == 0 ? 4u
        : ((debugView_ == 1 || (previewMode_ == 1 && debugView_ == 3)) ? 3u : 2u);
    const uint32_t frameTris = previewMode_ == 0
        ? static_cast<uint32_t>(avatarMesh_.indexCount() / 3)
        : (previewMode_ == 2
            ? static_cast<uint32_t>(p1aMesh_.indexCount() / 3)
            : static_cast<uint32_t>(anatomyMesh_.indexCount() / 3));
    stats_.endFrame(frameDraws, frameTris);
    if (stats_.frameCount() % 120 == 0) {
        PAE_LOGI("PAE_STATS", "%s", stats_.toString().c_str());
    }
}

} // namespace pae
