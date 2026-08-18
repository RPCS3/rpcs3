package net.rpcs3.ui.drivers

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
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Add
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.Download
import androidx.compose.material.icons.outlined.Edit
import androidx.compose.material.icons.outlined.ExpandLess
import androidx.compose.material.icons.outlined.ExpandMore
import androidx.compose.material.icons.outlined.Restore
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.LinearProgressIndicator
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
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.utils.DriverAsset
import net.rpcs3.utils.DriverRelease
import net.rpcs3.utils.DriverRepo

data class DriverDownloadState(
    val repos: List<DriverRepo> = emptyList(),
    val expandedRepo: String? = null,
    val expandedRelease: Long? = null,
    val loadingRepo: String? = null,
    val releases: Map<String, List<DriverRelease>> = emptyMap(),
    val errors: Map<String, String> = emptyMap(),
    val busyAsset: String? = null,
    val progress: Float = -1f,
    val progressLabel: String = ""
)

@Composable
fun DriverDownloadTab(
    state: DriverDownloadState,
    onRepoTapped: (DriverRepo) -> Unit,
    onReleaseTapped: (DriverRelease) -> Unit,
    onDownload: (DriverAsset) -> Unit,
    onAddRepo: (name: String, url: String) -> Unit,
    onEditRepo: (index: Int, name: String, url: String) -> Unit,
    onDeleteRepo: (index: Int) -> Unit,
    onRestoreDefaults: () -> Unit
) {
    var showAdd by remember { mutableStateOf(false) }
    var editing by remember { mutableStateOf<Pair<Int, DriverRepo>?>(null) }

    if (showAdd || editing != null) {
        RepoDialog(
            existing = editing?.second,
            onDismiss = {
                showAdd = false
                editing = null
            },
            onConfirm = { name, url ->
                val target = editing
                if (target != null) onEditRepo(target.first, name, url) else onAddRepo(name, url)
                showAdd = false
                editing = null
            }
        )
    }

    if (state.busyAsset != null) {
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
                fontWeight = FontWeight.Medium
            )
            Text(
                text = state.busyAsset,
                color = Rpcs.TextDim,
                fontSize = 10.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(8.dp))

            if (state.progress >= 0f) {
                LinearProgressIndicator(
                    progress = { state.progress },
                    modifier = Modifier.fillMaxWidth(),
                    color = Rpcs.Accent
                )
            } else {
                LinearProgressIndicator(
                    modifier = Modifier.fillMaxWidth(),
                    color = Rpcs.Accent
                )
            }
        }

        Spacer(Modifier.height(12.dp))
    }

    state.repos.forEachIndexed { index, repo ->
        RepoRow(
            repo = repo,
            expanded = state.expandedRepo == repo.apiUrl,
            loading = state.loadingRepo == repo.apiUrl,
            onClick = { onRepoTapped(repo) },
            onEdit = { editing = index to repo },
            onDelete = { onDeleteRepo(index) }
        )

        if (state.expandedRepo == repo.apiUrl) {
            val error = state.errors[repo.apiUrl]
            val releases = state.releases[repo.apiUrl].orEmpty()

            if (error != null) {
                Text(
                    text = error,
                    color = Rpcs.Danger,
                    fontSize = 11.sp,
                    modifier = Modifier.padding(start = 12.dp, top = 4.dp, bottom = 6.dp)
                )
            } else if (!state.loadingRepo.equals(repo.apiUrl) && releases.isEmpty()) {
                Text(
                    text = "No driver packages published in this repository.",
                    color = Rpcs.TextDim,
                    fontSize = 11.sp,
                    modifier = Modifier.padding(start = 12.dp, top = 4.dp, bottom = 6.dp)
                )
            }

            releases.forEach { release ->
                ReleaseRow(
                    release = release,
                    expanded = state.expandedRelease == release.id,
                    onClick = { onReleaseTapped(release) }
                )

                if (state.expandedRelease == release.id) {
                    release.assets.forEach { asset ->
                        AssetRow(
                            asset = asset,
                            busy = state.busyAsset == asset.name,
                            onDownload = { onDownload(asset) }
                        )
                    }
                }
            }
        }

        Spacer(Modifier.height(6.dp))
    }

    Spacer(Modifier.height(10.dp))

    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        SmallAction(
            label = "Add repository",
            icon = Icons.Outlined.Add,
            modifier = Modifier.weight(1f),
            onClick = { showAdd = true }
        )
        SmallAction(
            label = "Restore defaults",
            icon = Icons.Outlined.Restore,
            modifier = Modifier.weight(1f),
            onClick = onRestoreDefaults
        )
    }
}

@Composable
private fun RepoRow(
    repo: DriverRepo,
    expanded: Boolean,
    loading: Boolean,
    onClick: () -> Unit,
    onEdit: () -> Unit,
    onDelete: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(10.dp))
            .border(1.dp, Rpcs.OutlineSoft, RoundedCornerShape(10.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 12.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = repo.name,
                color = Rpcs.TextPrimary,
                fontSize = 12.sp,
                fontWeight = FontWeight.SemiBold,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Text(
                text = repo.repoUrl.removePrefix("https://github.com/"),
                color = Rpcs.TextDim,
                fontSize = 10.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }

        if (loading) {
            CircularProgressIndicator(
                modifier = Modifier.size(16.dp),
                color = Rpcs.Accent,
                strokeWidth = 2.dp
            )
            Spacer(Modifier.height(0.dp))
        }

        IconAction(Icons.Outlined.Edit, "Edit repository", onEdit)
        IconAction(Icons.Outlined.Delete, "Remove repository", onDelete)

        Icon(
            imageVector = if (expanded) Icons.Outlined.ExpandLess else Icons.Outlined.ExpandMore,
            contentDescription = null,
            tint = Rpcs.TextSecondary,
            modifier = Modifier.size(18.dp)
        )
    }
}

@Composable
private fun ReleaseRow(release: DriverRelease, expanded: Boolean, onClick: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = 12.dp, top = 4.dp)
            .background(Rpcs.SurfaceInset, RoundedCornerShape(8.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 10.dp, vertical = 8.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = release.title,
                    color = Rpcs.TextPrimary,
                    fontSize = 11.sp,
                    fontWeight = FontWeight.Medium,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
                Text(
                    text = release.subtitle,
                    color = Rpcs.TextDim,
                    fontSize = 9.sp
                )
            }

            Icon(
                imageVector = if (expanded) Icons.Outlined.ExpandLess else Icons.Outlined.ExpandMore,
                contentDescription = null,
                tint = Rpcs.TextSecondary,
                modifier = Modifier.size(16.dp)
            )
        }

        if (expanded && release.notes.isNotBlank()) {
            Spacer(Modifier.height(6.dp))
            Text(
                text = release.notes,
                color = Rpcs.TextSecondary,
                fontSize = 10.sp,
                lineHeight = 13.sp
            )
        }
    }
}

@Composable
private fun AssetRow(asset: DriverAsset, busy: Boolean, onDownload: () -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = 24.dp, top = 4.dp)
            .background(Rpcs.Surface, RoundedCornerShape(8.dp))
            .clickable(enabled = !busy, onClick = onDownload)
            .padding(horizontal = 10.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = asset.name,
                color = Rpcs.TextPrimary,
                fontSize = 11.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Text(
                text = asset.sizeLabel,
                color = Rpcs.TextDim,
                fontSize = 9.sp
            )
        }

        if (busy) {
            CircularProgressIndicator(
                modifier = Modifier.size(15.dp),
                color = Rpcs.Accent,
                strokeWidth = 2.dp
            )
        } else {
            Icon(
                imageVector = Icons.Outlined.Download,
                contentDescription = "Download and install",
                tint = Rpcs.Accent,
                modifier = Modifier.size(17.dp)
            )
        }
    }
}

@Composable
private fun IconAction(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    description: String,
    onClick: () -> Unit
) {
    Box(
        modifier = Modifier
            .size(30.dp)
            .clip(RoundedCornerShape(8.dp))
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Icon(
            imageVector = icon,
            contentDescription = description,
            tint = Rpcs.TextSecondary,
            modifier = Modifier.size(15.dp)
        )
    }
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
            .border(1.dp, Rpcs.OutlineSoft, RoundedCornerShape(10.dp))
            .clickable(onClick = onClick)
            .padding(vertical = 9.dp),
        horizontalArrangement = Arrangement.Center,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = Rpcs.Accent,
            modifier = Modifier.size(15.dp)
        )
        Spacer(Modifier.height(0.dp))
        Text(
            text = label,
            color = Rpcs.TextPrimary,
            fontSize = 11.sp,
            fontWeight = FontWeight.Medium,
            modifier = Modifier.padding(start = 6.dp)
        )
    }
}

@Composable
private fun RepoDialog(
    existing: DriverRepo?,
    onDismiss: () -> Unit,
    onConfirm: (String, String) -> Unit
) {
    var name by remember { mutableStateOf(existing?.name.orEmpty()) }
    var url by remember { mutableStateOf(existing?.repoUrl.orEmpty()) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(if (existing == null) "Add repository" else "Edit repository") },
        text = {
            Column {
                OutlinedTextField(
                    value = name,
                    onValueChange = { name = it },
                    singleLine = true,
                    label = { Text("Name") },
                    modifier = Modifier.fillMaxWidth()
                )
                Spacer(Modifier.height(8.dp))
                OutlinedTextField(
                    value = url,
                    onValueChange = { url = it },
                    singleLine = true,
                    label = { Text("GitHub releases URL") },
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Uri),
                    modifier = Modifier.fillMaxWidth()
                )
            }
        },
        confirmButton = {
            TextButton(
                enabled = url.isNotBlank(),
                onClick = { onConfirm(name, url) }
            ) {
                Text(stringResourceOk())
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(stringResourceCancel()) }
        }
    )
}

@Composable
private fun stringResourceOk() = androidx.compose.ui.res.stringResource(android.R.string.ok)

@Composable
private fun stringResourceCancel() = androidx.compose.ui.res.stringResource(android.R.string.cancel)
