package net.rpcs3

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

enum class Permission(val id: Int, val key: String) {
    @SuppressLint("InlinedApi")
    PostNotifications(100, Manifest.permission.POST_NOTIFICATIONS);

    fun checkPermission(context: Context) =
        ContextCompat.checkSelfPermission(context, key) == PackageManager.PERMISSION_GRANTED

    fun requestPermission(activity: Activity) {
        if (!checkPermission(activity)) {
            ActivityCompat.requestPermissions(activity, arrayOf(key), id)
        }
    }
}
