package net.rpcs3.ui.settings

import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.DocumentsContract
import android.util.Log
import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Share
import androidx.compose.material.icons.outlined.Build
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
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.documentfile.provider.DocumentFile
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.provider.AppDataDocumentProvider
import net.rpcs3.ui.common.ComposePreview
import net.rpcs3.ui.components.PaneNavRow
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.ui.settings.components.core.PreferenceIcon
import net.rpcs3.ui.settings.components.core.PreferenceValue
import net.rpcs3.ui.settings.components.preference.HomePreference
import net.rpcs3.ui.settings.components.preference.RegularPreference
import net.rpcs3.ui.settings.components.preference.SingleSelectionDialog
import net.rpcs3.ui.settings.components.preference.SliderPreference
import net.rpcs3.ui.settings.components.preference.SwitchPreference
import net.rpcs3.ui.settings.components.preference.TextPreference
import net.rpcs3.ui.theme.Dims
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
                                            text = "Search settings...",
                                            color = MaterialTheme.colorScheme.onSurfaceVariant
                                        )
                                    }
                                    innerTextField()
                                }
                            }
                        )
                    } else {
                        Text(
                            text = if (titlePath.isEmpty()) "Advanced Settings" else titlePath,
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
                            contentDescription = if (isSearching) "Close Search" else "Search"
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
                                           AlertDialogQueue.showDialog("Setting error", "Failed to assign $itemPath value $value")
                                        } else {
                                            itemObject.put("value", value)
                                            itemValue = value
                                        }
                                   },
                                   onLongClick = {
                                        AlertDialogQueue.showDialog(
                                            title = "Reset Setting",
                                            message = "Do you want to reset '$key' to its default value?",
                                            onConfirm = {
                                                if (RPCS3.instance.settingsSet(itemPath, def.toString(), RPCS3.settingsTitleId.value)) {
                                                    itemObject.put("value", def)
                                                    itemValue = def
                                                } else {
                                                    AlertDialogQueue.showDialog("Setting error", "Failed to reset $key")
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
                                            AlertDialogQueue.showDialog("Setting error", "Failed to assign $itemPath value $value")
                                        } else {
                                            itemObject.put("value", value)
                                            itemValue = value
                                        }
                                    },
                                    onLongClick = {
                                        AlertDialogQueue.showDialog(
                                            title = "Reset Setting",
                                            message = "Do you want to reset '$key' to its default value?",
                                            onConfirm = {
                                                if (RPCS3.instance.settingsSet(itemPath, "\"" + def + "\"", RPCS3.settingsTitleId.value)) {
                                                    itemObject.put("value", def)
                                                    itemValue = def
                                                } else {
                                                    AlertDialogQueue.showDialog("Setting error", "Failed to reset $key")
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
                                                    "Setting error",
                                                    "Failed to assign $itemPath value $value"
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
                                                title = "Reset Setting",
                                                message = "Do you want to reset '$key' to its default value?",
                                                onConfirm = {
                                                    if (RPCS3.instance.settingsSet(itemPath, def.toString(), RPCS3.settingsTitleId.value)) {
                                                        itemObject.put("value", def)
                                                        itemValue = def
                                                    } else {
                                                        AlertDialogQueue.showDialog("Setting error", "Failed to reset $key")
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
                                                    "Setting error",
                                                    "Failed to assign $itemPath value $value"
                                                )
                                            } else {
                                                itemObject.put("value", value.toDouble().toString())
                                                itemValue = value.toDouble()
                                            }
                                        },
                                        valueContent = { PreferenceValue(text = itemValue.toString()) },
                                        onLongClick = {
                                            AlertDialogQueue.showDialog(
                                                title = "Reset Setting",
                                                message = "Do you want to reset '$key' to its default value?",
                                                onConfirm = {
                                                    if (RPCS3.instance.settingsSet(itemPath, def.toString(), RPCS3.settingsTitleId.value)) {
                                                        itemObject.put("value", def)
                                                        itemValue = def
                                                    } else {
                                                        AlertDialogQueue.showDialog("Setting error", "Failed to reset $key")
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
                                            AlertDialogQueue.showDialog("Setting error", "Failed to assign $itemPath value $value")
                                        } else {
                                            itemObject.put("value", value)
                                            itemValue = value
                                        }
                                    },
                                    onLongClick = {
                                        AlertDialogQueue.showDialog(
                                            title = "Reset Setting",
                                            message = "Do you want to reset '$key' to its default value?",
                                            onConfirm = {
                                                if (RPCS3.instance.settingsSet(itemPath, JSONObject.quote(def), RPCS3.settingsTitleId.value)) {
                                                    itemObject.put("value", def)
                                                    itemValue = def
                                                } else {
                                                    AlertDialogQueue.showDialog("Setting error", "Failed to reset $key")
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


@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    modifier: Modifier = Modifier,
    navigateBack: () -> Unit,
    navigateTo: (path: String) -> Unit,
) {
    val context = LocalContext.current
    var selected by remember { mutableIntStateOf(0) }

    val tabs = listOf(
        PaneTab("General", Icons.Default.Settings),
        PaneTab("Graphics", Icons.Outlined.Build),
        PaneTab("Support", Icons.Default.Share)
    )

    PaneScaffold(
        title = "Settings",
        tabs = tabs,
        selected = selected,
        onSelect = { selected = it },
        onBack = navigateBack,
        modifier = modifier
    ) {
        when (selected) {
            0 -> {
                PaneSectionTitle("Emulator")
                PaneNavRow(
                    title = "Advanced Settings",
                    description = "Configure emulator advanced settings",
                    icon = Icons.Default.Settings,
                    onClick = { navigateTo("gameSettings?titleId=") }
                )
                Spacer(Modifier.height(Dims.RowSpacing))
                PaneNavRow(
                    title = "View Internal Directory",
                    description = "Open the RPCS3 directory in a file manager",
                    icon = Icons.Default.Folder,
                    onClick = {
                        context.launchBrowseIntent(Intent.ACTION_VIEW) or context.launchBrowseIntent()
                    }
                )
            }

            1 -> {
                PaneSectionTitle("Renderer")
                PaneNavRow(
                    title = "Custom GPU Driver",
                    description = "Install alternative drivers for better performance or accuracy",
                    icon = Icons.Outlined.Build,
                    onClick = {
                        if (RPCS3.instance.supportsCustomDriverLoading()) {
                            navigateTo("drivers")
                        } else {
                            AlertDialogQueue.showDialog(
                                title = "Custom drivers not supported",
                                message = "Custom driver loading isn't currently supported for this device",
                                confirmText = "Close",
                                dismissText = ""
                            )
                        }
                    }
                )
            }

            else -> {
                PaneSectionTitle("Troubleshooting")
                PaneNavRow(
                    title = "Share Log",
                    description = "Share the RPCS3 log file to debug issues",
                    icon = Icons.Default.Share,
                    onClick = {
                        val file = DocumentFile.fromSingleUri(
                            context,
                            DocumentsContract.buildDocumentUri(
                                AppDataDocumentProvider.AUTHORITY,
                                "${AppDataDocumentProvider.ROOT_ID}/cache/RPCS3.log"
                            )
                        )

                        if (file != null && file.exists() && file.length() != 0L) {
                            val intent = Intent(Intent.ACTION_SEND).apply {
                                setDataAndType(file.uri, "text/plain")
                                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                                putExtra(Intent.EXTRA_STREAM, file.uri)
                            }
                            context.startActivity(Intent.createChooser(intent, "Share Log File"))
                        } else {
                            Toast.makeText(context, "Log file not found!", Toast.LENGTH_SHORT).show()
                        }
                    }
                )
                Spacer(Modifier.height(Dims.RowSpacing))
                PaneNavRow(
                    title = "Diagnostics",
                    description = "Check setup, storage and graphics status",
                    icon = Icons.Default.Info,
                    onClick = { navigateTo("diagnostics") }
                )
            }
        }

        Spacer(Modifier.height(16.dp))
    }
}

@Preview
@Composable
private fun SettingsScreenPreview() {
    ComposePreview {
//        SettingsScreen {}
    }
}

private fun Context.launchBrowseIntent(
    action: String = "android.provider.action.BROWSE"
): Boolean {
    return try {
        val intent = Intent(action).apply {
            addCategory(Intent.CATEGORY_DEFAULT)
            data = DocumentsContract.buildRootUri(
                AppDataDocumentProvider.AUTHORITY,
                AppDataDocumentProvider.ROOT_ID
            )
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION or Intent.FLAG_GRANT_PREFIX_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivity(intent)
        true
    } catch (_: ActivityNotFoundException) {
        println("No activity found to handle $action intent")
        false
    }
}
