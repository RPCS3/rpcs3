package net.rpcs3.ui.drawer

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.outlined.ExitToApp
import androidx.compose.material.icons.outlined.Pause
import androidx.compose.material.icons.outlined.PlayArrow
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.ScrollableTabRow
import androidx.compose.material3.Tab
import androidx.compose.material3.TabRowDefaults
import androidx.compose.material3.TabRowDefaults.tabIndicatorOffset
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.RPCS3
import net.rpcs3.ui.settings.ControlsCategory
import net.rpcs3.ui.settings.ControlsSettings
import net.rpcs3.ui.settings.SettingsNodeContent
import net.rpcs3.ui.settings.categoriesOf
import net.rpcs3.ui.settings.iconForCategory
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.DrawerStyle
import net.rpcs3.ui.theme.SettingsStyle
import org.json.JSONObject

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
            enter = fadeIn(tween(180)) + scaleIn(tween(200), initialScale = 0.96f),
            exit = fadeOut(tween(140)) + scaleOut(tween(160), targetScale = 0.96f),
            modifier = Modifier.align(Alignment.Center)
        ) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .windowInsetsPadding(WindowInsets.systemBars)
                    .padding(horizontal = 18.dp, vertical = 10.dp)
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(SettingsStyle.BgDeep, RoundedCornerShape(18.dp))
                        .border(1.dp, DrawerStyle.RestingCardBorder, RoundedCornerShape(18.dp))
                        .clickable(
                            interactionSource = rememberInteraction(),
                            indication = null,
                            onClick = {}
                        )
                ) {
                    InGameSettingsPanel(
                        titleId = titleId,
                        modifier = Modifier.weight(1f)
                    )

                    Box(
                        Modifier
                            .fillMaxWidth()
                            .height(1.dp)
                            .background(SettingsStyle.Divider)
                    )

                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 12.dp, vertical = 10.dp),
                        horizontalArrangement = Arrangement.spacedBy(10.dp)
                    ) {
                        DrawerActionButton(
                            label = if (paused) "Resume" else "Pause",
                            icon = if (paused) Icons.Outlined.PlayArrow else Icons.Outlined.Pause,
                            modifier = Modifier.weight(1f),
                            onClick = onTogglePause
                        )
                        DrawerActionButton(
                            label = "Exit Game",
                            icon = Icons.AutoMirrored.Outlined.ExitToApp,
                            modifier = Modifier.weight(1f),
                            isExit = true,
                            onClick = onExit
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun InGameSettingsPanel(titleId: String, modifier: Modifier = Modifier) {
    var root by remember(titleId) { mutableStateOf<JSONObject?>(null) }
    var selected by remember(titleId) { mutableIntStateOf(0) }

    LaunchedEffect(titleId) {
        root = withContext(Dispatchers.IO) {
            runCatching { JSONObject(RPCS3.instance.settingsGet("", titleId)) }.getOrNull()
        }
    }

    val tree = root

    Box(modifier = modifier.fillMaxSize()) {
        if (tree == null) {
            CircularProgressIndicator(
                modifier = Modifier.align(Alignment.Center),
                color = SettingsStyle.AccentBlue
            )
            return@Box
        }

        val categories = remember(tree) { categoriesOf(tree).map { it.label } + ControlsCategory }

        Column(modifier = Modifier.fillMaxSize()) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(start = 16.dp, end = 16.dp, top = 12.dp, bottom = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = if (titleId.isEmpty()) "Settings" else titleId,
                    color = SettingsStyle.TextPrimary,
                    fontSize = 13.sp,
                    fontWeight = FontWeight.SemiBold
                )
            }

            ScrollableTabRow(
                selectedTabIndex = selected.coerceIn(0, (categories.size - 1).coerceAtLeast(0)),
                containerColor = Color.Transparent,
                contentColor = SettingsStyle.AccentBlue,
                edgePadding = 12.dp,
                divider = {},
                indicator = { positions ->
                    if (selected < positions.size) {
                        TabRowDefaults.SecondaryIndicator(
                            modifier = Modifier.tabIndicatorOffset(positions[selected]),
                            height = 2.dp,
                            color = SettingsStyle.AccentBlue
                        )
                    }
                }
            ) {
                categories.forEachIndexed { index, name ->
                    Tab(
                        selected = index == selected,
                        onClick = { selected = index },
                        selectedContentColor = SettingsStyle.AccentBlue,
                        unselectedContentColor = SettingsStyle.TextSecondary
                    ) {
                        Row(
                            modifier = Modifier.padding(horizontal = 14.dp, vertical = 10.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Icon(
                                imageVector = iconForCategory(name),
                                contentDescription = null,
                                modifier = Modifier.size(Dimens.IconSize)
                            )
                            Spacer(Modifier.width(8.dp))
                            Text(
                                text = name,
                                fontSize = Dimens.ValueSize,
                                fontWeight = if (index == selected) {
                                    FontWeight.SemiBold
                                } else {
                                    FontWeight.Normal
                                }
                            )
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

            val name = categories.getOrNull(selected)
            val node = name?.let { tree.optJSONObject(it) }

            if (name == ControlsCategory) {
                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .verticalScroll(rememberScrollState())
                        .padding(horizontal = 16.dp, vertical = 12.dp)
                ) {
                    ControlsSettings()
                }
            } else if (node != null) {
                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .verticalScroll(rememberScrollState())
                        .padding(horizontal = 16.dp, vertical = 12.dp),
                    verticalArrangement = Arrangement.spacedBy(Dimens.SectionGap)
                ) {
                    SettingsNodeContent(node = node, path = name, titleId = titleId)
                    Spacer(Modifier.height(Dimens.SectionGap))
                }
            }
        }
    }
}

@Composable
private fun DrawerActionButton(
    label: String,
    icon: ImageVector,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    isExit: Boolean = false
) {
    val background = if (isExit) DrawerStyle.TileExitResting else DrawerStyle.PaneInnerResting
    val border = if (isExit) DrawerStyle.TileExitPressed else DrawerStyle.RestingCardBorder
    val tint = if (isExit) Color(0xFFE07B6B) else DrawerStyle.TextPrimary

    Row(
        modifier = modifier
            .background(background, RoundedCornerShape(14.dp))
            .border(1.dp, border, RoundedCornerShape(14.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 12.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Center
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = tint,
            modifier = Modifier.size(18.dp)
        )
        Spacer(Modifier.width(8.dp))
        Text(
            text = label,
            color = tint,
            fontSize = Dimens.ValueSize,
            fontWeight = FontWeight.SemiBold
        )
    }
}
