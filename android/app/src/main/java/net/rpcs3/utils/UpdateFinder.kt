package net.rpcs3.utils

import android.util.Log
import android.util.Xml
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.xmlpull.v1.XmlPullParser
import java.io.StringReader
import java.net.HttpURLConnection
import java.net.URL
import java.security.SecureRandom
import java.security.cert.X509Certificate
import javax.net.ssl.HostnameVerifier
import javax.net.ssl.HttpsURLConnection
import javax.net.ssl.SSLContext
import javax.net.ssl.SSLSocketFactory
import javax.net.ssl.X509TrustManager

private const val TAG = "UpdateFinder"
private const val USER_AGENT = "PS3Native"
private const val INDEX_TTL_MS = 6L * 60L * 60L * 1000L

private val SONY_HOST_SUFFIXES = listOf(
    ".np.dl.playstation.net",
    ".prod-qa.dl.playstation.net"
)

data class UpdateMirror(
    val sourceName: String,
    val url: String,
    val insecureTls: Boolean = false
)

data class UpdateEntry(
    val titleId: String,
    val version: String,
    val name: String,
    val sizeBytes: Long,
    val sha1: String,
    val systemVersion: String,
    val mirrors: List<UpdateMirror>,
    val sources: List<String>
) {
    val key: String get() = "$titleId@$version"
}

data class UpdateLookup(
    val entries: List<UpdateEntry>,
    val errors: Map<String, String>
)

object UpdateFinder {
    private val indexCache = HashMap<String, Pair<Long, String>>()

    fun isSonyHost(host: String?): Boolean {
        val value = host.orEmpty().lowercase()
        return SONY_HOST_SUFFIXES.any { value.endsWith(it) }
    }

    private val permissiveFactory: SSLSocketFactory by lazy {
        val trustAll = object : X509TrustManager {
            override fun checkClientTrusted(chain: Array<X509Certificate>?, authType: String?) {}
            override fun checkServerTrusted(chain: Array<X509Certificate>?, authType: String?) {}
            override fun getAcceptedIssuers(): Array<X509Certificate> = emptyArray()
        }

        SSLContext.getInstance("TLS").apply {
            init(null, arrayOf(trustAll), SecureRandom())
        }.socketFactory
    }

    fun open(rawUrl: String, insecureTls: Boolean = false): HttpURLConnection {
        val url = URL(rawUrl)
        val connection = url.openConnection() as HttpURLConnection

        if (connection is HttpsURLConnection && (insecureTls || isSonyHost(url.host))) {
            connection.sslSocketFactory = permissiveFactory
            connection.hostnameVerifier = HostnameVerifier { _, _ -> true }
        }

        connection.requestMethod = "GET"
        connection.connectTimeout = 20000
        connection.readTimeout = 30000
        connection.instanceFollowRedirects = true
        connection.setRequestProperty("User-Agent", USER_AGENT)
        return connection
    }

    private fun fetchText(rawUrl: String, insecureTls: Boolean = false): String {
        val connection = open(rawUrl, insecureTls)

        try {
            val code = connection.responseCode

            if (code == 404) {
                return ""
            }

            if (code !in 200..299) {
                throw IllegalStateException("HTTP $code")
            }

            return connection.inputStream.bufferedReader().use { it.readText() }
        } finally {
            connection.disconnect()
        }
    }

    private fun cachedIndex(rawUrl: String, insecureTls: Boolean): String {
        val now = System.currentTimeMillis()
        val hit = indexCache[rawUrl]

        if (hit != null && now - hit.first < INDEX_TTL_MS) {
            return hit.second
        }

        val body = fetchText(rawUrl, insecureTls)
        indexCache[rawUrl] = now to body
        return body
    }

    fun clearCache() = indexCache.clear()

    suspend fun lookup(titleId: String, sources: List<UpdateSource>): UpdateLookup =
        withContext(Dispatchers.IO) {
            val serial = titleId.trim().uppercase()

            if (serial.isEmpty()) {
                return@withContext UpdateLookup(emptyList(), emptyMap())
            }

            val found = ArrayList<Pair<UpdateSource, UpdateCandidate>>()
            val errors = HashMap<String, String>()

            sources.filter { it.enabled }.forEach { source ->
                runCatching { queryOne(serial, source) }
                    .onSuccess { list -> list.forEach { found += source to it } }
                    .onFailure { error ->
                        Log.w(TAG, "Source ${source.name} failed for $serial", error)
                        errors[source.name] = error.message ?: error.javaClass.simpleName
                    }
            }

            UpdateLookup(merge(serial, found), errors)
        }

    private fun queryOne(titleId: String, source: UpdateSource): List<UpdateCandidate> {
        val url = source.resolve(titleId)

        return when (source.format) {
            UpdateSourceFormat.SonyVerXml -> parseVerXml(fetchText(url, source.insecureTls))
            UpdateSourceFormat.IndexHtml -> parseIndexHtml(
                if (source.perTitle) {
                    fetchText(url, source.insecureTls)
                } else {
                    cachedIndex(url, source.insecureTls)
                },
                titleId
            )

            UpdateSourceFormat.LinkScrape -> parseLinks(
                if (source.perTitle) {
                    fetchText(url, source.insecureTls)
                } else {
                    cachedIndex(url, source.insecureTls)
                },
                titleId
            )
        }
    }

    private fun merge(
        titleId: String,
        found: List<Pair<UpdateSource, UpdateCandidate>>
    ): List<UpdateEntry> {
        val byVersion = LinkedHashMap<String, MutableList<Pair<UpdateSource, UpdateCandidate>>>()

        found.forEach { (source, candidate) ->
            byVersion.getOrPut(candidate.version) { ArrayList() } += source to candidate
        }

        return byVersion.entries.map { (version, group) ->
            val best = group.firstOrNull { it.second.sha1.isNotEmpty() }?.second ?: group.first().second
            val mirrors = LinkedHashMap<String, UpdateMirror>()

            group.forEach { (source, candidate) ->
                mirrors.getOrPut(candidate.url) {
                    UpdateMirror(source.name, candidate.url, source.insecureTls)
                }
            }

            UpdateEntry(
                titleId = titleId,
                version = version,
                name = group.firstOrNull { it.second.name.isNotEmpty() }?.second?.name.orEmpty(),
                sizeBytes = group.maxOf { it.second.sizeBytes },
                sha1 = best.sha1,
                systemVersion = group.firstOrNull {
                    it.second.systemVersion.isNotEmpty()
                }?.second?.systemVersion.orEmpty(),
                mirrors = mirrors.values.toList(),
                sources = group.map { it.first.name }.distinct()
            )
        }.sortedBy { it.version }
    }

    internal fun parseVerXml(body: String): List<UpdateCandidate> {
        if (body.isBlank() || !body.contains("<titlepatch")) {
            return emptyList()
        }

        val parser = Xml.newPullParser()
        parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, false)
        parser.setInput(StringReader(body))

        val out = ArrayList<UpdateCandidate>()
        var pendingTitle = ""
        var current: UpdateCandidate? = null
        var inTitle = false

        while (true) {
            val event = parser.next()

            if (event == XmlPullParser.END_DOCUMENT) {
                break
            }

            when (event) {
                XmlPullParser.START_TAG -> when (parser.name) {
                    "package" -> current = UpdateCandidate(
                        version = normalizeVersion(parser.getAttributeValue(null, "version")),
                        name = "",
                        sizeBytes = parser.getAttributeValue(null, "size")?.toLongOrNull() ?: 0L,
                        sha1 = parser.getAttributeValue(null, "sha1sum").orEmpty(),
                        systemVersion = parser.getAttributeValue(null, "ps3_system_ver").orEmpty(),
                        url = parser.getAttributeValue(null, "url").orEmpty()
                    )

                    "TITLE" -> inTitle = true
                }

                XmlPullParser.TEXT -> if (inTitle) {
                    pendingTitle = parser.text.orEmpty().trim()
                }

                XmlPullParser.END_TAG -> when (parser.name) {
                    "TITLE" -> inTitle = false
                    "package" -> {
                        current?.let { entry ->
                            if (entry.url.isNotEmpty()) {
                                out += entry.copy(name = pendingTitle)
                            }
                        }
                        current = null
                    }
                }
            }
        }

        return out
    }

    private val indexRow = Regex(
        """<tr><td>([A-Za-z]{4}\d{5})<td>(.*?)<td>(.*?)<td>(https?://[^<\s]+)<td>(\d*)""",
        RegexOption.IGNORE_CASE
    )

    internal fun parseIndexHtml(body: String, titleId: String): List<UpdateCandidate> {
        if (body.isBlank()) {
            return emptyList()
        }

        return indexRow.findAll(body).mapNotNull { match ->
            val serial = match.groupValues[1].uppercase()
            if (serial != titleId) return@mapNotNull null

            UpdateCandidate(
                version = normalizeVersion(match.groupValues[3]),
                name = unescape(match.groupValues[2]),
                sizeBytes = match.groupValues[5].toLongOrNull() ?: 0L,
                sha1 = "",
                systemVersion = "",
                url = match.groupValues[4]
            )
        }.toList()
    }

    private val pkgHref = Regex("""https?://[^"'<>\s]+\.pkg""", RegexOption.IGNORE_CASE)
    private val fileVersion = Regex("""-A(\d{2})(\d{2})-""")

    internal fun parseLinks(body: String, titleId: String): List<UpdateCandidate> {
        if (body.isBlank()) {
            return emptyList()
        }

        return pkgHref.findAll(body).map { it.value }.distinct().mapNotNull { url ->
            if (!url.uppercase().contains(titleId)) {
                return@mapNotNull null
            }

            val match = fileVersion.find(url)
            val version = if (match == null) {
                "?"
            } else {
                "${match.groupValues[1]}.${match.groupValues[2]}"
            }

            UpdateCandidate(
                version = version,
                name = "",
                sizeBytes = 0L,
                sha1 = "",
                systemVersion = "",
                url = url
            )
        }.toList()
    }

    internal fun normalizeVersion(raw: String?): String {
        val value = raw.orEmpty().trim()

        if (value.isEmpty()) {
            return "?"
        }

        val digits = value.removePrefix("A")
        val numeric = digits.toDoubleOrNull()

        return if (numeric != null && !digits.contains('.') && digits.length == 4) {
            "${digits.substring(0, 2)}.${digits.substring(2)}"
        } else {
            value
        }
    }

    private fun unescape(raw: String) = raw
        .replace("&amp;", "&")
        .replace("&lt;", "<")
        .replace("&gt;", ">")
        .replace("&quot;", "\"")
        .replace("&#39;", "'")
        .replace(Regex("<[^>]+>"), "")
        .trim()
}

data class UpdateCandidate(
    val version: String,
    val name: String,
    val sizeBytes: Long,
    val sha1: String,
    val systemVersion: String,
    val url: String
)
