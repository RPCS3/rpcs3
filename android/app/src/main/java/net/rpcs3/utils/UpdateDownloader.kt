package net.rpcs3.utils

import android.content.Context
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.security.MessageDigest

private const val TAG = "UpdateDownloader"
private const val PKG_FOOTER_BYTES = 32

sealed class DownloadOutcome {
    data class Success(val file: File, val mirror: UpdateMirror) : DownloadOutcome()
    data class Failure(val attempts: List<String>) : DownloadOutcome()
}

object UpdateDownloader {
    fun cacheDir(context: Context): File =
        File(context.cacheDir, "updates").apply { mkdirs() }

    fun fileNameFor(entry: UpdateEntry, url: String): String {
        val fromUrl = url.substringAfterLast('/').substringBefore('?')

        return if (fromUrl.endsWith(".pkg", ignoreCase = true)) {
            fromUrl
        } else {
            "${entry.titleId}-${entry.version}.pkg"
        }
    }

    suspend fun download(
        context: Context,
        entry: UpdateEntry,
        onProgress: (downloaded: Long, total: Long, mirror: UpdateMirror) -> Unit
    ): DownloadOutcome = withContext(Dispatchers.IO) {
        val attempts = ArrayList<String>()

        entry.mirrors.forEach { mirror ->
            val target = File(cacheDir(context), fileNameFor(entry, mirror.url))

            val result = runCatching {
                fetchTo(mirror, target, entry) { done, total -> onProgress(done, total, mirror) }
            }

            val error = result.exceptionOrNull()

            if (error == null && result.getOrDefault(false)) {
                return@withContext DownloadOutcome.Success(target, mirror)
            }

            target.delete()
            val reason = error?.message ?: "checksum mismatch"
            Log.w(TAG, "Mirror ${mirror.sourceName} failed for ${entry.key}: $reason")
            attempts += "${mirror.sourceName}: $reason"
        }

        DownloadOutcome.Failure(attempts)
    }

    private fun fetchTo(
        mirror: UpdateMirror,
        target: File,
        entry: UpdateEntry,
        onProgress: (Long, Long) -> Unit
    ): Boolean {
        val connection = UpdateFinder.open(mirror.url, mirror.insecureTls)

        try {
            val code = connection.responseCode

            if (code !in 200..299) {
                throw IllegalStateException("HTTP $code")
            }

            val declared = connection.contentLengthLong.takeIf { it > 0 }
                ?: entry.sizeBytes
            var written = 0L

            connection.inputStream.use { input ->
                target.outputStream().use { output ->
                    val buffer = ByteArray(128 * 1024)

                    while (true) {
                        val read = input.read(buffer)
                        if (read <= 0) break
                        output.write(buffer, 0, read)
                        written += read
                        onProgress(written, declared)
                    }
                }
            }

            if (entry.sizeBytes > 0L && written != entry.sizeBytes) {
                throw IllegalStateException("size mismatch ($written of ${entry.sizeBytes})")
            }

            return verify(target, entry.sha1)
        } finally {
            connection.disconnect()
        }
    }

    fun verify(file: File, expectedSha1: String): Boolean {
        if (expectedSha1.isBlank()) {
            return file.length() > 0L
        }

        val length = file.length()

        if (length <= PKG_FOOTER_BYTES) {
            return false
        }

        val digest = MessageDigest.getInstance("SHA-1")
        var remaining = length - PKG_FOOTER_BYTES

        file.inputStream().use { input ->
            val buffer = ByteArray(128 * 1024)

            while (remaining > 0) {
                val want = minOf(buffer.size.toLong(), remaining).toInt()
                val read = input.read(buffer, 0, want)
                if (read <= 0) break
                digest.update(buffer, 0, read)
                remaining -= read
            }
        }

        val actual = digest.digest().joinToString("") { "%02x".format(it) }
        return actual.equals(expectedSha1.trim(), ignoreCase = true)
    }
}
