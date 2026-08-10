package net.rpcs3.ui.debug

import net.rpcs3.RPCS3
import org.json.JSONArray
import org.json.JSONObject

internal const val LogNodePath = "Log"
internal const val LogLevelOff = "Nothing"
internal const val LogLevelDiagnostic = "Error"

private val DiagnosticChannels = setOf("SPU", "PPU", "JIT", "LDR", "SYS", "RSX")

internal data class LogChannelState(
    val levels: List<String> = listOf(LogLevelOff),
    val channels: List<String> = emptyList(),
    val values: Map<String, String> = emptyMap()
)

private fun stringsOf(array: JSONArray?): List<String> {
    if (array == null) {
        return emptyList()
    }
    return List(array.length()) { array.optString(it, "") }.filter { it.isNotEmpty() }
}

private fun keysOf(node: JSONObject?): List<String> {
    if (node == null) {
        return emptyList()
    }
    val result = mutableListOf<String>()
    val keys = node.keys()
    while (keys.hasNext()) {
        result += keys.next()
    }
    return result
}

private fun readLogNode(): JSONObject? = runCatching {
    JSONObject(RPCS3.instance.settingsGet(LogNodePath, ""))
}.getOrNull()

private fun readRegisteredChannels(): List<String> = runCatching {
    stringsOf(JSONArray(RPCS3.instance.logChannels()))
}.getOrDefault(emptyList())

internal fun loadLogChannelState(): LogChannelState {
    val node = readLogNode()
    val stored = node?.optJSONObject("values")
    val values = keysOf(stored).associateWith { stored?.optString(it, LogLevelOff) ?: LogLevelOff }
    val channels = (readRegisteredChannels() + values.keys)
        .distinct()
        .sortedBy { it.lowercase() }

    return LogChannelState(
        levels = stringsOf(node?.optJSONArray("levels")).ifEmpty { listOf(LogLevelOff) },
        channels = channels,
        values = values
    )
}

internal fun writeLogLevels(updates: Map<String, String>): Boolean {
    if (updates.isEmpty()) {
        return true
    }

    val payload = JSONObject()
    updates.forEach { (channel, level) -> payload.put(channel, level) }

    if (!RPCS3.instance.settingsSet(LogNodePath, payload.toString(), "")) {
        return false
    }

    RPCS3.instance.settingsFlush()
    return true
}

internal fun applyDefaultLogLevels(): Int {
    val node = readLogNode() ?: return 0
    val stored = node.optJSONObject("values") ?: return 0
    val configured = keysOf(stored).toSet()

    if (configured.isEmpty()) {
        return 0
    }

    val missing = readRegisteredChannels().filterNot { configured.contains(it) }

    if (missing.isEmpty()) {
        return 0
    }

    val defaults = missing.associateWith {
        if (DiagnosticChannels.contains(it)) LogLevelDiagnostic else LogLevelOff
    }

    return if (writeLogLevels(defaults)) missing.size else 0
}
