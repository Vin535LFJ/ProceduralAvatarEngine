#include "RenderThread.h"

#include "core/config/EngineConfig.h"
#include "core/diagnostics/Log.h"
#include "core/math/Math.h"
#include "procedural/MeshTopology.h"
#include "procedural/humanoid/Humanoid.h"
#include "procedural/humanoid/canonical/ContinuousSkinGenerator.h"
#include "procedural/character/TorsoShoulderPatchGenerator.h"
#include "procedural/humanoid/anatomy/HumanAnatomyGenerator.h"
#include "procedural/human_template/BaseHumanRenderMeshBuilder.h"
#include "procedural/human_template/HumanTemplateLoader.h"
#include "skeleton/Skinning.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

#if defined(__ANDROID__)
#include <sys/resource.h>
#endif

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
uniform mat3 uNormalMatrix;
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
    vNormal = normalize(uNormalMatrix * mat3(skin) * aNormal);
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
        else idColor = vec3(0.36, 0.36, 0.40);
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

    // 0 skin, 1 hair, 2 shirt, 3 eye, 4 ink (iris/pupil/mouth), 5 pants/shoes.
    vec3 base = materialId < 0.5 ? uSkinColor
              : (materialId < 1.5 ? uHairColor
              : (materialId < 2.5 ? uClothColor
              : (materialId < 3.5 ? uEyeColor
              : (materialId < 4.5 ? vec3(0.035, 0.032, 0.038) : uPantsColor))));

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
    } else if (materialId < 2.5 || materialId >= 4.5) {
        rough = 0.72; specStrength = 0.05;
        float weave = noise3(vWorldPos * 240.0);
        base *= 0.90 + 0.14 * weave;
    } else if (materialId < 3.5) {
        rough = 0.12; specStrength = 0.65;
    } else {
        rough = 0.30; specStrength = 0.30; // ink
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

constexpr const char* kUvSkinnedVertex = R"GLSL(#version 300 es
layout(std140) uniform BoneBlock { mat4 uBones[22]; };
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in uvec4 aJoints;
layout(location = 3) in vec4 aWeights;
layout(location = 4) in vec4 aColor;
layout(location = 5) in vec2 aUv;
uniform mat4 uViewProj;
uniform mat4 uModel;
uniform mat3 uNormalMatrix;
out vec2 vUv;
out vec3 vNormal;
void main() {
    mat4 skin = uBones[aJoints.x] * aWeights.x
              + uBones[aJoints.y] * aWeights.y
              + uBones[aJoints.z] * aWeights.z
              + uBones[aJoints.w] * aWeights.w;
    vec4 world = uModel * skin * vec4(aPosition, 1.0);
    vUv = aUv;
    vNormal = normalize(uNormalMatrix * mat3(skin) * aNormal);
    gl_Position = uViewProj * world;
}
)GLSL";

constexpr const char* kUvCheckerFragment = R"GLSL(#version 300 es
precision highp float;
in vec2 vUv;
in vec3 vNormal;
uniform vec3 uLightDir;
uniform float uCheckerScale;
out vec4 fragColor;
void main() {
    vec2 tiled = vUv * uCheckerScale;
    float checker = mod(floor(tiled.x) + floor(tiled.y), 2.0);
    vec3 base = mix(vec3(0.08, 0.10, 0.14), vec3(0.82, 0.84, 0.88), checker);
    vec2 cell = fract(tiled);
    float edgeDistance = min(min(cell.x, 1.0 - cell.x), min(cell.y, 1.0 - cell.y));
    float edge = 1.0 - smoothstep(0.015, 0.055, edgeDistance);
    base = mix(base, vec3(0.95, 0.20, 0.42), edge * 0.85);
    float light = 0.38 + 0.62 * max(dot(normalize(vNormal), normalize(uLightDir)), 0.0);
    fragColor = vec4(base * light, 1.0);
}
)GLSL";

constexpr const char* kSkinnedValidationVertex = R"GLSL(#version 300 es
layout(std140) uniform BoneBlock { mat4 uBones[22]; };
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in uvec4 aJoints;
layout(location = 3) in vec4 aWeights;
uniform mat4 uModel;
uniform mat3 uNormalMatrix;
out vec3 vGpuPosition;
out vec3 vGpuNormal;
void main() {
    mat4 skin = uBones[aJoints.x] * aWeights.x
              + uBones[aJoints.y] * aWeights.y
              + uBones[aJoints.z] * aWeights.z
              + uBones[aJoints.w] * aWeights.w;
    vGpuPosition = (uModel * skin * vec4(aPosition, 1.0)).xyz;
    vGpuNormal = normalize(uNormalMatrix * mat3(skin) * aNormal);
    gl_Position = vec4(0.0);
}
)GLSL";

constexpr const char* kValidationFragment = R"GLSL(#version 300 es
precision mediump float;
out vec4 fragColor;
void main() { fragColor = vec4(0.0); }
)GLSL";

GLuint buildValidationProgram(std::string* error) {
    const auto compile = [error](GLenum type, const char* source) {
        const GLuint shader = glCreateShader(type);
        if (shader == 0) return GLuint{0};
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_TRUE) return shader;
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<size_t>(length > 1 ? length : 1));
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        if (error) *error = std::string("validation shader compile failed: ") + log.data();
        glDeleteShader(shader);
        return GLuint{0};
    };
    const GLuint vertex = compile(GL_VERTEX_SHADER, kSkinnedValidationVertex);
    if (vertex == 0) return 0;
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, kValidationFragment);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    const char* varyings[] = {"vGpuPosition", "vGpuNormal"};
    glTransformFeedbackVaryings(program, 2, varyings, GL_INTERLEAVED_ATTRIBS);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE && program != 0) return program;
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(static_cast<size_t>(length > 1 ? length : 1));
    glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
    if (error) *error = std::string("validation program link failed: ") + log.data();
    if (program != 0) glDeleteProgram(program);
    return 0;
}

constexpr float kMaxPositionErrorMeters = 5e-5f;
constexpr float kMaxMeanPositionErrorMeters = 2e-5f;
constexpr float kMaxNormalAngleRadians = 0.008726646f;

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
uniform mat3 uNormalMatrix;
uniform float uOffset;
out float vMaterial;
void main() {
    mat4 skin = uBones[aJoints.x] * aWeights.x
              + uBones[aJoints.y] * aWeights.y
              + uBones[aJoints.z] * aWeights.z
              + uBones[aJoints.w] * aWeights.w;
    vec4 world = uModel * skin * vec4(aPosition, 1.0);
    vec3 worldNormal = uNormalMatrix * mat3(skin) * aNormal;
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

// P1-VR-1A is a local torso/shoulder patch, not a full body. These cameras
// keep the patch at roughly 50–65% of viewport height and avoid the detached
// full-body ground shadow.
const Vec3 kP1aPresetEye[kPresetCount] = {
    {0.0f, 1.12f, 0.62f}, {0.0f, 1.12f, -0.62f},
    {0.62f, 1.12f, 0.0f}, {-0.62f, 1.12f, 0.0f},
    {0.44f, 1.12f, 0.44f}, {-0.44f, 1.12f, 0.44f},
    {0.30f, 1.25f, 0.33f}, {0.38f, 1.22f, 0.30f},
};
const Vec3 kP1aPresetTarget[kPresetCount] = {
    {0.0f, 1.12f, 0.0f}, {0.0f, 1.12f, 0.0f},
    {0.0f, 1.12f, 0.0f}, {0.0f, 1.12f, 0.0f},
    {0.0f, 1.12f, 0.0f}, {0.0f, 1.12f, 0.0f},
    {0.16f, 1.25f, 0.0f}, {0.27f, 1.24f, 0.02f},
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
        const auto stale = std::find_if(queue_.begin(), queue_.end(), [](const RenderCommand& queued) {
            return queued.type == RenderCommandType::SetExternalPose;
        });
        if (stale == queue_.end()) return;
        queue_.erase(stale);
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
    command.value = std::clamp(mode, 0, 4);
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

void RenderThread::postTemplateBytes(std::string bytes) {
    RenderCommand command;
    command.type = RenderCommandType::SetTemplateBytes;
    command.text = std::move(bytes);
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
           " scene_rev=" + std::to_string(sceneRevision_) + camera + " " + templateStatus_ + " " + startupError_;
}

void RenderThread::run() {
    const auto releasePendingWindows = [this]() {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            for (const RenderCommand& command : queue_) {
                if (command.window) ANativeWindow_release(command.window);
            }
            queue_.clear();
        }
        for (const RenderCommand& command : deferredCommands_) {
            if (command.window) ANativeWindow_release(command.window);
        }
        deferredCommands_.clear();
    };
    std::string error;
    eglReady_ = egl_.initialize(&error);
    if (!eglReady_) {
        startupError_ = error;
        PAE_LOGE("PAE_RENDER", "EGL init failed: %s", error.c_str());
        releasePendingWindows();
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
    releasePendingWindows();
}

void RenderThread::processCommands() {
    std::vector<RenderCommand> commands;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        commands.swap(queue_);
    }
    if (resourcesReady_ && !deferredCommands_.empty()) {
        commands.insert(commands.begin(), deferredCommands_.begin(), deferredCommands_.end());
        deferredCommands_.clear();
    }
    for (size_t index = 0; index < commands.size(); ++index) {
        const RenderCommand command = commands[index];
        if (!resourcesReady_ &&
            command.type != RenderCommandType::SurfaceCreated &&
            command.type != RenderCommandType::SurfaceDestroyed &&
            command.type != RenderCommandType::Shutdown) {
            deferredCommands_.push_back(command);
            continue;
        }
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
                const int nextVariant = command.palette[2].x > command.palette[2].z ? 1 : 0;
                const bool rebuild = nextVariant != outfitVariant_ && hasCachedAvatar_;
                SkinnedGpuMesh nextMesh;
                SkinnedGpuMesh nextLines;
                HumanoidResult humanoid;
                if (rebuild) {
                    BodyParameters params{};
                    params.detailLevel = detailLevelForQuality(static_cast<QualityLevel>(qualityLevel_));
                    if (nextVariant == 1) {
                        params.clothing.thickness = 0.018f;
                        params.clothing.shirtHem = 0.86f;
                        params.clothing.sleeveLength = 0.40f;
                        params.clothing.pantsLength = 0.82f;
                    }
                    humanoid = generateHumanoid(params, false);
                    if (!humanoid.valid || !nextMesh.upload(humanoid.mesh) ||
                        !nextLines.upload(buildAnatomyLineMesh(humanoid.mesh))) {
                        PAE_LOGE("PAE_RENDER", "appearance rebuild failed for variant %d", nextVariant);
                        break;
                    }
                }
                if (rebuild) {
                    if (!motion_.initialize(humanoid.skeleton, humanoid.joints)) {
                        PAE_LOGE("PAE_RENDER", "motion runtime reinit failed");
                        break;
                    }
                    cachedAvatarMesh_ = std::move(humanoid.mesh);
                    skeleton_ = humanoid.skeleton;
                    joints_ = humanoid.joints;
                    avatarMesh_ = std::move(nextMesh);
                    avatarLines_ = std::move(nextLines);
                    avatarGeometryDirty_ = false;
                    ++avatarRevision_;
                }
                appearance_ = command.palette;
                outfitVariant_ = nextVariant;
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
            case RenderCommandType::AnatomyPreview: {
                const int target = command.flag ? 1 : 0;
                if (!activatePreviewGpuResources(target)) {
                    PAE_LOGE("PAE_RENDER", "preview GPU resource activation failed mode=%d", target);
                    break;
                }
                previewMode_ = target;
                PAE_LOGI("PAE_RENDER", "applied anatomyPreview=%d", previewMode_);
                break;
            }
            case RenderCommandType::SetPreviewMode:
                if (command.value == 3 &&
                    !ensureCanonicalGeometry(detailLevelForQuality(
                        static_cast<QualityLevel>(qualityLevel_)))) {
                    PAE_LOGE("PAE_RENDER", "canonical body generation or upload failed");
                    break;
                }
                if (command.value == 4 && !ensureTemplateGeometry()) {
                    PAE_LOGE("PAE_RENDER", "human template load or upload failed: %s",
                             templateStatus_.c_str());
                    break;
                }
                if (!activatePreviewGpuResources(command.value)) {
                    PAE_LOGE("PAE_RENDER", "preview GPU resource activation failed mode=%d", command.value);
                    break;
                }
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
            case RenderCommandType::SetTemplateBytes:
                templateAssetBytes_ = command.text;
                hasTemplateAssetBytes_ = !templateAssetBytes_.empty();
                PAE_LOGI("PAE_RENDER", "template asset bytes received: %zu",
                         templateAssetBytes_.size());
                break;
            case RenderCommandType::SetQuality: {
                const int level = command.value;
                if (level < 0 || level > 2) break;
                const QualityLevel quality = static_cast<QualityLevel>(level);
                const int detailLevel = detailLevelForQuality(quality);
                if (previewMode_ == 4) {
                    // Static template: quality levels do not rebuild it.
                    qualityLevel_ = level;
                    PAE_LOGI("PAE_RENDER", "quality=%s static human template (no rebuild)",
                             qualityName(quality));
                    break;
                }
                if (previewMode_ == 3) {
                    if (ensureCanonicalGeometry(detailLevel)) {
                        qualityLevel_ = level;
                        avatarGeometryDirty_ = true;
                        PAE_LOGI("PAE_RENDER", "quality=%s canonical triangles=%u vertices=%zu",
                                 qualityName(quality),
                                 static_cast<uint32_t>(cachedCanonicalMesh_.indices.size() / 3),
                                 cachedCanonicalMesh_.vertices.size());
                    } else {
                        PAE_LOGE("PAE_RENDER", "canonical quality rebuild failed for level %d", level);
                    }
                    break;
                }
                if (previewMode_ == 2) {
                    hasCachedP1a_ = false;
                    cachedP1aCacheKey_.clear();
                    if (ensureP1aGeometry(detailLevel) &&
                        p1aMesh_.upload(cachedP1aMesh_) &&
                        p1aLines_.upload(buildAnatomyLineMesh(cachedP1aMesh_))) {
                        qualityLevel_ = level;
                        PAE_LOGI("PAE_RENDER", "quality=%s P1-VR-1A triangles=%u vertices=%u",
                                 qualityName(quality),
                                 static_cast<uint32_t>(cachedP1aMesh_.indices.size() / 3),
                                 static_cast<uint32_t>(cachedP1aMesh_.vertices.size()));
                    } else {
                        PAE_LOGE("PAE_RENDER", "P1-VR-1A quality rebuild failed for level %d", level);
                    }
                    break;
                }
                BodyParameters params{};
                params.detailLevel = detailLevel;
                const HumanoidResult humanoid = generateHumanoid(params, false);
                SkinnedGpuMesh nextMesh;
                SkinnedGpuMesh nextLines;
                if (!humanoid.valid || !nextMesh.upload(humanoid.mesh) ||
                    !nextLines.upload(buildAnatomyLineMesh(humanoid.mesh))) {
                    PAE_LOGE("PAE_RENDER", "quality rebuild failed for level %d", level);
                    break;
                }
                if (!motion_.initialize(humanoid.skeleton, humanoid.joints)) {
                    PAE_LOGE("PAE_RENDER", "motion runtime reinit failed");
                    break;
                }
                avatarMesh_ = std::move(nextMesh);
                avatarLines_ = std::move(nextLines);
                skeleton_ = humanoid.skeleton;
                joints_ = humanoid.joints;
                qualityLevel_ = level;
                cachedAvatarMesh_ = humanoid.mesh;
                hasCachedAvatar_ = true;
                avatarGeometryDirty_ = false;
                ++avatarRevision_;
                stats_.setBufferBytes(static_cast<uint64_t>(
                    humanoid.mesh.vertices.size() * 64 +
                    humanoid.mesh.indices.size() * sizeof(uint32_t) +
                    BonePaletteUboData::kBytes));
                PAE_LOGI("PAE_RENDER", "quality=%s triangles=%u vertices=%zu",
                         qualityName(quality),
                         static_cast<uint32_t>(humanoid.mesh.indices.size() / 3),
                         humanoid.mesh.vertices.size());
                break;
            }
            case RenderCommandType::Shutdown:
                stopRequested_ = true;
                break;
        }
        if (resourcesReady_ && !deferredCommands_.empty()) {
            commands.insert(commands.begin() + index + 1,
                            deferredCommands_.begin(), deferredCommands_.end());
            deferredCommands_.clear();
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
    if (!skinnedLineProgram_.build(kSkinnedVertex, kFlatColorFragment, error)) return false;
    if (!skinnedLineProgram_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "skinned line BoneBlock binding failed";
        return false;
    }
    if (!uvProgram_.build(kUvSkinnedVertex, kUvCheckerFragment, error)) return false;
    if (!uvProgram_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "UV BoneBlock binding failed";
        return false;
    }
    uniformLineViewProj_ = skinnedLineProgram_.uniform("uViewProj");
    uniformLineModel_ = skinnedLineProgram_.uniform("uModel");
    uniformLineColor_ = skinnedLineProgram_.uniform("uColor");
    if (!backgroundProgram_.build(kBackgroundVertex, kBackgroundFragment, error)) return false;
    if (!outlineProgram_.build(kOutlineVertex, kOutlineFragment, error)) return false;
    if (!outlineProgram_.bindUniformBlock("BoneBlock", 0)) {
        if (error) *error = "outline BoneBlock binding failed";
        return false;
    }
    if (!shadowProgram_.build(kFlatColorVertex, kFlatColorFragment, error)) return false;
    uniformOutlineViewProj_ = outlineProgram_.uniform("uViewProj");
    uniformOutlineModel_ = outlineProgram_.uniform("uModel");
    uniformOutlineNormalMatrix_ = outlineProgram_.uniform("uNormalMatrix");
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
    uniformAnatomyNormalMatrix_ = anatomyProgram_.uniform("uNormalMatrix");
    uniformAnatomyNeutral_ = anatomyProgram_.uniform("uNeutral");
    uniformAnatomyNormalViewProj_ = anatomyNormalProg_.uniform("uViewProj");
    uniformAnatomyNormalModel_ = anatomyNormalProg_.uniform("uModel");
    uniformAnatomyNormalNormalMatrix_ = anatomyNormalProg_.uniform("uNormalMatrix");
    uniformAnatomyClayViewProj_ = anatomyClayProg_.uniform("uViewProj");
    uniformAnatomyClayModel_ = anatomyClayProg_.uniform("uModel");
    uniformAnatomyClayNormalMatrix_ = anatomyClayProg_.uniform("uNormalMatrix");
    uniformAnatomyClayNeutral_ = anatomyClayProg_.uniform("uNeutral");
    uniformAnatomyClayKeyDir_ = anatomyClayProg_.uniform("uKeyDir");
    uniformAnatomyClayFillDir_ = anatomyClayProg_.uniform("uFillDir");
    uniformViewProj_ = skinnedProgram_.uniform("uViewProj");
    uniformModel_ = skinnedProgram_.uniform("uModel");
    uniformNormalMatrix_ = skinnedProgram_.uniform("uNormalMatrix");
    uniformCameraPos_ = skinnedProgram_.uniform("uCameraPos");
    uniformDebugMode_ = skinnedProgram_.uniform("uDebugMode");
    uniformSkin_ = skinnedProgram_.uniform("uSkinColor");
    uniformHair_ = skinnedProgram_.uniform("uHairColor");
    uniformCloth_ = skinnedProgram_.uniform("uClothColor");
    uniformEye_ = skinnedProgram_.uniform("uEyeColor");
    uniformPants_ = skinnedProgram_.uniform("uPantsColor");
    uniformUvViewProj_ = uvProgram_.uniform("uViewProj");
    uniformUvModel_ = uvProgram_.uniform("uModel");
    uniformUvNormalMatrix_ = uvProgram_.uniform("uNormalMatrix");
    uniformUvLightDir_ = uvProgram_.uniform("uLightDir");
    uniformUvCheckerScale_ = uvProgram_.uniform("uCheckerScale");
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

    // P1-VR-1A: structured bilateral torso/shoulder patch. The legacy P1-A
    // generator remains available as the baseline implementation.
    {
        if (!ensureP1aGeometry(detailLevelForQuality(
                static_cast<QualityLevel>(qualityLevel_)))) {
            PAE_LOGE("PAE_RENDER", "P1-VR-1A patch generation failed");
        }
        const auto uploadStarted = std::chrono::steady_clock::now();
        const bool meshUploaded = hasCachedP1a_ && p1aMesh_.upload(cachedP1aMesh_);
        const double meshUploadMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - uploadStarted).count();
        if (!meshUploaded) {
            PAE_LOGE("PAE_RENDER", "P1-VR-1A patch mesh upload failed");
        } else {
            const TopologyReport topology = analyzeTopology(cachedP1aMesh_);
            PAE_LOGI("PAE_RENDER",
                     "P1-VR-1A-REPORT verts=%u tris=%u components=%d boundary=%d nonmanifold=%d "
                     "degenerate=%d badnorm=%d gen_ms=%.3f state_ms=%.3f geometry_ms=%.3f "
                     "assembly_ms=%.3f weight_ms=%.3f boundaries=%zu",
                     patchReport_.vertexCount, patchReport_.triangleCount,
                     topology.components, topology.boundaryEdges, topology.nonManifoldEdges,
                     topology.degenerateTriangles, topology.invalidNormals,
                     patchReport_.generationMs, p1aTimings_.stateResolutionMs,
                     p1aTimings_.geometryMs, p1aTimings_.assemblyMs, p1aTimings_.weightMs,
                     patchReport_.boundaries.size());
        }
        const auto lineUploadStarted = std::chrono::steady_clock::now();
        const bool linesUploaded = hasCachedP1a_ &&
                                  p1aLines_.upload(buildAnatomyLineMesh(cachedP1aMesh_));
        const double lineUploadMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - lineUploadStarted).count();
        if (!linesUploaded) {
            PAE_LOGE("PAE_RENDER", "P1-VR-1A wireframe upload failed");
        }
        const uint64_t cpuMeshBytes = static_cast<uint64_t>(
            cachedP1aMesh_.vertices.size() * sizeof(Vertex) +
            cachedP1aMesh_.indices.size() * sizeof(uint32_t));
        const uint64_t gpuBufferBytes = cpuMeshBytes + BonePaletteUboData::kBytes;
        stats_.setBufferBytes(gpuBufferBytes);
        PAE_LOGI("PAE_RENDER",
                 "P1-VR-1A-UPLOAD mesh_ms=%.3f wire_ms=%.3f cpu_mesh_bytes=%llu gpu_buffer_bytes=%llu",
                 meshUploadMs, lineUploadMs,
                 static_cast<unsigned long long>(cpuMeshBytes),
                 static_cast<unsigned long long>(gpuBufferBytes));
#if defined(__ANDROID__)
        struct rusage usage {};
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            PAE_LOGI("PAE_RENDER", "P1-VR-1A-MEMORY peak_rss_kb=%ld", usage.ru_maxrss);
        }
#endif
    }

    if (previewMode_ == 3 && !ensureCanonicalGeometry(
            detailLevelForQuality(static_cast<QualityLevel>(qualityLevel_)))) {
        if (error) *error = "canonical body generation failed";
        return false;
    }

    if (previewMode_ == 4 && !ensureTemplateGeometry()) {
        if (error) *error = "human template load failed";
        return false;
    }

    resourcesReady_ = true;
    return true;
}

bool RenderThread::activatePreviewGpuResources(int mode) {
    if (mode == 4) {
        if (!templateMesh_.valid()) return false;
        avatarMesh_.release();
        avatarLines_.release();
        anatomyMesh_.release();
        anatomyLines_.release();
        p1aMesh_.release();
        p1aLines_.release();
        canonicalMesh_.release();
        canonicalLines_.release();
        canonicalUvMesh_.release();
        return true;
    }
    if (mode == 3) {
        if (!canonicalMesh_.valid() || !canonicalLines_.valid() || !canonicalUvMesh_.hasUv()) {
            return false;
        }
        avatarMesh_.release();
        avatarLines_.release();
        anatomyMesh_.release();
        anatomyLines_.release();
        p1aMesh_.release();
        p1aLines_.release();
        templateMesh_.release();
        templateLines_.release();
        return true;
    }

    HumanoidResult humanoid;
    const MeshData* source = nullptr;
    if (mode == 0 && hasCachedAvatar_) {
        if (avatarGeometryDirty_) {
            BodyParameters params{};
            params.detailLevel = detailLevelForQuality(static_cast<QualityLevel>(qualityLevel_));
            humanoid = generateHumanoid(params, false);
            if (!humanoid.valid) return false;
            source = &humanoid.mesh;
        } else {
            source = &cachedAvatarMesh_;
        }
    } else if (mode == 1 && hasCachedAnatomy_) {
        source = &cachedAnatomyMesh_;
    } else if (mode == 2 && hasCachedP1a_) {
        source = &cachedP1aMesh_;
    } else {
        return false;
    }

    SkinnedGpuMesh nextMesh;
    SkinnedGpuMesh nextLines;
    if (!nextMesh.upload(*source) ||
        !nextLines.upload(buildAnatomyLineMesh(*source))) {
        return false;
    }
    if (mode == 2) {
        skeleton_ = cachedP1aSkeleton_;
        joints_ = cachedP1aJoints_;
        if (!motion_.initialize(skeleton_, joints_)) return false;
    }
    if (mode == 0 && avatarGeometryDirty_) {
        if (!motion_.initialize(humanoid.skeleton, humanoid.joints)) return false;
        cachedAvatarMesh_ = humanoid.mesh;
        skeleton_ = humanoid.skeleton;
        joints_ = humanoid.joints;
        avatarGeometryDirty_ = false;
        ++avatarRevision_;
    }

    canonicalMesh_.release();
    canonicalLines_.release();
    canonicalUvMesh_.release();
    templateMesh_.release();
    templateLines_.release();
    if (mode == 0) {
        avatarMesh_ = std::move(nextMesh);
        avatarLines_ = std::move(nextLines);
    } else if (mode == 1) {
        anatomyMesh_ = std::move(nextMesh);
        anatomyLines_ = std::move(nextLines);
    } else {
        p1aMesh_ = std::move(nextMesh);
        p1aLines_ = std::move(nextLines);
    }
    return true;
}

bool RenderThread::ensureP1aGeometry(int detailLevel) {
#if defined(PAE_VR_R1_BASELINE)
    const shape::v2::CharacterDefinition definition = shape::v2::defaultR1CharacterDefinition();
#else
    const shape::v2::CharacterDefinition definition = shape::v2::defaultCharacterDefinition();
#endif
    const character::GeometryBuildRequest request = character::makeGeometryBuildRequest(
        definition, static_cast<uint32_t>(detailLevel), 2);
    const std::string cacheKey = request.cacheKey();
    if (hasCachedP1a_ && cachedP1aCacheKey_ == cacheKey) {
        skeleton_ = cachedP1aSkeleton_;
        joints_ = cachedP1aJoints_;
        if (!motion_.initialize(skeleton_, joints_)) return false;
        PAE_LOGI("PAE_RENDER", "geometry cache hit: p1a-vr (%s)", cacheKey.c_str());
        return true;
    }

    const auto started = std::chrono::steady_clock::now();
    const character::TorsoShoulderResult generated =
        character::TorsoShoulderPatchGenerator::generate(definition, request);
    if (!generated.valid) {
        PAE_LOGE("PAE_RENDER", "P1-VR-1A generation failed: %s", generated.error.c_str());
        return false;
    }
    cachedP1aMesh_ = generated.mesh;
    cachedP1aSkeleton_ = generated.state.skeleton;
    cachedP1aJoints_ = generated.state.joints;
    cachedP1aCacheKey_ = cacheKey;
    hasCachedP1a_ = true;
    patchReport_ = generated.report;
    p1aTimings_ = generated.timings;
    skeleton_ = cachedP1aSkeleton_;
    joints_ = cachedP1aJoints_;
    if (!motion_.initialize(skeleton_, joints_)) return false;
    const double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    PAE_LOGI("PAE_RENDER",
             "P1-VR-1A ready: verts=%u tris=%u components=%d boundary=%d nonmanifold=%d gen_ms=%.1f cache=%s",
             patchReport_.vertexCount, patchReport_.triangleCount,
             generated.topology.components, generated.topology.boundaryEdges,
             generated.topology.nonManifoldEdges, ms, cacheKey.c_str());
    return true;
}

bool RenderThread::ensureCanonicalGeometry(int detailLevel) {
    const auto started = std::chrono::steady_clock::now();
    const bool cached = hasCachedCanonical_ && canonicalDetailLevel_ == detailLevel;
    canonical::ContinuousSkinResult generated;
    canonical::RenderResult render;
    const MeshData* canonicalMesh = nullptr;
    const MeshData* renderMesh = nullptr;
    Skeleton nextSkeleton{};
    BodyJoints nextJoints{};
    uint64_t nextRevision = canonicalRevision_;

    if (cached) {
        PAE_LOGI("PAE_RENDER", "geometry cache hit: canonical (%u tris)",
                 static_cast<unsigned>(cachedCanonicalMesh_.indices.size() / 3));
        canonicalMesh = &cachedCanonicalMesh_;
        renderMesh = &cachedCanonicalRenderMesh_;
        nextSkeleton = cachedCanonicalSkeleton_;
        nextJoints = cachedCanonicalJoints_;
    } else {
        BodyParameters params{};
        params.detailLevel = static_cast<uint32_t>(detailLevel);
        generated = canonical::generateContinuousSkin(params, false);
        if (!generated.canonical.valid) {
            PAE_LOGE("PAE_RENDER",
                     "canonical body rejected: components=%d boundary=%d nonmanifold=%d "
                     "degen=%d dup=%d badnorm=%d weights_bad=%zu intersections=%zu",
                     generated.canonical.topology.components,
                     generated.canonical.topology.boundaryEdges,
                     generated.canonical.topology.nonManifoldEdges,
                     generated.canonical.topology.degenerateTriangles,
                     generated.canonical.topology.duplicateVertices,
                     generated.canonical.topology.invalidNormals,
                     generated.canonical.weights.invalidVertices,
                     generated.canonical.selfIntersections.intersectingPairs);
            return false;
        }
        nextSkeleton = generated.skeleton;
        nextJoints = generated.joints;
        nextRevision = canonicalRevision_ + 1;
        render = canonical::buildRenderMesh(generated.canonical);
        if (!render.valid) {
            PAE_LOGE("PAE_RENDER", "canonical Render Mesh rejected: charts=%zu invalid_uv=%zu",
                     render.uv.charts, render.uv.invalidUvVertices);
            return false;
        }
        canonicalMesh = &generated.canonical.mesh;
        renderMesh = &render.mesh;
    }

    SkinnedGpuMesh nextCanonical;
    SkinnedGpuMesh nextUv;
    SkinnedGpuMesh nextLines;
    if (!nextCanonical.upload(*canonicalMesh)) {
        PAE_LOGE("PAE_RENDER", "canonical body upload failed gl_error=0x%x",
                 static_cast<unsigned>(nextCanonical.lastError()));
        return false;
    }
    if (!nextUv.upload(*renderMesh)) {
        PAE_LOGE("PAE_RENDER", "canonical Render Mesh base upload failed gl_error=0x%x",
                 static_cast<unsigned>(nextUv.lastError()));
        return false;
    }
    if (!nextUv.uploadUv(*renderMesh, nextRevision)) {
        PAE_LOGE("PAE_RENDER", "canonical UV upload failed gl_error=0x%x vertices=%zu",
                 static_cast<unsigned>(nextUv.lastError()), renderMesh->vertices.size());
        return false;
    }
    if (!nextLines.upload(buildAnatomyLineMesh(*canonicalMesh))) {
        PAE_LOGE("PAE_RENDER", "canonical wireframe upload failed");
        return false;
    }
    if (validatedCanonicalRevision_ != nextRevision &&
        !validateCanonicalGpuPath(nextUv, *renderMesh, nextSkeleton, nextRevision)) {
        PAE_LOGE("PAE_RENDER", "canonical GPU/CPU validation failed");
        return false;
    }
    if (!motion_.initialize(nextSkeleton, nextJoints)) {
        PAE_LOGE("PAE_RENDER", "canonical motion runtime init failed");
        return false;
    }

    canonicalMesh_ = std::move(nextCanonical);
    canonicalUvMesh_ = std::move(nextUv);
    canonicalLines_ = std::move(nextLines);
    if (!cached) {
        cachedCanonicalMesh_ = generated.canonical.mesh;
        cachedCanonicalRenderMesh_ = std::move(render.mesh);
        cachedCanonicalSkeleton_ = nextSkeleton;
        cachedCanonicalJoints_ = nextJoints;
        canonicalRenderToCanonical_ = std::move(render.renderToCanonical);
        hasCachedCanonical_ = true;
        canonicalRevision_ = nextRevision;
        ++avatarRevision_;
    }
    skeleton_ = nextSkeleton;
    joints_ = nextJoints;
    canonicalDetailLevel_ = detailLevel;
    stats_.setBufferBytes(static_cast<uint64_t>(
        cachedCanonicalMesh_.vertices.size() * 64 +
        cachedCanonicalMesh_.indices.size() * sizeof(uint32_t) +
        cachedCanonicalRenderMesh_.vertices.size() * (64 + sizeof(Vec2)) +
        cachedCanonicalRenderMesh_.indices.size() * sizeof(uint32_t) +
        BonePaletteUboData::kBytes));
    const double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    PAE_LOGI("PAE_RENDER",
             "canonical body ready: verts=%zu tris=%u render_verts=%zu uv_charts=%zu gen_ms=%.0f",
             cachedCanonicalMesh_.vertices.size(),
             static_cast<unsigned>(cachedCanonicalMesh_.indices.size() / 3),
             cachedCanonicalRenderMesh_.vertices.size(),
             cachedCanonicalRenderMesh_.indices.size() / 3, ms);
    return true;
}

bool RenderThread::ensureTemplateGeometry() {
    // VR-1B WP2/WP2.1: read-only static human template. Normal operation loads
    // the .paht from APK assets (bytes delivered by the additive JNI entry
    // nativeLoadTemplateAsset). A pushed dev file at kTemplatePath overrides
    // the asset when present (rejection testing). No new JNI signature was
    // added to the frozen surface; the asset entry is purely additive.
    constexpr const char* kTemplatePath = "/data/local/tmp/pae-human-base.paht";
    const auto started = std::chrono::steady_clock::now();

    if (hasCachedTemplate_) {
        SkinnedGpuMesh nextMesh;
        SkinnedGpuMesh nextLines;
        if (!nextMesh.upload(cachedTemplateMesh_) ||
            !nextLines.upload(buildAnatomyLineMesh(cachedTemplateMesh_))) {
            templateStatus_ = "template=reupload-failed";
            PAE_LOGE("PAE_RENDER", "template re-upload failed gl_error=0x%x",
                     static_cast<unsigned>(nextMesh.lastError()));
            return false;
        }
        templateMesh_ = std::move(nextMesh);
        templateLines_ = std::move(nextLines);
        templateStatus_ = "template=cached verts=" + std::to_string(cachedTemplateMesh_.vertices.size());
        PAE_LOGI("PAE_RENDER", "geometry cache hit: human template (%u tris)",
                 static_cast<unsigned>(cachedTemplateMesh_.indices.size() / 3));
        return true;
    }

    // Source selection (WP2.1): a pushed dev file overrides the APK asset so
    // rejection tests can inject corrupted bytes; otherwise the asset bytes
    // delivered by the additive JNI entry are used. No external file is
    // required for normal operation.
    std::string bytes;
    const char* source = nullptr;
    if (std::FILE* f = std::fopen(kTemplatePath, "rb")) {
        char buffer[65536];
        size_t n = 0;
        while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0) bytes.append(buffer, n);
        const bool readError = std::ferror(f) != 0;
        std::fclose(f);
        if (readError) {
            templateStatus_ = "template=file-read-error";
            PAE_LOGE("PAE_RENDER", "human template dev-file read error: %s", kTemplatePath);
            return false;
        }
        source = "file";
    } else if (hasTemplateAssetBytes_) {
        bytes = templateAssetBytes_;
        source = "asset";
    } else {
        templateStatus_ = "template=no-source";
        PAE_LOGE("PAE_RENDER", "human template has no source: asset not loaded and no dev file");
        return false;
    }

    human_template::HumanTemplate t;
    std::string error;
    if (!human_template::loadTemplateBytes(bytes, t, error)) {
        templateStatus_ = std::string("template=rejected (") + error + ") source=" + source;
        PAE_LOGE("PAE_RENDER", "human template rejected: %s source=%s", error.c_str(), source);
        return false;
    }
    MeshData mesh;
    human_template::StaticRenderMeshReport report;
    if (!human_template::buildStaticRenderMesh(t, mesh, report, error)) {
        templateStatus_ = "template=mesh-failed (" + error + ")";
        PAE_LOGE("PAE_RENDER", "human template mesh failed: %s", error.c_str());
        return false;
    }
    // Ground the body: the source mesh is centred on the origin; the scene
    // convention (all other modes) is feet at y~0. Pure translation.
    const float groundOffset = -mesh.bounds.min.y;
    Aabb grounded;
    for (Vertex& v : mesh.vertices) {
        v.position.y += groundOffset;
        grounded.expand(v.position);
    }
    mesh.bounds = grounded;

    SkinnedGpuMesh nextMesh;
    SkinnedGpuMesh nextLines;
    if (!nextMesh.upload(mesh)) {
        templateStatus_ = "template=upload-failed";
        PAE_LOGE("PAE_RENDER", "human template upload failed gl_error=0x%x",
                 static_cast<unsigned>(nextMesh.lastError()));
        return false;
    }
    if (!nextLines.upload(buildAnatomyLineMesh(mesh))) {
        templateStatus_ = "template=wire-upload-failed";
        PAE_LOGE("PAE_RENDER", "human template wireframe upload failed");
        return false;
    }
    templateMesh_ = std::move(nextMesh);
    templateLines_ = std::move(nextLines);
    cachedTemplateMesh_ = std::move(mesh);
    hasCachedTemplate_ = true;
    stats_.setBufferBytes(static_cast<uint64_t>(
        cachedTemplateMesh_.vertices.size() * 64 +
        cachedTemplateMesh_.indices.size() * sizeof(uint32_t) +
        BonePaletteUboData::kBytes));
    const double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
    templateStatus_ = std::string("template=loaded source=") + source +
                      " verts=" + std::to_string(report.vertexCount) +
                      " tris=" + std::to_string(report.triangleCount);
    // v3 is deterministic, so re-serializing reproduces the exact file bytes:
    // this hash identifies the loaded asset in the device log.
    const std::string assetSha = human_template::sha256Hex(human_template::serializeTemplate(t));
    PAE_LOGI("PAE_RENDER",
             "human template ready: source=%s verts=%u tris=%u quads=%u degenerate=%u ground_offset=%.4f load_ms=%.0f sha256=%s",
             source, report.vertexCount, report.triangleCount, report.quadCount, report.degenerateTriangles,
             groundOffset, ms, assetSha.c_str());
    return true;
}

bool RenderThread::validateCanonicalGpuPath(const SkinnedGpuMesh& renderMesh,
                                            const MeshData& cpuMesh,
                                            const Skeleton& skeleton,
                                            uint64_t revision) {
    if (!renderMesh.hasUv() || renderMesh.vertexCount() == 0) return false;
    if (validationProgram_ == 0) {
        std::string error;
        validationProgram_ = buildValidationProgram(&error);
        if (validationProgram_ == 0) {
            PAE_LOGE("PAE_RENDER", "P1B validation shader build failed: %s", error.c_str());
            return false;
        }
        const GLuint block = glGetUniformBlockIndex(validationProgram_, "BoneBlock");
        if (block == GL_INVALID_INDEX) {
            PAE_LOGE("PAE_RENDER", "P1B validation BoneBlock not found");
            return false;
        }
        glUniformBlockBinding(validationProgram_, block, 0);
        uniformValidationModel_ = glGetUniformLocation(validationProgram_, "uModel");
        uniformValidationNormalMatrix_ = glGetUniformLocation(validationProgram_, "uNormalMatrix");
        if (uniformValidationModel_ < 0 || uniformValidationNormalMatrix_ < 0) {
            PAE_LOGE("PAE_RENDER", "P1B validation uniforms not found");
            return false;
        }
    }
    const auto zRotation = [](float degrees) {
        const float half = degrees * 3.14159265358979323846f / 360.0f;
        return Quat{0.0f, 0.0f, std::sin(half), std::cos(half)};
    };
    const auto xRotation = [](float degrees) {
        const float half = degrees * 3.14159265358979323846f / 360.0f;
        return Quat{std::sin(half), 0.0f, 0.0f, std::cos(half)};
    };
    std::array<Quat, BoneCount> representative{};
    representative.fill(Quat{});
    representative[BoneLeftUpperArm] = zRotation(40.0f);
    representative[BoneLeftLowerArm] = zRotation(-85.0f);
    representative[BoneLeftUpperLeg] = xRotation(-25.0f);
    representative[BoneLeftLowerLeg] = xRotation(55.0f);
    representative[BoneRightUpperLeg] = xRotation(-25.0f);
    representative[BoneRightLowerLeg] = xRotation(55.0f);

    struct ValidationCase {
        const char* name;
        std::array<Quat, BoneCount> rotations;
        Mat4 model;
    };
    std::array<Quat, BoneCount> bindRotations{};
    bindRotations.fill(Quat{});
    const ValidationCase cases[] = {
        {"bind", bindRotations, Mat4{}},
        {"elbow85_knee80", representative, Mat4{}},
        {"elbow85_knee80_scale_2_0.5_3", representative,
         Mat4::translation({0.25f, -0.5f, 0.75f}) * Mat4::scaling({2.0f, 0.5f, 3.0f})}
    };

    const uint32_t vertexCount = renderMesh.vertexCount();
    for (const ValidationCase& test : cases) {
        const PoseResult pose = computePose(skeleton, test.rotations);
        for (uint16_t bone = 0; bone < BoneCount; ++bone) {
            paletteData_.writeBone(bone, pose.palette[bone]);
        }
        boneUbo_.update(paletteData_);
        const std::vector<Vec3> cpuPositions = cpuSkinPositions(
            cpuMesh, pose.palette, test.model);
        const std::vector<Vec3> cpuNormals = cpuSkinNormals(
            cpuMesh, pose.palette, test.model);
        if (cpuPositions.size() != vertexCount || cpuNormals.size() != vertexCount) return false;

        while (glGetError() != GL_NO_ERROR) {}
        GLuint feedbackBuffer = 0;
        GLuint feedbackObject = 0;
        glGenBuffers(1, &feedbackBuffer);
        glGenTransformFeedbacks(1, &feedbackObject);
        if (feedbackBuffer == 0 || feedbackObject == 0) {
            if (feedbackBuffer != 0) glDeleteBuffers(1, &feedbackBuffer);
            if (feedbackObject != 0) glDeleteTransformFeedbacks(1, &feedbackObject);
            return false;
        }
        const GLsizeiptr feedbackBytes = static_cast<GLsizeiptr>(vertexCount) * 6 * sizeof(float);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedbackObject);
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuffer);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBytes, nullptr, GL_STREAM_READ);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedbackBuffer);
        glUseProgram(validationProgram_);
        glUniformMatrix4fv(uniformValidationModel_, 1, GL_FALSE, test.model.m.data());
        glUniformMatrix3fv(uniformValidationNormalMatrix_, 1, GL_FALSE,
                           modelNormalMatrix(test.model).m.data());
        renderMesh.bindUv();
        glEnable(GL_RASTERIZER_DISCARD);
        glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertexCount));
        glEndTransformFeedback();
        glDisable(GL_RASTERIZER_DISCARD);
        std::vector<float> gpuData(static_cast<size_t>(vertexCount) * 6, 0.0f);
        void* mapped = glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedbackBytes, GL_MAP_READ_BIT);
        if (mapped != nullptr) {
            std::memcpy(gpuData.data(), mapped, gpuData.size() * sizeof(float));
            glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
        }
        const GLenum glError = mapped == nullptr ? GL_INVALID_OPERATION : glGetError();
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        glBindVertexArray(0);
        glDeleteTransformFeedbacks(1, &feedbackObject);
        glDeleteBuffers(1, &feedbackBuffer);
        if (glError != GL_NO_ERROR) {
            PAE_LOGE("PAE_RENDER", "P1B-GPU-CPU case=%s gl_error=0x%x",
                     test.name, static_cast<unsigned>(glError));
            return false;
        }

        double positionErrorSum = 0.0;
        float maxPositionError = 0.0f;
        float maxNormalAngle = 0.0f;
        size_t abnormalNormals = 0;
        size_t invalidNormals = 0;
        size_t invalidPositions = 0;
        for (uint32_t vertex = 0; vertex < vertexCount; ++vertex) {
            const size_t offset = static_cast<size_t>(vertex) * 6;
            const Vec3 gpuPosition{gpuData[offset], gpuData[offset + 1], gpuData[offset + 2]};
            const Vec3 gpuNormal{gpuData[offset + 3], gpuData[offset + 4], gpuData[offset + 5]};
            if (!gpuPosition.finite() || !cpuPositions[vertex].finite()) {
                ++invalidPositions;
            } else {
                const float error = (gpuPosition - cpuPositions[vertex]).length();
                maxPositionError = std::fmax(maxPositionError, error);
                positionErrorSum += error;
            }
            if (!gpuNormal.finite() || !cpuNormals[vertex].finite() ||
                std::fabs(gpuNormal.length() - 1.0f) > 1e-3f ||
                std::fabs(cpuNormals[vertex].length() - 1.0f) > 1e-3f) {
                ++invalidNormals;
                continue;
            }
            const float dot = std::clamp(gpuNormal.dot(cpuNormals[vertex]), -1.0f, 1.0f);
            const float angle = std::acos(dot);
            maxNormalAngle = std::fmax(maxNormalAngle, angle);
            if (angle > kMaxNormalAngleRadians) ++abnormalNormals;
        }
        const float meanPositionError = static_cast<float>(positionErrorSum / vertexCount);
        const bool valid = invalidPositions == 0 && invalidNormals == 0 &&
                           maxPositionError <= kMaxPositionErrorMeters &&
                           meanPositionError <= kMaxMeanPositionErrorMeters &&
                           maxNormalAngle <= kMaxNormalAngleRadians && abnormalNormals == 0;
        PAE_LOGI("PAE_RENDER",
                 "P1B-GPU-CPU case=%s vertices=%u max_pos_m=%.9g mean_pos_m=%.9g "
                 "max_normal_deg=%.9g abnormal_normals=%zu invalid_positions=%zu "
                 "invalid_normals=%zu result=%s",
                 test.name, vertexCount, maxPositionError, meanPositionError,
                 maxNormalAngle * 180.0f / 3.14159265358979323846f,
                 abnormalNormals, invalidPositions, invalidNormals, valid ? "ok" : "failed");
        if (!valid) return false;
    }
    validatedCanonicalRevision_ = revision;
    return true;
}

void RenderThread::releaseGpuResources() {
    avatarMesh_.release();
    avatarLines_.release();
    canonicalMesh_.release();
    canonicalLines_.release();
    canonicalUvMesh_.release();
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
    templateMesh_.release();
    templateLines_.release();
    boneUbo_.release();
    if (validationProgram_ != 0) glDeleteProgram(validationProgram_);
    validationProgram_ = 0;
    uniformValidationModel_ = -1;
    uniformValidationNormalMatrix_ = -1;
    validatedCanonicalRevision_ = 0;
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
    glUniformMatrix3fv(uniformOutlineNormalMatrix_, 1, GL_FALSE, modelNormalMatrix(model).m.data());
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
    Vec3 cameraEye = resolveCameraEye();
    Vec3 cameraTarget = cameraTarget_;
    if (previewMode_ == 2) {
        const int index = std::clamp(cameraPreset_, 0, kPresetCount - 1);
        cameraEye = kP1aPresetEye[index];
        cameraTarget = kP1aPresetTarget[index];
    }
    const Mat4 view = Mat4::lookAt(cameraEye, cameraTarget, {0.0f, 1.0f, 0.0f});
    const Mat4 viewProjection = projection * view;
    const Mat4 model = Mat4::translation({0.0f, -0.05f, 0.0f});
    const Mat3 normalMatrix = modelNormalMatrix(model);

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
    if (previewMode_ == 1 || previewMode_ == 4) {
        // SDF anatomy preview and the static human template both show raw
        // bind-pose geometry: identity palette.
        for (uint16_t bone = 0; bone < BoneCount; ++bone) {
            paletteData_.writeBone(bone, Mat4{});
        }
        boneUbo_.update(paletteData_);
    } else {
        // P1-A patches and the production avatar both exercise real bone palettes.
        updateBonePalette(nowNs);
    }


    // Ground contact shadow (M10): blended disc under the feet, drawn before
    // the character so the avatar occludes it. The local P1-VR patch has no
    // feet/ground relationship, so its evidence camera omits the disc.
    if (previewMode_ != 2) {
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
    }

    // World-space reference grid. Drawn before the character so the avatar
    // occludes it, and in world space so the free camera reads the scene in
    // metres instead of against an empty backdrop.
    if (previewMode_ == 0 || previewMode_ == 3 || previewMode_ == 4) {
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
        glUniformMatrix3fv(uniformNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
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
            skinnedLineProgram_.use();
            glUniformMatrix4fv(uniformLineViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformLineModel_, 1, GL_FALSE, model.m.data());
            glUniform4f(uniformLineColor_, 0.10f, 0.42f, 0.16f, 1.0f);
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
            glUniformMatrix3fv(uniformAnatomyNormalNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            p1aMesh_.draw();
        } else if (debugView_ == 5) {
            skinnedProgram_.use();
            glUniformMatrix4fv(uniformViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            glUniform3f(skinnedProgram_.uniform("uLightDir"), 0.4f, 0.8f, 0.6f);
            glUniform1i(uniformDebugMode_, 5);
            p1aMesh_.draw();
        } else if (debugView_ == 4) {
            skinnedProgram_.use();
            glUniformMatrix4fv(uniformViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            glUniform3f(skinnedProgram_.uniform("uLightDir"), 0.4f, 0.8f, 0.6f);
            glUniform1i(uniformDebugMode_, 4);
            p1aMesh_.draw();
        } else {
            anatomyClayProg_.use();
            glUniformMatrix4fv(uniformAnatomyClayViewProj_, 1, GL_FALSE,
                               viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyClayModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformAnatomyClayNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            glUniform3f(uniformAnatomyClayNeutral_, 0.62f, 0.62f, 0.64f);
            glUniform3f(uniformAnatomyClayKeyDir_, 0.45f, 0.75f, 0.48f);
            glUniform3f(uniformAnatomyClayFillDir_, -0.55f, 0.15f, 0.35f);
            p1aMesh_.draw();
            if (debugView_ == 1) {
                glDepthRangef(0.0f, 0.0f);
                skinnedLineProgram_.use();
                glUniformMatrix4fv(uniformLineViewProj_, 1, GL_FALSE,
                                   viewProjection.m.data());
                glUniformMatrix4fv(uniformLineModel_, 1, GL_FALSE, model.m.data());
                glUniform4f(uniformLineColor_, 0.05f, 0.35f, 0.08f, 1.0f);
                p1aLines_.drawLines();
                glDepthRangef(0.0f, 1.0f);
            }
        }
    } else if (previewMode_ == 3) {
        if (debugView_ == 7) {
            uvProgram_.use();
            glUniformMatrix4fv(uniformUvViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformUvModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformUvNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            glUniform3f(uniformUvLightDir_, 0.4f, 0.8f, 0.6f);
            glUniform1f(uniformUvCheckerScale_, 6.0f);
            canonicalUvMesh_.drawUv();
        } else {
        const int debugMode = debugView_ == 2 ? 2
                            : debugView_ == 4 ? 4
                            : debugView_ == 5 ? 5
                            : 3;
        skinnedProgram_.use();
        glUniformMatrix4fv(uniformViewProj_, 1, GL_FALSE, viewProjection.m.data());
        glUniformMatrix4fv(uniformModel_, 1, GL_FALSE, model.m.data());
        glUniformMatrix3fv(uniformNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
        glUniform3f(uniformCameraPos_, cameraEye.x, cameraEye.y, cameraEye.z);
        glUniform3f(uniformSkin_, appearance_[0].x, appearance_[0].y, appearance_[0].z);
        glUniform3f(uniformHair_, appearance_[1].x, appearance_[1].y, appearance_[1].z);
        glUniform3f(uniformCloth_, appearance_[2].x, appearance_[2].y, appearance_[2].z);
        glUniform3f(uniformEye_, appearance_[3].x, appearance_[3].y, appearance_[3].z);
        glUniform3f(uniformPants_, appearance_[4].x, appearance_[4].y, appearance_[4].z);
        glUniform3f(skinnedProgram_.uniform("uLightDir"), 0.4f, 0.8f, 0.6f);
        glUniform1f(skinnedProgram_.uniform("uAmbient"), 0.42f);
        glUniform1f(skinnedProgram_.uniform("uToonSteps"), 3.0f);
        glUniform1f(skinnedProgram_.uniform("uRim"), 0.0f);
        glUniform1i(uniformDebugMode_, debugMode);
        canonicalMesh_.draw();
        if (debugView_ == 1) {
            glDepthRangef(0.0f, 0.0f);
            skinnedLineProgram_.use();
            glUniformMatrix4fv(uniformLineViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformLineModel_, 1, GL_FALSE, model.m.data());
            glUniform4f(uniformLineColor_, 0.05f, 0.35f, 0.08f, 1.0f);
            canonicalLines_.drawLines();
            glDepthRangef(0.0f, 1.0f);
        }
        }
    } else if (previewMode_ == 4) {
        // VR-1B WP2: static full-body human template. Gray clay by default
        // (the morphology acceptance view), normals and wireframe as debug.
        if (debugView_ == 2) {
            anatomyNormalProg_.use();
            glUniformMatrix4fv(uniformAnatomyNormalViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyNormalModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformAnatomyNormalNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            templateMesh_.draw();
        } else {
            anatomyClayProg_.use();
            glUniformMatrix4fv(uniformAnatomyClayViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyClayModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformAnatomyClayNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            glUniform3f(uniformAnatomyClayNeutral_, 0.62f, 0.62f, 0.64f);
            glUniform3f(uniformAnatomyClayKeyDir_, 0.45f, 0.75f, 0.48f);
            glUniform3f(uniformAnatomyClayFillDir_, -0.55f, 0.15f, 0.35f);
            templateMesh_.draw();
            if (debugView_ == 1) {
                glDepthRangef(0.0f, 0.0f);
                skinnedLineProgram_.use();
                glUniformMatrix4fv(uniformLineViewProj_, 1, GL_FALSE, viewProjection.m.data());
                glUniformMatrix4fv(uniformLineModel_, 1, GL_FALSE, model.m.data());
                glUniform4f(uniformLineColor_, 0.05f, 0.35f, 0.08f, 1.0f);
                templateLines_.drawLines();
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
            glUniformMatrix3fv(uniformAnatomyNormalNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            anatomyMesh_.draw();
        } else if (debugView_ == 3) {
            // M13 gray clay: continuous Lambert, no toon banding — the only
            // view morphology should be accepted in.
            anatomyClayProg_.use();
            glUniformMatrix4fv(uniformAnatomyClayViewProj_, 1, GL_FALSE,
                               viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyClayModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformAnatomyClayNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            glUniform3f(uniformAnatomyClayNeutral_, 0.62f, 0.62f, 0.64f);
            glUniform3f(uniformAnatomyClayKeyDir_, 0.45f, 0.75f, 0.48f);
            glUniform3f(uniformAnatomyClayFillDir_, -0.55f, 0.15f, 0.35f);
            anatomyMesh_.draw();
        } else {
            anatomyProgram_.use();
            glUniformMatrix4fv(uniformAnatomyViewProj_, 1, GL_FALSE, viewProjection.m.data());
            glUniformMatrix4fv(uniformAnatomyModel_, 1, GL_FALSE, model.m.data());
            glUniformMatrix3fv(uniformAnatomyNormalMatrix_, 1, GL_FALSE, normalMatrix.m.data());
            glUniform3f(uniformAnatomyNeutral_, 0.72f, 0.66f, 0.60f);
            // Neutral study lighting: without these the shared fragment shader
            // defaults to ambient=0/steps=0 and the whole mesh renders black.
            glUniform3f(anatomyProgram_.uniform("uLightDir"), 0.42f, 0.78f, 0.55f);
            glUniform1f(anatomyProgram_.uniform("uAmbient"), 0.36f);
            glUniform1f(anatomyProgram_.uniform("uToonSteps"), 6.0f);
            anatomyMesh_.draw();
            if (debugView_ == 1 || debugView_ == 3) {
                glDepthRangef(0.0f, 0.0f); // pull lines to the front
                skinnedLineProgram_.use();
                glUniformMatrix4fv(uniformLineViewProj_, 1, GL_FALSE,
                                   viewProjection.m.data());
                glUniformMatrix4fv(uniformLineModel_, 1, GL_FALSE, model.m.data());
                glUniform4f(uniformLineColor_, 0.05f, 0.35f, 0.08f, 1.0f);
                anatomyLines_.drawLines();
                glDepthRangef(0.0f, 1.0f);
            }
        }
    }

    egl_.swapBuffers();
    uint32_t frameDraws = previewMode_ == 2 ? 2u : 3u;
    if (previewMode_ == 0 || previewMode_ == 3 || previewMode_ == 4) frameDraws += 2;
    if ((previewMode_ == 0 && (debugView_ == 1 || debugView_ == 6)) ||
        (previewMode_ == 3 && debugView_ == 1) ||
        (previewMode_ == 2 && debugView_ == 1) ||
        (previewMode_ == 4 && debugView_ == 1) ||
        (previewMode_ == 1 && (debugView_ == 1 || debugView_ == 3))) {
        ++frameDraws;
    }
    const uint32_t frameTris = previewMode_ == 0
        ? static_cast<uint32_t>(avatarMesh_.indexCount() / 3)
        : (previewMode_ == 2
            ? static_cast<uint32_t>(p1aMesh_.indexCount() / 3)
            : (previewMode_ == 3
                ? static_cast<uint32_t>(canonicalMesh_.indexCount() / 3)
                : (previewMode_ == 4
                    ? static_cast<uint32_t>(templateMesh_.indexCount() / 3)
                    : static_cast<uint32_t>(anatomyMesh_.indexCount() / 3))));
    stats_.endFrame(frameDraws, frameTris);
    if (stats_.frameCount() % 120 == 0) {
        PAE_LOGI("PAE_STATS", "%s", stats_.toString().c_str());
    }
}

} // namespace pae
