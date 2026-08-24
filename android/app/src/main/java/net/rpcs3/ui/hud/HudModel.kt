package net.rpcs3.ui.hud

import android.content.Context
import android.content.SharedPreferences

enum class HudElement(val key: String, val label: String, val enabledByDefault: Boolean) {
    Fps("fps", "Frame rate", true),
    Frametime("frametime", "Frame time", true),
    Renderer("renderer", "Renderer", false),
    Gpu("gpu", "GPU load", true),
    Cpu("cpu", "CPU load", true),
    Ram("ram", "Memory", true),
    Battery("battery", "Battery", false),
    Power("power", "Power draw", false),
    Temperature("temp", "Temperature", false)
}

enum class HudMode(val horizontal: Boolean, val backdrop: Boolean) {
    HorizontalPlain(true, false),
    HorizontalShaded(true, true),
    VerticalPlain(false, false),
    VerticalShaded(false, true)
}

enum class HudAnchor(val label: String, val bearingDegrees: Float) {
    TopLeft("Top left", -45f),
    TopCenter("Top centre", 0f),
    TopRight("Top right", 45f),
    LeftCenter("Left", -90f),
    RightCenter("Right", 90f),
    BottomLeft("Bottom left", -135f),
    BottomCenter("Bottom centre", 180f),
    BottomRight("Bottom right", 135f)
}

data class HudSample(
    val fps: Float = 0f,
    val outputFps: Float = 0f,
    val frametimeMs: Float = 0f,
    val renderer: String = "",
    val cpuPercent: Int = -1,
    val gpuPercent: Int = -1,
    val ramUsedMb: Int = -1,
    val ramTotalMb: Int = -1,
    val batteryPercent: Int = -1,
    val watts: Float = Float.NaN,
    val temperatureC: Int = -1
)

object HudPrefs {
    const val NAME = "hud"

    private const val KEY_ENABLED = "hud_enabled"
    private const val KEY_MODE = "hud_mode"
    private const val KEY_ANCHOR = "hud_anchor"
    private const val KEY_POS_X = "hud_pos_x"
    private const val KEY_POS_Y = "hud_pos_y"
    private const val KEY_HAS_POSITION = "hud_has_position"
    private const val KEY_SCALE = "hud_scale"
    private const val KEY_ELEMENT = "hud_element_"
    private const val KEY_FRAMETIME_NUMERIC = "hud_frametime_numeric"

    fun of(context: Context): SharedPreferences =
        context.getSharedPreferences(NAME, Context.MODE_PRIVATE)

    fun isEnabled(prefs: SharedPreferences) = prefs.getBoolean(KEY_ENABLED, false)

    fun setEnabled(prefs: SharedPreferences, value: Boolean) {
        prefs.edit().putBoolean(KEY_ENABLED, value).apply()
    }

    fun mode(prefs: SharedPreferences): HudMode {
        val ordinal = prefs.getInt(KEY_MODE, HudMode.HorizontalShaded.ordinal)
        return HudMode.entries.getOrElse(ordinal) { HudMode.HorizontalShaded }
    }

    fun setMode(prefs: SharedPreferences, mode: HudMode) {
        prefs.edit().putInt(KEY_MODE, mode.ordinal).apply()
    }

    fun anchor(prefs: SharedPreferences): HudAnchor {
        val ordinal = prefs.getInt(KEY_ANCHOR, HudAnchor.TopCenter.ordinal)
        return HudAnchor.entries.getOrElse(ordinal) { HudAnchor.TopCenter }
    }

    fun setAnchor(prefs: SharedPreferences, anchor: HudAnchor) {
        prefs.edit()
            .putInt(KEY_ANCHOR, anchor.ordinal)
            .putBoolean(KEY_HAS_POSITION, false)
            .apply()
    }

    fun scale(prefs: SharedPreferences) = prefs.getFloat(KEY_SCALE, 1f)

    fun setScale(prefs: SharedPreferences, value: Float) {
        prefs.edit().putFloat(KEY_SCALE, value.coerceIn(0.6f, 2f)).apply()
    }

    fun hasPosition(prefs: SharedPreferences) = prefs.getBoolean(KEY_HAS_POSITION, false)

    fun position(prefs: SharedPreferences) =
        Pair(prefs.getFloat(KEY_POS_X, 0f), prefs.getFloat(KEY_POS_Y, 0f))

    fun setPosition(prefs: SharedPreferences, x: Float, y: Float) {
        prefs.edit()
            .putFloat(KEY_POS_X, x)
            .putFloat(KEY_POS_Y, y)
            .putBoolean(KEY_HAS_POSITION, true)
            .apply()
    }

    fun frametimeNumeric(prefs: SharedPreferences) =
        prefs.getBoolean(KEY_FRAMETIME_NUMERIC, false)

    fun setFrametimeNumeric(prefs: SharedPreferences, value: Boolean) {
        prefs.edit().putBoolean(KEY_FRAMETIME_NUMERIC, value).apply()
    }

    fun isElementEnabled(prefs: SharedPreferences, element: HudElement) =
        prefs.getBoolean(KEY_ELEMENT + element.key, element.enabledByDefault)

    fun setElementEnabled(prefs: SharedPreferences, element: HudElement, value: Boolean) {
        prefs.edit().putBoolean(KEY_ELEMENT + element.key, value).apply()
    }

    fun enabledElements(prefs: SharedPreferences) =
        HudElement.entries.filter { isElementEnabled(prefs, it) }.toSet()
}
