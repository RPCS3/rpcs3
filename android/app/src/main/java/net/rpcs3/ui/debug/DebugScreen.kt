package net.rpcs3.ui.debug

import android.content.Intent
import android.provider.DocumentsContract
import android.widget.Toast
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.BugReport
import androidx.compose.material.icons.outlined.Info
import androidx.compose.material.icons.outlined.LayersClear
import androidx.compose.material.icons.outlined.Search
import androidx.compose.material.icons.outlined.Share
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.runtime.snapshots.SnapshotStateMap
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.documentfile.provider.DocumentFile
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.provider.AppDataDocumentProvider
import net.rpcs3.ui.components.GhostButton
import net.rpcs3.ui.components.PaneNavRow
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.ui.components.SettingInlineDropdown
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.SettingsStyle

@Composable
fun DebugScreen(
    modifier: Modifier = Modifier,
    navigateToDiagnostics: () -> Unit,
    onClose: (() -> Unit)? = null
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    var selected by remember { mutableIntStateOf(0) }

    var levels by remember { mutableStateOf(listOf(LogLevelOff)) }
    var channels by remember { mutableStateOf(emptyList<String>()) }
    var loading by remember { mutableStateOf(true) }
    var filter by remember { mutableStateOf("") }
    val values = remember { SnapshotStateMap<String, String>() }

    LaunchedEffect(Unit) {
        val state = withContext(Dispatchers.IO) {
            applyDefaultLogLevels()
            loadLogChannelState()
        }
        levels = state.levels
        channels = state.channels
        values.clear()
        state.channels.forEach { values[it] = state.values[it] ?: LogLevelOff }
        loading = false
    }

    val visible = remember(channels, filter) {
        val query = filter.trim()
        if (query.isEmpty()) channels else channels.filter { it.contains(query, true) }
    }
    val activeCount = values.count { it.value != LogLevelOff }

    val tabs = listOf(PaneTab("Troubleshooting", Icons.Outlined.BugReport))

    PaneScaffold(
        title = "Debug",
        tabs = tabs,
        selected = selected,
        onSelect = { selected = it },
        onBack = onClose,
        modifier = modifier,
        scrollableContent = false,
        contentMaxWidth = Dims.WideContentMaxWidth
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            PaneSectionTitle(
                if (loading) {
                    "Logging  ·  reading channels…"
                } else {
                    "Logging  ·  $activeCount of ${channels.size} channels enabled"
                }
            )

            Row(verticalAlignment = Alignment.CenterVertically) {
                LogFilterField(
                    value = filter,
                    onValueChange = { filter = it },
                    modifier = Modifier.weight(1f)
                )
                Spacer(Modifier.width(10.dp))
                GhostButton(
                    label = "All Off",
                    icon = Icons.Outlined.LayersClear,
                    enabled = !loading && activeCount > 0,
                    tint = Rpcs.Warning,
                    onClick = {
                        val reset = channels.associateWith { LogLevelOff }
                        channels.forEach { values[it] = LogLevelOff }
                        scope.launch(Dispatchers.IO) { writeLogLevels(reset) }
                    }
                )
            }

            Spacer(Modifier.height(Dims.RowSpacing))

            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxWidth()
                    .background(Rpcs.SurfaceRaised, RoundedCornerShape(Dims.CardCorner))
                    .border(
                        Dims.BorderWidth,
                        Rpcs.Outline,
                        RoundedCornerShape(Dims.CardCorner)
                    )
            ) {
                if (visible.isEmpty()) {
                    Text(
                        text = if (loading) "Loading…" else "No channel matches \"$filter\"",
                        color = Rpcs.TextDim,
                        fontSize = 12.sp,
                        modifier = Modifier
                            .align(Alignment.Center)
                            .padding(Dims.ScreenPadding)
                    )
                } else {
                    LazyVerticalGrid(
                        columns = GridCells.Adaptive(Dims.ChannelColumnMinWidth),
                        modifier = Modifier.fillMaxSize(),
                        contentPadding = PaddingValues(horizontal = 14.dp, vertical = 10.dp),
                        verticalArrangement = Arrangement.spacedBy(6.dp),
                        horizontalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        items(visible, key = { it }) { channel ->
                            val level = values[channel] ?: LogLevelOff
                            SettingInlineDropdown(
                                label = channel,
                                entries = levels,
                                selectedIndex = levels.indexOf(level).coerceAtLeast(0),
                                highlighted = level != LogLevelOff,
                                onSelected = { index ->
                                    val next = levels.getOrNull(index) ?: return@SettingInlineDropdown
                                    values[channel] = next
                                    scope.launch(Dispatchers.IO) {
                                        writeLogLevels(mapOf(channel to next))
                                    }
                                }
                            )
                        }
                    }
                }
            }

            Spacer(Modifier.height(Dims.RowSpacing))

            Row(verticalAlignment = Alignment.CenterVertically) {
                PaneNavRow(
                    title = "Share Log",
                    description = "Share the RPCS3 log file",
                    icon = Icons.Outlined.Share,
                    modifier = Modifier.weight(1f),
                    onClick = {
                        val file = DocumentFile.fromSingleUri(
                            context,
                            DocumentsContract.buildDocumentUri(
                                AppDataDocumentProvider.AUTHORITY,
                                "${AppDataDocumentProvider.ROOT_ID}/cache/RPCS3.log"
                            )
                        )

                        if (file != null && file.exists() && file.length() != 0L) {
                            val intent = Intent(Intent.ACTION_SEND).apply {
                                setDataAndType(file.uri, "text/plain")
                                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                                putExtra(Intent.EXTRA_STREAM, file.uri)
                            }
                            context.startActivity(Intent.createChooser(intent, "Share Log File"))
                        } else {
                            Toast.makeText(context, "Log file not found!", Toast.LENGTH_SHORT)
                                .show()
                        }
                    }
                )
                Spacer(Modifier.width(Dims.RowSpacing))
                PaneNavRow(
                    title = "Diagnostics",
                    description = "Setup, storage and graphics",
                    icon = Icons.Outlined.Info,
                    modifier = Modifier.weight(1f),
                    onClick = navigateToDiagnostics
                )
            }
        }
    }
}

@Composable
private fun LogFilterField(
    value: String,
    onValueChange: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Row(
        modifier = modifier
            .background(SettingsStyle.InputSurface, RoundedCornerShape(Dimens.FieldCorner))
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else SettingsStyle.InputBorder,
                RoundedCornerShape(Dimens.FieldCorner)
            )
            .padding(
                horizontal = Dimens.FieldHorizontalPadding,
                vertical = Dimens.FieldVerticalPadding
            ),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            imageVector = Icons.Outlined.Search,
            contentDescription = null,
            tint = SettingsStyle.TextDim,
            modifier = Modifier.size(Dimens.ControlIconSize)
        )
        Spacer(Modifier.width(8.dp))
        Box(modifier = Modifier.weight(1f)) {
            if (value.isEmpty()) {
                Text(
                    text = "Filter channels",
                    color = SettingsStyle.TextDim,
                    fontSize = Dimens.ValueSize,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
            }
            BasicTextField(
                value = value,
                onValueChange = onValueChange,
                singleLine = true,
                interactionSource = interaction,
                cursorBrush = SolidColor(SettingsStyle.AccentBlue),
                textStyle = MaterialTheme.typography.bodyMedium.copy(
                    color = SettingsStyle.TextPrimary,
                    fontSize = Dimens.ValueSize,
                    fontWeight = FontWeight.Normal
                ),
                modifier = Modifier.fillMaxWidth()
            )
        }
    }
}
