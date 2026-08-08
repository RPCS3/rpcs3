package net.rpcs3.ui.patches

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.outlined.ArrowBack
import androidx.compose.material.icons.outlined.Healing
import androidx.compose.material.icons.outlined.Tune
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
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
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.RPCS3
import net.rpcs3.ui.components.PaneCard
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.ui.components.SectionLabel
import net.rpcs3.ui.components.SettingGroup
import net.rpcs3.ui.components.SettingSwitch
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.SettingsStyle
import org.json.JSONArray
import org.json.JSONObject

private data class Patch(
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

private fun parsePatches(raw: String): List<Patch> = runCatching {
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
        patches = withContext(Dispatchers.IO) {
            parsePatches(runCatching { RPCS3.instance.patchesGet(titleId) }.getOrDefault("[]"))
        }
        loading = false
    }

    val grouped = remember(patches) {
        patches.groupBy { it.group.ifEmpty { "Ungrouped" } }.toList().sortedBy { it.first }
    }
    var selected by remember(grouped) { mutableIntStateOf(0) }

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

    if (grouped.isEmpty()) {
        Column(
            modifier = modifier
                .fillMaxSize()
                .background(Rpcs.Background)
                .windowInsetsPadding(WindowInsets.systemBars)
        ) {
            EmptyPatches()
        }
        return
    }

    PaneScaffold(
        title = "Patches · $titleId",
        tabs = grouped.map { PaneTab(it.first, Icons.Outlined.Tune) },
        selected = selected.coerceIn(0, grouped.lastIndex),
        onSelect = { selected = it },
        onBack = onClose,
        modifier = modifier
    ) {
        val entries = grouped[selected.coerceIn(0, grouped.lastIndex)].second

        PaneSectionTitle("${entries.size} patch${if (entries.size == 1) "" else "es"}")
        PaneCard {
            entries.forEach { patch ->
                PatchRow(titleId = titleId, patch = patch)
            }
        }

        Spacer(Modifier.height(16.dp))
    }
}

@Composable
private fun PatchRow(titleId: String, patch: Patch) {
    var enabled by remember(patch.hash + patch.description) { mutableStateOf(patch.enabled) }
    val scope = rememberCoroutineScope()

    val detail = buildList {
        if (patch.author.isNotEmpty()) add("by ${patch.author}")
        if (patch.patchVersion.isNotEmpty()) add("v${patch.patchVersion}")
        if (patch.appVersion.isNotEmpty()) add("app ${patch.appVersion}")
    }.joinToString(" · ")

    SettingSwitch(
        label = patch.description,
        subtitle = detail.ifEmpty { null },
        checked = enabled,
        onCheckedChange = { next ->
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
    )
}

@Composable
private fun EmptyPatches() {
    Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier.padding(32.dp)
        ) {
            Box(
                modifier = Modifier
                    .size(56.dp)
                    .clip(RoundedCornerShape(16.dp))
                    .background(SettingsStyle.CardSurface),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    imageVector = Icons.Outlined.Healing,
                    contentDescription = null,
                    tint = SettingsStyle.TextSecondary,
                    modifier = Modifier.size(26.dp)
                )
            }
            Spacer(Modifier.height(14.dp))
            Text(
                text = "No patches for this title",
                color = SettingsStyle.TextPrimary,
                fontSize = 14.sp,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(Modifier.height(6.dp))
            Text(
                text = "Place patch .yml files in the patches folder and they will appear here.",
                color = SettingsStyle.TextSecondary,
                fontSize = 12.sp
            )
        }
    }
}
