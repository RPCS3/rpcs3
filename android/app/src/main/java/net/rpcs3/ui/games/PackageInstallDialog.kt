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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import net.rpcs3.ui.theme.SettingsStyle
import net.rpcs3.utils.PackageInfo
import net.rpcs3.utils.PackageInspector

@Composable
fun PackageInstallDialog(
    packages: List<PackageInfo>,
    scanning: Boolean,
    freeSpace: Long,
    onConfirm: (List<PackageInfo>) -> Unit,
    onDismiss: () -> Unit
) {
    var excluded by remember(packages) { mutableStateOf(setOf<String>()) }

    val ordered = remember(packages) { PackageInspector.installOrder(packages) }
    val selected = ordered.filter { it.valid && it.uri.toString() !in excluded }
    val requiredSize = selected.sumOf { it.dataSize }
    val corrupted = ordered.filter { !it.valid }
    val enoughSpace = freeSpace <= 0L || requiredSize <= freeSpace

    Dialog(onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(18.dp))
                .background(SettingsStyle.BgDeep)
                .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(18.dp))
                .padding(18.dp)
        ) {
            Text(
                text = if (scanning) "Reading packages" else "Install ${selected.size} package${if (selected.size == 1) "" else "s"}",
                color = SettingsStyle.TextPrimary,
                fontSize = 15.sp,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(Modifier.height(4.dp))
            Text(
                text = if (scanning) {
                    "Reading title and version information."
                } else {
                    "Updates install oldest version first so each one matches the version it expects."
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
                LazyColumn(modifier = Modifier.heightIn(max = 320.dp)) {
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
                    text = "Required ${PackageInspector.formatSize(requiredSize)}" +
                        if (freeSpace > 0) "  ·  Free ${PackageInspector.formatSize(freeSpace)}" else "",
                    color = if (enoughSpace) SettingsStyle.TextSecondary else SettingsStyle.DangerRed,
                    fontSize = 12.sp
                )

                if (!enoughSpace) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = "Not enough space",
                        color = SettingsStyle.DangerRed,
                        fontSize = 12.sp,
                        fontWeight = FontWeight.SemiBold
                    )
                }

                if (corrupted.isNotEmpty()) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        text = "${corrupted.size} file${if (corrupted.size == 1) "" else "s"} skipped as unreadable",
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
                DialogButton(text = "Cancel", accent = false, enabled = true, onClick = onDismiss)
                Spacer(Modifier.width(10.dp))
                DialogButton(
                    text = "Install",
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
                    append(info.typeLabel)
                    if (info.appVer.isNotEmpty()) append("  ·  v").append(info.appVer)
                    if (info.targetAppVer.isNotEmpty()) append(" from v").append(info.targetAppVer)
                    if (info.valid) append("  ·  ").append(PackageInspector.formatSize(info.dataSize))
                },
                color = SettingsStyle.TextSecondary,
                fontSize = 11.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }

        if (index != null) {
            Spacer(Modifier.width(8.dp))
            Text(
                text = "#$index",
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
