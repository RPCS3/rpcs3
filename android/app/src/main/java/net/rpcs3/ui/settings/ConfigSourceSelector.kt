package net.rpcs3.ui.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.R
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.SettingsStyle
import net.rpcs3.utils.ConfigSource
import net.rpcs3.utils.RecommendedEntry

@Composable
internal fun ConfigSourceSelector(
    titleId: String,
    source: ConfigSource,
    entry: RecommendedEntry?,
    onSelect: (ConfigSource) -> Unit,
    modifier: Modifier = Modifier
) {
    var expanded by remember { mutableStateOf(false) }
    val context = LocalContext.current
    val shape = RoundedCornerShape(Dims.InputCorner)
    val available = entry != null && !entry.isEmpty

    val recommendedLabel = stringResource(R.string.config_source_recommended)
    val globalLabel = stringResource(R.string.config_source_global)
    val recommendedDetail = when {
        entry == null -> stringResource(R.string.config_source_recommended_missing)
        entry.isEmpty -> stringResource(R.string.config_source_recommended_none)
        else -> pluralStringResource(
            R.plurals.config_source_recommended_detail,
            entry.settings.size,
            entry.settings.size
        )
    }

    fun choose(next: ConfigSource) {
        expanded = false

        if (next == source) {
            return
        }

        AlertDialogQueue.showDialog(
            title = context.getString(
                R.string.config_source_switch_title,
                if (next == ConfigSource.Recommended) recommendedLabel else globalLabel,
                titleId
            ),
            message = if (next == ConfigSource.Recommended) {
                context.getString(
                    R.string.config_source_switch_recommended_message,
                    entry?.settings?.size ?: 0,
                    titleId
                )
            } else {
                context.getString(R.string.config_source_switch_global_message, titleId)
            },
            confirmText = context.getString(R.string.action_apply),
            dismissText = context.getString(R.string.action_cancel),
            onConfirm = { onSelect(next) }
        )
    }

    Box(modifier = modifier) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .background(
                    if (expanded) {
                        SettingsStyle.AccentBlue.copy(alpha = 0.10f)
                    } else {
                        Color.Transparent
                    },
                    shape
                )
                .border(
                    Dims.BorderWidth,
                    if (expanded) {
                        SettingsStyle.AccentBlue.copy(alpha = 0.5f)
                    } else {
                        SettingsStyle.Divider
                    },
                    shape
                )
                .clickable { expanded = !expanded }
                .padding(horizontal = 10.dp, vertical = 7.dp)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = titleId,
                    modifier = Modifier.weight(1f),
                    color = SettingsStyle.TextPrimary,
                    fontSize = 12.sp,
                    fontWeight = FontWeight.SemiBold,
                    letterSpacing = 0.2.sp,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
                Icon(
                    imageVector = if (expanded) {
                        Icons.Default.KeyboardArrowUp
                    } else {
                        Icons.Default.KeyboardArrowDown
                    },
                    contentDescription = stringResource(R.string.config_source_expand),
                    tint = SettingsStyle.TextSecondary,
                    modifier = Modifier.size(18.dp)
                )
            }
            Text(
                text = if (source == ConfigSource.Recommended) recommendedLabel else globalLabel,
                color = if (source == ConfigSource.Recommended) {
                    SettingsStyle.AccentBlue
                } else {
                    SettingsStyle.TextSecondary
                },
                fontSize = 10.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }

        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false },
            shape = RoundedCornerShape(Dims.InputCorner),
            containerColor = Rpcs.SurfaceInset,
            modifier = Modifier.width(SettingsStyle.SidebarWidth)
        ) {
            SourceMenuItem(
                label = recommendedLabel,
                detail = recommendedDetail,
                selected = source == ConfigSource.Recommended,
                enabled = available,
                onClick = { choose(ConfigSource.Recommended) }
            )
            SourceMenuItem(
                label = globalLabel,
                detail = stringResource(R.string.config_source_global_detail),
                selected = source == ConfigSource.Global,
                enabled = true,
                onClick = { choose(ConfigSource.Global) }
            )
        }
    }
}

@Composable
private fun SourceMenuItem(
    label: String,
    detail: String,
    selected: Boolean,
    enabled: Boolean,
    onClick: () -> Unit
) {
    DropdownMenuItem(
        enabled = enabled,
        text = {
            Column {
                Text(
                    text = label,
                    color = when {
                        !enabled -> Rpcs.TextDim
                        selected -> SettingsStyle.AccentBlue
                        else -> SettingsStyle.TextPrimary
                    },
                    fontSize = 13.sp,
                    fontWeight = if (selected) FontWeight.Bold else FontWeight.Medium
                )
                Spacer(Modifier.size(2.dp))
                Text(
                    text = detail,
                    color = Rpcs.TextDim,
                    fontSize = 10.sp
                )
            }
        },
        onClick = onClick
    )
}
