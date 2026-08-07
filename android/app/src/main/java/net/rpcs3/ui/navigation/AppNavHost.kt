package net.rpcs3.ui.navigation

import android.content.Intent
import android.net.Uri
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
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.vectorResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
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
import net.rpcs3.RPCS3
import net.rpcs3.overlay.OverlayEditActivity
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.ui.drivers.GpuDriversScreen
import net.rpcs3.ui.games.GamesScreen
import net.rpcs3.ui.settings.AdvancedSettingsScreen
import net.rpcs3.ui.settings.GameSettingsScreen
import net.rpcs3.ui.settings.SettingsScreen
import net.rpcs3.utils.FileUtil
import org.json.JSONObject

@Preview
@Composable
fun AppNavHost() {
    val navController = rememberNavController()
    val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
    val scope = rememberCoroutineScope()
    val settingsTitleId = RPCS3.settingsTitleId.value
    val settings = remember(settingsTitleId) {
        mutableStateOf(JSONObject(RPCS3.instance.settingsGet("", settingsTitleId)))
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
                drawerState
            )
        }

        fun unwrapSetting(obj: JSONObject, path: String = "") {
            obj.keys().forEach self@{ key ->
                val item = obj[key]
                val elemPath = "$path@@$key"
                val elemObject = item as? JSONObject
                if (elemObject == null) {
                    Log.e("Main", "element is not object: settings$elemPath, $item")
                    return@self
                }

                if (elemObject.has("type")) {
                    return@self
                }

                Log.e("Main", "registration settings$elemPath")

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
            SettingsScreen(
                navigateBack = navController::navigateUp,
                navigateTo = { navController.navigate(it) },
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
                title = if (target.isEmpty()) "Global Settings" else target,
                onClose = navController::navigateUp
            )
        }

        composable(
            route = "drivers"
        ) {
            GpuDriversScreen(
                navigateBack = navController::navigateUp
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
    drawerState: androidx.compose.material3.DrawerState
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    // val prefs = remember { context.getSharedPreferences("app_prefs", Context.MODE_PRIVATE) }
    var emulatorState by remember { RPCS3.state }
    val emulatorActiveGame by remember { RPCS3.activeGame }

    val installPkgLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent(),
        onResult = { uri: Uri? ->
            if (uri != null) PrecompilerService.start(
                context,
                PrecompilerServiceAction.Install,
                uri
            )
        }
    )

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
                // TODO: FileUtil.saveGameFolderUri(prefs, it)
                val takeFlags = Intent.FLAG_GRANT_READ_URI_PERMISSION
                context.contentResolver.takePersistableUriPermission(it, takeFlags)
                FileUtil.installPackages(context, it)
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
                                "Firmware: " + (FirmwareRepository.version.value ?: "None")
                            )
                        },
                        selected = false,
                        icon = { Icon(Icons.Outlined.Build, contentDescription = null) },
                        badge = {
                            val progressChannel = FirmwareRepository.progressChannel
                            val progress = ProgressRepository.getItem(progressChannel.value)
                            val progressValue = progress?.value?.value
                            val maxValue = progress?.value?.max
                            Log.e("Main", "Update $progressChannel, $progress")
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
                        }, // Placeholder
                        onClick = {
                            if (FirmwareRepository.progressChannel.value == null) {
                                installFwLauncher.launch("*/*")
                            }
                        }
                    )
                    HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))
                    NavigationDrawerItem(
                        label = { Text("Settings") },
                        selected = false,
                        icon = { Icon(Icons.Default.Settings, null) },
                        onClick = navigateToSettings
                    )

                    NavigationDrawerItem(
                        label = { Text("Edit Overlay") },
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
                        label = { Text("System Info") },
                        selected = false,
                        icon = { Icon(Icons.Outlined.Info, contentDescription = null) },
                        onClick = {
                            AlertDialogQueue.showDialog("System Info", RPCS3.instance.systemInfo())
                        }
                    )
                }
            }
        }
    ) {
        var selectedTab by remember { mutableStateOf(LibraryTab.Library) }
        var searchQuery by remember { mutableStateOf("") }
        var gameSort by remember { mutableStateOf(GameSort.NameAscending) }

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
                        emulatorState != EmulatorState.Stopped &&
                        emulatorState != EmulatorState.Stopping
                    ) {
                        IconButton(onClick = {
                            emulatorState = EmulatorState.Stopped
                            RPCS3.instance.kill()
                        }) {
                            Icon(
                                imageVector = ImageVector.vectorResource(R.drawable.ic_stop),
                                contentDescription = "Stop"
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
                    DropUpFloatingActionButton(installPkgLauncher, gameFolderPickerLauncher)
                }
            },
        ) { innerPadding ->
            when (selectedTab) {
                LibraryTab.Library -> Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(innerPadding)
                ) {
                    GamesScreen(
                        navigateToGameSettings = navigateToGameSettings,
                        searchQuery = searchQuery,
                        sort = gameSort
                    )
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
    gameFolderPickerLauncher: ActivityResultLauncher<Uri?>
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
                        onClick = { installPkgLauncher.launch("*/*"); expanded = false },
                        containerColor = MaterialTheme.colorScheme.secondary
                    ) {
                        Icon(
                            painter = painterResource(id = R.drawable.ic_description),
                            contentDescription = "Select Game"
                        )
                    }
                    FloatingActionButton(
                        onClick = { gameFolderPickerLauncher.launch(null); expanded = false },
                        containerColor = MaterialTheme.colorScheme.secondary
                    ) {
                        Icon(
                            painter = painterResource(id = R.drawable.ic_folder),
                            contentDescription = "Select Folder"
                        )
                    }
                }
            }

            FloatingActionButton(
                onClick = { expanded = !expanded }
            ) {
                Icon(Icons.Filled.Add, contentDescription = "Add")
            }
        }
    }
}
