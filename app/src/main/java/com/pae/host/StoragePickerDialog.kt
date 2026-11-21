package com.pae.host

import android.app.Activity
import android.app.Dialog
import android.os.Environment
import android.util.Log
import android.widget.ArrayAdapter
import android.widget.ListView
import android.widget.Toast
import java.io.File

// In-app file browser for devices without a system document picker (common on
// leanback TV boxes: ACTION_OPEN_DOCUMENT resolves to no activity). The user
// still selects their own media from local storage; nothing is bundled with
// the app, so the zero-external-asset constraint is preserved.
class StoragePickerDialog(
    private val activity: Activity,
    private val title: String,
    private val extensions: Set<String>,
    private val onFilePicked: (File) -> Unit
) {
    private var current: File = Environment.getExternalStorageDirectory()
    private var dialog: Dialog? = null

    fun show() {
        val listView = ListView(activity)
        val adapter = ArrayAdapter<String>(activity, android.R.layout.simple_list_item_1)
        listView.adapter = adapter

        listView.setOnItemClickListener { _, _, position, _ ->
            val entry = listView.getItemAtPosition(position).toString()
            if (entry == UP) {
                current = current.parentFile ?: current
                refresh(adapter)
            } else {
                val target = File(current, entry)
                when {
                    target.isDirectory -> {
                        current = target
                        refresh(adapter)
                    }
                    extensions.isNotEmpty() && entry.substringAfterLast('.', "").lowercase() !in extensions -> {
                        Toast.makeText(activity, "unsupported file type", Toast.LENGTH_SHORT).show()
                    }
                    else -> {
                        dismiss()
                        onFilePicked(target)
                    }
                }
            }
        }

        dialog = Dialog(activity).apply {
            setTitle("$title — ${current.absolutePath}")
            setContentView(listView)
            setOnDismissListener { dialog = null }
            window?.setLayout(
                ViewGroupLayoutParams.MATCH_PARENT,
                ViewGroupLayoutParams.WRAP_CONTENT
            )
        }
        refresh(adapter)
        dialog?.show()
    }

    fun dismiss() {
        dialog?.dismiss()
        dialog = null
    }

    private fun refresh(adapter: ArrayAdapter<String>) {
        adapter.clear()
        dialog?.setTitle("$title — ${current.absolutePath}")
        val entries = current.listFiles()
        if (entries == null) {
            adapter.add("(directory not readable)")
            return
        }
        if (current.parentFile != null) adapter.add(UP)
        entries.sortedWith(compareByDescending<File> { it.isDirectory }.thenBy { it.name.lowercase() })
            .forEach { adapter.add(it.name) }
        if (adapter.isEmpty) adapter.add("(empty directory)")
    }

    private object ViewGroupLayoutParams {
        const val MATCH_PARENT = android.view.ViewGroup.LayoutParams.MATCH_PARENT
        const val WRAP_CONTENT = android.view.ViewGroup.LayoutParams.WRAP_CONTENT
    }

    companion object {
        private const val TAG = "PAE_PICKER"
        private const val UP = ".."

        val IMAGE_EXTS = setOf("png", "jpg", "jpeg", "webp", "bmp", "gif")
        val VIDEO_EXTS = setOf("mp4", "mkv", "webm", "avi", "mov", "3gp", "ts", "m4v")
    }
}
