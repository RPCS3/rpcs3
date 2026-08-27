package net.rpcs3.framegen

import android.content.Context
import android.content.SharedPreferences
import android.net.Uri
import android.provider.OpenableColumns
import androidx.compose.runtime.mutableStateOf
import net.rpcs3.R
import net.rpcs3.RPCS3
import org.json.JSONObject
import java.io.File

enum class FrameGenImportResult(val code: Int, val messageRes: Int) {
    Ok(0, R.string.framegen_import_ok),
    NotFound(1, R.string.framegen_import_not_found),
    Unreadable(2, R.string.framegen_import_unreadable),
    NotAnExecutable(3, R.string.framegen_import_not_executable),
    MissingShaders(4, R.string.framegen_import_missing_shaders),
    TranslationFailed(5, R.string.framegen_import_translation_failed),
    CacheUnusable(6, R.string.framegen_import_cache_unusable);

    companion object {
        fun fromCode(value: Int) = entries.firstOrNull { it.code == value } ?: CacheUnusable
    }
}

data class FrameGenState(
    val imported: Boolean = false,
    val variant: String = "none",
    val modules: Int = 0,
    val sourceSize: Long = 0L,
    val sourceName: String = "",
    val importedAt: Long = 0L,
    val ready: Boolean = false,
    val unsupported: Boolean = false,
    val width: Int = 0,
    val height: Int = 0,
    val flowWidth: Int = 0,
    val flowHeight: Int = 0,
    val guestWidth: Int = 0,
    val guestHeight: Int = 0
)

object FrameGenPrefs {
    const val NAME = "framegen"

    private const val KEY_ENABLED = "enabled"
    private const val KEY_MODE = "mode"
    private const val KEY_MULTIPLIER = "multiplier"
    private const val KEY_TARGET_RATE = "target_rate"
    private const val KEY_FLOW_SCALE = "flow_scale"
    private const val KEY_SOURCE_NAME = "source_name"
    private const val KEY_IMPORTED_AT = "imported_at"

    fun of(context: Context): SharedPreferences =
        context.getSharedPreferences(NAME, Context.MODE_PRIVATE)

    fun isEnabled(prefs: SharedPreferences) = prefs.getBoolean(KEY_ENABLED, false)

    fun setEnabled(prefs: SharedPreferences, value: Boolean) {
        prefs.edit().putBoolean(KEY_ENABLED, value).apply()
    }

    fun multiplier(prefs: SharedPreferences) =
        prefs.getInt(KEY_MULTIPLIER, 2).coerceIn(2, 4)

    fun setMultiplier(prefs: SharedPreferences, value: Int) {
        prefs.edit().putInt(KEY_MULTIPLIER, value.coerceIn(2, 4)).apply()
    }

    fun targetRate(prefs: SharedPreferences): Int {
        migrateMode(prefs)
        return prefs.getInt(KEY_TARGET_RATE, 0).coerceAtLeast(0)
    }

    fun setTargetRate(prefs: SharedPreferences, value: Int) {
        prefs.edit().putInt(KEY_TARGET_RATE, value.coerceAtLeast(0)).apply()
    }

    private fun migrateMode(prefs: SharedPreferences) {
        if (!prefs.contains(KEY_MODE)) {
            return
        }

        val wasAdaptive = prefs.getInt(KEY_MODE, 0) == 1
        prefs.edit()
            .putInt(KEY_TARGET_RATE, if (wasAdaptive) prefs.getInt(KEY_TARGET_RATE, 120) else 0)
            .remove(KEY_MODE)
            .apply()
    }

    fun preset(prefs: SharedPreferences): FrameGenPreset =
        FrameGenPreset.fromFlowScale(prefs.getInt(KEY_FLOW_SCALE, FrameGenPreset.Default.flowScale))

    fun setPreset(prefs: SharedPreferences, value: FrameGenPreset) {
        prefs.edit().putInt(KEY_FLOW_SCALE, value.flowScale).apply()
    }

    fun sourceName(prefs: SharedPreferences): String = prefs.getString(KEY_SOURCE_NAME, "").orEmpty()

    fun importedAt(prefs: SharedPreferences): Long = prefs.getLong(KEY_IMPORTED_AT, 0L)

    fun rememberSource(prefs: SharedPreferences, name: String, at: Long) {
        prefs.edit()
            .putString(KEY_SOURCE_NAME, name)
            .putLong(KEY_IMPORTED_AT, at)
            .apply()
    }

    fun forgetSource(prefs: SharedPreferences) {
        prefs.edit().remove(KEY_SOURCE_NAME).remove(KEY_IMPORTED_AT).apply()
    }
}

object FrameGen {
    val state = mutableStateOf(FrameGenState())

    fun cachePath(context: Context): String =
        File(File(context.filesDir, "framegen"), "shaders.bin").absolutePath

    fun refresh(context: Context): FrameGenState {
        val prefs = FrameGenPrefs.of(context)
        val raw = runCatching { JSONObject(RPCS3.instance.frameGenState(cachePath(context))) }
            .getOrNull()

        val imported = raw?.optBoolean("imported", false) == true

        val result = FrameGenState(
            imported = imported,
            variant = raw?.optString("variant").orEmpty().ifEmpty { "none" },
            modules = raw?.optInt("modules", 0) ?: 0,
            sourceSize = raw?.optLong("sourceSize", 0L) ?: 0L,
            sourceName = FrameGenPrefs.sourceName(prefs),
            importedAt = FrameGenPrefs.importedAt(prefs),
            ready = raw?.optBoolean("ready", false) == true,
            unsupported = raw?.optBoolean("unsupported", false) == true,
            width = raw?.optInt("width", 0) ?: 0,
            height = raw?.optInt("height", 0) ?: 0,
            flowWidth = raw?.optInt("flowWidth", 0) ?: 0,
            flowHeight = raw?.optInt("flowHeight", 0) ?: 0,
            guestWidth = raw?.optInt("guestWidth", 0) ?: 0,
            guestHeight = raw?.optInt("guestHeight", 0) ?: 0
        )

        state.value = result
        return result
    }

    fun push(context: Context) {
        val prefs = FrameGenPrefs.of(context)

        RPCS3.instance.frameGenConfigure(
            FrameGenPrefs.isEnabled(prefs) && state.value.imported,
            FrameGenPrefs.multiplier(prefs),
            FrameGenPrefs.targetRate(prefs),
            FrameGenPrefs.preset(prefs).flowScale
        )
    }

    fun pushRefreshRate(hz: Float) {
        if (hz > 0f) {
            RPCS3.instance.frameGenSetRefreshRate(hz)
        }
    }

    fun sync(context: Context) {
        refresh(context)
        push(context)
    }

    fun import(context: Context, uri: Uri): FrameGenImportResult {
        val prefs = FrameGenPrefs.of(context)

        val code = runCatching {
            context.contentResolver.openFileDescriptor(uri, "r").use { descriptor ->
                val fd = descriptor?.fd ?: return@runCatching FrameGenImportResult.Unreadable.code
                RPCS3.instance.frameGenImport(fd, cachePath(context))
            }
        }.getOrElse { FrameGenImportResult.Unreadable.code }

        val result = FrameGenImportResult.fromCode(code)

        if (result == FrameGenImportResult.Ok) {
            FrameGenPrefs.rememberSource(prefs, displayNameOf(context, uri), System.currentTimeMillis())
        }

        sync(context)
        return result
    }

    fun forget(context: Context) {
        RPCS3.instance.frameGenForget(cachePath(context))
        FrameGenPrefs.forgetSource(FrameGenPrefs.of(context))
        sync(context)
    }

    private fun displayNameOf(context: Context, uri: Uri): String {
        val fromProvider = runCatching {
            context.contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)
                ?.use { cursor ->
                    if (cursor.moveToFirst()) cursor.getString(0) else null
                }
        }.getOrNull()

        return fromProvider ?: uri.lastPathSegment?.substringAfterLast('/').orEmpty()
    }
}
