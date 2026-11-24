package com.pae.host

import android.content.Context
import android.util.Log
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import kotlin.math.abs
import kotlin.math.hypot

/**
 * Surface view with direct manipulation of the world-space camera.
 *
 * Gestures (single pointer):
 *  - drag            -> orbit (yaw / pitch)
 *  - two-finger pinch -> zoom
 *  - two-finger drag  -> pan
 *
 * The gesture stream is forwarded to the native renderer as camera commands.
 * This does not touch the frozen JNI signatures of the existing entry points;
 * it only adds one new call, `nativeCameraCommand`.
 */
class AvatarSurfaceView(context: Context) : SurfaceView(context), SurfaceHolder.Callback {

    private var lastX = 0f
    private var lastY = 0f
    private var lastSpan = 0f
    private var lastMidX = 0f
    private var lastMidY = 0f
    private var activePointers = 0

    init {
        holder.addCallback(this)
        isFocusable = true
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.i(TAG, "surfaceCreated")
        NativeBridge.surfaceCreated(holder.surface, width, height)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.i(TAG, "surfaceChanged ${width}x${height}")
        NativeBridge.surfaceChanged(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.i(TAG, "surfaceDestroyed")
        NativeBridge.surfaceDestroyed()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                activePointers = event.pointerCount
                lastX = event.x
                lastY = event.y
                if (event.pointerCount >= 2) {
                    lastSpan = spanOf(event)
                    lastMidX = midXOf(event)
                    lastMidY = midYOf(event)
                }
                return true
            }

            MotionEvent.ACTION_MOVE -> {
                if (event.pointerCount >= 2) {
                    val span = spanOf(event)
                    val midX = midXOf(event)
                    val midY = midYOf(event)
                    if (lastSpan > 1f && span > 1f) {
                        // Pinch: a larger span means the fingers moved apart,
                        // so the camera should move closer (zoom in).
                        NativeBridge.cameraCommand(CAM_ZOOM, lastSpan / span, 0f, 0f)
                    }
                    NativeBridge.cameraCommand(CAM_PAN, midX - lastMidX, midY - lastMidY, 0f)
                    lastSpan = span
                    lastMidX = midX
                    lastMidY = midY
                } else {
                    val dx = event.x - lastX
                    val dy = event.y - lastY
                    // "Grab the object" feel: dragging right moves the camera
                    // left, dragging down raises the camera.
                    NativeBridge.cameraCommand(CAM_ORBIT, -dx * 0.32f, dy * 0.26f, 0f)
                    lastX = event.x
                    lastY = event.y
                }
                return true
            }

            MotionEvent.ACTION_POINTER_UP -> {
                // Re-anchor to the surviving pointer so the view does not jump.
                val remaining = event.actionIndex
                lastX = if (remaining == 0) event.getX(1) else event.getX(0)
                lastY = if (remaining == 0) event.getY(1) else event.getY(0)
                activePointers = event.pointerCount - 1
                return true
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                activePointers = 0
                lastSpan = 0f
                performClick()
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }

    private fun spanOf(event: MotionEvent): Float {
        if (event.pointerCount < 2) return 0f
        return hypot(event.getX(0) - event.getX(1), event.getY(0) - event.getY(1))
    }

    private fun midXOf(event: MotionEvent): Float {
        if (event.pointerCount < 2) return event.x
        return (event.getX(0) + event.getX(1)) * 0.5f
    }

    private fun midYOf(event: MotionEvent): Float {
        if (event.pointerCount < 2) return event.y
        return (event.getY(0) + event.getY(1)) * 0.5f
    }

    companion object {
        private const val TAG = "PAE_SURFACE"
        const val CAM_ORBIT = 0
        const val CAM_ZOOM = 1
        const val CAM_PAN = 2
        const val CAM_RESET = 3
    }
}
