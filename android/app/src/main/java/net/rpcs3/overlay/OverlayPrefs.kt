package net.rpcs3.overlay

import android.content.Context
import android.content.SharedPreferences

object OverlayPrefs {
    const val PREFS_NAME = "PadOverlayPrefs"
    const val OPACITY_KEY = "overlay_opacity"
    const val ENABLED_KEY = "overlay_enabled"
    const val DEFAULT_OPACITY = 88
    const val MIN_SCALE = 25

    fun of(context: Context): SharedPreferences =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    fun getOpacity(context: Context): Int = of(context).getInt(OPACITY_KEY, DEFAULT_OPACITY)

    fun setOpacity(context: Context, percent: Int) {
        of(context).edit().putInt(OPACITY_KEY, percent.coerceIn(0, 100)).apply()
    }

    fun isEnabled(context: Context): Boolean = of(context).getBoolean(ENABLED_KEY, true)

    fun setEnabled(context: Context, enabled: Boolean) {
        of(context).edit().putBoolean(ENABLED_KEY, enabled).apply()
    }
}
