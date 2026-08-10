package net.rpcs3.ui.patches

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Healing
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs
import org.json.JSONArray

private data class CatalogPatch(
    val description: String,
    val title: String,
    val serials: String,
    val appVersions: String,
    val author: String,
    val group: String,
    val patchVersion: String
)

private fun parseCatalog(raw: String): List<CatalogPatch> = runCatching {
    val array = JSONArray(raw)
    (0 until array.length()).mapNotNull { index ->
        val item = array.optJSONObject(index) ?: return@mapNotNull null
        CatalogPatch(
            description = item.optString("description"),
            title = item.optString("title"),
            serials = item.optString("serials"),
            appVersions = item.optString("appVersions"),
            author = item.optString("author"),
            group = item.optString("group"),
            patchVersion = item.optString("patchVersion")
        )
    }
}.getOrDefault(emptyList())

private sealed interface CatalogRow {
    data class Header(val title: String, val count: Int) : CatalogRow
    data class Entry(val patch: CatalogPatch) : CatalogRow
}

@Composable
fun AllPatchesScreen(
    modifier: Modifier = Modifier,
    onClose: (() -> Unit)? = null
) {
    var loading by remember { mutableStateOf(true) }
    var patches by remember { mutableStateOf<List<CatalogPatch>>(emptyList()) }
    var reloadToken by remember { mutableIntStateOf(0) }

    LaunchedEffect(reloadToken) {
        loading = true
        patches = withContext(Dispatchers.IO) {
            parseCatalog(runCatching { RPCS3.instance.patchesAll() }.getOrDefault("[]"))
        }
        loading = false
    }

    val rows = remember(patches) {
        patches
            .groupBy { it.title.ifEmpty { "?" } }
            .toList()
            .sortedBy { it.first.lowercase() }
            .flatMap { (title, entries) ->
                listOf(CatalogRow.Header(title, entries.size)) +
                    entries.sortedBy { it.description.lowercase() }.map { CatalogRow.Entry(it) }
            }
    }

    val gameCount = remember(patches) { patches.map { it.title }.distinct().size }

    PaneScaffold(
        title = stringResource(R.string.patches_all_title),
        tabs = listOf(PaneTab(stringResource(R.string.patches_all_tab), Icons.Outlined.Healing)),
        selected = 0,
        onSelect = {},
        onBack = onClose,
        scrollableContent = false,
        modifier = modifier
    ) {
        PatchUpdateCard(onUpdated = { reloadToken++ })

        Spacer(Modifier.height(12.dp))

        if (loading) {
            Box(
                modifier = Modifier
                    .fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                CircularProgressIndicator(color = Rpcs.Accent)
            }
            return@PaneScaffold
        }

        Text(
            text = stringResource(R.string.patches_all_summary, patches.size, gameCount),
            color = Rpcs.TextSecondary,
            fontSize = 11.sp,
            fontWeight = FontWeight.SemiBold
        )
        Spacer(Modifier.height(4.dp))
        Text(
            text = stringResource(R.string.patches_all_hint),
            color = Rpcs.TextDim,
            fontSize = 11.sp,
            lineHeight = 14.sp
        )

        Spacer(Modifier.height(10.dp))

        LazyColumn(
            modifier = Modifier.fillMaxSize(),
            verticalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            items(rows) { row ->
                when (row) {
                    is CatalogRow.Header -> CatalogHeader(row)
                    is CatalogRow.Entry -> CatalogEntry(row.patch)
                }
            }
        }
    }
}

@Composable
private fun CatalogHeader(row: CatalogRow.Header) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(top = 10.dp, bottom = 2.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            modifier = Modifier.weight(1f),
            text = row.title,
            color = Rpcs.Accent,
            fontSize = 12.sp,
            fontWeight = FontWeight.SemiBold,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
        Text(
            text = row.count.toString(),
            color = Rpcs.TextDim,
            fontSize = 11.sp
        )
    }
}

@Composable
private fun CatalogEntry(patch: CatalogPatch) {
    val detail = buildList {
        if (patch.serials.isNotEmpty()) add(patch.serials)
        if (patch.appVersions.isNotEmpty()) add("app " + patch.appVersions)
        if (patch.author.isNotEmpty()) add("by " + patch.author)
        if (patch.patchVersion.isNotEmpty()) add("v" + patch.patchVersion)
    }.joinToString(" · ")

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(Dims.RowCorner))
            .border(Dims.BorderWidth, Rpcs.OutlineSoft, RoundedCornerShape(Dims.RowCorner))
            .padding(horizontal = 10.dp, vertical = 8.dp)
    ) {
        Text(
            text = patch.description,
            color = Rpcs.TextSecondary,
            fontSize = 12.sp,
            maxLines = 2,
            overflow = TextOverflow.Ellipsis
        )
        if (detail.isNotEmpty()) {
            Text(
                text = detail,
                color = Rpcs.TextDim,
                fontSize = 10.sp,
                lineHeight = 13.sp,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis
            )
        }
    }
}
