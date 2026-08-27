package net.rpcs3.ui.drawer

import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.animateDpAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.outlined.ExitToApp
import androidx.compose.material.icons.outlined.Close
import androidx.compose.material.icons.outlined.Pause
import androidx.compose.material.icons.outlined.PlayArrow
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.LocalMinimumInteractiveComponentSize
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
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
import androidx.compose.ui.layout.boundsInParent
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.ui.framegen.FrameGenPanel
import net.rpcs3.ui.hud.HudSettingsPanel
import net.rpcs3.ui.patches.InGamePatchesPanel
import net.rpcs3.ui.settings.ControlsSettings
import net.rpcs3.ui.settings.SettingsCategory
import net.rpcs3.ui.settings.SettingsCategoryContent
import net.rpcs3.ui.settings.iconForCategory
import net.rpcs3.ui.settings.topLevelCategoriesOf
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.DrawerStyle
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.SettingsStyle
import org.json.JSONObject

private const val TAB_HUD = "hud"
private const val TAB_FRAME_GEN = "frameGen"
private const val TAB_CONTROLS = "controls"
private const val TAB_PATCHES = "patches"

private val RailTileMinWidth = 60.dp
private val RailTileSpacing = 6.dp

private data class DrawerTab(
    val key: String,
    val label: String,
    val icon: ImageVector,
    val category: SettingsCategory? = null
)

private data class RailTileBounds(val offsetX: Float, val width: Float, val height: Float)

@Composable
private fun rememberInteraction(): MutableInteractionSource =
    remember { MutableInteractionSource() }

@Composable
fun InGameDrawer(
    visible: Boolean,
    titleId: String,
    paused: Boolean,
    onDismiss: () -> Unit,
    onTogglePause: () -> Unit,
    onExit: () -> Unit
) {
    Box(modifier = Modifier.fillMaxSize()) {
        AnimatedVisibility(
            visible = visible,
            enter = fadeIn(tween(160)),
            exit = fadeOut(tween(140))
        ) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(Color.Black.copy(alpha = 0.55f))
                    .clickable(
                        interactionSource = rememberInteraction(),
                        indication = null,
                        onClick = onDismiss
                    )
            )
        }

        AnimatedVisibility(
            visible = visible,
            enter = fadeIn(tween(180)) + slideInHorizontally(tween(220)) { -it / 3 },
            exit = fadeOut(tween(140)) + slideOutHorizontally(tween(180)) { -it / 3 },
            modifier = Modifier.align(Alignment.CenterStart)
        ) {
            BoxWithConstraints(
                modifier = Modifier
                    .fillMaxSize()
                    .windowInsetsPadding(WindowInsets.systemBars)
                    .padding(
                        start = DrawerStyle.StartPadding,
                        end = DrawerStyle.StartPadding,
                        top = DrawerStyle.VerticalPadding,
                        bottom = DrawerStyle.VerticalPadding
                    )
            ) {
                val shape = RoundedCornerShape(DrawerStyle.CornerRadius)
                val width = minOf(DrawerStyle.Width, maxWidth)

                Column(
                    modifier = Modifier
                        .align(Alignment.CenterStart)
                        .width(width)
                        .fillMaxHeight()
                        .background(SettingsStyle.BgDeep, shape)
                        .border(1.dp, DrawerStyle.RestingCardBorder, shape)
                        .clip(shape)
                        .clickable(
                            interactionSource = rememberInteraction(),
                            indication = null,
                            onClick = {}
                        )
                ) {
                    InGameMenu(
                        titleId = titleId,
                        paused = paused,
                        onClose = onDismiss,
                        onTogglePause = onTogglePause,
                        onExit = onExit,
                        modifier = Modifier.weight(1f)
                    )
                }
            }
        }
    }
}

@Composable
private fun InGameMenu(
    titleId: String,
    paused: Boolean,
    onClose: () -> Unit,
    onTogglePause: () -> Unit,
    onExit: () -> Unit,
    modifier: Modifier = Modifier
) {
    var root by remember(titleId) { mutableStateOf<JSONObject?>(null) }
    var selected by remember(titleId) { mutableStateOf(TAB_HUD) }

    LaunchedEffect(titleId) {
        root = withContext(Dispatchers.IO) {
            runCatching { JSONObject(RPCS3.instance.settingsGet("", titleId)) }.getOrNull()
        }
    }

    val tree = root

    val hudLabel = stringResource(R.string.settings_category_hud)
    val frameGenLabel = stringResource(R.string.settings_category_frame_gen)
    val controlsLabel = stringResource(R.string.settings_category_controls)
    val patchesLabel = stringResource(R.string.settings_category_patches)

    val tabs = remember(tree, hudLabel, frameGenLabel, controlsLabel, patchesLabel) {
        buildList {
            add(DrawerTab(TAB_HUD, hudLabel, iconForCategory(net.rpcs3.ui.hud.HudCategory)))
            add(
                DrawerTab(
                    TAB_FRAME_GEN,
                    frameGenLabel,
                    iconForCategory(net.rpcs3.ui.framegen.FrameGenCategory)
                )
            )

            tree?.let { loaded ->
                topLevelCategoriesOf(loaded).forEach { entry ->
                    add(DrawerTab(entry.label, entry.label, iconForCategory(entry.label), entry))
                }
            }

            add(
                DrawerTab(
                    TAB_CONTROLS,
                    controlsLabel,
                    iconForCategory(net.rpcs3.ui.settings.ControlsCategory)
                )
            )
            add(
                DrawerTab(
                    TAB_PATCHES,
                    patchesLabel,
                    iconForCategory(net.rpcs3.ui.patches.PatchesCategory)
                )
            )
        }
    }

    BoxWithConstraints(modifier = modifier.fillMaxSize()) {
        val scale = paneScaleFor(maxHeight)

        Column(modifier = Modifier.fillMaxSize()) {
            MenuHeader(
                titleId = titleId,
                scale = scale,
                onClose = onClose
            )

            TopRail(
                tabs = tabs,
                selected = selected,
                scale = scale,
                onSelect = { selected = it }
            )

            Box(
                Modifier
                    .fillMaxWidth()
                    .height(1.dp)
                    .background(SettingsStyle.Divider)
            )

            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxWidth()
                    .background(SettingsStyle.ContentBg)
            ) {
                if (tree == null) {
                    CircularProgressIndicator(
                        modifier = Modifier.align(Alignment.Center),
                        color = SettingsStyle.AccentBlue
                    )
                } else {
                    CompositionLocalProvider(
                        LocalMinimumInteractiveComponentSize provides 0.dp
                    ) {
                        AnimatedContent(
                            targetState = selected,
                            transitionSpec = {
                                fadeIn(tween(180, easing = FastOutSlowInEasing))
                                    .togetherWith(fadeOut(tween(120, easing = FastOutSlowInEasing)))
                            },
                            label = "drawerPane"
                        ) { key ->
                            Column(
                                modifier = Modifier
                                    .fillMaxSize()
                                    .verticalScroll(rememberScrollState())
                                    .padding(
                                        horizontal = 14.dp * scale,
                                        vertical = 12.dp * scale
                                    )
                            ) {
                                PaneContent(key = key, tabs = tabs, tree = tree, titleId = titleId)
                                Spacer(Modifier.height(Dimens.SectionGap))
                            }
                        }
                    }
                }
            }

            Box(
                Modifier
                    .fillMaxWidth()
                    .height(1.dp)
                    .background(SettingsStyle.Divider)
            )

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(DrawerStyle.PaneSurface)
                    .padding(horizontal = 12.dp, vertical = 8.dp * scale),
                horizontalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                DrawerActionButton(
                    label = stringResource(
                        if (paused) R.string.ingame_resume else R.string.ingame_pause
                    ),
                    icon = if (paused) Icons.Outlined.PlayArrow else Icons.Outlined.Pause,
                    scale = scale,
                    modifier = Modifier.weight(1f),
                    onClick = onTogglePause
                )
                DrawerActionButton(
                    label = stringResource(R.string.ingame_exit),
                    icon = Icons.AutoMirrored.Outlined.ExitToApp,
                    scale = scale,
                    modifier = Modifier.weight(1f),
                    isExit = true,
                    onClick = onExit
                )
            }
        }
    }
}

@Composable
private fun PaneContent(
    key: String,
    tabs: List<DrawerTab>,
    tree: JSONObject,
    titleId: String
) {
    when (key) {
        TAB_HUD -> HudSettingsPanel()
        TAB_FRAME_GEN -> FrameGenPanel()
        TAB_CONTROLS -> ControlsSettings()
        TAB_PATCHES -> InGamePatchesPanel(titleId = titleId)
        else -> {
            val entry = tabs.firstOrNull { it.key == key }?.category
            val node = entry?.let { target ->
                var cursor: JSONObject? = tree
                for (step in target.path) cursor = cursor?.optJSONObject(step)
                cursor
            }

            if (node != null) {
                SettingsCategoryContent(
                    node = node,
                    path = entry.path.joinToString("@@"),
                    titleId = titleId
                )
            }
        }
    }
}

@Composable
private fun MenuHeader(
    titleId: String,
    scale: Float,
    onClose: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(DrawerStyle.PaneSurface)
            .padding(start = 14.dp, end = 6.dp, top = 6.dp * scale, bottom = 2.dp * scale),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = stringResource(R.string.ingame_menu_title),
            color = SettingsStyle.TextPrimary,
            fontSize = 13.sp * scale,
            fontWeight = FontWeight.SemiBold
        )

        if (titleId.isNotEmpty()) {
            Spacer(Modifier.width(8.dp))
            Text(
                text = titleId,
                modifier = Modifier
                    .background(
                        SettingsStyle.AccentBlue.copy(alpha = 0.12f),
                        RoundedCornerShape(Dimens.BadgeCorner)
                    )
                    .padding(horizontal = 7.dp, vertical = 2.dp),
                color = SettingsStyle.AccentBlue,
                fontSize = 10.sp * scale,
                fontWeight = FontWeight.SemiBold,
                maxLines = 1
            )
        }

        Spacer(Modifier.weight(1f))

        val interaction = rememberInteraction()
        val focused by interaction.collectIsFocusedAsState()

        Box(
            modifier = Modifier
                .size(32.dp * scale)
                .clip(RoundedCornerShape(10.dp))
                .border(
                    if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                    if (focused) Rpcs.FocusBorder else Color.Transparent,
                    RoundedCornerShape(10.dp)
                )
                .clickable(interactionSource = interaction, indication = null, onClick = onClose),
            contentAlignment = Alignment.Center
        ) {
            Icon(
                imageVector = Icons.Outlined.Close,
                contentDescription = stringResource(R.string.action_close),
                tint = SettingsStyle.TextSecondary,
                modifier = Modifier.size(18.dp * scale)
            )
        }
    }
}

@Composable
private fun TopRail(
    tabs: List<DrawerTab>,
    selected: String,
    scale: Float,
    onSelect: (String) -> Unit
) {
    val density = LocalDensity.current
    val railScroll = rememberScrollState()
    val bounds = remember { mutableStateMapOf<String, RailTileBounds>() }
    val current = bounds[selected]

    val indicatorSpec = tween<Dp>(durationMillis = 220, easing = FastOutSlowInEasing)
    val indicatorX by animateDpAsState(
        targetValue = current?.let { with(density) { it.offsetX.toDp() } } ?: 0.dp,
        animationSpec = indicatorSpec,
        label = "railIndicatorX"
    )
    val indicatorWidth by animateDpAsState(
        targetValue = current?.let { with(density) { it.width.toDp() } } ?: 0.dp,
        animationSpec = indicatorSpec,
        label = "railIndicatorWidth"
    )
    val indicatorTop by animateDpAsState(
        targetValue = current?.let { with(density) { it.height.toDp() } } ?: 0.dp,
        animationSpec = indicatorSpec,
        label = "railIndicatorTop"
    )
    val indicatorAlpha by animateFloatAsState(
        targetValue = if (current != null) 1f else 0f,
        animationSpec = tween(160),
        label = "railIndicatorAlpha"
    )

    val thickness = 2.dp
    val inset = 8.dp * scale

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .background(DrawerStyle.TopRailSurface)
            .padding(start = 10.dp, end = 10.dp, top = 2.dp * scale)
    ) {
        if (current != null) {
            Box(
                modifier = Modifier
                    .offset(
                        x = indicatorX - with(density) { railScroll.value.toDp() } + inset,
                        y = indicatorTop - thickness
                    )
                    .width((indicatorWidth - inset * 2).coerceAtLeast(0.dp))
                    .height(thickness)
                    .graphicsLayer { alpha = indicatorAlpha }
                    .clip(RoundedCornerShape(thickness / 2))
                    .background(SettingsStyle.AccentBlue)
            )
        }

        Row(
            modifier = Modifier.horizontalScroll(railScroll),
            horizontalArrangement = Arrangement.spacedBy(RailTileSpacing),
            verticalAlignment = Alignment.CenterVertically
        ) {
            tabs.forEach { tab ->
                RailTile(
                    tab = tab,
                    selected = tab.key == selected,
                    scale = scale,
                    onClick = { onSelect(tab.key) },
                    onBoundsChanged = { bounds[tab.key] = it }
                )
            }
        }

        Box(modifier = Modifier.matchParentSize()) {
            RailEdgeFade(
                visible = railScroll.canScrollBackward,
                toStart = true,
                modifier = Modifier.align(Alignment.CenterStart)
            )
            RailEdgeFade(
                visible = railScroll.canScrollForward,
                toStart = false,
                modifier = Modifier.align(Alignment.CenterEnd)
            )
        }
    }
}

@Composable
private fun RailEdgeFade(visible: Boolean, toStart: Boolean, modifier: Modifier = Modifier) {
    val alpha by animateFloatAsState(
        targetValue = if (visible) 1f else 0f,
        animationSpec = tween(160),
        label = "railEdgeFade"
    )

    val colors = if (toStart) {
        listOf(DrawerStyle.TopRailSurface, Color.Transparent)
    } else {
        listOf(Color.Transparent, DrawerStyle.TopRailSurface)
    }

    Box(
        modifier = modifier
            .width(20.dp)
            .fillMaxHeight()
            .graphicsLayer { this.alpha = alpha }
            .background(Brush.horizontalGradient(colors))
    )
}

@Composable
private fun RailTile(
    tab: DrawerTab,
    selected: Boolean,
    scale: Float,
    onClick: () -> Unit,
    onBoundsChanged: (RailTileBounds) -> Unit
) {
    val interaction = rememberInteraction()
    val pressed by interaction.collectIsPressedAsState()
    val focused by interaction.collectIsFocusedAsState()

    val shape = RoundedCornerShape(12.dp)
    val background by animateColorAsState(
        targetValue = when {
            focused -> DrawerStyle.FocusFill
            pressed -> DrawerStyle.PaneSurfacePressed
            else -> Color.Transparent
        },
        animationSpec = tween(120),
        label = "railTileBackground"
    )
    val tint by animateColorAsState(
        targetValue = if (selected) SettingsStyle.AccentBlue else SettingsStyle.TextSecondary,
        animationSpec = tween(120),
        label = "railTileTint"
    )

    Column(
        modifier = Modifier
            .defaultMinSize(minWidth = RailTileMinWidth * scale)
            .onGloballyPositioned { coords ->
                val box = coords.boundsInParent()
                onBoundsChanged(RailTileBounds(box.left, box.width, box.height))
            }
            .clip(shape)
            .background(background)
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else Color.Transparent,
                shape
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick)
            .padding(
                start = 10.dp * scale,
                end = 10.dp * scale,
                top = 8.dp * scale,
                bottom = 7.dp * scale
            ),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Icon(
            imageVector = tab.icon,
            contentDescription = null,
            tint = tint,
            modifier = Modifier.size(20.dp * scale)
        )
        Spacer(Modifier.height(3.dp * scale))
        Text(
            text = tab.label,
            color = if (selected) SettingsStyle.TextPrimary else SettingsStyle.TextSecondary,
            fontSize = 11.sp * scale,
            fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Medium,
            letterSpacing = 0.2.sp,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
    }
}

@Composable
private fun DrawerActionButton(
    label: String,
    icon: ImageVector,
    scale: Float,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    isExit: Boolean = false
) {
    val interaction = rememberInteraction()
    val focused by interaction.collectIsFocusedAsState()

    val background = if (isExit) DrawerStyle.TileExitResting else DrawerStyle.PaneInnerResting
    val border = if (isExit) DrawerStyle.TileExitPressed else DrawerStyle.RestingCardBorder
    val tint = if (isExit) DrawerStyle.ExitTint else DrawerStyle.TextPrimary
    val shape = RoundedCornerShape(12.dp)

    Row(
        modifier = modifier
            .background(background, shape)
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else border,
                shape
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick)
            .padding(horizontal = 12.dp, vertical = 10.dp * scale),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Center
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = tint,
            modifier = Modifier.size(17.dp * scale)
        )
        Spacer(Modifier.width(8.dp))
        Text(
            text = label,
            color = tint,
            fontSize = Dimens.ValueSize * scale,
            fontWeight = FontWeight.SemiBold,
            maxLines = 1
        )
    }
}

private fun paneScaleFor(height: Dp): Float =
    (height.value / DrawerStyle.PaneScaleReferenceHeightDp)
        .coerceIn(DrawerStyle.PaneScaleMin, 1f)
