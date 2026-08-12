package net.rpcs3.ui.patches

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.Description
import androidx.compose.material.icons.outlined.Tune
import androidx.compose.material3.CircularProgressIndicator
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
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.ui.components.PaneActionButton
import net.rpcs3.ui.components.PaneCard
import net.rpcs3.ui.components.PaneEntryRow
import net.rpcs3.ui.components.PaneProgressOverlay
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.utils.PackageInspector
import org.json.JSONArray

@Composable
fun GamePatchesScreen(
    titleId: String,
    modifier: Modifier = Modifier,
    onClose: (() -> Unit)? = null
) {
    var loading by remember(titleId) { mutableStateOf(true) }
    var patches by remember(titleId) { mutableStateOf<List<Patch>>(emptyList()) }

    LaunchedEffect(titleId) {
        loading = true
        patches = withContext(Dispatchers.IO) { loadPatchesFor(titleId) }
        loading = false
    }

    var files by remember(titleId) { mutableStateOf<List<PatchFile>>(emptyList()) }
    var reloadToken by remember(titleId) { mutableIntStateOf(0) }
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    LaunchedEffect(reloadToken) {
        files = withContext(Dispatchers.IO) {
            parsePatchFiles(runCatching { RPCS3.instance.patchFiles() }.getOrDefault("[]"))
        }
        if (reloadToken > 0) {
            patches = withContext(Dispatchers.IO) { loadPatchesFor(titleId) }
        }
    }

    var importTotal by remember { mutableIntStateOf(0) }
    var importDone by remember { mutableIntStateOf(0) }
    var importLabel by remember { mutableStateOf<String?>(null) }
    var importHidden by remember { mutableStateOf(false) }

    val importer = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetMultipleContents()
    ) { uris: List<Uri> ->
        if (uris.isEmpty()) return@rememberLauncherForActivityResult

        importTotal = uris.size
        importDone = 0
        importLabel = null
        importHidden = false

        scope.launch(Dispatchers.IO) {
            var added = 0
            val failed = ArrayList<String>()

            uris.forEach { uri ->
                val name = PackageInspector.displayNameOf(context, uri)
                withContext(Dispatchers.Main) { importLabel = name }

                val descriptor = runCatching {
                    context.contentResolver.openAssetFileDescriptor(uri, "r")
                }.getOrNull()

                val fd = descriptor?.parcelFileDescriptor?.fd
                val ok = if (fd != null) {
                    runCatching { RPCS3.instance.patchImport(fd, name) }.getOrDefault(false)
                } else {
                    false
                }

                runCatching { descriptor?.close() }

                if (ok) added++ else failed += name

                withContext(Dispatchers.Main) { importDone++ }
            }

            withContext(Dispatchers.Main) {
                importTotal = 0
                importLabel = null
                reloadToken++
                if (failed.isNotEmpty()) {
                    AlertDialogQueue.showDialog(
                        title = context.getString(R.string.patches_import_failed_title),
                        message = context.getString(
                            R.string.patches_import_failed_message,
                            added,
                            failed.joinToString("\n")
                        )
                    )
                }
            }
        }
    }

    val thisGameLabel = stringResource(R.string.patches_section_this_game)
    val allTitlesLabel = stringResource(R.string.patches_section_all_titles)
    val rows = remember(patches, titleId, thisGameLabel, allTitlesLabel) {
        buildPatchRows(titleId, patches, thisGameLabel, allTitlesLabel)
    }
    var selected by remember(titleId) { mutableIntStateOf(0) }

    if (loading) {
        Box(
            modifier
                .fillMaxSize()
                .background(Rpcs.Background),
            contentAlignment = Alignment.Center
        ) {
            CircularProgressIndicator(color = Rpcs.Accent)
        }
        return
    }

    val tabs = listOf(
        PaneTab(stringResource(R.string.patches_tab_available), Icons.Outlined.Tune),
        PaneTab(stringResource(R.string.patches_tab_files), Icons.Outlined.Description)
    )
    val filesTab = tabs.lastIndex
    val current = selected.coerceIn(0, filesTab)

    Box(modifier.fillMaxSize()) {
        PaneScaffold(
            title = stringResource(R.string.patches_title, titleId),
            tabs = tabs,
            selected = current,
            onSelect = { selected = it },
            onBack = onClose
        ) {
            PatchUpdateCard(onUpdated = { reloadToken++ })

            Spacer(Modifier.height(14.dp))

            if (current == filesTab) {
                PaneSectionTitle(
                    if (files.isEmpty()) {
                        stringResource(R.string.patches_files_none)
                    } else {
                        pluralStringResource(
                            R.plurals.patches_file_count,
                            files.size,
                            files.size
                        )
                    }
                )

                if (files.isNotEmpty()) {
                    PaneCard {
                        files.forEach { file ->
                            PaneEntryRow(
                                title = file.name,
                                subtitle = pluralStringResource(
                                    R.plurals.patches_count,
                                    file.patchCount,
                                    file.patchCount
                                ) + "  ·  " + PackageInspector.formatSize(context, file.size),
                                actionIcon = Icons.Outlined.Delete,
                                actionDescription = stringResource(R.string.action_delete),
                                onAction = {
                                    AlertDialogQueue.showDialog(
                                        title = context.getString(
                                            R.string.patches_delete_title,
                                            file.name
                                        ),
                                        message = context.getString(
                                            R.string.patches_delete_message
                                        ),
                                        confirmText = context.getString(R.string.action_delete),
                                        dismissText = context.getString(R.string.action_cancel),
                                        onConfirm = {
                                            scope.launch(Dispatchers.IO) {
                                                runCatching { RPCS3.instance.patchFileDelete(file.name) }
                                                withContext(Dispatchers.Main) { reloadToken++ }
                                            }
                                        }
                                    )
                                }
                            )
                        }
                    }
                }
            } else if (patches.isNotEmpty()) {
                PaneSectionTitle(
                    pluralStringResource(R.plurals.patches_count, patches.size, patches.size)
                )
                PaneCard {
                    rows.forEach { row ->
                        PatchListRowContent(titleId = titleId, row = row)
                    }
                }
            } else {
                PaneSectionTitle(stringResource(R.string.patches_none_available))
            }

            Spacer(Modifier.height(14.dp))

            PaneActionButton(
                label = stringResource(R.string.patches_add),
                icon = Icons.Default.Add,
                enabled = true,
                onClick = { importer.launch("*/*") }
            )

            Spacer(Modifier.height(16.dp))
        }

        if (importTotal > 0 && !importHidden) {
            PaneProgressOverlay(
                title = stringResource(R.string.patches_importing),
                message = importLabel,
                value = importDone.toLong(),
                max = importTotal.toLong(),
                onHide = { importHidden = true }
            )
        }
    }
}

data class PatchFile(val name: String, val size: Long, val patchCount: Int)

private fun parsePatchFiles(raw: String): List<PatchFile> = runCatching {
    val array = JSONArray(raw)
    (0 until array.length()).mapNotNull { index ->
        val item = array.optJSONObject(index) ?: return@mapNotNull null
        PatchFile(
            name = item.optString("name"),
            size = item.optLong("size", 0L),
            patchCount = item.optInt("patchCount", 0)
        )
    }.sortedBy { it.name }
}.getOrDefault(emptyList())
