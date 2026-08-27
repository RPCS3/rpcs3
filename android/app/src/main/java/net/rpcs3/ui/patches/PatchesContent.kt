package net.rpcs3.ui.patches

import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
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
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.ui.components.SettingGroup
import net.rpcs3.ui.components.SettingsHint
import net.rpcs3.ui.components.SettingsSection
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.utils.PatchUpdater
import org.json.JSONArray

const val PatchesCategory = "Patches"

internal data class Patch(
    val hash: String,
    val description: String,
    val title: String,
    val serial: String,
    val appVersion: String,
    val author: String,
    val notes: String,
    val group: String,
    val patchVersion: String,
    val enabled: Boolean
)

internal fun parsePatches(raw: String): List<Patch> = runCatching {
    val array = JSONArray(raw)
    (0 until array.length()).mapNotNull { index ->
        val item = array.optJSONObject(index) ?: return@mapNotNull null
        Patch(
            hash = item.optString("hash"),
            description = item.optString("description"),
            title = item.optString("title"),
            serial = item.optString("serial"),
            appVersion = item.optString("appVersion"),
            author = item.optString("author"),
            notes = item.optString("notes"),
            group = item.optString("group"),
            patchVersion = item.optString("patchVersion"),
            enabled = item.optBoolean("enabled")
        )
    }
}.getOrDefault(emptyList())

internal fun loadPatchesFor(titleId: String): List<Patch> =
    parsePatches(runCatching { RPCS3.instance.patchesGet(titleId) }.getOrDefault("[]"))

@Composable
internal fun PatchToggleRow(titleId: String, patch: Patch, disambiguator: String? = null) {
    var enabled by remember(patch.hash + patch.description) { mutableStateOf(patch.enabled) }
    val scope = rememberCoroutineScope()
    val context = LocalContext.current
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    val fill by animateColorAsState(
        targetValue = if (enabled) Rpcs.SelectionFill else Color.Transparent,
        animationSpec = tween(160),
        label = "patchFill"
    )
    val stroke by animateColorAsState(
        targetValue = when {
            focused -> Rpcs.FocusBorder
            enabled -> Rpcs.SelectionBorder
            else -> Rpcs.OutlineSoft
        },
        animationSpec = tween(160),
        label = "patchStroke"
    )

    val detail = buildList {
        if (disambiguator != null) {
            add(disambiguator)
        }
        if (patch.author.isNotEmpty()) {
            add(context.getString(R.string.patches_detail_author, patch.author))
        }
        if (patch.patchVersion.isNotEmpty()) {
            add(context.getString(R.string.version_prefix, patch.patchVersion))
        }
        if (patch.appVersion.isNotEmpty()) {
            add(context.getString(R.string.patches_detail_app_version, patch.appVersion))
        }
    }.joinToString(" · ")

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 3.dp)
            .clip(RoundedCornerShape(Dims.RowCorner))
            .background(fill, RoundedCornerShape(Dims.RowCorner))
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                stroke,
                RoundedCornerShape(Dims.RowCorner)
            )
            .clickable(interactionSource = interaction, indication = null) {
                val next = !enabled
                val previous = enabled
                enabled = next
                scope.launch(Dispatchers.IO) {
                    val ok = runCatching {
                        RPCS3.instance.patchSet(
                            titleId,
                            patch.hash,
                            patch.description,
                            patch.title,
                            patch.serial,
                            patch.appVersion,
                            next
                        )
                    }.getOrDefault(false)

                    if (!ok) {
                        withContext(Dispatchers.Main) { enabled = previous }
                    }
                }
            }
            .padding(horizontal = 10.dp, vertical = 9.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = patch.description,
                color = if (enabled) Rpcs.TextPrimary else Rpcs.TextSecondary,
                fontSize = 12.sp,
                fontWeight = if (enabled) FontWeight.SemiBold else FontWeight.Normal,
                maxLines = 3,
                overflow = TextOverflow.Ellipsis
            )
            if (detail.isNotEmpty()) {
                Text(
                    text = detail,
                    color = Rpcs.TextDim,
                    fontSize = 11.sp,
                    lineHeight = 14.sp,
                    maxLines = 3,
                    overflow = TextOverflow.Ellipsis
                )
            }
        }

        Spacer(Modifier.width(10.dp))

        Box(
            modifier = Modifier.size(20.dp),
            contentAlignment = Alignment.Center
        ) {
            if (enabled) {
                Icon(
                    imageVector = Icons.Filled.CheckCircle,
                    contentDescription = stringResource(R.string.patches_state_on),
                    tint = Rpcs.Accent,
                    modifier = Modifier.size(18.dp)
                )
            } else {
                Box(
                    modifier = Modifier
                        .size(15.dp)
                        .clip(RoundedCornerShape(7.dp))
                        .border(1.dp, Rpcs.Outline, RoundedCornerShape(7.dp))
                )
            }
        }
    }
}

@Composable
internal fun PatchUpdateCard(onUpdated: () -> Unit) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val state by PatchUpdater.state.collectAsState()

    var busy by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        if (state is PatchUpdater.State.Unknown) {
            val staged = withContext(Dispatchers.IO) { PatchUpdater.pendingSha256() }
            if (staged != null) {
                PatchUpdater.checkIfDue(context)
            }
        }
    }

    val available = state is PatchUpdater.State.Available
    val working = busy || state is PatchUpdater.State.Checking ||
        state is PatchUpdater.State.Downloading

    val status = when (val current = state) {
        is PatchUpdater.State.Available -> stringResource(
            R.string.patches_update_available_detail,
            current.bytes / 1024L
        )

        is PatchUpdater.State.Checking -> stringResource(R.string.patches_update_checking)
        is PatchUpdater.State.Downloading -> stringResource(R.string.patches_update_downloading)
        is PatchUpdater.State.UpToDate -> stringResource(R.string.patches_update_current)
        is PatchUpdater.State.Failed -> current.message
        else -> stringResource(R.string.patches_update_idle)
    }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(
                if (available) Rpcs.SelectionFill else Rpcs.SurfaceRaised,
                RoundedCornerShape(Dims.CardCorner)
            )
            .border(
                Dims.BorderWidth,
                if (available) Rpcs.SelectionBorder else Rpcs.Outline,
                RoundedCornerShape(Dims.CardCorner)
            )
            .padding(horizontal = 14.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = stringResource(
                    if (available) {
                        R.string.patches_update_available
                    } else {
                        R.string.patches_update_title
                    }
                ),
                color = if (available) Rpcs.Accent else Rpcs.TextPrimary,
                fontSize = 13.sp,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = status,
                color = Rpcs.TextSecondary,
                fontSize = 11.sp,
                lineHeight = 14.sp,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis
            )
        }

        Spacer(Modifier.width(12.dp))

        if (working) {
            CircularProgressIndicator(
                color = Rpcs.Accent,
                strokeWidth = 2.dp,
                modifier = Modifier.size(20.dp)
            )
        } else {
            PatchUpdateButton(
                label = stringResource(
                    if (available) R.string.patches_update_download else R.string.patches_update_check
                ),
                onClick = {
                    busy = true
                    scope.launch {
                        if (available) {
                            val ok = PatchUpdater.download()
                            busy = false
                            if (ok) {
                                onUpdated()
                                AlertDialogQueue.showDialog(
                                    title = context.getString(R.string.patches_update_done_title),
                                    message = context.getString(R.string.patches_update_done_message)
                                )
                            } else {
                                AlertDialogQueue.showDialog(
                                    title = context.getString(R.string.patches_update_failed_title),
                                    message = context.getString(R.string.patches_update_failed_message)
                                )
                            }
                        } else {
                            val result = PatchUpdater.check(context)
                            busy = false
                            when (result) {
                                is PatchUpdater.State.UpToDate -> AlertDialogQueue.showDialog(
                                    title = context.getString(R.string.patches_update_none_title),
                                    message = context.getString(R.string.patches_update_none_message)
                                )

                                is PatchUpdater.State.Failed -> AlertDialogQueue.showDialog(
                                    title = context.getString(R.string.patches_update_failed_title),
                                    message = result.message
                                )

                                else -> Unit
                            }
                        }
                    }
                }
            )
        }
    }
}

@Composable
private fun PatchUpdateButton(label: String, onClick: () -> Unit) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Box(
        modifier = Modifier
            .background(Rpcs.SelectionFill, RoundedCornerShape(Dims.RowCorner))
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else Rpcs.SelectionBorder,
                RoundedCornerShape(Dims.RowCorner)
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick)
            .padding(horizontal = 14.dp, vertical = 8.dp)
    ) {
        Text(
            text = label,
            color = if (focused) Rpcs.AccentBright else Rpcs.Accent,
            fontSize = 12.sp,
            fontWeight = FontWeight.SemiBold
        )
    }
}

@Composable
fun InGamePatchesPanel(titleId: String, modifier: Modifier = Modifier) {
    var loading by remember(titleId) { mutableStateOf(true) }
    var patches by remember(titleId) { mutableStateOf<List<Patch>>(emptyList()) }
    var reloadToken by remember(titleId) { mutableStateOf(0) }

    LaunchedEffect(titleId, reloadToken) {
        loading = true
        patches = withContext(Dispatchers.IO) { loadPatchesFor(titleId) }
        loading = false
    }

    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(Dimens.SectionGap)
    ) {
        SettingsSection(title = stringResource(R.string.patches_section_source)) {
            PatchUpdateCard(onUpdated = { reloadToken++ })
        }

    SettingsSection(title = stringResource(R.string.patches_section_available)) {
        when {
            loading -> Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(80.dp),
                contentAlignment = Alignment.Center
            ) {
                CircularProgressIndicator(color = Rpcs.Accent, strokeWidth = 2.dp)
            }

            patches.isEmpty() -> SettingGroup {
                Text(
                    text = stringResource(R.string.patches_none_available),
                    color = Rpcs.TextSecondary,
                    fontSize = Dimens.ValueSize
                )
            }

            else -> {
                SettingsHint(text = stringResource(R.string.patches_ingame_hint))
                PatchGroupList(titleId = titleId, patches = patches)
            }
        }
    }
    }
}

internal sealed interface PatchListRow {
    data class Section(val label: String, val count: Int) : PatchListRow
    data class GroupLabel(val label: String) : PatchListRow
    data class Entry(val patch: Patch, val disambiguator: String?) : PatchListRow
}

internal fun shortHash(hash: String): String =
    hash.substringAfter('-').take(8).ifEmpty { hash.take(8) }

internal fun buildPatchRows(
    titleId: String,
    patches: List<Patch>,
    thisGameLabel: String,
    allTitlesLabel: String
): List<PatchListRow> {
    val repeated = patches
        .groupBy { it.description }
        .filterValues { it.size > 1 }
        .keys

    val own = patches.filter { it.serial.equals(titleId, ignoreCase = true) }
    val generic = patches.filter { !it.serial.equals(titleId, ignoreCase = true) }

    val rows = ArrayList<PatchListRow>()

    fun appendSection(label: String, entries: List<Patch>) {
        if (entries.isEmpty()) return

        rows.add(PatchListRow.Section(label, entries.size))

        entries
            .groupBy { it.group }
            .toList()
            .sortedWith(compareBy({ it.first.isEmpty() }, { it.first.lowercase() }))
            .forEach { (group, groupEntries) ->
                if (group.isNotEmpty()) {
                    rows.add(PatchListRow.GroupLabel(group))
                }
                groupEntries
                    .sortedBy { it.description.lowercase() }
                    .forEach { patch ->
                        rows.add(
                            PatchListRow.Entry(
                                patch = patch,
                                disambiguator = if (patch.description in repeated) {
                                    shortHash(patch.hash)
                                } else {
                                    null
                                }
                            )
                        )
                    }
            }
    }

    appendSection(thisGameLabel, own)
    appendSection(allTitlesLabel, generic)

    return rows
}

@Composable
internal fun PatchSectionHeader(row: PatchListRow.Section) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = 12.dp, bottom = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            modifier = Modifier.weight(1f),
            text = row.label,
            color = Rpcs.Accent,
            fontSize = 12.sp,
            fontWeight = FontWeight.SemiBold
        )
        Text(
            text = row.count.toString(),
            color = Rpcs.TextDim,
            fontSize = 11.sp
        )
    }
}

@Composable
internal fun PatchGroupLabel(row: PatchListRow.GroupLabel) {
    Text(
        text = row.label,
        color = Rpcs.TextSecondary,
        fontSize = 11.sp,
        fontWeight = FontWeight.SemiBold,
        modifier = Modifier.padding(top = 6.dp, bottom = 2.dp)
    )
}

@Composable
internal fun PatchListRowContent(titleId: String, row: PatchListRow) {
    when (row) {
        is PatchListRow.Section -> PatchSectionHeader(row)
        is PatchListRow.GroupLabel -> PatchGroupLabel(row)
        is PatchListRow.Entry -> PatchToggleRow(
            titleId = titleId,
            patch = row.patch,
            disambiguator = row.disambiguator
        )
    }
}

@Composable
internal fun PatchGroupList(
    titleId: String,
    patches: List<Patch>,
    modifier: Modifier = Modifier
) {
    val thisGame = stringResource(R.string.patches_section_this_game)
    val allTitles = stringResource(R.string.patches_section_all_titles)
    val rows = remember(patches, titleId, thisGame, allTitles) {
        buildPatchRows(titleId, patches, thisGame, allTitles)
    }

    Column(modifier = modifier.fillMaxWidth()) {
        rows.forEach { row ->
            PatchListRowContent(titleId = titleId, row = row)
        }
    }
}
