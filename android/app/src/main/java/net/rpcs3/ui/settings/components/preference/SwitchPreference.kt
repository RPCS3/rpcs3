package net.rpcs3.ui.settings.components.preference

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import net.rpcs3.ui.settings.components.core.MaterialSwitch
import androidx.compose.material3.SwitchColors
import androidx.compose.material3.SwitchDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.tooling.preview.PreviewLightDark
import net.rpcs3.ui.common.ComposePreview
import net.rpcs3.ui.settings.components.core.PreferenceIcon
import net.rpcs3.ui.settings.components.core.PreferenceSubtitle
import net.rpcs3.ui.settings.components.core.PreferenceTitle

/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Sat, Mar 08, 2025
 * Time: 6:59 pm
 */

@Composable
fun SwitchPreference(
    checked: Boolean,
    title: @Composable () -> Unit,
    leadingIcon: @Composable () -> Unit = {},
    modifier: Modifier = Modifier,
    subtitle: @Composable (() -> Unit)? = null,
    enabled: Boolean = true,
    switchColors: SwitchColors = SwitchDefaults.colors(),
    onClick: (Boolean) -> Unit,
    onLongClick: () -> Unit = {}
) {
    val onValueUpdated: (Boolean) -> Unit = { newValue -> onClick(newValue) }
    RegularPreference(
        title = title,
        subtitle = subtitle,
        modifier = modifier,
        leadingIcon = leadingIcon,
        trailingContent = {
            MaterialSwitch(
                checked = checked,
                onCheckedChange = { onValueUpdated(it) },
                enabled = enabled
            )
        },
        enabled = enabled,
        onClick = { onValueUpdated(!checked) },
        onLongClick = onLongClick
    )
}

@Composable
fun SwitchPreference(
    checked: Boolean,
    title: String,
    leadingIcon: ImageVector? = null,
    modifier: Modifier = Modifier,
    subtitle: @Composable (() -> Unit)? = null,
    enabled: Boolean = true,
    switchColors: SwitchColors = SwitchDefaults.colors(),
    onClick: (Boolean) -> Unit,
    onLongClick: () -> Unit = {}
) {
    SwitchPreference(
        checked = checked,
        title = { PreferenceTitle(title = title) },
        leadingIcon = { PreferenceIcon(icon = leadingIcon) },
        modifier = modifier,
        subtitle = subtitle,
        enabled = enabled,
        switchColors = switchColors,
        onClick = onClick,
        onLongClick = onLongClick
    )
}

@PreviewLightDark
@Composable
private fun SwitchPreview() {
    ComposePreview {
        var switchState by remember { mutableStateOf(true) }
        SwitchPreference(
            checked = switchState,
            title = "Enable Something",
            subtitle = { PreferenceSubtitle(text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.") },
            leadingIcon = Icons.Default.Build,
            onClick = {
                switchState = it
            }
        )
    }
}

@PreviewLightDark
@Composable
private fun SwitchDisabledPreview() {
    ComposePreview {
        var switchState by remember { mutableStateOf(true) }
        SwitchPreference(
            checked = switchState,
            title = "Enable Something",
            subtitle = { PreferenceSubtitle(text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.") },
            leadingIcon = Icons.Default.Build,
            enabled = false,
            onClick = {
                switchState = it
            }
        )
    }
}
