package net.rpcs3.ui.games

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.Extension
import androidx.compose.material.icons.outlined.SystemUpdateAlt
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.ProgressRepository
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.gameTitleId
import net.rpcs3.ui.components.PaneActionButton
import net.rpcs3.ui.components.PaneCard
import net.rpcs3.ui.components.PaneEntryRow
import net.rpcs3.ui.components.PaneProgressOverlay
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.utils.GameDetails
import net.rpcs3.utils.GameDetailsReader
import net.rpcs3.utils.PackageInspector
import org.json.JSONArray

data class InstalledUpdate(
    val path: String,
    val name: String,
    val title: String,
    val category: String,
    val version: String,
    val size: Long
) {
    val isUpdate: Boolean get() = category == "GD"
}

internal fun parseUpdates(raw: String): List<InstalledUpdate> = runCatching {
    val array = JSONArray(raw)
    (0 until array.length()).mapNotNull { index ->
        val item = array.optJSONObject(index) ?: return@mapNotNull null
        InstalledUpdate(
            path = item.optString("path"),
            name = item.optString("name"),
            title = item.optString("title"),
            category = item.optString("category"),
            version = item.optString("version"),
            size = item.optLong("size", 0L)
        )
    }
}.getOrDefault(emptyList())

@Composable
fun GameUpdatesScreen(
    gamePath: String,
    modifier: Modifier = Modifier,
    onClose: (() -> Unit)? = null
) {
    var entries by remember(gamePath) { mutableStateOf<List<InstalledUpdate>>(emptyList()) }
    var details by remember(gamePath) { mutableStateOf<GameDetails?>(null) }
    var reloadToken by remember(gamePath) { mutableIntStateOf(0) }
    var pendingPackages by remember { mutableStateOf<List<Uri>>(emptyList()) }
    val scope = rememberCoroutineScope()
    val context = LocalContext.current

    LaunchedEffect(gamePath, reloadToken) {
        withContext(Dispatchers.IO) {
            val resolved = GameDetailsReader.read(gamePath)
            val loaded = parseUpdates(
                runCatching { RPCS3.instance.installedUpdates(resolved.titleId) }.getOrDefault("[]")
            )
            withContext(Dispatchers.Main) {
                details = resolved
                entries = loaded
            }
        }
    }

    val picker = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetMultipleContents()
    ) { uris: List<Uri> ->
        if (uris.isNotEmpty()) {
            pendingPackages = uris
        }
    }

    var installProgress by remember { mutableStateOf<Long?>(null) }
    var overlayHidden by remember { mutableStateOf(false) }

    if (pendingPackages.isNotEmpty()) {
        PackageInstallFlow(
            uris = pendingPackages,
            onFinished = { pendingPackages = emptyList() },
            onInstallStarted = { id ->
                installProgress = id
                overlayHidden = false
            }
        )
    }

    val progressId = installProgress
    val progressState = remember(progressId) { ProgressRepository.getItem(progressId) }
    val progressEntry = progressState?.value
    val installDone = progressId != null && (progressEntry == null || progressEntry.isFinished())

    LaunchedEffect(progressId, installDone) {
        if (installDone) {
            installProgress = null
            overlayHidden = false
            reloadToken++
        }
    }

    var selected by remember { mutableIntStateOf(0) }
    val shown = if (selected == 0) entries.filter { it.isUpdate } else entries.filter { !it.isUpdate }

    Box(modifier.fillMaxSize()) {
        PaneScaffold(
            title = stringResource(
                R.string.updates_title,
                details?.titleId?.ifEmpty { null } ?: gameTitleId(gamePath)
            ),
            tabs = listOf(
                PaneTab(
                    stringResource(R.string.updates_tab_updates),
                    Icons.Outlined.SystemUpdateAlt
                ),
                PaneTab(
                    stringResource(R.string.updates_tab_other_data),
                    Icons.Outlined.Extension
                )
            ),
            selected = selected,
            onSelect = { selected = it },
            onBack = onClose
        ) {
            val updateTypeLabel = stringResource(R.string.updates_type_update)
            val dataTypeLabel = stringResource(R.string.updates_type_data)

            details?.versionLabel(context)?.takeIf { it.isNotEmpty() }?.let { label ->
                PaneSectionTitle(stringResource(R.string.updates_installed_version, label))
                Spacer(Modifier.height(4.dp))
            }

            PaneSectionTitle(
                if (shown.isEmpty()) {
                    stringResource(R.string.updates_none_installed)
                } else {
                    pluralStringResource(
                        R.plurals.updates_item_count,
                        shown.size,
                        shown.size
                    )
                }
            )

            if (shown.isNotEmpty()) {
                PaneCard {
                    shown.forEach { entry ->
                        PaneEntryRow(
                            title = entry.title.ifEmpty { entry.name },
                            subtitle = buildString {
                                append(entry.name)
                                if (entry.version.isNotEmpty()) {
                                    append("  ·  ")
                                        .append(
                                            context.getString(
                                                R.string.version_prefix,
                                                entry.version
                                            )
                                        )
                                }
                                append("  ·  ").append(
                                    if (entry.isUpdate) {
                                        updateTypeLabel
                                    } else {
                                        entry.category.ifEmpty { dataTypeLabel }
                                    }
                                )
                                append("  ·  ")
                                    .append(PackageInspector.formatSize(context, entry.size))
                            },
                            actionIcon = Icons.Outlined.Delete,
                            actionDescription = stringResource(R.string.action_uninstall),
                            onAction = {
                                AlertDialogQueue.showDialog(
                                    title = context.getString(
                                        R.string.updates_uninstall_title,
                                        entry.name
                                    ),
                                    message = context.getString(
                                        R.string.updates_uninstall_message,
                                        entry.path
                                    ),
                                    confirmText = context.getString(R.string.action_uninstall),
                                    dismissText = context.getString(R.string.action_cancel),
                                    onConfirm = {
                                        scope.launch(Dispatchers.IO) {
                                            val ok = runCatching {
                                                RPCS3.instance.uninstallUpdate(entry.path)
                                            }.getOrDefault(false)

                                            withContext(Dispatchers.Main) {
                                                if (ok) {
                                                    reloadToken++
                                                } else {
                                                    AlertDialogQueue.showDialog(
                                                        context.getString(
                                                            R.string.updates_uninstall_failed_title
                                                        ),
                                                        context.getString(
                                                            R.string.updates_uninstall_failed_message,
                                                            entry.path
                                                        )
                                                    )
                                                }
                                            }
                                        }
                                    }
                                )
                            }
                        )
                    }
                }
            }

            Spacer(Modifier.height(14.dp))

            PaneActionButton(
                label = stringResource(R.string.updates_install),
                icon = Icons.Default.Add,
                enabled = true,
                onClick = { picker.launch("*/*") }
            )

            Spacer(Modifier.height(16.dp))
        }

        if (progressId != null && !installDone && !overlayHidden) {
            PaneProgressOverlay(
                title = stringResource(R.string.updates_installing),
                message = progressEntry?.message?.value,
                value = progressEntry?.value?.longValue ?: 0L,
                max = progressEntry?.max?.longValue ?: 0L,
                onHide = { overlayHidden = true }
            )
        }
    }
}
