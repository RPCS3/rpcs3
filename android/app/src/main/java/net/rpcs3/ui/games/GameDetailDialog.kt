package net.rpcs3.ui.games

import net.rpcs3.ui.theme.Rpcs

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.spring
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
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
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.Healing
import androidx.compose.material.icons.outlined.SystemUpdateAlt
import androidx.compose.material.icons.outlined.Settings
import androidx.compose.material.icons.outlined.SportsEsports
import androidx.compose.material3.Icon
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
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

@Composable
fun GameDetailDialog(
    title: String,
    subtitle: String,
    iconPath: String?,
    onPlay: () -> Unit,
    onSettings: () -> Unit,
    onInstallUpdate: (() -> Unit)?,
    onPatches: (() -> Unit)?,
    onUninstall: (() -> Unit)?,
    onDismissRequest: () -> Unit
) {
    var confirmUninstall by remember { mutableStateOf(false) }

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
                        contentDescription = "Back",
                        tint = LaunchTextPrimary,
                        modifier = Modifier.size(22.dp)
                    )
                }

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
                        val actions = buildList {
                            add(ActionSpec(Icons.Outlined.Settings, "Settings", false, onSettings))
                            onInstallUpdate?.let {
                                add(ActionSpec(Icons.Outlined.SystemUpdateAlt, "Update", false, it))
                            }
                            onPatches?.let {
                                add(ActionSpec(Icons.Outlined.Healing, "Patches", false, it))
                            }
                            if (onUninstall != null) {
                                add(
                                    ActionSpec(Icons.Outlined.Delete, "Uninstall", true) {
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
                    name = title,
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
                text = "Play",
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
private fun UninstallConfirm(name: String, onCancel: () -> Unit, onConfirm: () -> Unit) {
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
                    text = "Uninstall $name?",
                    color = LaunchTextPrimary,
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Bold
                )
                Spacer(Modifier.height(8.dp))
                Text(
                    text = "This deletes the installed game data from this device.",
                    color = LaunchTextSecondary,
                    fontSize = 13.sp
                )
                Spacer(Modifier.height(18.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp, Alignment.End)
                ) {
                    Text(
                        text = "Cancel",
                        color = LaunchTextSecondary,
                        fontSize = 14.sp,
                        fontWeight = FontWeight.SemiBold,
                        modifier = Modifier
                            .clip(RoundedCornerShape(8.dp))
                            .clickable(onClick = onCancel)
                            .padding(horizontal = 14.dp, vertical = 8.dp)
                    )
                    Text(
                        text = "Uninstall",
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
