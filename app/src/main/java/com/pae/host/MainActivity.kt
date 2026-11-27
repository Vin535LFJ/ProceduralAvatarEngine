package com.pae.host

import android.Manifest
import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.HorizontalScrollView
import android.widget.LinearLayout
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class MainActivity : AppCompatActivity() {
    private lateinit var poseAdapter: PoseProviderAdapter
    private val palette = arrayOf(
        floatArrayOf(0.98f, 0.79f, 0.66f),  // skin
        floatArrayOf(0.26f, 0.15f, 0.09f),  // hair
        floatArrayOf(0.32f, 0.58f, 0.88f),  // cloth (shirt)
        floatArrayOf(1.00f, 1.00f, 1.00f),  // eye
        floatArrayOf(0.20f, 0.24f, 0.32f)   // pants
    )
    private val skinOptions = listOf(
        floatArrayOf(0.98f, 0.79f, 0.66f), floatArrayOf(0.80f, 0.55f, 0.38f),
        floatArrayOf(1.00f, 0.87f, 0.72f)
    )
    private val hairOptions = listOf(
        floatArrayOf(0.26f, 0.15f, 0.09f), floatArrayOf(0.85f, 0.65f, 0.15f),
        floatArrayOf(0.12f, 0.22f, 0.48f)
    )
    private val clothOptions = listOf(
        floatArrayOf(0.32f, 0.58f, 0.88f), floatArrayOf(0.82f, 0.22f, 0.26f),
        floatArrayOf(0.22f, 0.68f, 0.38f)
    )
    private val pantsOptions = listOf(
        floatArrayOf(0.20f, 0.24f, 0.32f), floatArrayOf(0.30f, 0.28f, 0.26f),
        floatArrayOf(0.16f, 0.18f, 0.22f)
    )
    private val backgrounds = listOf(
        0 to floatArrayOf(0.08f, 0.10f, 0.18f, 0.08f, 0.10f, 0.18f),
        1 to floatArrayOf(0.10f, 0.12f, 0.25f, 0.65f, 0.80f, 0.90f),
        2 to floatArrayOf(0.28f, 0.33f, 0.45f, 0.12f, 0.16f, 0.26f)
    )
    private var skinIndex = 0
    private var hairIndex = 0
    private var clothIndex = 0
    private var backgroundIndex = 2
    private var poseMode = -1
    private var qualityLevel = 1
    private var pantsIndex = 0
    private var anatomyOn = false
    private var previewMode = 0
    private var viewPreset = 0
    private var debugView = 0

    // One-shot image feed for the M7 pipeline verification: the picked image goes
    // through the same detector -> PoseFrame -> JNI path as live camera frames.
    private val imagePicker =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
            if (uri != null) feedTestImage(uri)
            else Log.i("PAE_UI", "image picker cancelled")
        }

    // M8: local video input. SAF on devices with a document picker, in-app
    // browser fallback on TV boxes (ADR-007).
    private val videoSource by lazy { VideoPoseSource(this) }
    private val videoPicker =
        registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
            if (uri != null) videoSource.open(uri)
            else Log.i("PAE_UI", "video picker cancelled")
        }
    private lateinit var playButton: Button
    private lateinit var speedButton: Button
    private lateinit var loopButton: Button
    private lateinit var videoControls: LinearLayout

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        NativeBridge.create()
        // VR-1B WP2.1: the static human template ships inside the APK; no
        // external file is required. A pushed dev file still overrides it.
        val templateAssetOk = NativeBridge.loadTemplateAsset(
            assets, "human_template/pae-human-base-mh-a8bc2d54.paht"
        )
        Log.i("PAE_UI", "template asset load -> $templateAssetOk")

        val root = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        val surface = AvatarSurfaceView(this)
        surface.layoutParams = LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f
        )
        root.addView(surface)

        val buttons = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setBackgroundColor(Color.rgb(24, 26, 34))
        }
        val row = HorizontalScrollView(this).apply { addView(buttons) }
        root.addView(row, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
        ))

        buttons.addView(actionButton("Skin") {
            skinIndex = (skinIndex + 1) % skinOptions.size
            palette[0] = skinOptions[skinIndex]
            pushAppearance()
        })
        buttons.addView(actionButton("Hair") {
            hairIndex = (hairIndex + 1) % hairOptions.size
            palette[1] = hairOptions[hairIndex]
            pushAppearance()
        })
        buttons.addView(actionButton("Cloth") {
            clothIndex = (clothIndex + 1) % clothOptions.size
            palette[2] = clothOptions[clothIndex]
            pushAppearance()
        })
        buttons.addView(actionButton("Pants") {
            pantsIndex = (pantsIndex + 1) % pantsOptions.size
            palette[4] = pantsOptions[pantsIndex]
            pushAppearance()
        })
        buttons.addView(actionButton("BG") {
            backgroundIndex = (backgroundIndex + 1) % backgrounds.size
            pushBackground()
        })
        buttons.addView(actionButton("Pose") {
            poseMode = if (poseMode >= 5) -1 else poseMode + 1
            NativeBridge.setPoseMode(poseMode)
        })
        buttons.addView(actionButton("Camera") {
            if (hasCameraPermission()) poseAdapter.start() else requestCameraPermission()
        })
        buttons.addView(actionButton("TestImg") {
            launchTestImage()
        })
        buttons.addView(actionButton("Video") {
            launchVideoPicker()
        })
        buttons.addView(actionButton("Quality") {
            qualityLevel = (qualityLevel + 1) % 3
            NativeBridge.setQuality(qualityLevel)
            Log.i("PAE_UI", "quality level -> $qualityLevel (0=Low,1=Medium,2=High)")
        })
        buttons.addView(actionButton("Anatomy") {
            anatomyOn = !anatomyOn
            previewMode = if (anatomyOn) 1 else 0
            NativeBridge.setPreviewMode(previewMode)
            Log.i("PAE_UI", "anatomy preview -> $anatomyOn (bind pose, neutral material)")
        })
        buttons.addView(actionButton("BodyMode") {
            previewMode = (previewMode + 1) % 5
            anatomyOn = previewMode == 1
            NativeBridge.setPreviewMode(previewMode)
            Log.i("PAE_UI", "body mode -> $previewMode (0 avatar, 1 anatomy, 2 patch, 3 canonical, 4 human template)")
        })
        buttons.addView(actionButton("View") {
            viewPreset = (viewPreset + 1) % 8
            NativeBridge.setCameraPreset(viewPreset)
            Log.i("PAE_UI", "camera preset -> $viewPreset (0F/1B/2L/3R/4-5 45deg/6 shoulder/7 face)")
        })
        buttons.addView(actionButton("ResetCam") {
            NativeBridge.resetCamera()
            Log.i("PAE_UI", "camera reset to evidence preset $viewPreset")
        })
        buttons.addView(actionButton("DbgView") {
            debugView = (debugView + 1) % 8
            NativeBridge.setDebugView(debugView)
            Log.i("PAE_UI", "debug view -> $debugView (0 shading, 1 wireframe, 2 normals, " +
                "3 gray clay, 4 material id, 5 skin weights, 6 outline study, 7 UV checker)")
        })
        buttons.addView(actionButton("Stats") {
            Log.i("PAE_UI", NativeBridge.stats())
        })

        videoControls = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setBackgroundColor(Color.rgb(34, 30, 24))
            visibility = ViewGroup.GONE
        }
        playButton = actionButton("Play") {
            if (videoSource.active()) {
                videoSource.togglePlayPause()
                playButton.text = if (videoSource.isPlaying()) "Pause" else "Play"
            }
        }
        videoControls.addView(playButton)
        videoControls.addView(actionButton("-10s") { if (videoSource.active()) videoSource.seekBy(-10_000_000L) })
        videoControls.addView(actionButton("+10s") { if (videoSource.active()) videoSource.seekBy(+10_000_000L) })
        speedButton = actionButton("1x") {
            if (videoSource.active()) {
                speedButton.text = "%.1fx".format(videoSource.cycleSpeed())
            }
        }
        videoControls.addView(speedButton)
        loopButton = actionButton("Loop Off") {
            if (videoSource.active()) {
                loopButton.text = if (videoSource.toggleLoop()) "Loop On" else "Loop Off"
            }
        }
        videoControls.addView(loopButton)
        videoControls.addView(actionButton("Replay") { if (videoSource.active()) videoSource.replay() })
        videoControls.addView(actionButton("Stop") {
            if (videoSource.active()) {
                videoSource.stop()
                videoControls.visibility = ViewGroup.GONE
            }
        })
        root.addView(videoControls, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
        ))

        setContentView(root)
        poseAdapter = PoseProviderAdapter(this)
        if (hasCameraPermission()) poseAdapter.start() else requestCameraPermission()

        // Debug autostart: adb shell am start ... --es debug_video /path/file.mp4
        // launches the video pipeline directly (deterministic test runs).
        intent?.getStringExtra("debug_video")?.let { debugPath ->
            Log.i("PAE_UI", "debug video autostart: $debugPath")
            videoSource.open(java.io.File(debugPath))
            videoControls.visibility = ViewGroup.VISIBLE
            playButton.text = "Pause"
        }

        // M12-A deterministic evidence driver: adb shell am start \
        //   -n com.pae.host/.MainActivity --ez debug_anatomy true \
        //   --ei debug_preset 3 --ei debug_view 1
        // Sets the anatomy preview state ABSOLUTELY (no button cycling), so
        // adb-driven screenshot matrices cannot desync the UI counters.
        // P1-A deterministic driver: --ei debug_generator 2 --ei debug_pose -1
        //   --ei debug_preset 0 --ei debug_view 3
        applyDebugExtras(intent)
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        applyDebugExtras(intent)
    }

    private fun applyDebugExtras(intent: Intent?) {
        intent?.let { it2 ->
            if (it2.getBooleanExtra("debug_anatomy", false)) {
                anatomyOn = true
                previewMode = 1
                NativeBridge.setAnatomyPreview(true)
            }
            val quality = it2.getIntExtra("debug_quality", Int.MIN_VALUE)
            if (quality in 0..2) {
                qualityLevel = quality
                NativeBridge.setQuality(quality)
            }
            val generator = it2.getIntExtra("debug_generator", -1)
            if (generator in 0..4) {
                previewMode = generator
                anatomyOn = generator == 1
                NativeBridge.setPreviewMode(generator)
            }
            val fixedPose = it2.getIntExtra("debug_pose", Int.MIN_VALUE)
            if (fixedPose != Int.MIN_VALUE) {
                poseMode = fixedPose
                NativeBridge.setPoseMode(fixedPose)
            }
            val dp = it2.getIntExtra("debug_preset", -1)
            if (dp in 0..7) {
                viewPreset = dp
                NativeBridge.setCameraPreset(dp)
            }
            val dv = it2.getIntExtra("debug_view", -1)
            if (dv in 0..7) {
                debugView = dv
                NativeBridge.setDebugView(dv)
            }
            // Deterministic camera driver for evidence captures:
            //   --ei debug_cam_op 0|1|2|3  --ef debug_cam_x .. --ef debug_cam_z
            // op 0 orbit (x = yaw deg, y = pitch deg), 1 zoom (x = multiplier),
            // 2 pan (x/y = screen pixels), 3 reset.
            val camOp = it2.getIntExtra("debug_cam_op", -1)
            if (camOp in 0..3) {
                NativeBridge.cameraCommand(
                    camOp,
                    it2.getFloatExtra("debug_cam_x", 0f),
                    it2.getFloatExtra("debug_cam_y", 0f),
                    it2.getFloatExtra("debug_cam_z", 0f)
                )
            }
            val outfit = it2.getIntExtra("debug_outfit", -1)
            if (outfit in 0..1) {
                palette[2] = if (outfit == 0) floatArrayOf(0.44f, 0.43f, 0.52f)
                             else floatArrayOf(0.52f, 0.44f, 0.40f)
                pushAppearance()
            }
            Log.i("PAE_UI", "debug extras: anatomy=$anatomyOn preset=$viewPreset view=$debugView outfit=$outfit")
        }
    }

    private fun showVideoStoragePicker() {
        StoragePickerDialog(this, "Pick video", StoragePickerDialog.VIDEO_EXTS) { file ->
            Log.i("PAE_UI", "video picked: $file")
            videoSource.open(file)
            videoControls.visibility = ViewGroup.VISIBLE
            playButton.text = "Pause"
            speedButton.text = "1x"
        }.show()
    }

    private fun hasCameraPermission(): Boolean =
        ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) ==
            PackageManager.PERMISSION_GRANTED

    private fun requestCameraPermission() {
        ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.CAMERA), 1001)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<out String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1001 && grantResults.isNotEmpty() &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED
        ) {
            poseAdapter.start()
        } else if (requestCode == 1001) {
            Log.w("PAE_UI", "camera permission denied; synthetic provider remains active")
        }
        if (requestCode == 1002 && grantResults.isNotEmpty() &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED
        ) {
            when (pendingPickerMode) {
                PickerMode.IMAGE -> showImageStoragePicker()
                PickerMode.VIDEO -> showVideoStoragePicker()
            }
        } else if (requestCode == 1002) {
            Log.w("PAE_UI", "storage permission denied; cannot browse local files")
        }
    }

    override fun onPause() {
        if (::poseAdapter.isInitialized) poseAdapter.stop()
        if (videoSource.isPlaying()) {
            videoSource.togglePlayPause()
            playButton.text = "Play"
        }
        super.onPause()
    }

    // Primary path is the system document picker; leanback boxes often ship
    // without DocumentsUI, so fall back to the in-app storage browser (ADR-007).
    private enum class PickerMode { IMAGE, VIDEO }

    private var pendingPickerMode = PickerMode.IMAGE

    private fun launchTestImage() {
        try {
            imagePicker.launch(arrayOf("image/*"))
        } catch (error: ActivityNotFoundException) {
            Log.w("PAE_UI", "no system document picker (${error.message}); using in-app browser")
            pendingPickerMode = PickerMode.IMAGE
            if (hasStoragePermission()) showImageStoragePicker() else requestStoragePermission()
        }
    }

    private fun launchVideoPicker() {
        try {
            videoPicker.launch(arrayOf("video/*"))
        } catch (error: ActivityNotFoundException) {
            Log.w("PAE_UI", "no system document picker (${error.message}); using in-app browser")
            pendingPickerMode = PickerMode.VIDEO
            if (hasStoragePermission()) showVideoStoragePicker() else requestStoragePermission()
        }
    }

    private fun showImageStoragePicker() {
        StoragePickerDialog(this, "Pick test image", StoragePickerDialog.IMAGE_EXTS) { file ->
            val bitmap = decodeSampled({ file.inputStream() }, maxSide = 1024)
            if (bitmap == null) {
                Log.e("PAE_UI", "test image decode failed: $file")
                Toast.makeText(this, "image decode failed", Toast.LENGTH_SHORT).show()
                return@StoragePickerDialog
            }
            Log.i("PAE_UI", "test image picked: ${bitmap.width}x${bitmap.height} ($file)")
            Toast.makeText(this, "running pose inference on picked image", Toast.LENGTH_SHORT).show()
            poseAdapter.processBitmap(bitmap, file.name)
        }.show()
    }

    private fun feedTestImage(uri: Uri) {
        val bitmap = decodeSampled({ contentResolver.openInputStream(uri) }, maxSide = 1024)
        if (bitmap == null) {
            Log.e("PAE_UI", "test image decode failed: $uri")
            Toast.makeText(this, "image decode failed", Toast.LENGTH_SHORT).show()
            return
        }
        Log.i("PAE_UI", "test image picked: ${bitmap.width}x${bitmap.height}")
        Toast.makeText(this, "running pose inference on picked image", Toast.LENGTH_SHORT).show()
        poseAdapter.processBitmap(bitmap, uri.lastPathSegment ?: "picked-image")
    }

    private fun decodeSampled(openStream: () -> java.io.InputStream?, maxSide: Int): Bitmap? = try {
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        openStream()?.use {
            BitmapFactory.decodeStream(it, null, bounds)
        }
        var sample = 1
        while (maxOf(bounds.outWidth, bounds.outHeight) / sample > maxSide) sample *= 2
        val options = BitmapFactory.Options().apply { inSampleSize = sample }
        openStream()?.use {
            BitmapFactory.decodeStream(it, null, options)
        }
    } catch (error: Exception) {
        Log.e("PAE_UI", "image decode error: ${error.message}")
        null
    }

    private fun hasStoragePermission(): Boolean =
        ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) ==
            PackageManager.PERMISSION_GRANTED

    private fun requestStoragePermission() {
        ActivityCompat.requestPermissions(
            this, arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE), 1002
        )
    }

    override fun onResume() {
        super.onResume()
        if (::poseAdapter.isInitialized && hasCameraPermission()) poseAdapter.start()
        if (videoSource.active()) {
            playButton.text = if (videoSource.isPlaying()) "Pause" else "Play"
        }
    }

    private fun actionButton(text: String, action: () -> Unit): Button =
        Button(this).apply {
            this.text = text
            setOnClickListener { action() }
        }

    private fun pushAppearance() {
        val packed = FloatArray(palette.size * 3)
        for (i in palette.indices) System.arraycopy(palette[i], 0, packed, i * 3, 3)
        NativeBridge.setAppearance(packed)
    }

    private fun pushBackground() {
        val (mode, colors) = backgrounds[backgroundIndex]
        NativeBridge.setBackground(mode, colors.copyOfRange(0, 3), colors.copyOfRange(3, 6))
    }

    override fun onDestroy() {
        Log.i("PAE_ACTIVITY", "activity onDestroy")
        videoSource.shutdown()
        if (::poseAdapter.isInitialized) poseAdapter.stop()
        NativeBridge.release()
        super.onDestroy()
    }
}
