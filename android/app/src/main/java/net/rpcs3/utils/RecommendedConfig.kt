package net.rpcs3.utils

import android.content.Context
import android.util.Log
import androidx.core.content.edit
import net.rpcs3.RPCS3
import org.json.JSONArray
import org.json.JSONObject

private const val TAG = "RecommendedConfig"
private const val ASSET_NAME = "recommended_configs.json"
private const val PREFS_NAME = "RecommendedConfig"
private const val SOURCE_PREFIX = "source_"
private const val SEEDED_PREFIX = "seeded_"

enum class ConfigSource {
    Recommended,
    Global;

    val tag: String
        get() = if (this == Recommended) "recommended" else "global"

    companion object {
        fun fromTag(tag: String?) = if (tag == "recommended") Recommended else Global
    }
}

data class RecommendedEntry(
    val titleId: String,
    val settings: Map<String, Any>
) {
    val isEmpty: Boolean get() = settings.isEmpty()
}

data class ConfigApplyResult(
    val applied: Int,
    val skipped: List<String>
)

object RecommendedConfigs {
    private var loaded = false
    private var games: Map<String, Int> = emptyMap()
    private var configs: List<Map<String, Any>> = emptyList()

    @Synchronized
    private fun load(context: Context) {
        if (loaded) {
            return
        }

        loaded = true

        runCatching {
            val raw = context.assets.open(ASSET_NAME).use { it.readBytes().decodeToString() }
            val root = JSONObject(raw)

            val parsedConfigs = ArrayList<Map<String, Any>>()
            val array = root.optJSONArray("configs") ?: JSONArray()
            for (index in 0 until array.length()) {
                val item = array.optJSONObject(index) ?: continue
                val settings = LinkedHashMap<String, Any>()
                val keys = item.keys()
                while (keys.hasNext()) {
                    val key = keys.next()
                    val value = item.opt(key) ?: continue
                    settings[key] = value
                }
                parsedConfigs.add(settings)
            }

            val parsedGames = HashMap<String, Int>()
            val entries = root.optJSONObject("games") ?: JSONObject()
            val ids = entries.keys()
            while (ids.hasNext()) {
                val id = ids.next()
                parsedGames[id] = entries.optInt(id, -1)
            }

            configs = parsedConfigs
            games = parsedGames
        }.onFailure {
            Log.e(TAG, "Failed to load $ASSET_NAME", it)
        }
    }

    private fun prefs(context: Context) =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    fun isListed(context: Context, titleId: String): Boolean {
        load(context)
        return titleId.isNotEmpty() && games.containsKey(titleId.uppercase())
    }

    fun entryFor(context: Context, titleId: String): RecommendedEntry? {
        load(context)

        if (titleId.isEmpty()) {
            return null
        }

        val index = games[titleId.uppercase()] ?: return null

        if (index < 0 || index >= configs.size) {
            return RecommendedEntry(titleId, emptyMap())
        }

        return RecommendedEntry(titleId, configs[index])
    }

    fun sourceOf(context: Context, titleId: String): ConfigSource {
        if (titleId.isEmpty()) {
            return ConfigSource.Global
        }

        return ConfigSource.fromTag(prefs(context).getString(SOURCE_PREFIX + titleId, null))
    }

    fun setSource(context: Context, titleId: String, source: ConfigSource): ConfigApplyResult {
        if (titleId.isEmpty()) {
            return ConfigApplyResult(0, emptyList())
        }

        prefs(context).edit {
            putString(SOURCE_PREFIX + titleId, source.tag)
            putBoolean(SEEDED_PREFIX + titleId, true)
        }

        return applyCurrent(context, titleId)
    }

    fun applyCurrent(context: Context, titleId: String): ConfigApplyResult {
        if (titleId.isEmpty()) {
            return ConfigApplyResult(0, emptyList())
        }

        runCatching { RPCS3.instance.settingsResetCustom(titleId) }

        val result = if (sourceOf(context, titleId) == ConfigSource.Recommended) {
            entryFor(context, titleId)?.let { write(it) } ?: ConfigApplyResult(0, emptyList())
        } else {
            Log.i(TAG, "$titleId: cleared custom config, following global settings")
            ConfigApplyResult(0, emptyList())
        }

        DriverSelection.apply(context, titleId)
        DriverFlags.apply(context, titleId)

        return result
    }

    fun seedIfNeeded(context: Context, titleId: String) {
        if (titleId.isEmpty() || prefs(context).getBoolean(SEEDED_PREFIX + titleId, false)) {
            return
        }

        val entry = entryFor(context, titleId)

        if (entry == null || entry.isEmpty || DriverSelection.customConfigExists(titleId)) {
            prefs(context).edit { putBoolean(SEEDED_PREFIX + titleId, true) }
            return
        }

        prefs(context).edit {
            putString(SOURCE_PREFIX + titleId, ConfigSource.Recommended.tag)
            putBoolean(SEEDED_PREFIX + titleId, true)
        }

        write(entry)
        DriverSelection.apply(context, titleId)
        DriverFlags.apply(context, titleId)
    }

    private fun write(entry: RecommendedEntry): ConfigApplyResult {
        if (entry.isEmpty) {
            return ConfigApplyResult(0, emptyList())
        }

        val tree = runCatching {
            JSONObject(RPCS3.instance.settingsGet("", entry.titleId))
        }.getOrNull() ?: return ConfigApplyResult(0, entry.settings.keys.toList())

        var applied = 0
        val skipped = ArrayList<String>()

        entry.settings.forEach { (path, value) ->
            val leaf = leafAt(tree, path)

            if (leaf == null) {
                skipped.add(path)
                return@forEach
            }

            val encoded = encode(leaf, value)

            if (encoded == null) {
                skipped.add(path)
                return@forEach
            }

            if (encoded == current(leaf)) {
                return@forEach
            }

            if (RPCS3.instance.settingsSet(path, encoded, entry.titleId)) {
                applied++
            } else {
                skipped.add(path)
            }
        }

        if (applied > 0) {
            RPCS3.instance.settingsFlush()
        }

        Log.i(
            TAG,
            "${entry.titleId}: applied $applied of ${entry.settings.size} recommended settings" +
                if (skipped.isEmpty()) "" else ", skipped $skipped"
        )

        return ConfigApplyResult(applied, skipped)
    }

    private fun leafAt(tree: JSONObject, path: String): JSONObject? {
        var cursor: JSONObject? = tree

        path.split("@@").forEach { step ->
            cursor = cursor?.optJSONObject(step)
        }

        return cursor?.takeIf { it.optString("type", "").isNotEmpty() }
    }

    private fun current(leaf: JSONObject): String? = when (leaf.optString("type", "")) {
        "bool" -> if (leaf.optBoolean("value")) "true" else "false"
        "enum", "string" -> JSONObject.quote(leaf.optString("value"))
        "uint", "int" -> leaf.optLong("value").toString()
        "float" -> leaf.optDouble("value").toString()
        "set" -> leaf.optJSONArray("value")?.toString()
        else -> null
    }

    private fun encode(leaf: JSONObject, value: Any): String? =
        when (leaf.optString("type", "")) {
            "bool" -> (value as? Boolean)?.let { if (it) "true" else "false" }

            "enum" -> {
                val text = value.toString()
                val variants = leaf.optJSONArray("variants")
                val known = variants != null &&
                    (0 until variants.length()).any { variants.optString(it) == text }
                if (known) JSONObject.quote(text) else null
            }

            "string" -> JSONObject.quote(value.toString())

            "uint", "int" -> {
                val number = (value as? Number)?.toLong() ?: value.toString().toLongOrNull()
                val min = leaf.optLong("min", Long.MIN_VALUE)
                val max = leaf.optLong("max", Long.MAX_VALUE)
                number?.takeIf { it in min..max }?.toString()
            }

            "float" -> ((value as? Number)?.toDouble() ?: value.toString().toDoubleOrNull())
                ?.toString()

            "set" -> (value as? JSONArray)?.takeIf { it.length() > 0 }?.toString()

            else -> null
        }
}
