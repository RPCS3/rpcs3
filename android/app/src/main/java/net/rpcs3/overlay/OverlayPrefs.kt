package net.rpcs3.overlay

import android.content.Context

object OverlayPrefs {
    const val PREFS_NAME = "PadOverlayPrefs"
    const val OPACITY_KEY = "overlay_opacity"
    const val DEFAULT_OPACITY = 88

    fun getOpacity(context: Context): Int =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getInt(OPACITY_KEY, DEFAULT_OPACITY)

    fun setOpacity(context: Context, percent: Int) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putInt(OPACITY_KEY, percent.coerceIn(0, 100))
            .apply()
    }
}
