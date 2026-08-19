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
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.CloudDownload
import androidx.compose.material.icons.outlined.Refresh
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.R
import net.rpcs3.ui.components.PaneActionButton
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.utils.PackageInspector
import net.rpcs3.utils.UpdateEntry

data class UpdateDownloadState(
    val loading: Boolean = false,
    val entries: List<UpdateEntry> = emptyList(),
    val errors: Map<String, String> = emptyMap(),
    val sourceCount: Int = 0,
    val searched: Boolean = false,
    val busyKey: String? = null,
    val progress: Float = -1f,
    val progressLabel: String = "",
    val failure: String? = null
)

@Composable
fun GameUpdateDownloadTab(
    state: UpdateDownloadState,
    installedVersions: Set<String>,
    onRefresh: () -> Unit,
    onDownload: (UpdateEntry) -> Unit,
    onManageSources: () -> Unit
) {
    val context = LocalContext.current

    PaneSectionTitle(
        pluralStringResource(
            R.plurals.updater_source_count,
            state.sourceCount,
            state.sourceCount
        )
    )

    Spacer(Modifier.height(8.dp))

    if (state.busyKey != null) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .background(Rpcs.SurfaceRaised, RoundedCornerShape(10.dp))
                .border(1.dp, Rpcs.OutlineSoft, RoundedCornerShape(10.dp))
                .padding(12.dp)
        ) {
            Text(
                text = state.progressLabel,
                color = Rpcs.TextPrimary,
                fontSize = 12.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(8.dp))

            if (state.progress >= 0f) {
                LinearProgressIndicator(
                    progress = { state.progress },
                    modifier = Modifier.fillMaxWidth(),
                    color = Rpcs.Accent,
                    trackColor = Rpcs.SurfaceInset
                )
            } else {
                LinearProgressIndicator(
                    modifier = Modifier.fillMaxWidth(),
                    color = Rpcs.Accent,
                    trackColor = Rpcs.SurfaceInset
                )
            }
        }

        Spacer(Modifier.height(12.dp))
    }

    if (state.failure != null) {
        Text(
            text = state.failure,
            color = Rpcs.Danger,
            fontSize = 11.sp,
            modifier = Modifier.padding(bottom = 8.dp)
        )
    }

    if (state.loading) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 18.dp),
            horizontalArrangement = Arrangement.Center
        ) {
            CircularProgressIndicator(
                modifier = Modifier.size(26.dp),
                color = Rpcs.Accent,
                strokeWidth = 3.dp
            )
        }
    } else if (state.entries.isEmpty()) {
        Text(
            text = if (state.searched) {
                stringResource(R.string.updater_none_found)
            } else {
                stringResource(R.string.updater_not_searched)
            },
            color = Rpcs.TextSecondary,
            fontSize = 12.sp
        )
        Spacer(Modifier.height(10.dp))
    } else {
        state.entries.forEach { entry ->
            UpdateDownloadRow(
                entry = entry,
                installed = entry.version in installedVersions,
                busy = state.busyKey == entry.key,
                sizeLabel = PackageInspector.formatSize(context, entry.sizeBytes),
                onDownload = { onDownload(entry) }
            )
            Spacer(Modifier.height(6.dp))
        }
    }

    if (state.errors.isNotEmpty()) {
        Spacer(Modifier.height(4.dp))
        state.errors.forEach { (source, message) ->
            Text(
                text = stringResource(R.string.updater_source_failed, source, message),
                color = Rpcs.Warning,
                fontSize = 11.sp
            )
        }
    }

    Spacer(Modifier.height(14.dp))

    PaneActionButton(
        label = stringResource(R.string.updater_search),
        icon = Icons.Outlined.Refresh,
        enabled = !state.loading && state.busyKey == null,
        onClick = onRefresh
    )

    Spacer(Modifier.height(8.dp))

    PaneActionButton(
        label = stringResource(R.string.updater_manage_sources),
        icon = Icons.Outlined.CloudDownload,
        enabled = true,
        onClick = onManageSources
    )

    Spacer(Modifier.height(16.dp))
}

@Composable
private fun UpdateDownloadRow(
    entry: UpdateEntry,
    installed: Boolean,
    busy: Boolean,
    sizeLabel: String,
    onDownload: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(10.dp))
            .border(1.dp, Rpcs.OutlineSoft, RoundedCornerShape(10.dp))
            .clickable(enabled = !busy && !installed, onClick = onDownload)
            .padding(horizontal = 12.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = stringResource(R.string.version_prefix, entry.version),
                color = Rpcs.TextPrimary,
                fontSize = 13.sp,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = buildString {
                    append(sizeLabel)
                    append("  ·  ")
                    append(
                        pluralStringResource(
                            R.plurals.updater_mirror_count,
                            entry.sources.size,
                            entry.sources.size
                        )
                    )
                    if (entry.systemVersion.isNotEmpty()) {
                        append("  ·  ").append(
                            stringResource(R.string.updater_firmware_needed, entry.systemVersion)
                        )
                    }
                },
                color = Rpcs.TextSecondary,
                fontSize = 11.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = entry.sources.joinToString(", "),
                color = Rpcs.TextDim,
                fontSize = 10.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }

        Spacer(Modifier.width(10.dp))

        when {
            busy -> CircularProgressIndicator(
                modifier = Modifier.size(18.dp),
                color = Rpcs.Accent,
                strokeWidth = 2.dp
            )

            installed -> Text(
                text = stringResource(R.string.updater_installed),
                color = Rpcs.Success,
                fontSize = 11.sp,
                fontWeight = FontWeight.SemiBold
            )

            else -> Box(
                modifier = Modifier
                    .background(Rpcs.SelectionFill, RoundedCornerShape(8.dp))
                    .border(1.dp, Rpcs.SelectionBorder, RoundedCornerShape(8.dp))
                    .clickable(onClick = onDownload)
                    .padding(horizontal = 10.dp, vertical = 6.dp)
            ) {
                Icon(
                    imageVector = Icons.Outlined.CloudDownload,
                    contentDescription = stringResource(R.string.updater_download),
                    tint = Rpcs.Accent,
                    modifier = Modifier.size(16.dp)
                )
            }
        }
    }
}
