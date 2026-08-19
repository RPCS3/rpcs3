package net.rpcs3.ui.updater

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
import androidx.compose.material.icons.outlined.Add
import androidx.compose.material.icons.outlined.CloudDownload
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.Edit
import androidx.compose.material.icons.outlined.Restore
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.R
import net.rpcs3.ui.components.PaneCard
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.utils.UpdateFinder
import net.rpcs3.utils.UpdateSource
import net.rpcs3.utils.UpdateSourceFormat
import net.rpcs3.utils.UpdateSources

@Composable
fun UpdaterScreen(onClose: () -> Unit) {
    val context = LocalContext.current
    val prefs = remember { UpdateSources.prefsOf(context) }
    var sources by remember { mutableStateOf(UpdateSources.load(prefs)) }
    var showAdd by remember { mutableStateOf(false) }
    var editing by remember { mutableStateOf<Pair<Int, UpdateSource>?>(null) }

    fun persist(next: List<UpdateSource>) {
        UpdateSources.save(prefs, next)
        UpdateFinder.clearCache()
        sources = next
    }

    if (showAdd || editing != null) {
        SourceDialog(
            existing = editing?.second,
            onDismiss = {
                showAdd = false
                editing = null
            },
            onConfirm = { name, url, format, insecure ->
                val normalized = UpdateSources.normalize(name, url, format, insecure)
                val target = editing

                if (target != null) {
                    persist(sources.toMutableList().also { it[target.first] = normalized })
                } else {
                    persist(sources + normalized)
                }

                showAdd = false
                editing = null
            }
        )
    }

    PaneScaffold(
        title = stringResource(R.string.updater_title),
        tabs = listOf(PaneTab(stringResource(R.string.updater_tab_sources), Icons.Outlined.CloudDownload)),
        selected = 0,
        onSelect = {},
        onBack = onClose
    ) {
        PaneSectionTitle(stringResource(R.string.updater_sources_help))
        Spacer(Modifier.height(10.dp))

        if (sources.isEmpty()) {
            Text(
                text = stringResource(R.string.updater_no_sources),
                color = Rpcs.TextSecondary,
                fontSize = 12.sp
            )
            Spacer(Modifier.height(10.dp))
        } else {
            PaneCard {
                sources.forEachIndexed { index, source ->
                    SourceRow(
                        source = source,
                        onToggle = {
                            persist(
                                sources.toMutableList().also {
                                    it[index] = source.copy(enabled = !source.enabled)
                                }
                            )
                        },
                        onEdit = { editing = index to source },
                        onDelete = {
                            persist(sources.toMutableList().also { it.removeAt(index) })
                        }
                    )
                }
            }
        }

        Spacer(Modifier.height(14.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            SmallAction(
                label = stringResource(R.string.updater_add_source),
                icon = Icons.Outlined.Add,
                modifier = Modifier.weight(1f),
                onClick = { showAdd = true }
            )
            SmallAction(
                label = stringResource(R.string.updater_restore_defaults),
                icon = Icons.Outlined.Restore,
                modifier = Modifier.weight(1f),
                onClick = { persist(UpdateSources.withDefaultsRestored(sources)) }
            )
        }

        Spacer(Modifier.height(16.dp))
    }
}

@Composable
private fun SourceRow(
    source: UpdateSource,
    onToggle: () -> Unit,
    onEdit: () -> Unit,
    onDelete: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(onClick = onToggle)
            .padding(horizontal = 12.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            modifier = Modifier
                .size(10.dp)
                .background(
                    if (source.enabled) Rpcs.Success else Rpcs.TextDim,
                    RoundedCornerShape(5.dp)
                )
        )

        Spacer(Modifier.width(10.dp))

        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = source.name,
                color = if (source.enabled) Rpcs.TextPrimary else Rpcs.TextDim,
                fontSize = 13.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = source.urlTemplate,
                color = Rpcs.TextSecondary,
                fontSize = 10.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = if (source.insecureTls) {
                    stringResource(R.string.updater_format_insecure, formatLabel(source.format))
                } else {
                    formatLabel(source.format)
                },
                color = if (source.insecureTls) Rpcs.Warning else Rpcs.TextDim,
                fontSize = 10.sp
            )
        }

        Spacer(Modifier.width(8.dp))

        Icon(
            imageVector = Icons.Outlined.Edit,
            contentDescription = stringResource(R.string.updater_edit_source),
            tint = Rpcs.TextSecondary,
            modifier = Modifier
                .size(30.dp)
                .clickable(onClick = onEdit)
                .padding(6.dp)
        )

        Icon(
            imageVector = Icons.Outlined.Delete,
            contentDescription = stringResource(R.string.updater_delete_source),
            tint = Rpcs.Danger,
            modifier = Modifier
                .size(30.dp)
                .clickable(onClick = onDelete)
                .padding(6.dp)
        )
    }
}

@Composable
private fun formatLabel(format: UpdateSourceFormat) = when (format) {
    UpdateSourceFormat.SonyVerXml -> stringResource(R.string.updater_format_verxml)
    UpdateSourceFormat.IndexHtml -> stringResource(R.string.updater_format_index)
    UpdateSourceFormat.LinkScrape -> stringResource(R.string.updater_format_links)
}

@Composable
private fun SourceDialog(
    existing: UpdateSource?,
    onDismiss: () -> Unit,
    onConfirm: (name: String, url: String, format: UpdateSourceFormat?, insecure: Boolean) -> Unit
) {
    var name by remember { mutableStateOf(existing?.name.orEmpty()) }
    var url by remember { mutableStateOf(existing?.urlTemplate.orEmpty()) }
    var format by remember { mutableStateOf(existing?.format) }
    var insecure by remember { mutableStateOf(existing?.insecureTls ?: false) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = {
            Text(
                if (existing == null) {
                    stringResource(R.string.updater_add_source)
                } else {
                    stringResource(R.string.updater_edit_source)
                }
            )
        },
        text = {
            Column {
                OutlinedTextField(
                    value = name,
                    onValueChange = { name = it },
                    label = { Text(stringResource(R.string.updater_source_name)) },
                    singleLine = true
                )
                Spacer(Modifier.height(10.dp))
                OutlinedTextField(
                    value = url,
                    onValueChange = {
                        url = it
                        format = null
                    },
                    label = { Text(stringResource(R.string.updater_source_url)) },
                    singleLine = true
                )
                Spacer(Modifier.height(8.dp))
                Text(
                    text = stringResource(R.string.updater_source_url_help),
                    color = Rpcs.TextSecondary,
                    fontSize = 11.sp
                )
                Spacer(Modifier.height(10.dp))

                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clickable { insecure = !insecure }
                        .padding(vertical = 6.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Box(
                        modifier = Modifier
                            .size(16.dp)
                            .background(
                                if (insecure) Rpcs.Warning else Rpcs.SurfaceInset,
                                RoundedCornerShape(4.dp)
                            )
                            .border(
                                1.dp,
                                if (insecure) Rpcs.Warning else Rpcs.Outline,
                                RoundedCornerShape(4.dp)
                            )
                    )
                    Spacer(Modifier.width(8.dp))
                    Text(
                        text = stringResource(R.string.updater_insecure_tls),
                        color = if (insecure) Rpcs.Warning else Rpcs.TextSecondary,
                        fontSize = 11.sp
                    )
                }

                Spacer(Modifier.height(6.dp))

                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    UpdateSourceFormat.entries.forEach { option ->
                        val active = (format ?: UpdateSources.guessFormat(url)) == option

                        Box(
                            modifier = Modifier
                                .background(
                                    if (active) Rpcs.SelectionFill else Rpcs.SurfaceInset,
                                    RoundedCornerShape(8.dp)
                                )
                                .border(
                                    1.dp,
                                    if (active) Rpcs.SelectionBorder else Rpcs.Outline,
                                    RoundedCornerShape(8.dp)
                                )
                                .clickable { format = option }
                                .padding(horizontal = 8.dp, vertical = 6.dp)
                        ) {
                            Text(
                                text = formatLabel(option),
                                color = if (active) Rpcs.Accent else Rpcs.TextSecondary,
                                fontSize = 10.sp
                            )
                        }
                    }
                }
            }
        },
        confirmButton = {
            TextButton(
                enabled = url.isNotBlank(),
                onClick = { onConfirm(name, url, format, insecure) }
            ) {
                Text(stringResource(android.R.string.ok))
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text(stringResource(android.R.string.cancel))
            }
        }
    )
}

@Composable
private fun SmallAction(
    label: String,
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    Row(
        modifier = modifier
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(10.dp))
            .border(1.dp, Rpcs.Outline, RoundedCornerShape(10.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 10.dp, vertical = 9.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Center
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = Rpcs.Accent,
            modifier = Modifier.size(16.dp)
        )
        Spacer(Modifier.width(6.dp))
        Text(
            text = label,
            color = Rpcs.TextPrimary,
            fontSize = 12.sp,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
    }
}
