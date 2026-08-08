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
import androidx.compose.foundation.interaction.collectIsFocusedAsState
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
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.RPCS3
import net.rpcs3.ui.settings.ControlsCategory
import net.rpcs3.ui.settings.ControlsSettings
import net.rpcs3.ui.settings.SettingsCategory
import net.rpcs3.ui.settings.SettingsNodeContent
import net.rpcs3.ui.settings.categoriesOf
import net.rpcs3.ui.settings.iconForCategory
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.DrawerStyle
import net.rpcs3.ui.theme.Rpcs
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
                        onClose = onDismiss,
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
private fun InGameSettingsPanel(
    titleId: String,
    modifier: Modifier = Modifier,
    onClose: (() -> Unit)? = null
) {
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

        val categories = remember(tree) {
            categoriesOf(tree) + SettingsCategory(ControlsCategory, listOf(ControlsCategory), null)
        }

        Column(modifier = Modifier.fillMaxSize()) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(start = 16.dp, end = 8.dp, top = 10.dp, bottom = 6.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    modifier = Modifier.weight(1f),
                    text = if (titleId.isEmpty()) "Settings" else titleId,
                    color = SettingsStyle.TextPrimary,
                    fontSize = 13.sp,
                    fontWeight = FontWeight.SemiBold
                )

                if (onClose != null) {
                    Box(
                        modifier = Modifier
                            .size(34.dp)
                            .clip(RoundedCornerShape(10.dp))
                            .clickable(onClick = onClose),
                        contentAlignment = Alignment.Center
                    ) {
                        Icon(
                            imageVector = Icons.Outlined.Close,
                            contentDescription = "Close",
                            tint = SettingsStyle.TextPrimary,
                            modifier = Modifier.size(19.dp)
                        )
                    }
                }
            }

            Box(
                Modifier
                    .fillMaxWidth()
                    .height(1.dp)
                    .background(SettingsStyle.Divider)
            )

            Row(modifier = Modifier.fillMaxSize()) {
                Column(
                    modifier = Modifier
                        .width(SettingsStyle.SidebarWidth)
                        .fillMaxHeight()
                        .background(SettingsStyle.SidebarBg)
                        .verticalScroll(rememberScrollState())
                        .padding(vertical = 8.dp),
                    verticalArrangement = Arrangement.spacedBy(1.dp)
                ) {
                    categories.forEachIndexed { index, entry ->
                        DrawerSidebarItem(
                            icon = iconForCategory(entry.label),
                            label = entry.label,
                            isSelected = index == selected,
                            nested = entry.parent != null,
                            onClick = { selected = index }
                        )
                    }
                }

                Box(
                    Modifier
                        .fillMaxHeight()
                        .width(1.dp)
                        .background(SettingsStyle.Divider)
                )

            val entry = categories.getOrNull(selected)
            val name = entry?.label
            val node = entry?.let { target ->
                var cursor: JSONObject? = tree
                for (step in target.path) cursor = cursor?.optJSONObject(step)
                cursor
            }

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
                    SettingsNodeContent(
                        node = node,
                        path = entry.path.joinToString("@@"),
                        titleId = titleId,
                        includeSubGroups = entry.parent != null
                    )
                    Spacer(Modifier.height(Dimens.SectionGap))
                }
            }
            }
        }
    }
}

@Composable
private fun DrawerSidebarItem(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    label: String,
    isSelected: Boolean,
    nested: Boolean,
    onClick: () -> Unit
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = if (nested) 16.dp else 6.dp, end = 6.dp)
            .clip(RoundedCornerShape(9.dp))
            .background(if (isSelected) DrawerStyle.FocusFill else Color.Transparent)
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else Color.Transparent,
                RoundedCornerShape(9.dp)
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick)
            .padding(horizontal = 10.dp, vertical = 9.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = if (isSelected || focused) SettingsStyle.AccentBlue else SettingsStyle.TextSecondary,
                modifier = Modifier.size(Dimens.IconSize)
            )
            Spacer(Modifier.width(8.dp))
            Text(
                text = label,
                color = if (isSelected || focused) SettingsStyle.TextPrimary else SettingsStyle.TextSecondary,
                fontSize = Dimens.ValueSize,
                fontWeight = if (isSelected) FontWeight.SemiBold else FontWeight.Normal,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis,
                lineHeight = 14.sp
            )
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
    val tint = if (isExit) DrawerStyle.ExitTint else DrawerStyle.TextPrimary

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
