package net.rpcs3.ui.settings.components.base

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import net.rpcs3.ui.common.ComposePreview
import net.rpcs3.ui.settings.components.LocalPreferenceState
import net.rpcs3.ui.settings.components.core.PreferenceIcon
import net.rpcs3.ui.settings.components.core.PreferenceSubtitle
import net.rpcs3.ui.settings.components.core.PreferenceTitle


/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Wed, Mar 05, 2025
 * Time: 1:03 am
 */


/**
 * A composable function that creates a base layout for a preference item.
 *
 * @param title title of the preference.
 * @param modifier The modifier applied to the preference container.
 * @param subContent Optional composable content to display below the title.
 * @param leadingContent Optional composable content to display at the start of the preference item.
 * This is typically used for icons or other visual cues.
 *
 * @param trailingContent Optional composable content to display at the end of the preference item.
 * This is typically used for switches, checkboxes, or other interactive elements.
 *
 * @param shape The shape of the preference surface.
 * @param tonalElevation The tonal elevation of the preference surface.
 * @param shadowElevation The shadow elevation of the preference surface.
 * @param enabled Whether the preference is enabled or disabled.
 * @param onClick callback invoked when the preference item is clicked.
 *
 * @see Surface
 */

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun BasePreference(
    title: @Composable () -> Unit,
    modifier: Modifier = Modifier,
    subContent: @Composable (() -> Unit)? = null,
    value: @Composable (() -> Unit)? = null,
    leadingContent: @Composable (() -> Unit)? = null,
    trailingContent: @Composable (() -> Unit)? = null,
    shape: Shape = RoundedCornerShape(0),
    tonalElevation: Dp = 0.dp,
    shadowElevation: Dp = 0.dp,
    enabled: Boolean = true,
    onClick: () -> Unit,
    onLongClick: () -> Unit = {}
) {
    CompositionLocalProvider(
        LocalPreferenceState provides enabled
    ) {
        val preferenceOnClick: () -> Unit = {
            if (enabled) onClick()
        }
        Surface(
            modifier = modifier.combinedClickable(
                onClick = preferenceOnClick,
                onLongClick = onLongClick
            ),
            shape = shape,
            tonalElevation = tonalElevation,
            shadowElevation = shadowElevation
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp)
                    .heightIn(min = 72.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                leadingContent?.invoke()
                Column(
                    modifier = Modifier.weight(1f),
                    verticalArrangement = Arrangement.spacedBy(4.dp, Alignment.CenterVertically),
                ) {
                    title()
                    subContent?.invoke()
                    value?.invoke()
                }
                trailingContent?.invoke()
            }
        }
    }
}

@Preview
@Composable
private fun BasePreferencePreview() {
    ComposePreview {
        BasePreference(
            title = { PreferenceTitle("Preference Title") },
            subContent = { PreferenceSubtitle("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ullamcorper tempor imperdiet. Tempor magna proident pariatur nonumy iusto, sint laborum possim accumsan, elit nonummy facer enim autem eiusmod lobortis reprehenderit molestie vel esse aliquyam cupiditat velit nisi aliquid ipsum. Erat accusam reprehenderit. Feugiat aliquyam iure. Nisi ex officia.", maxLines = 2) },
            leadingContent = { PreferenceIcon(Icons.Default.Search) },
            trailingContent = { PreferenceIcon(Icons.AutoMirrored.Default.KeyboardArrowRight) },
            onClick = {}
        )
    }
}

@Preview
@Composable
private fun BasePreferenceDisabledPreview() {
    ComposePreview {
        BasePreference(
            title = { PreferenceTitle("Preference Title") },
            subContent = { PreferenceSubtitle("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ullamcorper tempor imperdiet. Tempor magna proident pariatur nonumy iusto, sint laborum possim accumsan, elit nonummy facer enim autem eiusmod lobortis reprehenderit molestie vel esse aliquyam cupiditat velit nisi aliquid ipsum. Erat accusam reprehenderit. Feugiat aliquyam iure. Nisi ex officia.", maxLines = 2) },
            leadingContent = { PreferenceIcon(Icons.Default.Search) },
            trailingContent = { PreferenceIcon(Icons.AutoMirrored.Default.KeyboardArrowRight) },
            enabled = false,
            onClick = {}
        )
    }
}
