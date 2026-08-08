package net.rpcs3.ui.games

import android.content.Intent
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.wrapContentHeight
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.Lock
import androidx.compose.material.icons.outlined.Settings
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.pulltorefresh.PullToRefreshBox
import androidx.compose.material3.pulltorefresh.PullToRefreshDefaults
import androidx.compose.material3.pulltorefresh.rememberPullToRefreshState
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.vectorResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.CardDefaults
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.sp
import net.rpcs3.ui.theme.SettingsStyle
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.ui.draw.clip
import net.rpcs3.ui.library.GameSort
import androidx.compose.foundation.basicMarquee
import androidx.compose.foundation.focusable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import net.rpcs3.ui.components.chasingBorder
import androidx.compose.foundation.border
import coil3.compose.AsyncImage
import net.rpcs3.EmulatorState
import net.rpcs3.FirmwareRepository
import net.rpcs3.Game
import net.rpcs3.GameFlag
import net.rpcs3.GameInfo
import net.rpcs3.GameProgress
import net.rpcs3.GameProgressType
import net.rpcs3.gameTitleId
import net.rpcs3.GameRepository
import net.rpcs3.ProgressRepository
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.RPCS3Activity
import net.rpcs3.utils.FileUtil
import net.rpcs3.dialogs.AlertDialogQueue
import java.io.File
import kotlin.concurrent.thread
import kotlin.text.substringAfterLast

private fun withAlpha(color: Color, alpha: Float): Color {
    return Color(
        red = color.red, green = color.green, blue = color.blue, alpha = alpha
    )
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun GameItem(
    game: Game,
    navigateToGameSettings: (titleId: String) -> Unit = {},
    navigateToGamePatches: (titleId: String) -> Unit = {},
    modifier: Modifier = Modifier
) {
    val deleteScope = rememberCoroutineScope()
    val context = LocalContext.current
    var pendingPackages by remember { mutableStateOf<List<Uri>>(emptyList()) }

    val updatePickerLauncher = rememberLauncherForActivityResult(
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
    val menuExpanded = remember { mutableStateOf(false) }
    val iconExists = remember { mutableStateOf(false) }
    var emulatorState by remember { RPCS3.state }
    val emulatorActiveGame by remember { RPCS3.activeGame }

    val installKeyLauncher =
        rememberLauncherForActivityResult(contract = ActivityResultContracts.GetContent()) { uri: Uri? ->
            if (uri != null) {
                val descriptor = context.contentResolver.openAssetFileDescriptor(uri, "r")
                val fd = descriptor?.parcelFileDescriptor?.fd

                if (fd != null) {
                    val installProgress = ProgressRepository.create(context, "License Installation")

                    game.addProgress(GameProgress(installProgress, GameProgressType.Compile))

                    thread(isDaemon = true) {
                        if (!RPCS3.instance.installKey(fd, installProgress, game.info.path)) {
                            try {
                                ProgressRepository.onProgressEvent(installProgress, -1, 0)
                            } catch (e: Exception) {
                                e.printStackTrace()
                            }
                        }

                        try {
                            descriptor.close()
                        } catch (e: Exception) {
                            e.printStackTrace()
                        }
                    }
                } else {
                    try {
                        descriptor?.close()
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }
        }

    val detailVisible = remember { mutableStateOf(false) }
    val tileInteraction = remember { MutableInteractionSource() }
    val tileFocused by tileInteraction.collectIsFocusedAsState()

    fun uninstallGame() {
        val deleteProgress = ProgressRepository.create(context, "Deleting Game")
        game.addProgress(GameProgress(deleteProgress, GameProgressType.Compile))
        ProgressRepository.onProgressEvent(deleteProgress, -1, 0L)
        val path = File(game.info.path)
        if (path.exists()) {
            deleteScope.launch {
                withContext(Dispatchers.IO) { path.deleteRecursively() }
                FileUtil.deleteCache(context, game.info.path.substringAfterLast("/")) { success ->
                    if (!success) {
                        AlertDialogQueue.showDialog(
                            title = "Unexpected Error",
                            message = "Failed to delete game cache",
                            confirmText = "Close",
                            dismissText = ""
                        )
                    }
                    ProgressRepository.onProgressEvent(deleteProgress, 100, 100)
                    GameRepository.remove(game)
                }
            }
        }
    }

    fun launchGame() {
                    if (game.hasFlag(GameFlag.Locked)) {
                        AlertDialogQueue.showDialog(
                            title = "Missing key",
                            message = "This game requires key to play",
                            onConfirm = { installKeyLauncher.launch("*/*") },
                            onDismiss = {},
                            confirmText = "Install RAP file"
                        )

                        return
                    }

                    if (FirmwareRepository.version.value == null) {
                        AlertDialogQueue.showDialog(
                            title = "Firmware Missing",
                            message = "Please install the required firmware to continue."
                        )
                    } else if (FirmwareRepository.progressChannel.value != null) {
                        AlertDialogQueue.showDialog(
                            title = "Firmware Missing",
                            message = "Please wait until firmware installs successfully to continue."
                        )
                    } else if (game.info.path != "$" && game.findProgress(
                            arrayOf(
                                GameProgressType.Install, GameProgressType.Remove
                            )
                        ) == null
                    ) {
                        if (game.findProgress(GameProgressType.Compile) != null) {
                            AlertDialogQueue.showDialog(
                                title = "Game compiling isn't finished yet",
                                message = "Please wait until game compiles to continue."
                            )
                        } else {
                            GameRepository.onBoot(game)
                            val emulatorWindow = Intent(
                                context, RPCS3Activity::class.java
                            )
                            emulatorWindow.putExtra("path", game.info.path)
                            context.startActivity(emulatorWindow)
                        }
                    }
    }

    Column(modifier = modifier) {
        DropdownMenu(
            expanded = menuExpanded.value, onDismissRequest = { menuExpanded.value = false }) {
            if (game.progressList.isEmpty()) {
                DropdownMenuItem(
                    text = { Text("Settings") },
                    leadingIcon = { Icon(Icons.Outlined.Settings, contentDescription = null) },
                    onClick = {
                        menuExpanded.value = false
                        navigateToGameSettings(gameTitleId(game.info.path))
                    }
                )
                DropdownMenuItem(
                    text = { Text("Delete") },
                    leadingIcon = { Icon(Icons.Outlined.Delete, contentDescription = null) },
                    onClick = {
                        menuExpanded.value = false
                        val deleteProgress = ProgressRepository.create(context, "Deleting Game")
                        game.addProgress(GameProgress(deleteProgress, GameProgressType.Compile))
                        ProgressRepository.onProgressEvent(deleteProgress, -1, 0L)
                        val path = File(game.info.path)
                        if (path.exists()) {
                            deleteScope.launch {
                                withContext(Dispatchers.IO) { path.deleteRecursively() }
                                FileUtil.deleteCache(context, game.info.path.substringAfterLast("/")) { success ->
                                    if (!success) {
                                        AlertDialogQueue.showDialog(
                                            title = "Unexpected Error",
                                            message = "Failed to delete game cache",
                                            confirmText = "Close",
                                            dismissText = ""
                                        )
                                    }
                                    ProgressRepository.onProgressEvent(deleteProgress, 100, 100)
                                    GameRepository.remove(game)
                                }
                            }
                        }
                    }
                )
            }
        }

        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier
                .fillMaxSize()
                .then(
                    if (tileFocused) {
                        Modifier
                    } else {
                        Modifier.border(
                            1.dp,
                            SettingsStyle.CardBorder,
                            RoundedCornerShape(12.dp)
                        )
                    }
                )
                .chasingBorder(isFocused = tileFocused, cornerRadius = 12.dp)
                .background(SettingsStyle.CardSurface, RoundedCornerShape(12.dp))
                .focusable(interactionSource = tileInteraction)
                .combinedClickable(
                    interactionSource = tileInteraction,
                    indication = null,
                    onClick = { detailVisible.value = true },
                    onLongClick = {
                        if (game.info.name.value != "VSH") {
                            menuExpanded.value = true
                        }
                    }
                )
        ) {
            if (game.info.iconPath.value != null && !iconExists.value) {
                if (game.progressList.isNotEmpty()) {
                    val progressId = ProgressRepository.getItem(game.progressList.first().id)
                    if (progressId != null) {
                        val progressValue = progressId.value.value
                        val progressMax = progressId.value.max

                        iconExists.value =
                            (progressMax.longValue != 0L && progressValue.longValue == progressMax.longValue) || File(
                                game.info.iconPath.value!!
                            ).exists()
                    }
                } else {
                    iconExists.value = File(game.info.iconPath.value!!).exists()
                }
            }

            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f)
                    .clip(RoundedCornerShape(topStart = 12.dp, topEnd = 12.dp))
            ) {
                if (game.info.iconPath.value != null && iconExists.value) {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.Center,
                        modifier = Modifier.fillMaxSize()
                    ) {
                        AsyncImage(
                            model = game.info.iconPath.value,
                            contentScale = if (game.info.name.value == "VSH") ContentScale.Fit else ContentScale.Crop,
                            contentDescription = null,
                            modifier = Modifier.fillMaxSize()
                        )
                    }
                }

                if (game.progressList.isNotEmpty()) {
                    Row(
                        modifier = Modifier
                            .fillMaxSize()
                            .background(withAlpha(Color.DarkGray, 0.6f))
                    ) {}

                    val progressChannel = game.progressList.first().id
                    val progress = ProgressRepository.getItem(progressChannel)
                    val progressValue = progress?.value?.value
                    val maxValue = progress?.value?.max

                    if (progressValue != null && maxValue != null) {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.Center,
                            modifier = Modifier.fillMaxSize()
                        ) {
                            if (maxValue.longValue != 0L) {
                                CircularProgressIndicator(
                                    modifier = Modifier
                                        .width(64.dp)
                                        .height(64.dp),
                                    color = MaterialTheme.colorScheme.secondary,
                                    trackColor = MaterialTheme.colorScheme.surfaceVariant,
                                    progress = {
                                        progressValue.longValue.toFloat() / maxValue.longValue.toFloat()
                                    },
                                )
                            } else {
                                CircularProgressIndicator(
                                    modifier = Modifier
                                        .width(64.dp)
                                        .height(64.dp),
                                    color = MaterialTheme.colorScheme.secondary,
                                    trackColor = MaterialTheme.colorScheme.surfaceVariant,
                                )
                            }
                        }
                    }
                } else if (emulatorState == EmulatorState.Paused && emulatorActiveGame == game.info.path) {
                    Card(modifier = Modifier.padding(5.dp)) {
                        Icon(
                            imageVector = ImageVector.vectorResource(R.drawable.ic_play),
                            contentDescription = null
                        )
                    }
                }

                if (game.hasFlag(GameFlag.Locked) || game.hasFlag(GameFlag.Trial)) {
                    Row(
                        verticalAlignment = Alignment.Top,
                        horizontalArrangement = Arrangement.End,
                        modifier = Modifier.fillMaxSize()
                    ) {
                        Card(
                            onClick = {
                                installKeyLauncher.launch("*/*")
                            }) {

                            Icon(
                                Icons.Outlined.Lock,
                                contentDescription = "Game is locked",
                                modifier = Modifier
                                    .size(30.dp)
                                    .padding(7.dp)
                            )
                        }
                    }
                }

            }

            Text(
                text = game.info.name.value ?: gameTitleId(game.info.path),
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 4.dp, vertical = 4.dp)
                    .then(
                        if (tileFocused) {
                            Modifier.basicMarquee(iterations = Int.MAX_VALUE)
                        } else {
                            Modifier
                        }
                    ),
                style = MaterialTheme.typography.bodySmall,
                color = SettingsStyle.TextPrimary,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                textAlign = TextAlign.Center
            )
        }
    }
    if (detailVisible.value) {
        GameDetailDialog(
            title = game.info.name.value ?: gameTitleId(game.info.path),
            subtitle = gameTitleId(game.info.path),
            iconPath = if (iconExists.value) game.info.iconPath.value else null,
            onPlay = {
                detailVisible.value = false
                launchGame()
            },
            onSettings = {
                detailVisible.value = false
                navigateToGameSettings(gameTitleId(game.info.path))
            },
            onInstallUpdate = {
                detailVisible.value = false
                updatePickerLauncher.launch("*/*")
            },
            onPatches = {
                detailVisible.value = false
                navigateToGamePatches(gameTitleId(game.info.path))
            },
            onUninstall = if (game.info.name.value == "VSH") {
                null
            } else {
                {
                    detailVisible.value = false
                    uninstallGame()
                }
            },
            onDismissRequest = { detailVisible.value = false }
        )
    }

}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GamesScreen(
    navigateToGameSettings: (titleId: String) -> Unit = {},
    navigateToGamePatches: (titleId: String) -> Unit = {},
    searchQuery: String = "",
    sort: GameSort = GameSort.NameAscending
) {
    val games = remember { GameRepository.list() }
    val isRefreshing = remember { mutableStateOf(false) }
    val coroutineScope = rememberCoroutineScope()
    val state = rememberPullToRefreshState()

    val gameInProgress = games.find { it.progressList.isNotEmpty() }

    PullToRefreshBox(
        isRefreshing = isRefreshing.value,
        state = state,
        onRefresh = {
            if (gameInProgress == null) {
                isRefreshing.value = true
                thread {
                    GameRepository.clear()
                    RPCS3.instance.collectGameInfo(
                        RPCS3.rootDirectory + "/config/dev_hdd0/game", -1
                    )
                    RPCS3.instance.collectGameInfo(RPCS3.rootDirectory + "/config/games", -1)
                    Thread.sleep(300)
                    isRefreshing.value = false
                }
            }
        },
        indicator = {
            if (gameInProgress == null) {
                PullToRefreshDefaults.Indicator(
                    state = state,
                    isRefreshing = isRefreshing.value,
                    modifier = Modifier.align(Alignment.TopCenter),
                    color = MaterialTheme.colorScheme.onPrimary,
                    containerColor = MaterialTheme.colorScheme.primary
                )
            }
        },
    ) {
        val shown = remember(games.toList(), searchQuery, sort) {
            games
                .filter { game ->
                    searchQuery.isBlank() ||
                        (game.info.name.value ?: "").contains(searchQuery, ignoreCase = true)
                }
                .sortedWith(
                    compareBy(String.CASE_INSENSITIVE_ORDER) { game ->
                        game.info.name.value ?: gameTitleId(game.info.path)
                    }
                )
                .let { if (sort == GameSort.NameDescending) it.reversed() else it }
        }

        BoxWithConstraints(
            modifier = Modifier
                .fillMaxSize()
                .background(SettingsStyle.BgDeep)
        ) {
            val columns = if (maxWidth > 600.dp) 4 else 2
            val spacing = 12.dp
            val edge = 14.dp
            val columnWidth = (maxWidth - edge * 2 - spacing * (columns - 1)) / columns
            val rowHeight = minOf((maxHeight - spacing - edge * 2) / 2, columnWidth * 1.25f)

            LazyVerticalGrid(
                columns = GridCells.Fixed(columns),
                horizontalArrangement = Arrangement.spacedBy(spacing),
                verticalArrangement = Arrangement.spacedBy(spacing),
                contentPadding = PaddingValues(edge),
                modifier = Modifier.fillMaxSize()
            ) {
                items(count = shown.size, key = { index -> shown[index].info.path }) { index ->
                    GameItem(
                        game = shown[index],
                        navigateToGameSettings = navigateToGameSettings,
                        navigateToGamePatches = navigateToGamePatches,
                        modifier = Modifier.height(rowHeight)
                    )
                }
            }
        }
    }
}

@Preview
@Composable
fun GamesScreenPreview() {
    listOf(
        "Minecraft", "Skate 3", "Mirror's Edge", "Demon's Souls"
    ).forEach { x -> GameRepository.addPreview(arrayOf(GameInfo(x, x))) }

    GamesScreen()
}
