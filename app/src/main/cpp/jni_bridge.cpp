#include <jni.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "core/diagnostics/Log.h"
#include "runtime/EngineRuntime.h"

#include <cmath>

namespace {

pae::EngineRuntime* runtimeFromHandle(jlong handle) {
    return reinterpret_cast<pae::EngineRuntime*>(handle);
}

} // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_pae_host_NativeBridge_nativeCreate(JNIEnv*, jclass) {
    auto* runtime = new pae::EngineRuntime();
    std::string error;
    if (!runtime->start(&error)) {
        PAE_LOGE("PAE_JNI", "runtime start failed: %s", error.c_str());
        delete runtime;
        return 0;
    }
    PAE_LOGI("PAE_JNI", "runtime created");
    return reinterpret_cast<jlong>(runtime);
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSurfaceCreated(JNIEnv* env, jclass, jlong handle,
                                                    jobject surface, jint width, jint height) {
    auto* runtime = runtimeFromHandle(handle);
    if (!runtime || !surface) return;
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        PAE_LOGE("PAE_JNI", "ANativeWindow_fromSurface returned null");
        return;
    }
    runtime->surfaceCreated(window, width, height);
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSurfaceChanged(JNIEnv*, jclass, jlong handle,
                                                    jint width, jint height) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->surfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSurfaceDestroyed(JNIEnv*, jclass, jlong handle) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->surfaceDestroyed();
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSetAppearance(JNIEnv* env, jclass, jlong handle,
                                                   jfloatArray colors) {
    auto* runtime = runtimeFromHandle(handle);
    if (!runtime || !colors || env->GetArrayLength(colors) < 12) return;
    const jsize length = env->GetArrayLength(colors);
    jfloat* values = env->GetFloatArrayElements(colors, nullptr);
    if (!values) return;
    std::array<pae::Vec3, 5> palette{};
    const int colorCount = length >= 15 ? 5 : 4; // backward compatible
    for (int i = 0; i < colorCount; ++i) {
        palette[i] = {values[i * 3], values[i * 3 + 1], values[i * 3 + 2]};
    }
    runtime->setAppearance(palette);
    env->ReleaseFloatArrayElements(colors, values, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSetBackground(JNIEnv* env, jclass, jlong handle,
                                                   jint mode, jfloatArray colors) {
    auto* runtime = runtimeFromHandle(handle);
    if (!runtime || !colors || env->GetArrayLength(colors) < 6) return;
    jfloat* values = env->GetFloatArrayElements(colors, nullptr);
    if (!values) return;
    const pae::Vec3 colorA{values[0], values[1], values[2]};
    const pae::Vec3 colorB{values[3], values[4], values[5]};
    runtime->setBackground(static_cast<int>(mode), colorA, colorB);
    env->ReleaseFloatArrayElements(colors, values, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSetPoseMode(JNIEnv*, jclass, jlong handle, jint mode) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->setPoseMode(static_cast<int>(mode));
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativePushPose(JNIEnv* env, jclass, jlong handle,
                                              jfloatArray landmarks, jfloat confidence,
                                              jboolean hasPerson, jlong timestampNs,
                                              jint width, jint height, jint rotation,
                                              jboolean mirrored) {
    auto* runtime = runtimeFromHandle(handle);
    if (!runtime || !landmarks) return;
    const jsize length = env->GetArrayLength(landmarks);
    if (length < static_cast<jsize>(pae::LandmarkCount * 4)) return;
    jfloat* values = env->GetFloatArrayElements(landmarks, nullptr);
    if (!values) return;

    pae::PoseFrame frame;
    frame.timestampNs = static_cast<int64_t>(timestampNs);
    frame.coordinateSpace = pae::CoordinateSpace::PixelImage;
    frame.poseConfidence = static_cast<float>(confidence);
    frame.hasPerson = hasPerson == JNI_TRUE;
    frame.imageWidth = static_cast<uint32_t>(width);
    frame.imageHeight = static_cast<uint32_t>(height);
    frame.rotationDegrees = static_cast<int>(rotation);
    frame.mirrored = mirrored == JNI_TRUE;
    frame.landmarkCount = pae::LandmarkCount;
    for (uint32_t i = 0; i < pae::LandmarkCount; ++i) {
        pae::PoseLandmark& landmark = frame.landmarks[i];
        landmark.id = static_cast<uint16_t>(i);
        landmark.position = {values[i * 4], values[i * 4 + 1], values[i * 4 + 2]};
        landmark.visibility = values[i * 4 + 3];
        landmark.presence = landmark.visibility;
        landmark.valid = landmark.visibility > 0.05f &&
                         std::isfinite(landmark.position.x) && std::isfinite(landmark.position.y);
    }
    runtime->pushPose(frame);
    env->ReleaseFloatArrayElements(landmarks, values, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeResetMotion(JNIEnv*, jclass, jlong handle) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->resetMotion();
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSetQuality(JNIEnv*, jclass, jlong handle, jint level) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->setQuality(static_cast<int>(level));
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSetAnatomyPreview(JNIEnv*, jclass, jlong handle,
                                                       jboolean on) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->setAnatomyPreview(on == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSetCameraPreset(JNIEnv*, jclass, jlong handle, jint preset) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->setCameraPreset(static_cast<int>(preset));
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeSetDebugView(JNIEnv*, jclass, jlong handle, jint view) {
    if (auto* runtime = runtimeFromHandle(handle)) runtime->setDebugView(static_cast<int>(view));
}

JNIEXPORT jstring JNICALL
Java_com_pae_host_NativeBridge_nativeGetStats(JNIEnv* env, jclass, jlong handle) {
    auto* runtime = runtimeFromHandle(handle);
    const std::string stats = runtime ? runtime->stats() : std::string("runtime unavailable");
    return env->NewStringUTF(stats.c_str());
}

JNIEXPORT void JNICALL
Java_com_pae_host_NativeBridge_nativeShutdown(JNIEnv*, jclass, jlong handle) {
    auto* runtime = runtimeFromHandle(handle);
    if (!runtime) return;
    runtime->shutdown();
    delete runtime;
    PAE_LOGI("PAE_JNI", "runtime released");
}

} // extern "C"
