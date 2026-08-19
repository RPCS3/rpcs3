package net.rpcs3.utils

import android.content.Context
import android.content.SharedPreferences
import android.util.Log
import org.json.JSONArray
import org.json.JSONObject

private const val TAG = "UpdateSources"
private const val PREFS_NAME = "ps3_update_sources"
private const val PREFS_KEY = "sources"
private const val PREFS_VERSION_KEY = "sources_version"
private const val DEFAULTS_VERSION = 1

enum class UpdateSourceFormat {
    SonyVerXml,
    IndexHtml,
    LinkScrape;

    companion object {
        fun fromTag(tag: String) = when (tag) {
            "index" -> IndexHtml
            "links" -> LinkScrape
            else -> SonyVerXml
        }
    }

    val tag: String
        get() = when (this) {
            SonyVerXml -> "verxml"
            IndexHtml -> "index"
            LinkScrape -> "links"
        }
}

data class UpdateSource(
    val name: String,
    val urlTemplate: String,
    val format: UpdateSourceFormat,
    val enabled: Boolean = true,
    val insecureTls: Boolean = false
) {
    val perTitle: Boolean
        get() = urlTemplate.contains(TITLE_ID_TOKEN, ignoreCase = true)

    fun resolve(titleId: String): String =
        urlTemplate
            .replace(TITLE_ID_TOKEN, titleId.uppercase())
            .replace(TITLE_ID_TOKEN.lowercase(), titleId.lowercase())

    companion object {
        const val TITLE_ID_TOKEN = "{TITLEID}"
    }
}

object UpdateSources {
    private const val PSN_NAME = "PlayStation Network"
    private const val PSN_URL =
        "https://a0.ww.np.dl.playstation.net/tpl/np/{TITLEID}/{TITLEID}-ver.xml"

    fun defaults() = listOf(
        UpdateSource(PSN_NAME, PSN_URL, UpdateSourceFormat.SonyVerXml)
    )

    fun normalize(
        name: String,
        rawUrl: String,
        format: UpdateSourceFormat?,
        insecureTls: Boolean = false
    ): UpdateSource {
        val url = rawUrl.trim()
        val resolved = format ?: guessFormat(url)
        val label = name.trim().ifBlank {
            runCatching { java.net.URI(url).host }.getOrNull().orEmpty().ifBlank { url }
        }

        return UpdateSource(label, url, resolved, insecureTls = insecureTls)
    }

    fun guessFormat(url: String): UpdateSourceFormat {
        val lower = url.lowercase()

        return when {
            lower.contains("-ver.xml") || lower.endsWith(".xml") -> UpdateSourceFormat.SonyVerXml
            lower.endsWith(".html") || lower.endsWith(".htm") -> UpdateSourceFormat.IndexHtml
            else -> UpdateSourceFormat.LinkScrape
        }
    }

    fun load(prefs: SharedPreferences): List<UpdateSource> {
        val raw = prefs.getString(PREFS_KEY, null) ?: return defaults()

        return runCatching {
            val array = JSONArray(raw)
            (0 until array.length()).mapNotNull { index ->
                val item = array.optJSONObject(index) ?: return@mapNotNull null
                val url = item.optString("url")
                if (url.isBlank()) return@mapNotNull null

                UpdateSource(
                    name = item.optString("name", url),
                    urlTemplate = url,
                    format = UpdateSourceFormat.fromTag(item.optString("format")),
                    enabled = item.optBoolean("enabled", true),
                    insecureTls = item.optBoolean("insecureTls", false)
                )
            }
        }.getOrElse {
            Log.e(TAG, "Failed to parse stored update sources", it)
            defaults()
        }
    }

    fun save(prefs: SharedPreferences, sources: List<UpdateSource>) {
        val array = JSONArray()

        sources.forEach { source ->
            array.put(
                JSONObject()
                    .put("name", source.name)
                    .put("url", source.urlTemplate)
                    .put("format", source.format.tag)
                    .put("enabled", source.enabled)
                    .put("insecureTls", source.insecureTls)
            )
        }

        prefs.edit()
            .putString(PREFS_KEY, array.toString())
            .putInt(PREFS_VERSION_KEY, DEFAULTS_VERSION)
            .apply()
    }

    fun withDefaultsRestored(sources: List<UpdateSource>): List<UpdateSource> {
        val known = sources.map { it.urlTemplate }.toHashSet()
        return sources + defaults().filter { it.urlTemplate !in known }
    }

    fun prefsOf(context: Context): SharedPreferences =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
}
