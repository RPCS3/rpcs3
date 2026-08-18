package net.rpcs3.utils

import android.content.Context
import android.content.SharedPreferences
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.util.Locale

private const val TAG = "DriverRepos"
private const val PREFS_KEY = "custom_driver_repos"

data class DriverRepo(
    val name: String,
    val repoUrl: String,
    val apiUrl: String
)

data class DriverAsset(
    val id: Long,
    val name: String,
    val downloadUrl: String,
    val sizeLabel: String
)

data class DriverRelease(
    val id: Long,
    val title: String,
    val subtitle: String,
    val notes: String,
    val assets: List<DriverAsset>
)

object DriverRepos {
    private const val WINNATIVE_NAME = "WinNative Components"
    private const val WINNATIVE_REPO = "https://github.com/nicholasx417/WinNative-Components/releases"
    private const val WINNATIVE_API =
        "https://api.github.com/repos/nicholasx417/WinNative-Components/releases"

    private const val ADRENO_NAME = "AdrenoToolsDrivers"
    private const val ADRENO_REPO = "https://github.com/K11MCH1/AdrenoToolsDrivers/releases"
    private const val ADRENO_API =
        "https://api.github.com/repos/K11MCH1/AdrenoToolsDrivers/releases"

    fun defaults() = listOf(
        DriverRepo(WINNATIVE_NAME, WINNATIVE_REPO, WINNATIVE_API),
        DriverRepo(ADRENO_NAME, ADRENO_REPO, ADRENO_API)
    )

    fun normalize(name: String, rawUrl: String): DriverRepo {
        var url = rawUrl.trim()

        if (url.startsWith("https://github.com/") && !url.contains("api.github.com")) {
            url = url.replace("https://github.com/", "https://api.github.com/repos/")
            if (!url.endsWith("/releases")) {
                url = "$url/releases"
            }
        }

        val repoUrl = url.replace("api.github.com/repos", "github.com")
        return DriverRepo(name = name.ifBlank { repoUrl.substringAfterLast('/') }, repoUrl = repoUrl, apiUrl = url)
    }

    fun load(prefs: SharedPreferences): List<DriverRepo> {
        val raw = prefs.getString(PREFS_KEY, null) ?: return defaults()

        return runCatching {
            val array = JSONArray(raw)
            (0 until array.length()).mapNotNull { index ->
                val item = array.optJSONObject(index) ?: return@mapNotNull null
                val apiUrl = item.optString("apiUrl")
                if (apiUrl.isBlank()) return@mapNotNull null

                DriverRepo(
                    name = item.optString("name", "Unknown repo"),
                    repoUrl = item.optString("repoUrl"),
                    apiUrl = apiUrl
                )
            }
        }.getOrElse {
            Log.e(TAG, "Failed to parse stored repos", it)
            defaults()
        }
    }

    fun save(prefs: SharedPreferences, repos: List<DriverRepo>) {
        val array = JSONArray()

        repos.forEach { repo ->
            array.put(
                JSONObject()
                    .put("name", repo.name)
                    .put("repoUrl", repo.repoUrl)
                    .put("apiUrl", repo.apiUrl)
            )
        }

        prefs.edit().putString(PREFS_KEY, array.toString()).apply()
    }

    fun withDefaultsRestored(repos: List<DriverRepo>): List<DriverRepo> {
        val known = repos.map { it.apiUrl }.toHashSet()
        return repos + defaults().filter { it.apiUrl !in known }
    }

    suspend fun fetchReleases(repo: DriverRepo): List<DriverRelease> = withContext(Dispatchers.IO) {
        val apiUrl = when {
            !repo.apiUrl.contains("api.github.com") -> repo.apiUrl
            repo.apiUrl.contains("?") -> "${repo.apiUrl}&per_page=100"
            else -> "${repo.apiUrl}?per_page=100"
        }

        val connection = (URL(apiUrl).openConnection() as HttpURLConnection).apply {
            requestMethod = "GET"
            connectTimeout = 15000
            readTimeout = 15000
            setRequestProperty("Accept", "application/vnd.github+json")
            setRequestProperty("User-Agent", "PS3Native")
        }

        try {
            val ok = connection.responseCode in 200..299
            val body = (if (ok) connection.inputStream else connection.errorStream)
                ?.bufferedReader()?.use { it.readText() }.orEmpty()

            if (!ok) {
                throw IllegalStateException(
                    body.ifBlank { "GitHub request failed with HTTP ${connection.responseCode}" }
                )
            }

            val array = JSONArray(body)
            (0 until array.length()).mapNotNull { index ->
                val release = array.optJSONObject(index) ?: return@mapNotNull null
                val assets = zipAssets(release.optJSONArray("assets"))
                if (assets.isEmpty()) return@mapNotNull null

                val tag = release.optString("tag_name")
                val name = release.optString("name").ifBlank { tag }

                DriverRelease(
                    id = release.optLong("id"),
                    title = name.ifBlank { "Unnamed" },
                    subtitle = subtitleFor(tag, release.optString("published_at"), assets.size),
                    notes = release.optString("body").lineSequence()
                        .map { it.trim() }
                        .filter { it.isNotEmpty() }
                        .take(6)
                        .joinToString("\n"),
                    assets = assets
                )
            }
        } finally {
            connection.disconnect()
        }
    }

    private fun zipAssets(array: JSONArray?): List<DriverAsset> {
        if (array == null) {
            return emptyList()
        }

        return (0 until array.length()).mapNotNull { index ->
            val asset = array.optJSONObject(index) ?: return@mapNotNull null
            val name = asset.optString("name").trim()
            val url = asset.optString("browser_download_url")

            if (!name.lowercase(Locale.ROOT).endsWith(".zip") || url.isBlank()) {
                return@mapNotNull null
            }

            DriverAsset(
                id = asset.optLong("id"),
                name = name,
                downloadUrl = url,
                sizeLabel = formatBytes(asset.optLong("size"))
            )
        }
    }

    private fun subtitleFor(tag: String, publishedAt: String, assetCount: Int): String {
        val parts = mutableListOf<String>()
        if (tag.isNotBlank()) parts += tag
        if (publishedAt.length >= 10) parts += publishedAt.substring(0, 10)
        parts += if (assetCount == 1) "1 file" else "$assetCount files"
        return parts.joinToString(" · ")
    }

    fun formatBytes(bytes: Long): String = when {
        bytes <= 0L -> "—"
        bytes >= 1024L * 1024L -> String.format(Locale.US, "%.1f MB", bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> String.format(Locale.US, "%.0f KB", bytes / 1024.0)
        else -> "$bytes B"
    }

    fun prefsOf(context: Context): SharedPreferences =
        context.getSharedPreferences("gpu_driver_repos", Context.MODE_PRIVATE)
}
