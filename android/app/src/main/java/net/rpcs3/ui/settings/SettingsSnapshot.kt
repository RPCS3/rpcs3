package net.rpcs3.ui.settings

import net.rpcs3.RPCS3
import org.json.JSONObject

internal fun snapshotOf(root: JSONObject): JSONObject = JSONObject(root.toString())

private fun encode(type: String, value: Any?): String? = when (type) {
    "bool" -> if (value as? Boolean == true) "true" else "false"
    "enum", "string" -> JSONObject.quote(value?.toString().orEmpty())
    "uint", "int" -> (value as? Number)?.toLong()?.toString() ?: value?.toString()
    "float" -> (value as? Number)?.toDouble()?.toString() ?: value?.toString()
    else -> null
}

private fun collect(node: JSONObject, prefix: String, out: MutableMap<String, Pair<String, Any?>>) {
    val keys = node.keys()
    while (keys.hasNext()) {
        val key = keys.next()
        val child = node.optJSONObject(key) ?: continue
        val path = if (prefix.isEmpty()) key else "$prefix@@$key"
        val type = child.optString("type", "")

        if (type.isEmpty()) {
            collect(child, path, out)
        } else {
            out[path] = type to child.opt("value")
        }
    }
}

internal fun revertTo(baseline: JSONObject, current: JSONObject, titleId: String): Int {
    val before = mutableMapOf<String, Pair<String, Any?>>()
    val after = mutableMapOf<String, Pair<String, Any?>>()
    collect(baseline, "", before)
    collect(current, "", after)

    var reverted = 0

    before.forEach { (path, entry) ->
        val (type, oldValue) = entry
        val newValue = after[path]?.second

        if (oldValue?.toString() == newValue?.toString()) {
            return@forEach
        }

        val encoded = encode(type, oldValue) ?: return@forEach

        if (RPCS3.instance.settingsSet(path, encoded, titleId)) {
            reverted++
        }
    }

    if (reverted > 0) {
        RPCS3.instance.settingsFlush()
    }

    return reverted
}
