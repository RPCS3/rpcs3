package net.rpcs3.ui.settings

import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import net.rpcs3.dialogs.AlertDialogQueue
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.RPCS3
import net.rpcs3.ui.components.SettingDropdown
import net.rpcs3.ui.components.SettingSlider
import net.rpcs3.ui.components.InfoRow
import net.rpcs3.ui.components.SettingSwitch
import net.rpcs3.ui.components.SettingTextField
import org.json.JSONArray
import org.json.JSONObject

private fun variantsOf(item: JSONObject): List<String> {
    val array: JSONArray = item.optJSONArray("variants") ?: return emptyList()
    return List(array.length()) { array.optString(it, "") }
}

private val settingsWriter = CoroutineScope(SupervisorJob() + Dispatchers.IO)

private fun commit(path: String, value: String, titleId: String, label: String): Boolean {
    settingsWriter.launch {
        if (!RPCS3.instance.settingsSet(path, value, titleId)) {
            withContext(Dispatchers.Main) {
                AlertDialogQueue.showDialog("Setting error", "Failed to assign $label")
            }
            return@launch
        }

        RPCS3.instance.settingsFlush()

        RPCS3.instance.takeSettingsSaveFailure()?.let { target ->
            withContext(Dispatchers.Main) {
                AlertDialogQueue.showDialog(
                    "Settings not saved",
                    "PS3Native could not write $target. The change will be lost when you " +
                        "close the app."
                )
            }
        }
    }
    return true
}

@Composable
fun SettingItem(
    label: String,
    item: JSONObject,
    path: String,
    titleId: String,
    modifier: Modifier = Modifier
) {
    when (item.optString("type", "")) {
        "bool" -> {
            var value by remember(path) { mutableStateOf(item.optBoolean("value")) }
            val default = item.optBoolean("default")
            SettingSwitch(
                label = label + if (value == default) "" else " *",
                checked = value,
                modifier = modifier,
                onCheckedChange = { next ->
                    if (commit(path, if (next) "true" else "false", titleId, label)) {
                        item.put("value", next)
                        value = next
                    }
                }
            )
        }

        "enum" -> {
            val variants = remember(path) { variantsOf(item) }
            var value by remember(path) { mutableStateOf(item.optString("value")) }
            val default = item.optString("default")
            val index = variants.indexOf(value).coerceAtLeast(0)
            SettingDropdown(
                label = label + if (value == default) "" else " *",
                entries = variants,
                selectedIndex = index,
                modifier = modifier,
                onSelected = { next ->
                    val chosen = variants.getOrNull(next) ?: return@SettingDropdown
                    if (commit(path, JSONObject.quote(chosen), titleId, label)) {
                        item.put("value", chosen)
                        value = chosen
                    }
                }
            )
        }

        "uint", "int" -> {
            val min = item.optLong("min", 0L)
            val max = item.optLong("max", 100L)
            var value by remember(path) { mutableFloatStateOf(item.optLong("value").toFloat()) }
            val default = item.optLong("default")

            if (min < max) {
                SettingSlider(
                    label = label + if (value.toLong() == default) "" else " *",
                    value = value,
                    valueRange = min.toFloat()..max.toFloat(),
                    valueText = value.toLong().toString(),
                    modifier = modifier,
                    onValueChange = { value = it },
                    onValueChangeFinished = {
                        val next = value.toLong()
                        if (commit(path, next.toString(), titleId, label)) {
                            item.put("value", next)
                        }
                    }
                )
            } else {
                var text by remember(path) { mutableStateOf(item.optLong("value").toString()) }
                SettingTextField(
                    label = label,
                    value = text,
                    modifier = modifier,
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                    onValueChange = { next ->
                        text = next
                        next.toLongOrNull()?.let {
                            if (commit(path, it.toString(), titleId, label)) {
                                item.put("value", it)
                            }
                        }
                    }
                )
            }
        }

        "float" -> {
            val min = item.optDouble("min", 0.0).toFloat()
            val max = item.optDouble("max", 1.0).toFloat()
            var value by remember(path) { mutableFloatStateOf(item.optDouble("value").toFloat()) }
            val default = item.optDouble("default").toFloat()

            SettingSlider(
                label = label + if (value == default) "" else " *",
                value = value,
                valueRange = min..max,
                valueText = String.format("%.2f", value),
                modifier = modifier,
                onValueChange = { value = it },
                onValueChangeFinished = {
                    if (commit(path, value.toString(), titleId, label)) {
                        item.put("value", value.toDouble())
                    }
                }
            )
        }

        "string" -> {
            var value by remember(path) { mutableStateOf(item.optString("value")) }
            val default = item.optString("default")
            SettingTextField(
                label = label + if (value == default) "" else " *",
                value = value,
                placeholder = default,
                modifier = modifier,
                onValueChange = { next ->
                    value = next
                    if (commit(path, JSONObject.quote(next), titleId, label)) {
                        item.put("value", next)
                    }
                }
            )
        }

        else -> {
            val raw = item.opt("value")?.toString().orEmpty()
            if (raw.isNotEmpty()) {
                InfoRow(label = label, value = raw.take(80))
            }
        }
    }
}
