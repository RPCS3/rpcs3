package net.rpcs3.utils

import android.content.Context
import android.util.Log
import io.ktor.client.HttpClient
import io.ktor.client.request.get
import io.ktor.client.statement.bodyAsText
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.withContext
import net.rpcs3.RPCS3
import org.json.JSONObject
import java.io.File
import java.security.MessageDigest

private const val TAG = "PatchUpdater"

object PatchUpdater {
    const val PATCH_ENGINE_VERSION = "1.2"

    private const val ENDPOINT = "https://rpcs3.net/compatibility?patch&api=v1&v="
    private const val PREFS_NAME = "PatchUpdaterPrefs"
    private const val KEY_LAST_CHECK = "lastCheckMillis"
    private const val CHECK_INTERVAL_MS = 24L * 60L * 60L * 1000L

    sealed interface State {
        data object Unknown : State
        data object Checking : State
        data object UpToDate : State
        data object Downloading : State
        data class Available(val sha256: String, val bytes: Long) : State
        data class Failed(val message: String) : State
    }

    private val httpClient = HttpClient()

    private val _state = MutableStateFlow<State>(State.Unknown)
    val state: StateFlow<State> = _state

    fun patchesDir(): File = File(RPCS3.rootDirectory + "config/patches")

    fun patchFile(): File = File(patchesDir(), "patch.yml")

    private fun pendingFile(): File = File(patchesDir(), "patch.yml.pending")

    fun sha256Hex(bytes: ByteArray): String {
        val digest = MessageDigest.getInstance("SHA-256").digest(bytes)
        val out = StringBuilder(digest.size * 2)
        for (byte in digest) out.append("%02x".format(byte))
        return out.toString()
    }

    private fun localSha256(): String? = runCatching {
        val file = patchFile()
        if (file.isFile) sha256Hex(file.readBytes()) else null
    }.getOrNull()

    fun localPatchExists(): Boolean = patchFile().isFile

    fun pendingSha256(): String? = runCatching {
        val file = pendingFile()
        if (file.isFile) sha256Hex(file.readBytes()) else null
    }.getOrNull()

    private fun markChecked(context: Context) {
        runCatching {
            context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
                .edit()
                .putLong(KEY_LAST_CHECK, System.currentTimeMillis())
                .apply()
        }
    }

    private fun lastCheck(context: Context): Long = runCatching {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .getLong(KEY_LAST_CHECK, 0L)
    }.getOrDefault(0L)

    suspend fun check(context: Context): State = withContext(Dispatchers.IO) {
        _state.value = State.Checking

        val result = runCatching {
            var url = ENDPOINT + PATCH_ENGINE_VERSION
            localSha256()?.let { url += "&sha256=$it" }

            val body = httpClient.get(url).bodyAsText()
            val json = JSONObject(body)

            when (val code = json.optInt("return_code", -255)) {
                1 -> {
                    pendingFile().delete()
                    State.UpToDate
                }

                0 -> {
                    val version = json.optString("version")
                    if (version != PATCH_ENGINE_VERSION) {
                        State.Failed("Server returned patch engine version $version")
                    } else {
                        val content = json.optString("patch")
                        val declared = json.optString("sha256").lowercase()
                        val actual = sha256Hex(content.toByteArray(Charsets.UTF_8))

                        when {
                            content.isEmpty() -> State.Failed("Server returned no patch data")
                            declared != actual -> State.Failed("Checksum mismatch")
                            actual == localSha256() -> {
                                pendingFile().delete()
                                State.UpToDate
                            }

                            else -> {
                                patchesDir().mkdirs()
                                pendingFile().writeText(content)
                                State.Available(actual, content.toByteArray(Charsets.UTF_8).size.toLong())
                            }
                        }
                    }
                }

                else -> State.Failed("Server error $code")
            }
        }.getOrElse { error ->
            Log.e(TAG, "Patch check failed", error)
            State.Failed(error.message ?: "Network error")
        }

        markChecked(context)
        _state.value = result
        result
    }

    suspend fun checkIfDue(context: Context) {
        if (System.currentTimeMillis() - lastCheck(context) < CHECK_INTERVAL_MS) {
            val staged = pendingSha256()
            if (staged != null && staged != localSha256()) {
                _state.value = State.Available(staged, pendingFile().length())
            }
            return
        }

        check(context)
    }

    suspend fun download(): Boolean = withContext(Dispatchers.IO) {
        val pending = pendingFile()
        if (!pending.isFile) {
            _state.value = State.Failed("Nothing staged to install")
            return@withContext false
        }

        _state.value = State.Downloading

        val ok = runCatching {
            val target = patchFile()
            if (target.isFile) {
                val backup = File(target.parentFile, "patch.yml.old")
                backup.delete()
                target.copyTo(backup, overwrite = true)
            }
            pending.copyTo(target, overwrite = true)
            pending.delete()
            true
        }.getOrElse { error ->
            Log.e(TAG, "Patch install failed", error)
            false
        }

        _state.value = if (ok) State.UpToDate else State.Failed("Could not write patch.yml")
        ok
    }
}
