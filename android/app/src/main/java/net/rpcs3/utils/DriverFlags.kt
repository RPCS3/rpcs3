package net.rpcs3.utils

import android.content.Context
import android.system.Os
import androidx.core.content.edit

object DriverFlags {
    private const val PREFS_NAME = "GpuDriverFlags"
    private const val GLOBAL_KEY = "global"
    private const val ENV_NAME = "TU_DEBUG"

    val defaults = listOf("sysmem", "noconform")

    private fun prefs(context: Context) =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    private fun keyFor(titleId: String?) =
        if (titleId.isNullOrEmpty()) GLOBAL_KEY else "game_$titleId"

    private fun decode(raw: String) =
        raw.split(',').map { it.trim() }.filter { it.isNotEmpty() }

    fun globalFlags(context: Context): List<String> {
        val stored = prefs(context).getString(GLOBAL_KEY, null) ?: return defaults
        return decode(stored)
    }

    fun hasOverride(context: Context, titleId: String?) =
        !titleId.isNullOrEmpty() && prefs(context).contains(keyFor(titleId))

    fun flagsFor(context: Context, titleId: String?): List<String> {
        if (titleId.isNullOrEmpty()) {
            return globalFlags(context)
        }

        val stored = prefs(context).getString(keyFor(titleId), null)
            ?: return globalFlags(context)

        return decode(stored)
    }

    fun setFlags(context: Context, titleId: String?, flags: List<String>) {
        prefs(context).edit {
            putString(keyFor(titleId), flags.joinToString(","))
        }
    }

    fun clearOverride(context: Context, titleId: String) {
        prefs(context).edit { remove(keyFor(titleId)) }
    }

    fun apply(context: Context, titleId: String?) {
        val value = flagsFor(context, titleId).joinToString(",")

        runCatching {
            if (value.isEmpty()) {
                Os.unsetenv(ENV_NAME)
            } else {
                Os.setenv(ENV_NAME, value, true)
            }
        }
    }
}
