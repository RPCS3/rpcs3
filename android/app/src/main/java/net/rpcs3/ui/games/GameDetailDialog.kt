package net.rpcs3.ui.games

import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs

import androidx.annotation.StringRes
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.spring
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.outlined.ArrowBack
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.Healing
import androidx.compose.material.icons.outlined.SystemUpdateAlt
import androidx.compose.material.icons.outlined.Settings
import androidx.compose.material.icons.outlined.SportsEsports
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.window.DialogWindowProvider
import android.view.WindowManager
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import coil3.compose.AsyncImage
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.R
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.utils.CacheKind
import net.rpcs3.utils.CacheSurvey
import net.rpcs3.utils.CacheUtil
import net.rpcs3.utils.GameDetailsReader

private val LaunchBlack = Color.Black
private val LaunchCard = Rpcs.SurfaceInset
private val LaunchAccent = Rpcs.Accent
private val LaunchTextPrimary = Rpcs.TextPrimary
private val LaunchTextSecondary = Rpcs.TextSecondary
private val LaunchDanger = Rpcs.Danger

private data class ActionSpec(
    val icon: ImageVector,
    val label: String,
    val danger: Boolean,
    val onClick: () -> Unit
)

private data class CacheAction(
    val kind: CacheKind,
    @StringRes val labelRes: Int,
    @StringRes val descriptionRes: Int
)

private val cacheActions = listOf(
    CacheAction(
        CacheKind.Ppu,
        R.string.cache_clear_ppu,
        R.string.cache_clear_ppu_description
    ),
    CacheAction(
        CacheKind.Shaders,
        R.string.cache_clear_shaders,
        R.string.cache_clear_shaders_description
    ),
    CacheAction(
        CacheKind.Spu,
        R.string.cache_clear_spu,
        R.string.cache_clear_spu_description
    ),
    CacheAction(
        CacheKind.Hdd1,
        R.string.cache_clear_hdd1,
        R.string.cache_clear_hdd1_description
    ),
    CacheAction(
        CacheKind.All,
        R.string.cache_clear_all,
        R.string.cache_clear_all_description
    )
)

@Composable
fun GameDetailDialog(
    title: String,
    subtitle: String,
    version: String,
    iconPath: String?,
    gamePath: String,
    onPlay: () -> Unit,
    onSettings: () -> Unit,
    onInstallUpdate: (() -> Unit)?,
    onPatches: (() -> Unit)?,
    uninstallLabel: String = stringResource(R.string.action_uninstall),
    uninstallTitle: String = stringResource(R.string.games_uninstall_title, title),
    uninstallMessage: String = stringResource(R.string.games_uninstall_message),
    onUninstall: (() -> Unit)?,
    onDismissRequest: () -> Unit
) {
    var confirmUninstall by remember { mutableStateOf(false) }
    var cacheSerial by remember(gamePath) { mutableStateOf("") }
    var cacheSurvey by remember(gamePath) { mutableStateOf<Map<CacheKind, CacheSurvey>?>(null) }
    var surveyToken by remember(gamePath) { mutableIntStateOf(0) }

    LaunchedEffect(gamePath) {
        cacheSerial = withContext(Dispatchers.IO) {
            GameDetailsReader.read(gamePath).titleId
        }
    }

    LaunchedEffect(cacheSerial, surveyToken) {
        if (cacheSerial.isEmpty()) {
            return@LaunchedEffect
        }
        cacheSurvey = withContext(Dispatchers.IO) {
            CacheUtil.survey(cacheSerial)
        }
    }

    Dialog(
        onDismissRequest = onDismissRequest,
        properties = DialogProperties(usePlatformDefaultWidth = false)
    ) {
        val dialogWindow = (LocalView.current.parent as? DialogWindowProvider)?.window
        LaunchedEffect(dialogWindow) {
            dialogWindow?.let { win ->
                WindowCompat.setDecorFitsSystemWindows(win, false)
                win.attributes = win.attributes.apply {
                    layoutInDisplayCutoutMode =
                        WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
                }
                WindowInsetsControllerCompat(win, win.decorView).apply {
                    hide(WindowInsetsCompat.Type.systemBars())
                    systemBarsBehavior =
                        WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                }
            }
        }

        Box(
            Modifier
                .fillMaxSize()
                .background(LaunchBlack)
        ) {
            if (iconPath != null) {
                AsyncImage(
                    model = iconPath,
                    contentDescription = null,
                    modifier = Modifier.fillMaxSize(),
                    contentScale = ContentScale.Crop,
                    alignment = Alignment.Center
                )
            } else {
                Box(
                    Modifier
                        .fillMaxSize()
                        .background(
                            Brush.radialGradient(
                                colors = listOf(
                                    LaunchAccent.copy(alpha = 0.34f),
                                    LaunchCard,
                                    LaunchBlack
                                ),
                                radius = 980f
                            )
                        ),
                    contentAlignment = Alignment.Center
                ) {
                    Icon(
                        Icons.Outlined.SportsEsports,
                        contentDescription = null,
                        tint = LaunchTextPrimary.copy(alpha = 0.18f),
                        modifier = Modifier.size(132.dp)
                    )
                }
            }

            Box(
                Modifier
                    .fillMaxSize()
                    .background(
                        Brush.horizontalGradient(
                            colorStops = arrayOf(
                                0.0f to LaunchBlack.copy(alpha = 0.90f),
                                0.36f to LaunchBlack.copy(alpha = 0.58f),
                                0.72f to LaunchBlack.copy(alpha = 0.18f),
                                1.0f to LaunchBlack.copy(alpha = 0.62f)
                            )
                        )
                    )
            )
            Box(
                Modifier
                    .fillMaxSize()
                    .background(
                        Brush.verticalGradient(
                            colorStops = arrayOf(
                                0.0f to LaunchBlack.copy(alpha = 0.54f),
                                0.36f to Color.Transparent,
                                0.72f to LaunchBlack.copy(alpha = 0.32f),
                                1.0f to LaunchBlack.copy(alpha = 0.94f)
                            )
                        )
                    )
            )

            Box(
                Modifier
                    .fillMaxSize()
                    .windowInsetsPadding(WindowInsets.systemBars)
            ) {
                Box(
                    modifier = Modifier
                        .align(Alignment.TopStart)
                        .padding(start = 18.dp, top = 14.dp)
                        .size(40.dp)
                        .clip(CircleShape)
                        .background(LaunchBlack.copy(alpha = 0.45f))
                        .clickable(onClick = onDismissRequest),
                    contentAlignment = Alignment.Center
                ) {
                    Icon(
                        Icons.AutoMirrored.Outlined.ArrowBack,
                        contentDescription = stringResource(R.string.action_back),
                        tint = LaunchTextPrimary,
                        modifier = Modifier.size(22.dp)
                    )
                }

                CacheMenu(
                    modifier = Modifier
                        .align(Alignment.TopEnd)
                        .padding(end = 18.dp, top = 14.dp),
                    serial = cacheSerial,
                    survey = cacheSurvey,
                    onCleared = { surveyToken++ }
                )

                val actionIconSize = 46.dp
                val actionIconSpacing = 8.dp
                val actionSlots = 4
                val actionWidth =
                    actionIconSize * actionSlots + actionIconSpacing * (actionSlots - 1)

                Row(
                    modifier = Modifier
                        .align(Alignment.BottomStart)
                        .fillMaxWidth()
                        .padding(start = 22.dp, end = 22.dp, bottom = 48.dp),
                    verticalAlignment = Alignment.Bottom
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = title,
                            color = LaunchTextPrimary,
                            fontSize = 28.sp,
                            fontWeight = FontWeight.Bold,
                            maxLines = 2,
                            overflow = TextOverflow.Ellipsis
                        )
                        if (subtitle.isNotEmpty()) {
                            Spacer(Modifier.height(4.dp))
                            Text(
                                text = subtitle,
                                color = LaunchTextSecondary,
                                fontSize = 13.sp,
                                maxLines = 1,
                                overflow = TextOverflow.Ellipsis
                            )
                        }
                        if (version.isNotEmpty()) {
                            Spacer(Modifier.height(6.dp))
                            Text(
                                text = version,
                                color = LaunchAccent,
                                fontSize = 12.sp,
                                fontWeight = FontWeight.SemiBold,
                                maxLines = 1,
                                overflow = TextOverflow.Ellipsis
                            )
                        }
                    }

                    Spacer(Modifier.width(18.dp))

                    Column(
                        modifier = Modifier.width(actionWidth),
                        verticalArrangement = Arrangement.spacedBy(12.dp),
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        PlayButton(
                            modifier = Modifier.fillMaxWidth(),
                            onClick = onPlay
                        )
                        val settingsLabel = stringResource(R.string.detail_settings)
                        val updateLabel = stringResource(R.string.detail_update)
                        val patchesLabel = stringResource(R.string.detail_patches)
                        val actions = buildList {
                            add(
                                ActionSpec(
                                    Icons.Outlined.Settings,
                                    settingsLabel,
                                    false,
                                    onSettings
                                )
                            )
                            onInstallUpdate?.let {
                                add(
                                    ActionSpec(
                                        Icons.Outlined.SystemUpdateAlt,
                                        updateLabel,
                                        false,
                                        it
                                    )
                                )
                            }
                            onPatches?.let {
                                add(ActionSpec(Icons.Outlined.Healing, patchesLabel, false, it))
                            }
                            if (onUninstall != null) {
                                add(
                                    ActionSpec(Icons.Outlined.Delete, uninstallLabel, true) {
                                        confirmUninstall = true
                                    }
                                )
                            }
                        }

                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(actionIconSpacing),
                            verticalAlignment = Alignment.Top
                        ) {
                            actions.forEach { spec ->
                                Column(
                                    modifier = Modifier.width(actionIconSize),
                                    horizontalAlignment = Alignment.CenterHorizontally
                                ) {
                                    IconActionButton(
                                        icon = spec.icon,
                                        contentDescription = spec.label,
                                        size = actionIconSize,
                                        danger = spec.danger,
                                        onClick = spec.onClick
                                    )
                                    Spacer(Modifier.height(4.dp))
                                    Text(
                                        text = spec.label,
                                        color = if (spec.danger) LaunchDanger else LaunchTextSecondary,
                                        fontSize = 9.sp,
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis,
                                        textAlign = TextAlign.Center
                                    )
                                }
                            }
                        }
                    }
                }
            }

            if (confirmUninstall && onUninstall != null) {
                UninstallConfirm(
                    title = uninstallTitle,
                    message = uninstallMessage,
                    action = uninstallLabel,
                    onCancel = { confirmUninstall = false },
                    onConfirm = {
                        confirmUninstall = false
                        onUninstall()
                    }
                )
            }
        }
    }
}

@Composable
private fun CacheMenu(
    serial: String,
    survey: Map<CacheKind, CacheSurvey>?,
    onCleared: () -> Unit,
    modifier: Modifier = Modifier
) {
    var expanded by remember { mutableStateOf(false) }
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()
    val shape = RoundedCornerShape(Dims.InputCorner)
    val context = LocalContext.current

    fun confirmClear(action: CacheAction, amount: CacheSurvey) {
        expanded = false
        AlertDialogQueue.showDialog(
            title = context.getString(
                R.string.cache_confirm_title,
                context.getString(action.labelRes)
            ),
            message = context.getString(
                R.string.cache_confirm_message,
                context.getString(action.descriptionRes),
                amount.entries,
                CacheUtil.formatBytes(amount.bytes),
                serial
            ),
            onConfirm = {
                CacheUtil.clear(serial, action.kind) { result ->
                    onCleared()
                    AlertDialogQueue.showDialog(
                        title = context.getString(
                            if (result.failed == 0) {
                                R.string.cache_cleared_title
                            } else {
                                R.string.cache_cleared_partial_title
                            }
                        ),
                        message = if (result.failed == 0) {
                            context.getString(
                                R.string.cache_cleared_message,
                                result.removed,
                                CacheUtil.formatBytes(result.bytes),
                                serial
                            )
                        } else {
                            context.getString(
                                R.string.cache_cleared_partial_message,
                                result.removed,
                                CacheUtil.formatBytes(result.bytes),
                                serial,
                                result.failed
                            )
                        },
                        confirmText = context.getString(R.string.action_close),
                        dismissText = ""
                    )
                }
            },
            confirmText = context.getString(R.string.action_clear),
            dismissText = context.getString(R.string.action_cancel)
        )
    }

    Box(modifier = modifier) {
        Row(
            modifier = Modifier
                .height(40.dp)
                .clip(shape)
                .background(LaunchBlack.copy(alpha = 0.45f))
                .border(
                    if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                    if (focused) Rpcs.FocusBorder else LaunchTextSecondary.copy(alpha = 0.65f),
                    shape
                )
                .clickable(
                    interactionSource = interaction,
                    indication = null
                ) { expanded = !expanded }
                .padding(start = 14.dp, end = 8.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = stringResource(R.string.cache_menu_label),
                color = LaunchTextPrimary,
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold
            )
            Spacer(Modifier.width(4.dp))
            Icon(
                imageVector = if (expanded) {
                    Icons.Default.KeyboardArrowDown
                } else {
                    Icons.Default.KeyboardArrowUp
                },
                contentDescription = stringResource(
                    if (expanded) R.string.cache_menu_collapse else R.string.cache_menu_expand
                ),
                tint = LaunchTextPrimary,
                modifier = Modifier.size(20.dp)
            )
        }

        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false },
            shape = RoundedCornerShape(Dims.InputCorner),
            containerColor = LaunchCard
        ) {
            cacheActions.forEach { action ->
                if (action.kind == CacheKind.All) {
                    HorizontalDivider(color = Rpcs.OutlineSoft)
                }

                val amount = survey?.get(action.kind)
                val ready = serial.isNotEmpty() && amount != null
                val hasData = ready && amount.entries > 0

                DropdownMenuItem(
                    enabled = hasData,
                    text = {
                        Column {
                            Text(
                                text = stringResource(action.labelRes),
                                color = if (hasData) LaunchTextPrimary else Rpcs.TextDim,
                                fontSize = 14.sp,
                                fontWeight = if (action.kind == CacheKind.All) {
                                    FontWeight.Bold
                                } else {
                                    FontWeight.Medium
                                }
                            )
                            Text(
                                text = when {
                                    !ready -> stringResource(R.string.cache_scanning)
                                    amount.entries == 0 -> {
                                        stringResource(R.string.cache_nothing_to_clear)
                                    }

                                    else -> stringResource(
                                        R.string.cache_entry_summary,
                                        amount.entries,
                                        CacheUtil.formatBytes(amount.bytes)
                                    )
                                },
                                color = if (hasData) LaunchTextSecondary else Rpcs.TextDim,
                                fontSize = 11.sp
                            )
                        }
                    },
                    onClick = {
                        if (amount != null) {
                            confirmClear(action, amount)
                        }
                    }
                )
            }
        }
    }
}

@Composable
private fun PlayButton(modifier: Modifier = Modifier, onClick: () -> Unit) {
    val interaction = remember { MutableInteractionSource() }
    val pressed by interaction.collectIsPressedAsState()
    val scale by animateFloatAsState(
        targetValue = if (pressed) 0.96f else 1f,
        animationSpec = spring(dampingRatio = 0.5f, stiffness = 600f),
        label = "playScale"
    )

    val shape = RoundedCornerShape(14.dp)

    Box(
        modifier = modifier
            .height(56.dp)
            .graphicsLayer {
                scaleX = scale
                scaleY = scale
            }
            .clip(shape)
            .background(
                Brush.horizontalGradient(
                    colors = listOf(
                        Rpcs.GradientCyan.copy(alpha = 0.38f),
                        LaunchAccent.copy(alpha = 0.38f),
                        Rpcs.GradientViolet.copy(alpha = 0.38f)
                    )
                )
            )
            .background(
                Brush.verticalGradient(
                    0.00f to Color.White.copy(alpha = 0.28f),
                    0.35f to Color.White.copy(alpha = 0.06f),
                    0.55f to Color.Transparent,
                    1.00f to Color.Black.copy(alpha = 0.12f)
                )
            )
            .border(1.dp, Color.White.copy(alpha = 0.18f), shape)
            .clickable(interactionSource = interaction, indication = null, onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(
                Icons.Default.PlayArrow,
                contentDescription = null,
                tint = LaunchTextPrimary,
                modifier = Modifier.size(24.dp)
            )
            Spacer(Modifier.width(8.dp))
            Text(
                text = stringResource(R.string.detail_play),
                color = LaunchTextPrimary,
                fontSize = 16.sp,
                fontWeight = FontWeight.Bold
            )
        }
    }
}

@Composable
private fun IconActionButton(
    icon: ImageVector,
    contentDescription: String,
    onClick: () -> Unit,
    size: Dp = 46.dp,
    danger: Boolean = false
) {
    val tint = if (danger) LaunchDanger else LaunchTextPrimary
    Box(
        modifier = Modifier
            .size(size)
            .clip(RoundedCornerShape(12.dp))
            .background(LaunchBlack.copy(alpha = 0.42f))
            .border(1.dp, Color.White.copy(alpha = 0.14f), RoundedCornerShape(12.dp))
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Icon(
            icon,
            contentDescription = contentDescription,
            tint = tint,
            modifier = Modifier.size(20.dp)
        )
    }
}

@Composable
private fun UninstallConfirm(
    title: String,
    message: String,
    action: String,
    onCancel: () -> Unit,
    onConfirm: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(LaunchBlack.copy(alpha = 0.72f))
            .clickable(
                interactionSource = remember { MutableInteractionSource() },
                indication = null,
                onClick = onCancel
            ),
        contentAlignment = Alignment.Center
    ) {
        Surface(
            modifier = Modifier
                .fillMaxWidth(0.7f)
                .clickable(
                    interactionSource = remember { MutableInteractionSource() },
                    indication = null,
                    onClick = {}
                ),
            shape = RoundedCornerShape(16.dp),
            color = LaunchCard,
            border = BorderStroke(1.dp, Color.White.copy(alpha = 0.14f))
        ) {
            Column(modifier = Modifier.padding(20.dp)) {
                Text(
                    text = title,
                    color = LaunchTextPrimary,
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Bold
                )
                Spacer(Modifier.height(8.dp))
                Text(
                    text = message,
                    color = LaunchTextSecondary,
                    fontSize = 13.sp
                )
                Spacer(Modifier.height(18.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp, Alignment.End)
                ) {
                    Text(
                        text = stringResource(R.string.action_cancel),
                        color = LaunchTextSecondary,
                        fontSize = 14.sp,
                        fontWeight = FontWeight.SemiBold,
                        modifier = Modifier
                            .clip(RoundedCornerShape(8.dp))
                            .clickable(onClick = onCancel)
                            .padding(horizontal = 14.dp, vertical = 8.dp)
                    )
                    Text(
                        text = action,
                        color = LaunchDanger,
                        fontSize = 14.sp,
                        fontWeight = FontWeight.Bold,
                        modifier = Modifier
                            .clip(RoundedCornerShape(8.dp))
                            .clickable(onClick = onConfirm)
                            .padding(horizontal = 14.dp, vertical = 8.dp)
                    )
                }
            }
        }
    }
}
