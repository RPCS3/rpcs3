package net.rpcs3.ui.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import net.rpcs3.ui.components.CollapsibleSection
import net.rpcs3.ui.components.SectionLabel
import net.rpcs3.ui.components.SettingGroup
import net.rpcs3.ui.components.ThinDivider
import net.rpcs3.ui.theme.Dimens
import org.json.JSONObject

private val CategoryOrder = listOf(
    "Video",
    "Core",
    "Audio",
    "Input/Output",
    "System",
    "Net",
    "VFS",
    "Savestate",
    "Miscellaneous"
)

internal fun topLevelCategoriesOf(root: JSONObject): List<SettingsCategory> =
    categoriesOf(root)
        .filter { it.parent == null }
        .sortedBy { entry ->
            val rank = CategoryOrder.indexOf(entry.label)
            if (rank >= 0) rank else CategoryOrder.size
        }

private fun splitNode(node: JSONObject): Pair<List<Pair<String, JSONObject>>, List<Pair<String, JSONObject>>> {
    val leaves = ArrayList<Pair<String, JSONObject>>()
    val branches = ArrayList<Pair<String, JSONObject>>()

    val keys = node.keys()
    while (keys.hasNext()) {
        val key = keys.next()
        val child = node.optJSONObject(key) ?: continue
        if (child.optString("type", "").isEmpty()) {
            branches.add(key to child)
        } else {
            leaves.add(key to child)
        }
    }

    return leaves to branches
}

@Composable
internal fun SettingsCategoryContent(
    node: JSONObject,
    path: String,
    titleId: String,
    modifier: Modifier = Modifier
) {
    val (leaves, branches) = splitNode(node)

    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(Dimens.SectionGap)
    ) {
        if (leaves.isNotEmpty()) {
            SettingGroup {
                leaves.forEachIndexed { index, (key, item) ->
                    if (index > 0) {
                        ThinDivider()
                    }
                    SettingItem(
                        label = key,
                        item = item,
                        path = "$path@@$key",
                        titleId = titleId
                    )
                }
            }
        }

        branches.forEach { (key, child) ->
            CollapsibleSection(title = key) {
                SettingsSubtreeContent(
                    node = child,
                    path = "$path@@$key",
                    titleId = titleId
                )
            }
        }
    }
}

@Composable
private fun SettingsSubtreeContent(
    node: JSONObject,
    path: String,
    titleId: String
) {
    val (leaves, branches) = splitNode(node)

    leaves.forEachIndexed { index, (key, item) ->
        if (index > 0) {
            ThinDivider()
        }
        SettingItem(
            label = key,
            item = item,
            path = "$path@@$key",
            titleId = titleId
        )
    }

    branches.forEach { (key, child) ->
        SectionLabel(text = key.uppercase())
        SettingsSubtreeContent(
            node = child,
            path = "$path@@$key",
            titleId = titleId
        )
    }
}
