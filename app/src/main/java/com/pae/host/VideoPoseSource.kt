package com.pae.host

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.ImageFormat
import android.graphics.PixelFormat
import android.graphics.YuvImage
import android.media.Image
import android.media.ImageReader
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.net.Uri
import android.os.ParcelFileDescriptor
import android.renderscript.Allocation
import android.renderscript.Element
import android.renderscript.RenderScript
import android.renderscript.ScriptIntrinsicYuvToRGB
import android.renderscript.Type
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.google.android.gms.tasks.Tasks
import com.google.mlkit.vision.common.InputImage
import com.google.mlkit.vision.pose.PoseDetection
import com.google.mlkit.vision.pose.PoseDetectorOptionsBase
import com.google.mlkit.vision.pose.accurate.AccuratePoseDetectorOptions
import java.io.ByteArrayOutputStream
import java.io.File
import java.nio.ByteBuffer
import java.util.concurrent.ArrayBlockingQueue
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

// M8: decodes a user-selected local video, runs the same ML Kit pipeline as the
// camera source, and pushes PoseFrames whose timestamps are the video PTS.
// The decode thread paces against the video clock (wall time * speed anchored
// at PTS), so slow inference skips stale frames instead of building a queue.
class VideoPoseSource(private val activity: AppCompatActivity) {

    enum class State { IDLE, PLAYING, PAUSED, ENDED, STOPPED, ERROR }

    private class Command(val action: Int, val value: Long) {
        companion object {
            const val PLAY = 0
            const val PAUSE = 1
            const val SEEK_TO = 2
            const val SPEED = 3 // stored as permille in value
            const val RESTART = 4
            const val STOP = 5
        }
    }

    private val detector = PoseDetection.getClient(
        AccuratePoseDetectorOptions.Builder()
            .setDetectorMode(PoseDetectorOptionsBase.SINGLE_IMAGE_MODE)
            .build()
    )

    private val commands = ArrayBlockingQueue<Command>(16)
    private val running = AtomicBoolean(false)
    @Volatile private var state = State.IDLE
    @Volatile private var speedPermille = 1000L
    private var thread: Thread? = null
    private var dataSource: Source? = null
    private var durationUs = 0L

    // Video clock anchor: videoNowUs = anchorPtsUs + (wall - anchorWallUs) * speed
    @Volatile private var anchorPtsUs = 0L
    @Volatile private var anchorWallUs = 0L
    @Volatile private var eosInputQueued = false

    private sealed class Source
    private class UriSource(val uri: Uri) : Source()
    private class FileSource(val file: File) : Source()

    // ---- Public API (UI thread) ----

    fun open(uri: Uri) = start(UriSource(uri))
    fun open(file: File) = start(FileSource(file))

    fun togglePlayPause() {
        post(if (state == State.PLAYING) Command(Command.PAUSE, 0) else Command(Command.PLAY, 0))
    }

    fun seekBy(deltaUs: Long) {
        val duration = durationUs.coerceAtLeast(0L)
        val now = videoNowUs().coerceIn(0L, duration)
        post(Command(Command.SEEK_TO, (now + deltaUs).coerceIn(0L, duration)))
    }

    fun cycleSpeed(): Float {
        val next = when (speedPermille) {
            500L -> 2000L
            2000L -> 1000L
            else -> 500L
        }
        post(Command(Command.SPEED, next))
        return next / 1000f
    }

    fun replay() = post(Command(Command.RESTART, 0))
    fun stop() = post(Command(Command.STOP, 0))
    fun active(): Boolean = running.get() && state != State.STOPPED && state != State.IDLE
    fun isPlaying(): Boolean = state == State.PLAYING
    fun currentState(): State = state

    // ---- Implementation ----

    private fun start(source: Source) {
        shutdown()
        dataSource = source
        state = State.IDLE
        running.set(true)
        commands.clear()
        thread = Thread({ decodeLoop(source) }, "pae-video-decode").apply {
            start()
        }
        Log.i(TAG, "video source started: $source")
    }

    /** Stop the current session and wait for the decode thread to exit. */
    fun shutdown() {
        if (thread?.isAlive == true) {
            commands.clear()
            commands.offer(Command(Command.STOP, 0))
            thread?.join(3000)
        }
        thread = null
        running.set(false)
    }

    private fun post(command: Command) {
        if (!commands.offer(command)) {
            Log.w(TAG, "command queue full; dropping $command")
        }
    }

    private fun videoNowUs(): Long {
        if (state != State.PLAYING) return anchorPtsUs
        val wall = System.nanoTime() / 1000L
        return anchorPtsUs + ((wall - anchorWallUs) * speedPermille / 1000L)
    }

    private fun anchor(ptsUs: Long) {
        anchorPtsUs = ptsUs
        anchorWallUs = System.nanoTime() / 1000L
    }

    private fun decodeLoop(source: Source) {
        var extractor: MediaExtractor? = null
        var codec: MediaCodec? = null
        var reader: ImageReader? = null
        var pfd: ParcelFileDescriptor? = null
        try {
            extractor = MediaExtractor()
            when (source) {
                is UriSource -> {
                    pfd = activity.contentResolver.openFileDescriptor(source.uri, "r")
                        ?: throw IllegalStateException("cannot open media URI")
                    extractor.setDataSource(pfd.fileDescriptor)
                }
                is FileSource -> extractor.setDataSource(source.file.absolutePath)
            }
            var trackIndex = -1
            var format: MediaFormat? = null
            for (i in 0 until extractor.trackCount) {
                val trackFormat = extractor.getTrackFormat(i)
                val mime = trackFormat.getString(MediaFormat.KEY_MIME) ?: continue
                if (mime.startsWith("video/")) {
                    extractor.selectTrack(i)
                    trackIndex = i
                    format = trackFormat
                    break
                }
            }
            if (trackIndex < 0 || format == null) {
                throw IllegalStateException("no video track found")
            }
            val mime = format.getString(MediaFormat.KEY_MIME)!!
            val width = format.getInteger(MediaFormat.KEY_WIDTH)
            val height = format.getInteger(MediaFormat.KEY_HEIGHT)
            val rotation = if (format.containsKey(MediaFormat.KEY_ROTATION))
                format.getInteger(MediaFormat.KEY_ROTATION) else 0
            durationUs = if (format.containsKey(MediaFormat.KEY_DURATION))
                format.getLong(MediaFormat.KEY_DURATION) else 0L
            Log.i(TAG, "video track: $mime ${width}x${height} rot=$rotation " +
                "duration=${durationUs / 1000}s")

            reader = ImageReader.newInstance(width, height, ImageFormat.YUV_420_888, 3)
            codec = MediaCodec.createDecoderByType(mime)
            codec.configure(format, reader.surface, null, 0)
            codec.start()
            state = State.PLAYING
            anchor(0)

            val info = MediaCodec.BufferInfo()
            var endOfStream = false
            eosInputQueued = false
            var decoded = 0L
            var skipped = 0L
            var inferred = 0L
            while (running.get()) {
                val command = commands.poll()
                if (command != null) {
                    when (applyCommand(command, codec, extractor)) {
                        CommandResult.STOP -> break
                        CommandResult.RESTARTED -> endOfStream = false
                        else -> {}
                    }
                }
                if (state != State.PLAYING) {
                    Thread.sleep(30)
                    continue
                }
                if (endOfStream) {
                    state = State.ENDED
                    Log.i(TAG, "video playback reached end of stream")
                    continue
                }

                // Feed one input buffer.
                val inIndex = codec.dequeueInputBuffer(10_000)
                if (inIndex >= 0) {
                    val buffer = codec.getInputBuffer(inIndex)!!
                    val size = extractor.readSampleData(buffer, 0)
                    if (size < 0) {
                        codec.queueInputBuffer(inIndex, 0, 0, 0,
                            MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                        eosInputQueued = true
                    } else {
                        codec.queueInputBuffer(inIndex, 0, size, extractor.sampleTime, 0)
                        extractor.advance()
                    }
                }

                // Drain one output buffer and render to the image reader.
                val outIndex = codec.dequeueOutputBuffer(info, 10_000)
                if (outIndex < 0) continue
                val ptsUs = info.presentationTimeUs
                if (info.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) {
                    codec.releaseOutputBuffer(outIndex, false)
                    // After flush() a decoder may still hand back the stale EOS
                    // buffer from before the seek; only honour EOS when this
                    // session actually queued an EOS input.
                    if (eosInputQueued) endOfStream = true
                    continue
                }
                codec.releaseOutputBuffer(outIndex, true) // render to reader.surface
                decoded++

                // Pace against the video clock: frames far in the future wait,
                // frames stale beyond SKIP_THRESHOLD skip inference (timeline first).
                val now = videoNowUs()
                if (ptsUs > now) {
                    val waitMs = ((ptsUs - now) / speedPermille.coerceAtLeast(1L))
                        .coerceIn(1L, 100L)
                    Thread.sleep(waitMs)
                }
                val staleUs = videoNowUs() - ptsUs
                if (staleUs > SKIP_THRESHOLD_US && state == State.PLAYING) {
                    skipped++
                    if (skipped % 30 == 1L) {
                        Log.i(TAG, "video stats: decoded=$decoded skipped=$skipped " +
                            "inferred=$inferred pts=%.2fs".format(ptsUs / 1_000_000f))
                    }
                    continue
                }

                val image = acquireImage(reader) ?: continue
                val bitmap = try {
                    imageToBitmap(image)
                } finally {
                    image.close() // every path closes the frame buffer
                }
                if (bitmap == null) continue
                val startedAt = System.nanoTime()
                val input = InputImage.fromBitmap(bitmap, rotation)
                val pose = Tasks.await(detector.process(input), 5, TimeUnit.SECONDS)
                val packed = PoseFrameCodec.packAndPush(
                    pose, ptsUs * 1000L, bitmap.width, bitmap.height,
                    rotation, mirrored = false
                )
                inferred++
                if (bitmap !== rsBitmap) bitmap.recycle() // rsBitmap is reused
                if (inferred % 30 == 1L) {
                    Log.i(TAG, "video stats: decoded=$decoded skipped=$skipped " +
                        "inferred=$inferred pts=%.2fs inference_ms=%.1f confidence=%.2f"
                            .format(ptsUs / 1_000_000f,
                                (System.nanoTime() - startedAt) / 1e6f, packed.confidence))
                }
            }
        } catch (error: Exception) {
            if (running.get()) {
                state = State.ERROR
                Log.e(TAG, "video decode failed: ${error.message}", error)
                toast("video error: ${error.message ?: error.javaClass.simpleName}")
            }
        } finally {
            state = State.STOPPED
            try { codec?.stop() } catch (_: Exception) {}
            try { codec?.release() } catch (_: Exception) {}
            try { reader?.close() } catch (_: Exception) {}
            try { extractor?.release() } catch (_: Exception) {}
            try { pfd?.close() } catch (_: Exception) {}
            try { rsContext?.destroy() } catch (_: Exception) {}
            rsContext = null; rsScript = null; rsInput = null; rsOutput = null; rsBitmap = null
            Log.i(TAG, "video decode loop exited")
        }
    }

    private enum class CommandResult { NONE, STOP, RESTARTED }

    /** Applies a playback command on the decode thread. */
    private fun applyCommand(command: Command, codec: MediaCodec,
                             extractor: MediaExtractor): CommandResult {
        when (command.action) {
            Command.STOP -> {
                running.set(false)
                return CommandResult.STOP
            }
            Command.PLAY -> {
                if (state == State.ENDED || state == State.PAUSED || state == State.IDLE) {
                    anchor(anchorPtsUs)
                    state = State.PLAYING
                    Log.i(TAG, "playback resumed at %.2fs".format(anchorPtsUs / 1_000_000f))
                }
            }
            Command.PAUSE -> {
                if (state == State.PLAYING) {
                    anchorPtsUs = videoNowUs()
                    state = State.PAUSED
                    Log.i(TAG, "playback paused at %.2fs".format(anchorPtsUs / 1_000_000f))
                }
            }
            Command.SEEK_TO -> {
                performSeek(codec, extractor, command.value, "seek")
                return CommandResult.RESTARTED
            }
            Command.RESTART -> {
                performSeek(codec, extractor, 0L, "replay")
                return CommandResult.RESTARTED
            }
            Command.SPEED -> {
                anchorPtsUs = videoNowUs()
                speedPermille = command.value
                anchor(anchorPtsUs)
                NativeBridge.resetMotion() // filter state must not bridge a speed change
                Log.i(TAG, "speed changed to ${speedPermille / 1000.0f}x at %.2fs"
                    .format(anchorPtsUs / 1_000_000f))
            }
        }
        return CommandResult.NONE
    }

    private fun performSeek(codec: MediaCodec, extractor: MediaExtractor, targetUs: Long, label: String) {
        codec.flush()
        eosInputQueued = false // fresh decode session; stale EOS buffers are discarded
        extractor.seekTo(targetUs, MediaExtractor.SEEK_TO_CLOSEST_SYNC)
        anchor(targetUs)
        state = State.PLAYING
        NativeBridge.resetMotion() // stale pre-seek pose must not survive the jump
        Log.i(TAG, "$label to %.2fs (motion filters reset)".format(targetUs / 1_000_000f))
    }

    private fun acquireImage(reader: ImageReader): Image? {
        val deadline = System.nanoTime() + 300_000_000L
        while (running.get() && state == State.PLAYING) {
            val image = reader.acquireLatestImage()
            if (image != null) return image
            if (System.nanoTime() > deadline) return null
            Thread.sleep(4)
        }
        return null
    }

    // Video decoders output YCbCr_420_888 on essentially every device
    // (measured on the target box: producer 0x15 vs RGBA reader 0x1 crash).
    // Convert YUV -> RGB with RenderScript (fast path); fall back to a
    // YuvImage JPEG round trip if RenderScript is unavailable.
    private var rsContext: RenderScript? = null
    private var rsScript: ScriptIntrinsicYuvToRGB? = null
    private var rsInput: Allocation? = null
    private var rsOutput: Allocation? = null
    private var rsBitmap: Bitmap? = null
    private var nv21Buffer: ByteArray? = null

    /** Image (YUV_420_888 or RGBA_8888) -> ARGB_8888 bitmap for inference. */
    private fun imageToBitmap(image: Image): Bitmap? = when (image.format) {
        ImageFormat.YUV_420_888 -> yuvImageToBitmap(image)
        PixelFormat.RGBA_8888 -> rgbaImageToBitmap(image)
        else -> null
    }

    private fun rgbaImageToBitmap(image: Image): Bitmap? {
        val plane = image.planes[0]
        val width = image.width
        val height = image.height
        val rowStride = plane.rowStride
        val rowBytes = width * 4
        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        val source = plane.buffer.duplicate()
        source.rewind()
        if (rowStride == rowBytes) {
            bitmap.copyPixelsFromBuffer(source)
        } else {
            val contiguous = ByteBuffer.allocate(rowBytes * height)
            val row = ByteArray(rowBytes)
            for (y in 0 until height) {
                source.position(y * rowStride)
                source.get(row, 0, rowBytes)
                contiguous.put(row)
            }
            contiguous.rewind()
            bitmap.copyPixelsFromBuffer(contiguous)
        }
        return bitmap
    }

    private fun yuvImageToBitmap(image: Image): Bitmap? {
        val nv21 = assembleNv21(image) ?: return null
        val width = image.width
        val height = image.height
        if (ensureRsConverter(width, height, nv21.size)) {
            val input = rsInput!!
            input.copyFrom(nv21)
            rsScript!!.setInput(input)
            rsScript!!.forEach(rsOutput)
            val bitmap = rsBitmap!!
            rsOutput!!.copyTo(bitmap)
            return bitmap
        }
        return jpegFallback(nv21, width, height)
    }

    /** General YUV_420_888 (any strides/pixel strides) -> NV21 byte array. */
    private fun assembleNv21(image: Image): ByteArray? {
        if (image.format != ImageFormat.YUV_420_888) return null
        val width = image.width
        val height = image.height
        val ySize = width * height
        val nv21 = nv21Buffer?.takeIf { it.size == ySize + ySize / 2 }
            ?: ByteArray(ySize + ySize / 2).also { nv21Buffer = it }
        val yPlane = image.planes[0]
        val yBuf = yPlane.buffer.duplicate()
        yBuf.rewind()
        if (yPlane.pixelStride == 1 && yPlane.rowStride == width) {
            yBuf.get(nv21, 0, ySize)
        } else {
            var pos = 0
            for (row in 0 until height) {
                val base = row * yPlane.rowStride
                for (col in 0 until width) {
                    nv21[pos++] = yBuf.get(base + col)
                }
            }
        }
        val uPlane = image.planes[1]
        val vPlane = image.planes[2]
        val uBuf = uPlane.buffer.duplicate()
        val vBuf = vPlane.buffer.duplicate()
        uBuf.rewind()
        vBuf.rewind()
        var pos = ySize
        for (row in 0 until height / 2) {
            for (col in 0 until width / 2) {
                nv21[pos++] = vBuf.get(row * vPlane.rowStride + col * vPlane.pixelStride)
                nv21[pos++] = uBuf.get(row * uPlane.rowStride + col * uPlane.pixelStride)
            }
        }
        return nv21
    }

    private fun ensureRsConverter(width: Int, height: Int, yuvSize: Int): Boolean {
        if (rsContext != null) return rsScript != null
        return try {
            val rs = RenderScript.create(activity)
            val inputType = Type.createX(rs, Element.U8(rs), yuvSize)
            val outputType = Type.createXY(rs, Element.RGBA_8888(rs), width, height)
            rsContext = rs
            rsScript = ScriptIntrinsicYuvToRGB.create(rs, Element.U8_4(rs))
            rsInput = Allocation.createTyped(rs, inputType)
            rsOutput = Allocation.createTyped(rs, outputType)
            rsBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
            Log.i(TAG, "YUV->RGB conversion path: RenderScript ${width}x${height}")
            true
        } catch (error: Exception) {
            Log.w(TAG, "RenderScript unavailable (${error.message}); using JPEG fallback")
            rsContext = null
            rsScript = null
            rsInput = null
            rsOutput = null
            rsBitmap = null
            false
        }
    }

    private fun jpegFallback(nv21: ByteArray, width: Int, height: Int): Bitmap? = try {
        val yuv = YuvImage(nv21, ImageFormat.NV21, width, height, null)
        val out = ByteArrayOutputStream()
        yuv.compressToJpeg(android.graphics.Rect(0, 0, width, height), 92, out)
        val bytes = out.toByteArray()
        BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
    } catch (error: Exception) {
        Log.e(TAG, "JPEG fallback conversion failed: ${error.message}")
        null
    }

    private fun toast(message: String) {
        activity.runOnUiThread {
            Toast.makeText(activity, message, Toast.LENGTH_SHORT).show()
        }
    }

    companion object {
        private const val TAG = "PAE_VIDEO"
        private const val SKIP_THRESHOLD_US = 500_000L // infer frames < 0.5s stale
    }
}
