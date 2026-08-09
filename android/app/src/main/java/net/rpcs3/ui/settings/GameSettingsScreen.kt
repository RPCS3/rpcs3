package net.rpcs3.ui.settings

import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
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
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Article
import androidx.compose.material.icons.outlined.Folder
import androidx.compose.material.icons.outlined.HourglassEmpty
import androidx.compose.material.icons.outlined.Layers
import androidx.compose.material.icons.outlined.Memory
import androidx.compose.material.icons.outlined.MoreHoriz
import androidx.compose.material.icons.outlined.Public
import androidx.compose.material.icons.outlined.Save
import androidx.compose.material.icons.outlined.SettingsApplications
import androidx.compose.material.icons.outlined.Speed
import androidx.compose.material.icons.outlined.SportsEsports
import androidx.compose.material.icons.outlined.Tune
import androidx.compose.material.icons.outlined.Videocam
import androidx.compose.material.icons.outlined.VolumeUp
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.RPCS3
import net.rpcs3.ui.components.GhostButton
import net.rpcs3.ui.components.SectionLabel
import net.rpcs3.ui.components.SettingGroup
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.SettingsStyle
import org.json.JSONObject

internal fun iconForCategory(name: String): ImageVector = when (name) {
    "Core" -> Icons.Outlined.Memory
    "Video" -> Icons.Outlined.Videocam
    "Audio" -> Icons.Outlined.VolumeUp
    "Input/Output" -> Icons.Outlined.SportsEsports
    "Net" -> Icons.Outlined.Public
    "System" -> Icons.Outlined.SettingsApplications
    "VFS" -> Icons.Outlined.Folder
    "Savestate" -> Icons.Outlined.Save
    "Performance Overlay" -> Icons.Outlined.Speed
    "Shader Loading Dialog" -> Icons.Outlined.HourglassEmpty
    "Vulkan" -> Icons.Outlined.Layers
    "Miscellaneous" -> Icons.Outlined.Tune
    "Log" -> Icons.Outlined.Article
    ControlsCategory -> Icons.Outlined.SportsEsports
    else -> Icons.Outlined.MoreHoriz
}

internal data class SettingsCategory(
    val label: String,
    val path: List<String>,
    val parent: String?
)

private fun subGroupsOf(node: JSONObject): List<String> {
    val names = ArrayList<String>()
    val keys = node.keys()
    while (keys.hasNext()) {
        val key = keys.next()
        val child = node.optJSONObject(key) ?: continue
        if (child.optString("type", "").isEmpty()) names.add(key)
    }
    return names
}

private fun hasSettings(node: JSONObject): Boolean {
    val keys = node.keys()
    while (keys.hasNext()) {
        val child = node.optJSONObject(keys.next()) ?: continue
        if (child.optString("type", "").isNotEmpty()) return true
        if (hasSettings(child)) return true
    }
    return false
}

internal fun categoriesOf(root: JSONObject): List<SettingsCategory> {
    val out = ArrayList<SettingsCategory>()
    for (name in subGroupsOf(root)) {
        val node = root.optJSONObject(name) ?: continue
        if (!hasSettings(node)) continue
        out.add(SettingsCategory(name, listOf(name), null))
        for (sub in subGroupsOf(node)) {
            val child = node.optJSONObject(sub) ?: continue
            if (!hasSettings(child)) continue
            out.add(SettingsCategory(sub, listOf(name, sub), name))
        }
    }
    return out
}

@Composable
fun GameSettingsScreen(
    titleId: String,
    title: String,
    modifier: Modifier = Modifier,
    onClose: (() -> Unit)? = null
) {
    var root by remember(titleId) { mutableStateOf<JSONObject?>(null) }
    var baseline by remember(titleId) { mutableStateOf<JSONObject?>(null) }
    var selected by remember(titleId) { mutableIntStateOf(0) }
    val scope = rememberCoroutineScope()

    LaunchedEffect(titleId) {
        root = withContext(Dispatchers.IO) {
            runCatching { JSONObject(RPCS3.instance.settingsGet("", titleId)) }.getOrNull()
        }
        baseline = root?.let { snapshotOf(it) }
    }

    val tree = root

    Box(
        modifier = modifier
            .fillMaxSize()
            .background(SettingsStyle.BgDeep)
            .windowInsetsPadding(WindowInsets.systemBars)
    ) {
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
        val currentName = categories.getOrNull(selected)?.label

        Row(modifier = Modifier.fillMaxSize()) {
            Column(
                modifier = Modifier
                    .width(SettingsStyle.SidebarWidth)
                    .fillMaxHeight()
                    .background(SettingsStyle.SidebarBg)
                    .padding(top = 14.dp, bottom = 12.dp)
            ) {
                Text(
                    text = title,
                    modifier = Modifier.padding(start = 16.dp, end = 16.dp, bottom = 10.dp),
                    color = SettingsStyle.TextPrimary,
                    fontSize = 11.sp,
                    fontWeight = FontWeight.SemiBold,
                    letterSpacing = 0.2.sp,
                    lineHeight = 15.sp,
                    maxLines = 2,
                    overflow = TextOverflow.Ellipsis
                )
                Box(
                    Modifier
                        .padding(horizontal = 12.dp)
                        .fillMaxWidth()
                        .height(1.dp)
                        .background(SettingsStyle.Divider)
                )
                Spacer(Modifier.height(8.dp))

                Column(
                    modifier = Modifier
                        .weight(1f)
                        .verticalScroll(rememberScrollState()),
                    verticalArrangement = Arrangement.spacedBy(1.dp)
                ) {
                    categories.forEachIndexed { index, entry ->
                        SidebarItem(
                            icon = iconForCategory(entry.label),
                            label = entry.label,
                            isSelected = index == selected,
                            nested = entry.parent != null,
                            onClick = { selected = index }
                        )
                    }
                }

                if (onClose != null) {
                    Box(
                        Modifier
                            .padding(horizontal = 12.dp)
                            .fillMaxWidth()
                            .height(1.dp)
                            .background(SettingsStyle.Divider)
                    )
                    Spacer(Modifier.height(8.dp))
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 12.dp),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        GhostButton(
                            label = "Cancel",
                            accent = true,
                            tint = Rpcs.Danger,
                            onClick = {
                                val snapshot = baseline
                                scope.launch(Dispatchers.IO) {
                                    if (snapshot != null) {
                                        revertTo(snapshot, tree, titleId)
                                    }
                                    withContext(Dispatchers.Main) { onClose() }
                                }
                            },
                            modifier = Modifier.weight(1f)
                        )
                        GhostButton(
                            label = "Save",
                            accent = true,
                            tint = Rpcs.Success,
                            onClick = {
                                scope.launch(Dispatchers.IO) {
                                    RPCS3.instance.settingsFlush()
                                    withContext(Dispatchers.Main) { onClose() }
                                }
                            },
                            modifier = Modifier.weight(1f)
                        )
                    }
                }
            }

            Box(
                Modifier
                    .width(1.dp)
                    .fillMaxHeight()
                    .background(SettingsStyle.Divider)
            )

            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
                    .background(SettingsStyle.ContentBg)
                    .widthIn(max = Dims.ContentMaxWidth)
            ) {
                AnimatedContent(
                    targetState = selected,
                    transitionSpec = {
                        val direction = if (targetState > initialState) 1 else -1
                        (slideInHorizontally(tween(220)) { direction * it / 6 } + fadeIn(tween(200)))
                            .togetherWith(
                                slideOutHorizontally(tween(180)) { -direction * it / 6 } +
                                    fadeOut(tween(120))
                            )
                    },
                    label = "settingsSection"
                ) { index ->
                    val entry = categories.getOrNull(index)
                    val name = entry?.label
                    val node = entry?.let { e ->
                        var cursor: JSONObject? = tree
                        for (step in e.path) cursor = cursor?.optJSONObject(step)
                        cursor
                    }

                    if (name == ControlsCategory) {
                        Column(
                            modifier = Modifier
                                .fillMaxSize()
                                .verticalScroll(rememberScrollState())
                                .padding(horizontal = 20.dp, vertical = 14.dp)
                        ) {
                            ControlsSettings()
                        }
                    } else if (node == null) {
                        Box(Modifier.fillMaxSize())
                    } else {
                        Column(
                            modifier = Modifier
                                .fillMaxSize()
                                .verticalScroll(rememberScrollState())
                                .padding(horizontal = 20.dp, vertical = 14.dp),
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
}

@Composable
private fun SidebarItem(
    icon: ImageVector,
    label: String,
    isSelected: Boolean,
    onClick: () -> Unit,
    nested: Boolean = false
) {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = if (nested) 20.dp else 8.dp, end = 8.dp)
            .background(
                if (isSelected) SettingsStyle.AccentBlue.copy(alpha = 0.10f) else Color.Transparent,
                RoundedCornerShape(8.dp)
            )
            .border(
                1.dp,
                if (isSelected) SettingsStyle.AccentBlue.copy(alpha = 0.5f) else Color.Transparent,
                RoundedCornerShape(8.dp)
            )
            .clickable(onClick = onClick)
            .padding(horizontal = 12.dp, vertical = 8.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = if (isSelected) SettingsStyle.AccentBlue else SettingsStyle.TextDim,
                modifier = Modifier.size(Dimens.IconSize)
            )
            Spacer(Modifier.width(10.dp))
            Text(
                text = label,
                color = if (isSelected) SettingsStyle.TextPrimary else SettingsStyle.TextSecondary,
                fontSize = Dimens.ValueSize,
                fontWeight = if (isSelected) FontWeight.SemiBold else FontWeight.Normal,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis
            )
        }
    }
}

@Composable
internal fun SettingsNodeContent(
    node: JSONObject,
    path: String,
    titleId: String,
    includeSubGroups: Boolean = true
) {
    val leaves = ArrayList<Pair<String, JSONObject>>()
    val branches = ArrayList<Pair<String, JSONObject>>()

    val keys = node.keys()
    while (keys.hasNext()) {
        val key = keys.next()
        val child = node.optJSONObject(key) ?: continue
        if (child.optString("type", "").isEmpty()) {
            branches.add(key to child)
        } else {
            leaves.add(key to child)
        }
    }

    if (leaves.isNotEmpty()) {
        SettingGroup {
            leaves.forEach { (key, item) ->
                SettingItem(
                    label = key,
                    item = item,
                    path = "$path@@$key",
                    titleId = titleId
                )
            }
        }
    }

    if (includeSubGroups) {
        branches.forEach { (key, child) ->
            SectionLabel(text = key)
            SettingsNodeContent(node = child, path = "$path@@$key", titleId = titleId)
        }
    }
}
