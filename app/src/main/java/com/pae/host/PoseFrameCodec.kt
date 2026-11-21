package com.pae.host

import com.google.mlkit.vision.pose.Pose

// Shared ML Kit Pose -> PoseFrame packing used by camera (M7) and video (M8)
// sources. Landmarks are pixel coordinates; monocular Z is not metric and is
// ignored (Spec §7.6).
object PoseFrameCodec {
    const val LANDMARK_COUNT = 33

    data class Packed(val confidence: Float, val landmarkCount: Int)

    fun packAndPush(
        pose: Pose, timestampNs: Long, width: Int, height: Int,
        rotation: Int, mirrored: Boolean
    ): Packed {
        val landmarks = FloatArray(LANDMARK_COUNT * 4)
        val found = pose.allPoseLandmarks
        var visibilitySum = 0f
        for (landmark in found) {
            val index = landmark.landmarkType
            if (index !in 0 until LANDMARK_COUNT) continue
            landmarks[index * 4] = landmark.position.x
            landmarks[index * 4 + 1] = landmark.position.y
            landmarks[index * 4 + 2] = 0f // monocular Z is not metric; ignored
            landmarks[index * 4 + 3] = landmark.inFrameLikelihood
            visibilitySum += landmark.inFrameLikelihood
        }
        val confidence = if (found.isEmpty()) 0f else visibilitySum / found.size
        NativeBridge.pushPose(
            landmarks, confidence, found.isNotEmpty(),
            timestampNs, width, height, rotation, mirrored
        )
        return Packed(confidence, found.size)
    }
}
