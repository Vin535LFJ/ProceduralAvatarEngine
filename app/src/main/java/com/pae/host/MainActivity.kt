package com.pae.host

import android.Manifest
import android.app.Activity
import android.content.ActivityNotFoundException
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
