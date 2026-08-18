package net.rpcs3.ui.navigation

import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.DocumentsContract
import android.util.Log
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.tween
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Album
import androidx.compose.material.icons.outlined.BugReport
import androidx.compose.material.icons.outlined.Folder
import androidx.compose.material.icons.outlined.MonitorHeart
import androidx.compose.material.icons.outlined.CheckCircle
import androidx.compose.material.icons.outlined.Healing
import androidx.compose.material.icons.outlined.Memory
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.outlined.Build
import androidx.compose.material.icons.outlined.Info
import androidx.compose.material3.CenterAlignedTopAppBar
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DrawerValue
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalDrawerSheet
import androidx.compose.material3.ModalNavigationDrawer
import androidx.compose.material3.NavigationDrawerItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.rememberDrawerState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.res.vectorResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlin.concurrent.thread
import kotlinx.coroutines.launch
import net.rpcs3.EmulatorState
import net.rpcs3.FirmwareRepository
import net.rpcs3.PrecompilerService
import net.rpcs3.PrecompilerServiceAction
import net.rpcs3.ProgressRepository
import net.rpcs3.R
import androidx.navigation.NavType
import androidx.navigation.navArgument
import androidx.compose.runtime.LaunchedEffect
import net.rpcs3.ui.library.CompatibilityScreen
import net.rpcs3.ui.library.GameSort
import net.rpcs3.ui.library.LibraryTab
import net.rpcs3.ui.library.LibraryTopBar
import net.rpcs3.ui.setup.SetupWizard
import net.rpcs3.ui.setup.hasStorageAccess
import net.rpcs3.ui.setup.requestStorageAccess
import net.rpcs3.ui.setup.StorageAccessDialog
import net.rpcs3.ui.debug.DebugScreen
import net.rpcs3.ui.diagnostics.DiagnosticsScreen
import net.rpcs3.provider.AppDataDocumentProvider
import net.rpcs3.RPCS3
import net.rpcs3.RPCS3Activity
import net.rpcs3.overlay.OverlayEditActivity
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.ui.drivers.GpuDriversScreen
import net.rpcs3.ui.games.GameSourceChoiceDialog
import net.rpcs3.ui.games.GamesScreen
import net.rpcs3.ui.games.GameUpdatesScreen
import net.rpcs3.ui.games.PackageInstallFlow
import net.rpcs3.ui.settings.AdvancedSettingsScreen
import net.rpcs3.ui.patches.AllPatchesScreen
import net.rpcs3.ui.patches.GamePatchesScreen
import net.rpcs3.ui.settings.GameSettingsScreen
import net.rpcs3.utils.FileUtil
import org.json.JSONObject

@Preview
@Composable
fun AppNavHost() {
    val navController = rememberNavController()
    val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
    val scope = rememberCoroutineScope()
    val settingsTitleId = RPCS3.settingsTitleId.value
    val settings = remember(settingsTitleId) { mutableStateOf(JSONObject()) }

    LaunchedEffect(settingsTitleId) {
        val loaded = withContext(Dispatchers.IO) {
            runCatching { JSONObject(RPCS3.instance.settingsGet("", settingsTitleId)) }
                .getOrDefault(JSONObject())
        }
        settings.value = loaded
    }

    BackHandler(enabled = drawerState.isOpen) {
        scope.launch {
            drawerState.close()
        }
    }

    AlertDialogQueue.AlertDialog()

    NavHost(
        navController = navController,
        startDestination = "games"
    ) {
        composable(
            route = "games"
        ) {
            GamesDestination(
                navigateToSettings = {
                    RPCS3.settingsTitleId.value = ""
                    navController.navigate("settings")
                },
                navigateToGameSettings = { titleId ->
                    navController.navigate("gameSettings?titleId=" + Uri.encode(titleId))
                },
                navigateToDrivers = { navController.navigate("drivers") },
                navigateToAllPatches = { navController.navigate("allPatches") },
                navigateToGamePatches = { titleId ->
                    navController.navigate("gamePatches?titleId=" + Uri.encode(titleId))
                },
                navigateToGameUpdates = { gamePath ->
                    navController.navigate("gameUpdates?path=" + Uri.encode(gamePath))
                },
                navigateToDiagnostics = { navController.navigate("diagnostics") },
                navigateToDebug = { navController.navigate("debug") },
                drawerState
            )
        }

        fun unwrapSetting(obj: JSONObject, path: String = "") {
            obj.keys().forEach self@{ key ->
                val item = obj[key]
                val elemPath = "$path@@$key"
                val elemObject = item as? JSONObject
                if (elemObject == null) {
                    return@self
                }

                if (elemObject.has("type")) {
                    return@self
                }


                composable(
                    route = "settings$elemPath"
                ) {
                    AdvancedSettingsScreen(
                        navigateBack = navController::navigateUp,
                        navigateTo = { navController.navigate(it) },
                        settings = elemObject,
                        path = elemPath
                    )
                }

                unwrapSetting(elemObject, elemPath)
            }
        }

        composable(
            route = "settings@@$"
        ) {
            AdvancedSettingsScreen(
                navigateBack = navController::navigateUp,
                navigateTo = { navController.navigate(it) },
                settings = settings.value,
            )
        }

        composable(
            route = "settings"
        ) {
            LaunchedEffect(Unit) { RPCS3.settingsTitleId.value = "" }
            GameSettingsScreen(
                titleId = "",
                title = stringResource(R.string.settings_title_global),
                onClose = navController::navigateUp
            )
        }

        composable(route = "debug") {
            DebugScreen(
                navigateToDiagnostics = { navController.navigate("diagnostics") },
                onClose = navController::navigateUp
            )
        }

        composable(
            route = "gameSettings?titleId={titleId}",
            arguments = listOf(
                navArgument("titleId") {
                    type = NavType.StringType
                    defaultValue = ""
                }
            )
        ) { entry ->
            val target = entry.arguments?.getString("titleId").orEmpty()
            LaunchedEffect(target) { RPCS3.settingsTitleId.value = target }
            GameSettingsScreen(
                titleId = target,
                title = target.ifEmpty { stringResource(R.string.settings_title_global) },
                onClose = navController::navigateUp
            )
        }

        composable(
            route = "gamePatches?titleId={titleId}",
            arguments = listOf(
                navArgument("titleId") {
                    type = NavType.StringType
                    defaultValue = ""
                }
            )
        ) { entry ->
            GamePatchesScreen(
                titleId = entry.arguments?.getString("titleId").orEmpty(),
                onClose = navController::navigateUp
            )
        }

        composable(
            route = "gameUpdates?path={path}",
            arguments = listOf(
                navArgument("path") {
                    type = NavType.StringType
                    defaultValue = ""
                }
            )
        ) { entry ->
            GameUpdatesScreen(
                gamePath = entry.arguments?.getString("path").orEmpty(),
                onClose = navController::navigateUp
            )
        }

        composable(route = "diagnostics") {
            DiagnosticsScreen(onClose = { navController.popBackStack() })
        }

        composable(
            route = "drivers"
        ) {
            GpuDriversScreen(
                navigateBack = navController::navigateUp
            )
        }

        composable(
            route = "allPatches"
        ) {
            AllPatchesScreen(
                onClose = navController::navigateUp
            )
        }

        unwrapSetting(settings.value)
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GamesDestination(
    navigateToSettings: () -> Unit,
    navigateToGameSettings: (titleId: String) -> Unit,
    navigateToDrivers: () -> Unit,
    navigateToAllPatches: () -> Unit,
    navigateToGamePatches: (titleId: String) -> Unit,
    navigateToGameUpdates: (titleId: String) -> Unit,
    navigateToDiagnostics: () -> Unit,
    navigateToDebug: () -> Unit,
    drawerState: androidx.compose.material3.DrawerState
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    var emulatorState by remember { RPCS3.state }
    val emulatorActiveGame by remember { RPCS3.activeGame }

    var pendingPackages by remember { mutableStateOf<List<Uri>>(emptyList()) }

    val installPkgLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetMultipleContents(),
        onResult = { uris: List<Uri> ->
            if (uris.isNotEmpty()) {
                pendingPackages = uris
            }
        }
    )

    if (pendingPackages.isNotEmpty()) {
        PackageInstallFlow(
            uris = pendingPackages,
            onFinished = { pendingPackages = emptyList() }
        )
    }

    var pendingIso by remember { mutableStateOf<Uri?>(null) }
    var pendingFolder by remember { mutableStateOf<Uri?>(null) }
    var needsStorage by remember { mutableStateOf(false) }
    val firmwareLoaded by FirmwareRepository.loaded
    var setupActive by remember { mutableStateOf<Boolean?>(null) }

    LaunchedEffect(firmwareLoaded) {
        if (firmwareLoaded && setupActive == null) {
            setupActive = !hasStorageAccess() || FirmwareRepository.version.value == null
        }
    }

    val showSetup = setupActive == true

    val bootIsoLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent(),
        onResult = { uri: Uri? -> pendingIso = uri }
    )

    if (needsStorage) {
        StorageAccessDialog(
            onGrant = {
                needsStorage = false
                requestStorageAccess(context)
            },
            onDismiss = { needsStorage = false }
        )
    }

    pendingIso?.let { uri ->
        GameSourceChoiceDialog(
            title = stringResource(R.string.iso_title),
            subtitle = stringResource(R.string.iso_subtitle),
            sourceIcon = Icons.Outlined.Album,
            directBootDetail = stringResource(R.string.iso_direct_boot_detail),
            installDetail = stringResource(R.string.iso_install_detail),
            onDirectBoot = {
                pendingIso = null
                PrecompilerService.start(context, PrecompilerServiceAction.AddIso, uri)
            },
            onInstall = {
                pendingIso = null
                PrecompilerService.start(context, PrecompilerServiceAction.Install, uri)
            },
            onDismiss = { pendingIso = null }
        )
    }

    pendingFolder?.let { uri ->
        GameSourceChoiceDialog(
            title = stringResource(R.string.folder_title),
            subtitle = stringResource(R.string.folder_subtitle),
            sourceIcon = Icons.Outlined.Folder,
            directBootDetail = stringResource(R.string.folder_direct_boot_detail),
            installDetail = stringResource(R.string.folder_install_detail),
            onDirectBoot = {
                pendingFolder = null
                FileUtil.directBootFolder(context, uri)
            },
            onInstall = {
                pendingFolder = null
                FileUtil.installPackages(context, uri)
            },
            onDismiss = { pendingFolder = null }
        )
    }

    val installFwLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent(),
        onResult = { uri: Uri? ->
            if (uri != null) PrecompilerService.start(
                context,
                PrecompilerServiceAction.InstallFirmware,
                uri
            )
        }
    )

    val gameFolderPickerLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocumentTree(),
        onResult = { uri: Uri? ->
            uri?.let {
                val takeFlags = Intent.FLAG_GRANT_READ_URI_PERMISSION
                context.contentResolver.takePersistableUriPermission(it, takeFlags)
                pendingFolder = it
            }
        }
    )

    ModalNavigationDrawer(
        drawerState = drawerState,
        gesturesEnabled = drawerState.isOpen,
        drawerContent = {
            ModalDrawerSheet {
                Column(
                    modifier = Modifier
                        .padding(horizontal = 16.dp)
                        .verticalScroll(
                            rememberScrollState()
                        )
                ) {
                    Spacer(Modifier.height(12.dp))

                    NavigationDrawerItem(
                        label = {
                            Text(
                                stringResource(
                                    R.string.drawer_firmware,
                                    FirmwareRepository.version.value
                                        ?: stringResource(R.string.drawer_firmware_none)
                                )
                            )
                        },
                        selected = false,
                        icon = { Icon(Icons.Outlined.Build, contentDescription = null) },
                        badge = {
                            val progressChannel = FirmwareRepository.progressChannel
                            val progress = ProgressRepository.getItem(progressChannel.value)
                            val progressValue = progress?.value?.value
                            val maxValue = progress?.value?.max
                            if (progressValue != null && maxValue != null) {
                                if (maxValue.longValue != 0L) {
                                    CircularProgressIndicator(
                                        modifier = Modifier
                                            .width(32.dp)
                                            .height(32.dp),
                                        color = MaterialTheme.colorScheme.secondary,
                                        trackColor = MaterialTheme.colorScheme.surfaceVariant,
                                        progress = {
                                            progressValue.longValue.toFloat() / maxValue.longValue.toFloat()
                                        },
                                    )
                                } else {
                                    CircularProgressIndicator(
                                        modifier = Modifier
                                            .width(32.dp)
                                            .height(32.dp),
                                        color = MaterialTheme.colorScheme.secondary,
                                        trackColor = MaterialTheme.colorScheme.surfaceVariant,
                                    )
                                }
                            }
                        },
                        onClick = {
                            if (FirmwareRepository.progressChannel.value == null) {
                                installFwLauncher.launch("*/*")
                            }
                        }
                    )
                    HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                    NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_settings)) },
                        selected = false,
                        icon = { Icon(Icons.Default.Settings, null) },
                        onClick = {
                            scope.launch { drawerState.close() }
                            navigateToSettings()
                        }
                    )

                    NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_gpu_drivers)) },
                        selected = false,
                        icon = { Icon(Icons.Outlined.Memory, null) },
                        onClick = {
                            scope.launch { drawerState.close() }
                            navigateToDrivers()
                        }
                    )

                    NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_edit_overlay)) },
                        selected = false,
                        icon = { Icon(painter = painterResource(id = R.drawable.ic_show_osc), null) },
                        onClick = {
                            context.startActivity(
                                Intent(
                                    context,
                                    OverlayEditActivity::class.java
                                )
                            )
                        }
                    )

                    NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_patches)) },
                        selected = false,
                        icon = { Icon(Icons.Outlined.Healing, null) },
                        onClick = {
                            scope.launch { drawerState.close() }
                            navigateToAllPatches()
                        }
                    )

                    NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_debug)) },
                        selected = false,
                        icon = { Icon(Icons.Outlined.BugReport, null) },
                        onClick = {
                            scope.launch { drawerState.close() }
                            navigateToDebug()
                        }
                    )

                    NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_diagnostics)) },
                        selected = false,
                        icon = { Icon(Icons.Outlined.MonitorHeart, null) },
                        onClick = {
                            scope.launch { drawerState.close() }
                            navigateToDiagnostics()
                        }
                    )

                    NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_system_info)) },
                        selected = false,
                        icon = { Icon(Icons.Outlined.Info, contentDescription = null) },
                        onClick = {
                            scope.launch {
                                val info = withContext(Dispatchers.IO) {
                                    runCatching { RPCS3.instance.systemInfo() }.getOrDefault("")
                                }
                                AlertDialogQueue.showDialog(
                                    context.getString(R.string.system_info_title),
                                    info
                                )
                            }
                        }
                    )
                NavigationDrawerItem(
                        label = { Text(stringResource(R.string.drawer_setup)) },
                        selected = false,
                        icon = { Icon(Icons.Outlined.CheckCircle, null) },
                        onClick = {
                            scope.launch { drawerState.close() }
                            setupActive = true
                        }
                    )

                    }
            }
        }
    ) {
        var selectedTab by remember { mutableStateOf(LibraryTab.Library) }
        var searchQuery by remember { mutableStateOf("") }
        var gameSort by remember { mutableStateOf(GameSort.NameAscending) }

        if (showSetup) {
            SetupWizard(
                onInstallFirmware = { installFwLauncher.launch("*/*") },
                onFinish = { setupActive = false },
                onSkip = { setupActive = false }
            )
            return@ModalNavigationDrawer
        }

        val libraryHeader: @Composable () -> Unit = {
            LibraryTopBar(
                selectedTab = selectedTab,
                onTabSelected = { selectedTab = it },
                searchQuery = searchQuery,
                onSearchQueryChange = { searchQuery = it },
                sort = gameSort,
                onSortChange = { gameSort = it },
                onMenuClick = {
                    scope.launch {
                        if (drawerState.isClosed) drawerState.open() else drawerState.close()
                    }
                },
                trailing = {
                    if (emulatorActiveGame != null &&
                        emulatorState.isEmulationActive &&
                        emulatorState != EmulatorState.Stopping
                    ) {
                        IconButton(onClick = {
                            emulatorState = EmulatorState.Stopped
                            thread { RPCS3.instance.kill() }
                        }) {
                            Icon(
                                imageVector = ImageVector.vectorResource(R.drawable.ic_stop),
                                contentDescription = stringResource(R.string.action_stop)
                            )
                        }
                    }
                }
            )
        }

        Scaffold(
            contentWindowInsets = WindowInsets.systemBars,
            topBar = {
                if (selectedTab == LibraryTab.Library) {
                    libraryHeader()
                }
            },
            floatingActionButton = {
                if (selectedTab == LibraryTab.Library) {
                    DropUpFloatingActionButton(
                        installPkgLauncher,
                        gameFolderPickerLauncher,
                        bootIsoLauncher,
                        onNeedsStorage = { needsStorage = true }
                    )
                }
            },
        ) { innerPadding ->
            when (selectedTab) {
                LibraryTab.Library -> Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(innerPadding)
                ) {
                    Column(modifier = Modifier.fillMaxSize()) {
                        GamesScreen(
                            navigateToGameSettings = navigateToGameSettings,
                            navigateToGamePatches = navigateToGamePatches,
                            navigateToGameUpdates = navigateToGameUpdates,
                            searchQuery = searchQuery,
                            sort = gameSort
                        )
                    }

                    FloatingActionButton(
                        onClick = {
                            context.launchBrowseIntent(Intent.ACTION_VIEW) or context.launchBrowseIntent()
                        },
                        modifier = Modifier
                            .align(Alignment.BottomStart)
                            .padding(32.dp)
                    ) {
                        Icon(
                            imageVector = Icons.Outlined.Folder,
                            contentDescription = stringResource(R.string.library_browse_internal)
                        )
                    }
                }

                LibraryTab.Compatibility -> CompatibilityScreen(
                    modifier = Modifier.padding(
                        bottom = innerPadding.calculateBottomPadding()
                    ),
                    header = libraryHeader
                )
            }
        }
    }
}

@Composable
fun DropUpFloatingActionButton(
    installPkgLauncher: ActivityResultLauncher<String>,
    gameFolderPickerLauncher: ActivityResultLauncher<Uri?>,
    bootIsoLauncher: ActivityResultLauncher<String>,
    onNeedsStorage: () -> Unit
) {
    var expanded by remember { mutableStateOf(false) }

    Box(
        modifier = Modifier.padding(16.dp),
        contentAlignment = androidx.compose.ui.Alignment.BottomEnd
    ) {
        Column(
            verticalArrangement = Arrangement.spacedBy(8.dp),
            horizontalAlignment = androidx.compose.ui.Alignment.End
        ) {
            AnimatedVisibility(
                visible = expanded,
                enter = androidx.compose.animation.expandVertically(
                    animationSpec = tween(300, easing = FastOutSlowInEasing)
                ),
                exit = androidx.compose.animation.shrinkVertically(
                    animationSpec = tween(200, easing = FastOutSlowInEasing)
                )
            ) {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    FloatingActionButton(
                        onClick = {
                            expanded = false
                            if (hasStorageAccess()) {
                                bootIsoLauncher.launch("*/*")
                            } else {
                                onNeedsStorage()
                            }
                        },
                        containerColor = MaterialTheme.colorScheme.secondary
                    ) {
                        Icon(
                            imageVector = Icons.Outlined.Album,
                            contentDescription = stringResource(R.string.library_boot_disc_image)
                        )
                    }
                    FloatingActionButton(
                        onClick = { installPkgLauncher.launch("*/*"); expanded = false },
                        containerColor = MaterialTheme.colorScheme.secondary
                    ) {
                        Icon(
                            painter = painterResource(id = R.drawable.ic_description),
                            contentDescription = stringResource(R.string.library_install)
                        )
                    }
                    FloatingActionButton(
                        onClick = {
                            expanded = false
                            if (hasStorageAccess()) {
                                gameFolderPickerLauncher.launch(null)
                            } else {
                                onNeedsStorage()
                            }
                        },
                        containerColor = MaterialTheme.colorScheme.secondary
                    ) {
                        Icon(
                            painter = painterResource(id = R.drawable.ic_folder),
                            contentDescription = stringResource(R.string.library_folder)
                        )
                    }
                }
            }

            FloatingActionButton(
                onClick = { expanded = !expanded }
            ) {
                Icon(Icons.Filled.Add, contentDescription = stringResource(R.string.action_add))
            }
        }
    }
}

private fun Context.launchBrowseIntent(
    action: String = "android.provider.action.BROWSE"
): Boolean {
    return try {
        val intent = Intent(action).apply {
            addCategory(Intent.CATEGORY_DEFAULT)
            data = DocumentsContract.buildRootUri(
                AppDataDocumentProvider.authorityOf(this@launchBrowseIntent),
                AppDataDocumentProvider.ROOT_ID
            )
            addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION or Intent.FLAG_GRANT_PREFIX_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivity(intent)
        true
    } catch (_: ActivityNotFoundException) {
        Log.w("AppNavHost", "No activity found to handle $action intent")
        false
    }
}
