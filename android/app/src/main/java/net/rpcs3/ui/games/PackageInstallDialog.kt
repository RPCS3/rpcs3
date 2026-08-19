package net.rpcs3.ui.games

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Check
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import net.rpcs3.R
import net.rpcs3.ui.theme.SettingsStyle
import net.rpcs3.utils.PackageInfo
import net.rpcs3.utils.PackageInspector

@Composable
fun PackageInstallDialog(
    packages: List<PackageInfo>,
    scanning: Boolean,
    freeSpace: Long,
    onConfirm: (List<PackageInfo>) -> Unit,
    onDismiss: () -> Unit,
    expectedTitleId: String = ""
) {
    var excluded by remember(packages) { mutableStateOf(setOf<String>()) }

    val ordered = remember(packages) { PackageInspector.installOrder(packages) }
    val selected = ordered.filter { it.valid && it.uri.toString() !in excluded }
    val requiredSize = selected.sumOf { it.dataSize }
    val corrupted = ordered.filter { !it.valid }
    val foreign = if (expectedTitleId.isEmpty()) {
        emptyList()
    } else {
        selected.filter { it.titleId.isNotEmpty() && it.titleId != expectedTitleId }
    }
    val enoughSpace = freeSpace <= 0L || requiredSize <= freeSpace

    val context = LocalContext.current
    val configuration = LocalConfiguration.current
    val screenHeight = configuration.screenHeightDp.dp
    val listMaxHeight = (configuration.screenHeightDp - 250).coerceAtLeast(120).dp

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false)
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth(0.94f)
                .widthIn(max = 620.dp)
                .heightIn(max = screenHeight - 24.dp)
                .clip(RoundedCornerShape(18.dp))
                .background(SettingsStyle.BgDeep)
                .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(18.dp))
                .padding(horizontal = 18.dp, vertical = 14.dp)
        ) {
            Text(
                text = if (scanning) {
                    stringResource(R.string.packages_reading)
                } else {
                    pluralStringResource(
                        R.plurals.packages_install_count,
                        selected.size,
                        selected.size
                    )
                },
                color = SettingsStyle.TextPrimary,
                fontSize = 15.sp,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(Modifier.height(4.dp))
            Text(
                text = if (scanning) {
                    stringResource(R.string.packages_reading_detail)
                } else {
                    stringResource(R.string.packages_install_detail)
                },
                color = SettingsStyle.TextSecondary,
                fontSize = 12.sp
            )

            Spacer(Modifier.height(14.dp))

            if (scanning) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.Center
                ) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(28.dp),
                        color = SettingsStyle.AccentBlue,
                        strokeWidth = 3.dp
                    )
                }
            } else {
                LazyColumn(modifier = Modifier.heightIn(max = listMaxHeight)) {
                    items(ordered) { info ->
                        val key = info.uri.toString()
                        val isSelected = info.valid && key !in excluded

                        PackageRow(
                            info = info,
                            index = if (isSelected) selected.indexOf(info) + 1 else null,
                            selected = isSelected,
                            onToggle = {
                                if (!info.valid) return@PackageRow
                                excluded = if (key in excluded) excluded - key else excluded + key
                            }
                        )
                        Spacer(Modifier.height(6.dp))
                    }
                }

                Spacer(Modifier.height(12.dp))

                Text(
                    text = stringResource(
                        R.string.packages_required_size,
                        PackageInspector.formatSize(context, requiredSize)
                    ) + if (freeSpace > 0) {
                        stringResource(
                            R.string.packages_free_size,
                            PackageInspector.formatSize(context, freeSpace)
                        )
                    } else {
                        ""
                    },
                    color = if (enoughSpace) SettingsStyle.TextSecondary else SettingsStyle.DangerRed,
                    fontSize = 12.sp
                )

                if (!enoughSpace) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = stringResource(R.string.packages_not_enough_space),
                        color = SettingsStyle.DangerRed,
                        fontSize = 12.sp,
                        fontWeight = FontWeight.SemiBold
                    )
                }

                if (corrupted.isNotEmpty()) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = pluralStringResource(
                            R.plurals.packages_skipped_count,
                            corrupted.size,
                            corrupted.size
                        ),
                        color = SettingsStyle.WarningAmber,
                        fontSize = 12.sp
                    )
                }

                if (foreign.isNotEmpty()) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = stringResource(
                            R.string.packages_other_title,
                            foreign.joinToString(", ") { it.titleId }.take(120),
                            expectedTitleId
                        ),
                        color = SettingsStyle.WarningAmber,
                        fontSize = 12.sp
                    )
                }
            }

            Spacer(Modifier.height(16.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.End
            ) {
                DialogButton(
                    text = stringResource(R.string.action_cancel),
                    accent = false,
                    enabled = true,
                    onClick = onDismiss
                )
                Spacer(Modifier.width(10.dp))
                DialogButton(
                    text = stringResource(R.string.action_install),
                    accent = true,
                    enabled = !scanning && selected.isNotEmpty() && enoughSpace,
                    onClick = { onConfirm(selected) }
                )
            }
        }
    }
}

@Composable
private fun PackageRow(
    info: PackageInfo,
    index: Int?,
    selected: Boolean,
    onToggle: () -> Unit
) {
    val context = LocalContext.current

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(10.dp))
            .background(
                if (selected) SettingsStyle.AccentBlue.copy(alpha = 0.10f) else SettingsStyle.CardSurface
            )
            .border(
                1.dp,
                if (selected) SettingsStyle.AccentBlue.copy(alpha = 0.45f) else SettingsStyle.CardBorder,
                RoundedCornerShape(10.dp)
            )
            .clickable(onClick = onToggle)
            .padding(horizontal = 10.dp, vertical = 9.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            modifier = Modifier
                .size(20.dp)
                .clip(RoundedCornerShape(5.dp))
                .background(if (selected) SettingsStyle.AccentBlue else Color.Transparent)
                .border(
                    1.dp,
                    if (selected) SettingsStyle.AccentBlue else SettingsStyle.CheckBorder,
                    RoundedCornerShape(5.dp)
                ),
            contentAlignment = Alignment.Center
        ) {
            if (selected) {
                Icon(
                    imageVector = Icons.Default.Check,
                    contentDescription = null,
                    tint = Color.White,
                    modifier = Modifier.size(14.dp)
                )
            }
        }

        Spacer(Modifier.width(10.dp))

        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = info.label,
                color = if (info.valid) SettingsStyle.TextPrimary else SettingsStyle.DangerRed,
                fontSize = 13.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = buildString {
                    if (info.titleId.isNotEmpty()) append(info.titleId).append("  ·  ")
                    append(info.typeLabel(context))
                    if (info.appVer.isNotEmpty()) {
                        append("  ·  ")
                            .append(context.getString(R.string.version_prefix, info.appVer))
                    }
                    if (info.targetAppVer.isNotEmpty()) {
                        append(
                            context.getString(
                                R.string.packages_target_version,
                                info.targetAppVer
                            )
                        )
                    }
                    if (info.valid) {
                        append("  ·  ")
                            .append(PackageInspector.formatSize(context, info.dataSize))
                    }
                },
                color = SettingsStyle.TextSecondary,
                fontSize = 11.sp,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis
            )
        }

        if (index != null) {
            Spacer(Modifier.width(8.dp))
            Text(
                text = stringResource(R.string.packages_index, index),
                color = SettingsStyle.AccentBlue,
                fontSize = 11.sp,
                fontWeight = FontWeight.SemiBold
            )
        }
    }
}

@Composable
private fun DialogButton(
    text: String,
    accent: Boolean,
    enabled: Boolean,
    onClick: () -> Unit
) {
    val background = when {
        !enabled -> SettingsStyle.CardSurface
        accent -> SettingsStyle.AccentBlue.copy(alpha = 0.16f)
        else -> SettingsStyle.CardSurface
    }
    val border = when {
        !enabled -> SettingsStyle.CardBorder
        accent -> SettingsStyle.AccentBlue
        else -> SettingsStyle.CardBorder
    }
    val textColor = when {
        !enabled -> SettingsStyle.TextDim
        accent -> SettingsStyle.AccentBlue
        else -> SettingsStyle.TextSecondary
    }

    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(9.dp))
            .background(background)
            .border(1.dp, border, RoundedCornerShape(9.dp))
            .then(if (enabled) Modifier.clickable(onClick = onClick) else Modifier)
            .padding(horizontal = 18.dp, vertical = 9.dp)
    ) {
        Text(text = text, color = textColor, fontSize = 13.sp, fontWeight = FontWeight.SemiBold)
    }
}
