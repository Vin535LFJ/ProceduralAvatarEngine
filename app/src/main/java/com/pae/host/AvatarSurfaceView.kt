package com.pae.host

import android.content.Context
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView

class AvatarSurfaceView(context: Context) : SurfaceView(context), SurfaceHolder.Callback {

    init {
        holder.addCallback(this)
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

    companion object {
        private const val TAG = "PAE_SURFACE"
    }
}
