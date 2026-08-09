package net.rpcs3.ui.settings

import android.util.Log
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LargeTopAppBar
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableDoubleStateOf
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.ui.settings.components.core.PreferenceValue
import net.rpcs3.ui.settings.components.preference.RegularPreference
import net.rpcs3.ui.settings.components.preference.SingleSelectionDialog
import net.rpcs3.ui.settings.components.preference.SliderPreference
import net.rpcs3.ui.settings.components.preference.SwitchPreference
import net.rpcs3.ui.settings.components.preference.TextPreference
import org.json.JSONObject

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AdvancedSettingsScreen(
    modifier: Modifier = Modifier,
    navigateBack: () -> Unit,
    navigateTo: (path: String) -> Unit,
    settings: JSONObject,
    path: String = ""
) {
    val context = LocalContext.current
    val settingValue = remember { mutableStateOf(settings) }
    var searchQuery by remember { mutableStateOf("") }
    var isSearching by remember { mutableStateOf(false) }

    val topBarScrollBehavior = TopAppBarDefaults.enterAlwaysScrollBehavior()
    Scaffold(
        modifier = Modifier
            .nestedScroll(topBarScrollBehavior.nestedScrollConnection)
            .then(modifier),
        topBar = {
            val titlePath = path.replace("@@", " / ")
            LargeTopAppBar(
                title = {
                    if (isSearching) {
                        BasicTextField(
                            value = searchQuery,
                            onValueChange = { searchQuery = it },
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 8.dp),
                            singleLine = true,
                            textStyle = TextStyle(
                                color = MaterialTheme.colorScheme.onSurface,
                                fontSize = 20.sp
                            ),
                            decorationBox = { innerTextField ->
                                Box(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .background(
                                            MaterialTheme.colorScheme.surfaceVariant,
                                            shape = RoundedCornerShape(8.dp)
                                        )
                                        .padding(8.dp)
                                ) {
                                    if (searchQuery.isEmpty()) {
                                        Text(
                                            text = stringResource(R.string.advanced_search_placeholder),
                                            color = MaterialTheme.colorScheme.onSurfaceVariant
                                        )
                                    }
                                    innerTextField()
                                }
                            }
                        )
                    } else {
                        Text(
                            text = titlePath.ifEmpty {
                                stringResource(R.string.advanced_title)
                            },
                            fontWeight = FontWeight.Medium
                        )
                    }
                },
                scrollBehavior = topBarScrollBehavior,
                navigationIcon = {
                    IconButton(onClick = navigateBack) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Default.KeyboardArrowLeft,
                            contentDescription = null
                        )
                    }
                },
                actions = {
                    IconButton(
                        onClick = {
                            if (isSearching) {
                                searchQuery = ""
                                isSearching = false
                            } else {
                                isSearching = true
                            }
                        }
                    ) {
                        Icon(
                            imageVector = if (isSearching) Icons.Default.Close else Icons.Default.Search,
                            contentDescription = stringResource(
                                if (isSearching) {
                                    R.string.advanced_search_close
                                } else {
                                    R.string.advanced_search
                                }
                            )
                        )
                    }
                }
            )
        }
    ) { contentPadding ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(contentPadding),
        ) {
            val filteredKeys = settings.keys().asSequence()
                .filter { it.contains(searchQuery, ignoreCase = true) }
                .toList()
                
            filteredKeys.forEach { key ->
                val itemPath = "$path@@$key"
                item(key = key) {
                    val itemObject = settingValue.value[key] as? JSONObject

                    if (itemObject != null) {
                        when (val type = if (itemObject.has("type")) itemObject.getString("type") else null) {
                             null -> {
                                RegularPreference(
                                    title = key,
                                    leadingIcon = null,
                                    onClick = {
                                        navigateTo("settings$itemPath")
                                    }
                                )  
                            }

                            "bool" -> {
                                var itemValue by remember {  mutableStateOf(itemObject.getBoolean("value"))  }
                                val def = itemObject.getBoolean("default")
                                SwitchPreference (
                                    checked = itemValue,
                                    title = key + if (itemValue == def) "" else " *",
                                    leadingIcon = null,
                                    onClick = { value ->
                                        if (!RPCS3.instance.settingsSet(itemPath, if (value) "true" else "false", RPCS3.settingsTitleId.value)) {
                                           AlertDialogQueue.showDialog(
                                               context.getString(R.string.settings_error_title),
                                               context.getString(
                                                   R.string.settings_error_assign_value,
                                                   itemPath,
                                                   value
                                               )
                                           )
                                        } else {
                                            itemObject.put("value", value)
                                            itemValue = value
                                        }
                                   },
                                   onLongClick = {
                                        AlertDialogQueue.showDialog(
                                            title = context.getString(
                                                R.string.settings_reset_item_title
                                            ),
                                            message = context.getString(
                                                R.string.settings_reset_item_message,
                                                key
                                            ),
                                            onConfirm = {
                                                if (RPCS3.instance.settingsSet(itemPath, def.toString(), RPCS3.settingsTitleId.value)) {
                                                    itemObject.put("value", def)
                                                    itemValue = def
                                                } else {
                                                    AlertDialogQueue.showDialog(
                                                        context.getString(
                                                            R.string.settings_error_title
                                                        ),
                                                        context.getString(
                                                            R.string.settings_error_reset,
                                                            key
                                                        )
                                                    )
                                                }
                                            }
                                        )
                                    }
                                )
                            }

                            "enum" -> {
                                var itemValue by remember {  mutableStateOf(itemObject.getString("value"))  }
                                val def = itemObject.getString("default")
                                val variantsJson = itemObject.getJSONArray("variants")
                                val variants = ArrayList<String>()
                                for (i in 0..<variantsJson.length()) {
                                    variants.add(variantsJson.getString(i))
                                }

                                SingleSelectionDialog(
                                    currentValue = if (itemValue in variants) itemValue else variants[0],
                                    values = variants,
                                    icon = null,
                                    title = key + if (itemValue == def) "" else " *",
                                    onValueChange = {
                                            value ->
                                        if (!RPCS3.instance.settingsSet(itemPath, "\"" + value + "\"", RPCS3.settingsTitleId.value)) {
                                            AlertDialogQueue.showDialog(
                                               context.getString(R.string.settings_error_title),
                                               context.getString(
                                                   R.string.settings_error_assign_value,
                                                   itemPath,
                                                   value
                                               )
                                           )
                                        } else {
                                            itemObject.put("value", value)
                                            itemValue = value
                                        }
                                    },
                                    onLongClick = {
                                        AlertDialogQueue.showDialog(
                                            title = context.getString(
                                                R.string.settings_reset_item_title
                                            ),
                                            message = context.getString(
                                                R.string.settings_reset_item_message,
                                                key
                                            ),
                                            onConfirm = {
                                                if (RPCS3.instance.settingsSet(itemPath, "\"" + def + "\"", RPCS3.settingsTitleId.value)) {
                                                    itemObject.put("value", def)
                                                    itemValue = def
                                                } else {
                                                    AlertDialogQueue.showDialog(
                                                        context.getString(
                                                            R.string.settings_error_title
                                                        ),
                                                        context.getString(
                                                            R.string.settings_error_reset,
                                                            key
                                                        )
                                                    )
                                                }
                                            }
                                        )
                                    }
                                )
                            }

                            "uint", "int" -> {
                                var max = 0L
                                var min = 0L
                                var initialItemValue = 0L
                                var def = 0L
                                try {
                                    initialItemValue = itemObject.getString("value").toLong()
                                    max = itemObject.getString("max").toLong()
                                    min = itemObject.getString("min").toLong()
                                    def = itemObject.getString("default").toLong()
                                } catch (e: Exception) {
                                    e.printStackTrace()
                                }
                                var itemValue by remember { mutableLongStateOf(initialItemValue) }
                                if (min < max) {
                                    SliderPreference(
                                        value = itemValue.toFloat(),
                                        valueRange = min.toFloat()..max.toFloat(),
                                        title = key + if (itemValue == def) "" else " *",
                                        steps = (max - min).toInt(),
                                        onValueChange = { value ->
                                            if (!RPCS3.instance.settingsSet(
                                                    itemPath,
                                                    value.toLong().toString(),
                                                    RPCS3.settingsTitleId.value
                                                )
                                            ) {
                                                AlertDialogQueue.showDialog(
                                                    context.getString(
                                                        R.string.settings_error_title
                                                    ),
                                                    context.getString(
                                                        R.string.settings_error_assign_value,
                                                        itemPath,
                                                        value
                                                    )
                                                )
                                            } else {
                                                itemObject.put(
                                                    "value",
                                                    value.toLong().toString()
                                                )
                                                itemValue = value.toLong()
                                            }
                                        },
                                        valueContent = { PreferenceValue(text = itemValue.toString()) },
                                        onLongClick = {
                                            AlertDialogQueue.showDialog(
                                                title = context.getString(
                                                    R.string.settings_reset_item_title
                                                ),
                                                message = context.getString(
                                                    R.string.settings_reset_item_message,
                                                    key
                                                ),
                                                onConfirm = {
                                                    if (RPCS3.instance.settingsSet(itemPath, def.toString(), RPCS3.settingsTitleId.value)) {
                                                        itemObject.put("value", def)
                                                        itemValue = def
                                                    } else {
                                                        AlertDialogQueue.showDialog(
                                                        context.getString(
                                                            R.string.settings_error_title
                                                        ),
                                                        context.getString(
                                                            R.string.settings_error_reset,
                                                            key
                                                        )
                                                    )
                                                    }
                                                }
                                            )
                                        }
                                    )
                                }
                            }

                            "float" -> {
                                var itemValue by remember {  mutableDoubleStateOf(itemObject.getString("value").toDouble())  }
                                val max = if (itemObject.has("max"))  itemObject.getString("max").toDouble() else 0.0
                                val min =  if (itemObject.has("min"))  itemObject.getString("min").toDouble() else 0.0
                                val def =  if (itemObject.has("default"))  itemObject.getString("default").toDouble() else 0.0

                                if (min < max) {
                                    SliderPreference(
                                        value = itemValue.toFloat(),
                                        valueRange = min.toFloat()..max.toFloat(),
                                        title = key + if (itemValue == def) "" else " *",
                                        steps = (max - min + 1).toInt(),
                                        onValueChange = { value ->
                                            if (!RPCS3.instance.settingsSet(
                                                    itemPath,
                                                    value.toString(),
                                                    RPCS3.settingsTitleId.value
                                                )
                                            ) {
                                                AlertDialogQueue.showDialog(
                                                    context.getString(
                                                        R.string.settings_error_title
                                                    ),
                                                    context.getString(
                                                        R.string.settings_error_assign_value,
                                                        itemPath,
                                                        value
                                                    )
                                                )
                                            } else {
                                                itemObject.put("value", value.toDouble().toString())
                                                itemValue = value.toDouble()
                                            }
                                        },
                                        valueContent = { PreferenceValue(text = itemValue.toString()) },
                                        onLongClick = {
                                            AlertDialogQueue.showDialog(
                                                title = context.getString(
                                                    R.string.settings_reset_item_title
                                                ),
                                                message = context.getString(
                                                    R.string.settings_reset_item_message,
                                                    key
                                                ),
                                                onConfirm = {
                                                    if (RPCS3.instance.settingsSet(itemPath, def.toString(), RPCS3.settingsTitleId.value)) {
                                                        itemObject.put("value", def)
                                                        itemValue = def
                                                    } else {
                                                        AlertDialogQueue.showDialog(
                                                        context.getString(
                                                            R.string.settings_error_title
                                                        ),
                                                        context.getString(
                                                            R.string.settings_error_reset,
                                                            key
                                                        )
                                                    )
                                                    }
                                                }
                                            )
                                        }
                                    )
                                }
                            }

                            "string" -> {
                                var itemValue by remember { mutableStateOf(itemObject.getString("value")) }
                                val def = itemObject.getString("default")
                                TextPreference(
                                    title = key + if (itemValue == def) "" else " *",
                                    value = itemValue,
                                    placeholder = def,
                                    leadingIcon = null,
                                    onValueChange = { value ->
                                        if (!RPCS3.instance.settingsSet(itemPath, JSONObject.quote(value), RPCS3.settingsTitleId.value)) {
                                            AlertDialogQueue.showDialog(
                                               context.getString(R.string.settings_error_title),
                                               context.getString(
                                                   R.string.settings_error_assign_value,
                                                   itemPath,
                                                   value
                                               )
                                           )
                                        } else {
                                            itemObject.put("value", value)
                                            itemValue = value
                                        }
                                    },
                                    onLongClick = {
                                        AlertDialogQueue.showDialog(
                                            title = context.getString(
                                                R.string.settings_reset_item_title
                                            ),
                                            message = context.getString(
                                                R.string.settings_reset_item_message,
                                                key
                                            ),
                                            onConfirm = {
                                                if (RPCS3.instance.settingsSet(itemPath, JSONObject.quote(def), RPCS3.settingsTitleId.value)) {
                                                    itemObject.put("value", def)
                                                    itemValue = def
                                                } else {
                                                    AlertDialogQueue.showDialog(
                                                        context.getString(
                                                            R.string.settings_error_title
                                                        ),
                                                        context.getString(
                                                            R.string.settings_error_reset,
                                                            key
                                                        )
                                                    )
                                                }
                                            }
                                        )
                                    }
                                )
                            }

                            else -> {
                                Log.e("Main", "Unimplemented setting type $type")
                            }
                        }
                    }
                }
            }
        }
    }
}
