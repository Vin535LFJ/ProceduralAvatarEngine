package com.pae.host

import android.view.Surface

object NativeBridge {
    private var handle: Long = 0L

    init {
        System.loadLibrary("pae_native")
    }

    private external fun nativeCreate(): Long
    private external fun nativeSurfaceCreated(handle: Long, surface: Surface, width: Int, height: Int)
    private external fun nativeSurfaceChanged(handle: Long, width: Int, height: Int)
    private external fun nativeSurfaceDestroyed(handle: Long)
    private external fun nativeGetStats(handle: Long): String
    private external fun nativeShutdown(handle: Long)
    private external fun nativeSetAppearance(handle: Long, colors: FloatArray)
    private external fun nativeSetBackground(handle: Long, mode: Int, colors: FloatArray)
    private external fun nativeSetPoseMode(handle: Long, mode: Int)
    private external fun nativePushPose(
        handle: Long, landmarks: FloatArray, confidence: Float, hasPerson: Boolean,
        timestampNs: Long, width: Int, height: Int, rotation: Int, mirrored: Boolean
    )
    private external fun nativeResetMotion(handle: Long)
    private external fun nativeSetQuality(handle: Long, level: Int)

    fun create() {
        if (handle == 0L) handle = nativeCreate()
    }

    fun surfaceCreated(surface: Surface, width: Int, height: Int) {
        if (handle != 0L) nativeSurfaceCreated(handle, surface, width, height)
    }

    fun surfaceChanged(width: Int, height: Int) {
        if (handle != 0L) nativeSurfaceChanged(handle, width, height)
    }

    fun surfaceDestroyed() {
        if (handle != 0L) nativeSurfaceDestroyed(handle)
    }

    fun stats(): String = if (handle != 0L) nativeGetStats(handle) else "runtime unavailable"

    fun setAppearance(colors: FloatArray) {
        if (handle != 0L) nativeSetAppearance(handle, colors)
    }

    fun setBackground(mode: Int, colorA: FloatArray, colorB: FloatArray) {
        if (handle != 0L) nativeSetBackground(handle, mode, colorA + colorB)
    }

    fun setPoseMode(mode: Int) {
        if (handle != 0L) nativeSetPoseMode(handle, mode)
    }

    fun pushPose(
        landmarks: FloatArray, confidence: Float, hasPerson: Boolean,
        timestampNs: Long, width: Int, height: Int, rotation: Int, mirrored: Boolean
    ) {
        if (handle != 0L) {
            nativePushPose(handle, landmarks, confidence, hasPerson, timestampNs,
                width, height, rotation, mirrored)
        }
    }

    fun resetMotion() {
        if (handle != 0L) nativeResetMotion(handle)
    }

    fun setQuality(level: Int) {
        if (handle != 0L) nativeSetQuality(handle, level)
    }

    fun release() {
        if (handle != 0L) {
            nativeShutdown(handle)
            handle = 0L
        }
    }
}
