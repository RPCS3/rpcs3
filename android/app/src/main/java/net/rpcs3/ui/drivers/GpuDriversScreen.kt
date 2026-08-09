package net.rpcs3.ui.drivers

import android.content.Context
import android.content.pm.ApplicationInfo
import android.content.res.Configuration
import android.net.Uri
import android.util.Log
import android.widget.Toast
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.animateContentSize
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarDuration
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.SwipeToDismissBox
import androidx.compose.material3.SwipeToDismissBoxValue
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.rememberSwipeToDismissBoxState
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.BasicAlertDialog
import androidx.compose.material3.RadioButton
import androidx.compose.material3.TextButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
import androidx.compose.material3.Divider
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.Alignment
import androidx.core.content.edit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import androidx.compose.material.icons.outlined.Memory
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.ui.components.PaneActionButton
import net.rpcs3.ui.settings.DriverFlagsSection
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.utils.GpuDriverHelper
import net.rpcs3.utils.GpuDriverInstallResult
import net.rpcs3.utils.GpuDriverMetadata
import net.rpcs3.utils.DriversFetcher
import net.rpcs3.utils.DriversFetcher.FetchResult
import net.rpcs3.utils.DriversFetcher.FetchResultOutput
import net.rpcs3.utils.DriversFetcher.DownloadResult
import java.io.File
import java.io.FileInputStream

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GpuDriversScreen(navigateBack: () -> Unit) {
    val context = LocalContext.current
    var drivers by remember { mutableStateOf(GpuDriverHelper.getInstalledDrivers(context)) }
    var selectedDriver by remember { mutableStateOf<String?>(null) }
    val prefs = remember { context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE) }
    var isInstalling by remember { mutableStateOf(false) }
    val coroutineScope = rememberCoroutineScope()
    val snackbarHostState = remember { SnackbarHostState() }
    val topBarScrollBehavior = TopAppBarDefaults.enterAlwaysScrollBehavior()
    var showDriverDialog by remember { mutableStateOf(false) }
    var shouldHandleGpuDriverImport by remember { mutableStateOf(false) }
    var shouldFetchAndShowDrivers by remember { mutableStateOf(false) }
    var repoUrl by remember { mutableStateOf<String?>(null) }
    var driverToDownload by remember { mutableStateOf<Pair<String, String>?>(null) }
    var shouldDownloadDriver by remember { mutableStateOf(false) }
    
    val driverPickerLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        uri?.let {
            isInstalling = true
            coroutineScope.launch(Dispatchers.IO) {
                try {
                    context.contentResolver.openInputStream(it)?.use { stream ->
                        val result = GpuDriverHelper.installDriver(context, stream)
                        if (result == GpuDriverInstallResult.Success) {
                            val updatedDrivers = GpuDriverHelper.getInstalledDrivers(context)
                            withContext(Dispatchers.Main) {
                                drivers = updatedDrivers
                            }
                        }
                        withContext(Dispatchers.Main) {
                            isInstalling = false
                            snackbarHostState.showSnackbar(
                                message = GpuDriverHelper.resolveInstallResultToString(
                                    context,
                                    result
                                ),
                                actionLabel = context.getString(R.string.action_dismiss),
                                duration = SnackbarDuration.Short
                            )
                        }
                    }
                } catch (e: Exception) {
                    Log.e("GpuDriver", "Error installing driver: ${e.message}")
                }
            }
        }
    }

    selectedDriver = prefs.getString("selected_gpu_driver", "Default")

    if (showDriverDialog) {
        DriverDialog(
            onDismiss = { showDriverDialog = false },
            onInstallClick = {
                driverPickerLauncher.launch("application/zip")
            },
            onImportClick = {
                shouldHandleGpuDriverImport = true
            }
        )
    }

    if (shouldHandleGpuDriverImport) {
        handleGpuDriverImport(
            onDismiss = { shouldHandleGpuDriverImport = false },
            onFetchClick = { url ->
                repoUrl = url
                shouldFetchAndShowDrivers = true
            }
        )
    }

    if (shouldFetchAndShowDrivers) {
        fetchAndShowDrivers(
            repoUrl = repoUrl!!,
            bypassValidation = false,
            onDismiss = { shouldFetchAndShowDrivers = false },
            onDownloadDriver = { url, name ->
                driverToDownload = Pair(url, name)
                shouldDownloadDriver = true
            }
        )
    }

    if (shouldDownloadDriver) {
        downloadDriver(
            chosenUrl = driverToDownload!!.first,
            chosenName = driverToDownload!!.second,
            onDismiss = { 
                shouldDownloadDriver = false 
                coroutineScope.launch(Dispatchers.IO) {
                    val updatedDrivers = GpuDriverHelper.getInstalledDrivers(context)
                    withContext(Dispatchers.Main) {
                        drivers = updatedDrivers
                    }
                }
            }
        )
    }

    Box(modifier = Modifier.fillMaxSize()) {
        PaneScaffold(
            title = stringResource(R.string.drivers_title),
            tabs = listOf(
                PaneTab(stringResource(R.string.drivers_tab_installed), Icons.Outlined.Memory)
            ),
            selected = 0,
            onSelect = {},
            onBack = navigateBack
        ) {
            PaneSectionTitle(stringResource(R.string.drivers_select))

            drivers.entries.toList().forEach { (file, metadata) ->
                DriverItem(
                    file = file,
                    metadata = metadata,
                    isSelected = metadata.label == selectedDriver,
                    onSelect = {
                        selectedDriver = metadata.label
                        prefs.edit {
                            putString(
                                "selected_gpu_driver", selectedDriver ?: ""
                            )
                        }

                        val path = if (metadata.name == "Default") "" else file.path

                        coroutineScope.launch(Dispatchers.IO) {
                            RPCS3.instance.settingsSet("Video@@Vulkan@@Custom Driver@@Path", "\"" + path + "\"", "")
                            RPCS3.instance.settingsSet("Video@@Vulkan@@Custom Driver@@Internal Data Directory", "\"" + context.filesDir + "\"", "")
                        }
                    },
                    onDelete = if (metadata.name == "Default") null else { driverFile ->
                        coroutineScope.launch {
                            if (driverFile.deleteRecursively()) {
                                drivers = GpuDriverHelper.getInstalledDrivers(context)
                            }
                        }
                    })
            }

            Spacer(modifier = Modifier.height(14.dp))

            PaneActionButton(
                label = stringResource(
                    if (isInstalling) R.string.drivers_installing else R.string.drivers_add
                ),
                icon = Icons.Default.Add,
                enabled = !isInstalling,
                onClick = { showDriverDialog = true }
            )

            Spacer(modifier = Modifier.height(18.dp))

            DriverFlagsSection(null)

            Spacer(modifier = Modifier.height(16.dp))
        }

        SnackbarHost(
            hostState = snackbarHostState,
            modifier = Modifier.align(Alignment.BottomCenter)
        )
    }
}

@Composable
fun DriverItemContent(
    metadata: GpuDriverMetadata,
    isSelected: Boolean,
    onSelect: () -> Unit,
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
            .clickable { onSelect() },
        colors = CardDefaults.cardColors(
            containerColor = if (isSelected) {
                MaterialTheme.colorScheme.primary.copy(alpha = 0.20f)
            } else {
                MaterialTheme.colorScheme.surfaceVariant
            }
        ),
        border = if (isSelected) {
            BorderStroke(1.5.dp, MaterialTheme.colorScheme.primary)
        } else {
            null
        },
        shape = RoundedCornerShape(8.dp),
        elevation = CardDefaults.elevatedCardElevation(defaultElevation = 4.dp)
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = metadata.label,
                style = MaterialTheme.typography.bodyLarge,
                fontWeight = if (isSelected) FontWeight.SemiBold else FontWeight.Normal,
                color = if (isSelected) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurface
            )
            Text(
                text = metadata.description,
                style = MaterialTheme.typography.bodyMedium,
                color = if (isSelected) MaterialTheme.colorScheme.onSurface else MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}

@Composable
fun DriverItem(
    file: File,
    metadata: GpuDriverMetadata,
    isSelected: Boolean,
    onSelect: () -> Unit,
    onDelete: ((File) -> Unit)?
) {
    if (onDelete == null) {
        DriverItemContent(metadata, isSelected, onSelect)
        return
    }
    val dismissState = rememberSwipeToDismissBoxState(
        confirmValueChange = {
            if (it == SwipeToDismissBoxValue.EndToStart) {
                onDelete(file)
                true
            } else {
                false
            }
        })

    SwipeToDismissBox(
        modifier = Modifier.animateContentSize(),
        state = dismissState,
        backgroundContent = {
            if (dismissState.dismissDirection != SwipeToDismissBoxValue.Settled) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(
                            MaterialTheme.colorScheme.onErrorContainer,
                            shape = RoundedCornerShape(8.dp)
                        )
                        .padding(end = 16.dp)
                        .padding(vertical = 4.dp),
                    contentAlignment = androidx.compose.ui.Alignment.CenterEnd
                ) {
                    Icon(
                        imageVector = Icons.Default.Delete,
                        contentDescription = stringResource(R.string.action_delete),
                        tint = Color.White
                    )
                }
            }
        },
        content = { DriverItemContent(metadata, isSelected, onSelect) },
        enableDismissFromEndToStart = true,
        enableDismissFromStartToEnd = false
    )
}

@Composable
fun DriverDialog(
    onDismiss: () -> Unit,
    onInstallClick: () -> Unit,
    onImportClick: () -> Unit
) {
    val items = listOf(
        stringResource(R.string.drivers_import),
        stringResource(R.string.drivers_install)
    )
    var selectedItemIndex by remember { mutableStateOf(0) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = {
            Text(text = stringResource(R.string.drivers_choose))
        },
        text = {
            Column {
                items.forEachIndexed { index, text ->
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        modifier = Modifier
                            .fillMaxWidth()
                            .clickable { selectedItemIndex = index }
                            .padding(8.dp)
                    ) {
                        RadioButton(
                            selected = selectedItemIndex == index,
                            onClick = { selectedItemIndex = index }
                        )
                        Text(text = text, modifier = Modifier.padding(start = 8.dp))
                    }
                }
            }
        },
        confirmButton = {
            TextButton(onClick = {
                if (selectedItemIndex == 1) {
                    onInstallClick()
                } else {
                    onImportClick()
                }
                onDismiss()
            }) {
                Text(text = stringResource(android.R.string.ok))
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text(text = stringResource(android.R.string.cancel))
            }
        }
    )
}

@Composable
fun handleGpuDriverImport(
    onDismiss: () -> Unit,
    onFetchClick: (String) -> Unit
) {
    var textInputValue by remember { mutableStateOf("https://github.com/K11MCH1/AdrenoToolsDrivers") }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = {
            Text(text = stringResource(R.string.drivers_enter_repo_url))
        },
        text = {
            Column {
                OutlinedTextField(
                    value = textInputValue,
                    onValueChange = { textInputValue = it },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        },
        confirmButton = {
            TextButton(onClick = {
                if (textInputValue.isNotEmpty()) {
                    onFetchClick(textInputValue)
                }
                onDismiss()
            }) {
                Text(text = stringResource(R.string.action_fetch))
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text(text = stringResource(android.R.string.cancel))
            }
        }
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun fetchAndShowDrivers(
    repoUrl: String,
    bypassValidation: Boolean = false,
    onDismiss: () -> Unit,
    onDownloadDriver: (String, String) -> Unit
) {
    val context = LocalContext.current
    var isLoading by remember { mutableStateOf(true) }
    var fetchResult by remember { mutableStateOf<FetchResult?>(null) }
    var fetchedDrivers by remember { mutableStateOf<List<Pair<String, String?>>>(emptyList()) }
    var chosenIndex by remember { mutableStateOf(0) }
    val scrollState = rememberScrollState()
    val hasScrolled = remember { derivedStateOf { scrollState.value > 0 } }

    LaunchedEffect(Unit) {
        val fetchOutput = DriversFetcher.fetchReleases(context, repoUrl, bypassValidation)
        isLoading = false

        fetchResult = when (fetchOutput.result) {
            is FetchResult.Error, is FetchResult.Warning -> fetchOutput.result
            else -> null
        }
        if (fetchOutput.result is FetchResult.Success) fetchedDrivers = fetchOutput.fetchedDrivers
    }

    fetchResult?.let {
        val errorMessage = when (it) {
            is FetchResult.Error -> it.message!!
            is FetchResult.Warning -> it.message!!
            else -> stringResource(R.string.drivers_fetch_unexpected, repoUrl)
        }

        AlertDialog(
            onDismissRequest = onDismiss,
            title = { Text(stringResource(R.string.drivers_error_title)) },
            text = { Text(errorMessage) },
            confirmButton = {
                TextButton(onClick = onDismiss) {
                    Text(text = stringResource(android.R.string.ok))
                }
            }
        )
        return
    }

    if (isLoading) {
        AlertDialog(
            onDismissRequest = onDismiss,
            title = { Text(stringResource(R.string.drivers_fetching)) },
            text = {
                Column(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(stringResource(R.string.drivers_please_wait))
                }
            },
            confirmButton = {}
        )
        return
    }

    val isLandscape = LocalConfiguration.current.orientation == Configuration.ORIENTATION_LANDSCAPE
    val maxHeight = if (isLandscape) 168.dp else 300.dp
    
    BasicAlertDialog(
        onDismissRequest = onDismiss,
        content = {
            Surface(
                shape = MaterialTheme.shapes.medium,
                tonalElevation = 6.dp,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = 16.dp)
            ) {
                Column(modifier = Modifier.padding(vertical = 16.dp)) {
                    Text(
                        stringResource(R.string.drivers_list_title),
                        modifier = Modifier.padding(horizontal = 16.dp),
                        style = MaterialTheme.typography.headlineSmall
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    
                    if (hasScrolled.value) {
                        Divider()
                    }
                    
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .heightIn(max = maxHeight)
                            .verticalScroll(scrollState)
                    ) {
                        fetchedDrivers.forEachIndexed { index, driver ->
                            Row(
                                verticalAlignment = Alignment.CenterVertically,
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .clickable { chosenIndex = index }
                                    .padding(vertical = 4.dp, horizontal = 16.dp)
                            ) {
                                RadioButton(
                                    selected = chosenIndex == index,
                                    onClick = { chosenIndex = index }
                                )
                                Text(text = driver.first, modifier = Modifier.padding(start = 8.dp))
                            }
                        }
                    }

                    if (hasScrolled.value) {
                        Divider()
                    }

                    Spacer(modifier = Modifier.height(16.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.End
                    ) {
                        TextButton(onClick = onDismiss) {
                            Text(text = stringResource(android.R.string.cancel))
                        }
                        Spacer(modifier = Modifier.width(8.dp))
                        TextButton(
                            onClick = {
                                val chosenDriver = fetchedDrivers[chosenIndex]
                                onDownloadDriver(chosenDriver.second!!, chosenDriver.first!!)
                                onDismiss()
                            }, 
                            modifier = Modifier.padding(end = 16.dp)
                        ) {
                            Text(text = stringResource(R.string.action_import))
                        }
                    }
                }
            }
        }
    )
}

@Composable
fun downloadDriver(
    chosenUrl: String,
    chosenName: String,
    onDismiss: () -> Unit
) {
    var progress by remember { mutableStateOf(0f) }
    var isIndeterminate by remember { mutableStateOf(true) }
    var downloadCompleted by remember { mutableStateOf(false) }
    val context = LocalContext.current

    LaunchedEffect(Unit) {
        withContext(Dispatchers.IO) {
            val driverFile = File("${context.getExternalFilesDir(null)!!.absolutePath}/cache/$chosenName.zip")
            if (!driverFile.exists()) driverFile.createNewFile()

            val result = DriversFetcher.downloadAsset(
                context,
                chosenUrl,
                driverFile
            ) { downloadedBytes, totalBytes ->
                if (totalBytes > 0) {
                    isIndeterminate = false
                    progress = downloadedBytes.toFloat() / totalBytes
                }
            }

            if (result is DownloadResult.Success) {
                withContext(Dispatchers.Main) {
                    val installResult = GpuDriverHelper.installDriver(context, FileInputStream(driverFile))
                    Toast.makeText(
                        context,
                        GpuDriverHelper.resolveInstallResultToString(context, installResult),
                        Toast.LENGTH_LONG
                    ).show()
                    downloadCompleted = true
                    if (installResult == GpuDriverInstallResult.Success) {
                        onDismiss()
                    }
                }
            } else if (result is DownloadResult.Error) {
                withContext(Dispatchers.Main) {
                    Toast.makeText(
                        context,
                        context.getString(
                            R.string.drivers_import_failed,
                            chosenName,
                            result.message
                        ),
                        Toast.LENGTH_SHORT
                    ).show()
                    onDismiss()
                }
            }

            driverFile.delete()
        }
    }

    AlertDialog(
        onDismissRequest = { if (!isIndeterminate) onDismiss() },
        title = { Text(stringResource(R.string.drivers_downloading)) },
        text = {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                if (isIndeterminate) {
                    LinearProgressIndicator(
                        modifier = Modifier.fillMaxWidth()
                    )
                } else {
                    LinearProgressIndicator(
                        progress = progress,
                        modifier = Modifier.fillMaxWidth()
                    )
                }
                Spacer(modifier = Modifier.height(8.dp))
                if (!isIndeterminate) {
                    Text(text = stringResource(R.string.percent_value, (progress * 100).toInt()))
                }
            }
        },
        confirmButton = {
            if (downloadCompleted) {
                TextButton(onClick = onDismiss) {
                    Text(stringResource(R.string.action_ok))
                }
            }
        }
    )
}
