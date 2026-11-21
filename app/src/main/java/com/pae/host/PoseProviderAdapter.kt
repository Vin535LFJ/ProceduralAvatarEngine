package com.pae.host

import android.graphics.Bitmap
import android.os.SystemClock
import android.util.Log
import android.util.Size
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraSelector
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import com.google.mlkit.vision.common.InputImage
import com.google.mlkit.vision.pose.PoseDetection
import com.google.mlkit.vision.pose.PoseDetectorOptionsBase
import com.google.mlkit.vision.pose.accurate.AccuratePoseDetectorOptions
import com.google.mlkit.vision.pose.Pose
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger

class PoseProviderAdapter(private val activity: AppCompatActivity) {
    private val executor = Executors.newSingleThreadExecutor()
    private val running = AtomicBoolean(false)
    private var cameraProvider: ProcessCameraProvider? = null

    private val options = AccuratePoseDetectorOptions.Builder()
        .setDetectorMode(PoseDetectorOptionsBase.STREAM_MODE)
        .build()
    private val detector = PoseDetection.getClient(options)

    private val inFlight = AtomicInteger(0)
    private val frameCounter = AtomicInteger(0)
    private var lastPoseNs = 0L

    fun start() {
        if (!running.compareAndSet(false, true)) return
        try {
            val future = ProcessCameraProvider.getInstance(activity)
            future.addListener({
                try {
                    cameraProvider = future.get()
                    val analysis = ImageAnalysis.Builder()
                        .setTargetResolution(Size(640, 480))
                        .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                        .build()
                    analysis.setAnalyzer(executor, this::analyze)
                    bindWithFallback(analysis)
                } catch (error: Exception) {
                    Log.e(TAG, "camera provider failed: ${error.message}", error)
                    running.set(false)
                }
            }, ContextCompat.getMainExecutor(activity))
        } catch (error: Exception) {
            Log.e(TAG, "camera provider failed: ${error.message}", error)
            running.set(false)
        }
    }

    // Some TV boxes declare camera features but resolve no FRONT camera at runtime.
    // Try front, then back, then an unconstrained selector, and log the outcome.
    private fun bindWithFallback(analysis: ImageAnalysis) {
        val selectors = listOf(
            "FRONT" to CameraSelector.DEFAULT_FRONT_CAMERA,
            "BACK" to CameraSelector.DEFAULT_BACK_CAMERA,
            "ANY" to CameraSelector.Builder().build()
        )
        for ((name, selector) in selectors) {
            try {
                cameraProvider?.bindToLifecycle(activity, selector, analysis)
                Log.i(TAG, "camera bound with ImageAnalysis KEEP_ONLY_LATEST (selector=$name)")
                return
            } catch (error: Exception) {
                Log.w(TAG, "camera selector $name failed: ${error.message}")
                cameraProvider?.unbindAll()
            }
        }
        Log.e(TAG, "no camera resolved on this device; synthetic provider stays active")
        running.set(false)
    }

    fun stop() {
        running.set(false)
        cameraProvider?.unbindAll()
        cameraProvider = null
    }

    // One-shot pipeline verification: a user-picked image (SAF) feeds the exact
    // detector -> PoseFrame -> JNI path as the live camera. No asset is bundled.
    fun processBitmap(bitmap: Bitmap, label: String) {
        executor.execute {
            val started = SystemClock.elapsedRealtimeNanos()
            inFlight.incrementAndGet()
            val image = InputImage.fromBitmap(bitmap, 0)
            detector.process(image)
                .addOnSuccessListener { pose ->
                    pushPoseResult(
                        pose, SystemClock.elapsedRealtimeNanos() - started,
                        bitmap.width, bitmap.height, rotation = 0, mirrored = false
                    )
                }
                .addOnFailureListener { error ->
                    Log.w(TAG, "static-image inference failed ($label): ${error.message}")
                }
                .addOnCompleteListener {
                    inFlight.decrementAndGet()
                    val elapsedMs = (SystemClock.elapsedRealtimeNanos() - started) / 1_000_000f
                    Log.i(TAG, "static-image done ($label): %.2f ms".format(elapsedMs))
                    bitmap.recycle()
                }
        }
    }

    @OptIn(ExperimentalGetImage::class)
    private fun analyze(proxy: ImageProxy) {
        val started = SystemClock.elapsedRealtimeNanos()
        val mediaImage = proxy.image
        if (mediaImage == null || !running.get()) {
            proxy.close()
            return
        }
        val image = InputImage.fromMediaImage(mediaImage, proxy.imageInfo.rotationDegrees)
        inFlight.incrementAndGet()
        detector.process(image)
            .addOnSuccessListener { pose ->
                pushPoseResult(
                    pose, SystemClock.elapsedRealtimeNanos() - started,
                    proxy.width, proxy.height,
                    proxy.imageInfo.rotationDegrees, mirrored = true // front camera mirror
                )
            }
            .addOnFailureListener { error ->
                Log.w(TAG, "pose inference failed: ${error.message}")
            }
            .addOnCompleteListener {
                inFlight.decrementAndGet()
                proxy.close() // always close, including failure paths
            }
    }

    private fun pushPoseResult(pose: Pose, elapsedNs: Long, width: Int, height: Int,
                               rotation: Int, mirrored: Boolean) {
        val packed = PoseFrameCodec.packAndPush(
            pose, SystemClock.elapsedRealtimeNanos(), width, height, rotation, mirrored
        )
        if (FRAME_LOG.incrementAndGet() % 30 == 1) {
            val timestampNs = SystemClock.elapsedRealtimeNanos()
            val poseFps = if (lastPoseNs > 0 && timestampNs > lastPoseNs) {
                1e9f / (timestampNs - lastPoseNs)
            } else 0f
            lastPoseNs = timestampNs
            Log.i(TAG,
                "pose stats: inference_ms=%.2f confidence=%.2f landmarks=%d in_flight=%d pose_fps=%.1f"
                    .format(elapsedNs / 1_000_000f, packed.confidence, packed.landmarkCount,
                        inFlight.get(), poseFps))
        }
    }

    companion object {
        private const val TAG = "PAE_POSE"
        private const val LANDMARK_COUNT = 33
        private val FRAME_LOG = AtomicInteger(0)
    }
}
