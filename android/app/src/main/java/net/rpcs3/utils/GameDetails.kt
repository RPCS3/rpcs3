package net.rpcs3.utils

import android.content.Context
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.gameTitleId
import org.json.JSONObject

data class GameDetails(
    val titleId: String,
    val title: String,
    val version: String,
    val baseVersion: String,
    val category: String,
    val updated: Boolean
) {
    fun versionLabel(context: Context): String = when {
        version.isEmpty() -> ""
        updated && baseVersion.isNotEmpty() -> {
            context.getString(R.string.game_version_updated, version, baseVersion)
        }

        else -> context.getString(R.string.game_version, version)
    }
}

object GameDetailsReader {
    fun read(path: String): GameDetails {
        val fallback = GameDetails(gameTitleId(path), "", "", "", "", false)

        val raw = runCatching { RPCS3.instance.gameDetails(path) }.getOrNull() ?: return fallback

        return runCatching {
            val json = JSONObject(raw)
            val titleId = json.optString("titleId")

            GameDetails(
                titleId = titleId.ifEmpty { fallback.titleId },
                title = json.optString("title"),
                version = json.optString("version"),
                baseVersion = json.optString("baseVersion"),
                category = json.optString("category"),
                updated = json.optBoolean("updated", false)
            )
        }.getOrDefault(fallback)
    }
}
