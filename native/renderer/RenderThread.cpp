#include "RenderThread.h"

#include "core/config/EngineConfig.h"
#include "core/diagnostics/Log.h"
#include "core/math/Math.h"
#include "procedural/humanoid/Humanoid.h"
#include "procedural/humanoid/anatomy/HumanAnatomyGenerator.h"
#include "skeleton/Skinning.h"

#include <chrono>
#include <set>
#include <cmath>
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
void main() {
    mat4 skin = uBones[aJoints.x] * aWeights.x
              + uBones[aJoints.y] * aWeights.y
              + uBones[aJoints.z] * aWeights.z
              + uBones[aJoints.w] * aWeights.w;
    vec4 world = uModel * skin * vec4(aPosition, 1.0);
    vNormal = normalize(mat3(uModel) * mat3(skin) * aNormal);
    vColor = aColor;
    gl_Position = uViewProj * world;
}
)GLSL";

constexpr const char* kSkinnedFragment = R"GLSL(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec4 vColor;
uniform vec3 uSkinColor;
uniform vec3 uHairColor;
uniform vec3 uClothColor;
uniform vec3 uEyeColor;
uniform vec3 uPantsColor;
uniform vec3 uLightDir;
uniform float uAmbient;
uniform float uToonSteps;
uniform float uRim;
out vec4 fragColor;
void main() {
    float materialId = vColor.w;
    vec3 base = materialId < 0.5 ? uSkinColor
              : (materialId < 1.5 ? uHairColor
              : (materialId < 2.5 ? uClothColor
              : (materialId < 3.5 ? uEyeColor
              : (materialId < 4.5 ? uPantsColor : vec3(0.045, 0.04, 0.06)))));
    base *= 0.85 + 0.15 * clamp(vColor.rgb, 0.0, 1.0);
    vec3 n = normalize(vNormal);
    float ndl = max(dot(n, normalize(uLightDir)), 0.0);
    float toon = floor(ndl * uToonSteps) / max(uToonSteps, 1.0);
    float shade = mix(uAmbient, 1.0, toon);
    float rim = pow(1.0 - abs(n.z), 3.0) * uRim;
    fragColor = vec4(base * shade + vec3(rim), 1.0);
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

} // namespace

RenderThread::RenderThread(uint32_t maxQueuedCommands)
    : maxQueuedCommands_(maxQueuedCommands) {}

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

void RenderThread::postCameraPreset(int preset) {
    RenderCommand command;
    command.type = RenderCommandType::CameraPreset;
    command.value = preset;
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
    return stats_.toString() + " avatar_rev=" + std::to_string(avatarRevision_) +
           " scene_rev=" + std::to_string(sceneRevision_) + " " + startupError_;
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
            case RenderCommandType::SetAppearance:
                appearance_ = command.palette;
                ++sceneRevision_; // material-only change: no geometry rebuild
                break;
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
                break;
            case RenderCommandType::CameraPreset:
                cameraPreset_ = command.value;
                break;
            case RenderCommandType::DebugView:
                debugView_ = command.value;
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
    uniformAnatomyViewProj_ = anatomyProgram_.uniform("uViewProj");
    uniformAnatomyModel_ = anatomyProgram_.uniform("uModel");
    uniformAnatomyNeutral_ = anatomyProgram_.uniform("uNeutral");
    uniformAnatomyNormalViewProj_ = anatomyNormalProg_.uniform("uViewProj");
    uniformAnatomyNormalModel_ = anatomyNormalProg_.uniform("uModel");
    uniformViewProj_ = skinnedProgram_.uniform("uViewProj");
    uniformModel_ = skinnedProgram_.uniform("uModel");
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
    const HumanoidResult humanoid = generateHumanoid(BodyParameters{}, false);
    if (!humanoid.valid || !avatarMesh_.upload(humanoid.mesh)) {
        if (error) *error = "skinned humanoid upload failed";
        return false;
    }
    skeleton_ = humanoid.skeleton;
    joints_ = humanoid.joints;
    if (!motion_.initialize(skeleton_, joints_)) {
        if (error) *error = "motion runtime init failed";
        return false;
    }
    ++avatarRevision_;
    stats_.setBufferBytes(static_cast<uint64_t>(
        humanoid.mesh.vertices.size() * 64 + humanoid.mesh.indices.size() * sizeof(uint32_t) +
        BonePaletteUboData::kBytes));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // M12-A: unified SDF upper body for the Anatomy Preview Mode.
    {
        anatomy::HumanAnatomyParameters anatomyParams;
        const auto anatomyStart = std::chrono::steady_clock::now();
        const BodyParameters anatomyBodyParams{};
        auto anatomy = anatomy::generateAnatomyUpperBody(
            anatomyParams, anatomyBodyParams, anatomy::AnatomyResolution{});
        const auto anatomyEnd = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(
            anatomyEnd - anatomyStart).count();
        if (!anatomy.valid || !anatomyMesh_.upload(anatomy.mesh)) {
            PAE_LOGE("PAE_RENDER", "anatomy mesh upload failed");
        } else {
            PAE_LOGI("PAE_RENDER", "anatomy preview ready: %u tris, %.0f ms",
                     static_cast<unsigned>(anatomy.mesh.indices.size() / 3), ms);
        }
        if (!anatomyLines_.upload(buildAnatomyLineMesh(anatomy.mesh))) {
            PAE_LOGE("PAE_RENDER", "anatomy wireframe upload failed");
        }
    }

    resourcesReady_ = true;
    return true;
}

void RenderThread::releaseGpuResources() {
    avatarMesh_.release();
    shadowMesh_.release();
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
    // M12: fixed evidence cameras (index per §8 of the anatomy preview spec).
    static const Vec3 kPresetEye[] = {
        {0.0f, 0.9f, 3.1f},    // 0 FRONT (M11 default)
        {0.0f, 0.9f, -3.1f},   // 1 BACK
        {3.1f, 0.9f, 0.0f},    // 2 LEFT (+X side)
        {-3.1f, 0.9f, 0.0f},   // 3 RIGHT
        {2.3f, 0.9f, 2.3f},    // 4 45 LEFT
        {-2.3f, 0.9f, 2.3f},   // 5 45 RIGHT
        {0.55f, 1.5f, 1.15f},  // 6 SHOULDER CLOSEUP
        {0.0f, 1.25f, 1.4f},   // 7 TORSO CLOSEUP
    };
    static const Vec3 kPresetTarget[] = {
        {0.0f, 0.9f, 0.0f}, {0.0f, 0.9f, 0.0f}, {0.0f, 0.9f, 0.0f},
        {0.0f, 0.9f, 0.0f}, {0.0f, 0.9f, 0.0f}, {0.0f, 0.9f, 0.0f},
        {0.16f, 1.32f, 0.0f}, {0.0f, 1.18f, 0.0f},
    };
    const int preset = std::clamp(cameraPreset_, 0, 7);
    const Mat4 view = previewMode_ != 0
        ? Mat4::lookAt(kPresetEye[preset], kPresetTarget[preset], {0.0f, 1.0f, 0.0f})
        : Mat4::lookAt({0.0f, 0.9f, 3.1f}, {0.0f, 0.9f, 0.0f}, {0.0f, 1.0f, 0.0f});
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
    if (previewMode_ != 0) {
        // Anatomy preview shows the raw bind-pose geometry: identity palette.
        for (uint16_t bone = 0; bone < BoneCount; ++bone) {
            paletteData_.writeBone(bone, Mat4{});
        }
        boneUbo_.update(paletteData_);
    } else {
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

    if (previewMode_ == 0) {
        skinnedProgram_.use();
        glUniformMatrix4fv(uniformViewProj_, 1, GL_FALSE, viewProjection.m.data());
        glUniformMatrix4fv(uniformModel_, 1, GL_FALSE, model.m.data());
        glUniform3f(uniformSkin_, appearance_[0].x, appearance_[0].y, appearance_[0].z);
        glUniform3f(uniformHair_, appearance_[1].x, appearance_[1].y, appearance_[1].z);
        glUniform3f(uniformCloth_, appearance_[2].x, appearance_[2].y, appearance_[2].z);
        glUniform3f(uniformEye_, appearance_[3].x, appearance_[3].y, appearance_[3].z);
        glUniform3f(uniformPants_, appearance_[4].x, appearance_[4].y, appearance_[4].z);
        glUniform3f(skinnedProgram_.uniform("uLightDir"), 0.4f, 0.8f, 0.6f);
        glUniform1f(skinnedProgram_.uniform("uAmbient"), 0.42f);
        glUniform1f(skinnedProgram_.uniform("uToonSteps"), 3.0f);
        glUniform1f(skinnedProgram_.uniform("uRim"), 0.12f);
        avatarMesh_.draw();
        drawOutline(viewProjection, model);
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
        } else {
            anatomyProgram_.use();
            glUniformMatrix4fv(uniformAnatomyViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyModel_, 1, GL_FALSE, model.m.data());
            glUniform3f(uniformAnatomyNeutral_, 0.72f, 0.66f, 0.60f);
            anatomyMesh_.draw();
            if (debugView_ == 1) {
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
    const uint32_t frameDraws = previewMode_ != 0
        ? (debugView_ == 1 ? 2u : 3u)
        : 4u;
    const uint32_t frameTris = previewMode_ != 0
        ? static_cast<uint32_t>(anatomyMesh_.indexCount() / 3)
        : static_cast<uint32_t>(avatarMesh_.indexCount() / 3);
    stats_.endFrame(frameDraws, frameTris);
    if (stats_.frameCount() % 120 == 0) {
        PAE_LOGI("PAE_STATS", "%s", stats_.toString().c_str());
    }
}

} // namespace pae
