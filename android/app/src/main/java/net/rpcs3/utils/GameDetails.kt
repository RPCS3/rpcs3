package net.rpcs3.utils

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
    val versionLabel: String
        get() = when {
            version.isEmpty() -> ""
            updated && baseVersion.isNotEmpty() -> "v$version (disc v$baseVersion)"
            else -> "v$version"
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
