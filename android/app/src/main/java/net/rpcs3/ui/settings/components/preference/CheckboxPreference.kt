package net.rpcs3.ui.settings.components.preference

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CheckboxColors
import androidx.compose.material3.CheckboxDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.tooling.preview.PreviewLightDark
import net.rpcs3.ui.common.ComposePreview
import net.rpcs3.ui.settings.components.core.PreferenceIcon
import net.rpcs3.ui.settings.components.core.PreferenceSubtitle
import net.rpcs3.ui.settings.components.core.PreferenceTitle


/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Sat, Mar 08, 2025
 * Time: 7:26 pm
 */

@Composable
fun CheckboxPreference(
    checked: Boolean,
    title: @Composable () -> Unit,
    leadingIcon: @Composable () -> Unit,
    modifier: Modifier = Modifier,
    subtitle: @Composable (() -> Unit)? = null,
    enabled: Boolean = true,
    checkboxColors: CheckboxColors = CheckboxDefaults.colors(),
    onClick: (Boolean) -> Unit
) {
    val onValueUpdated: (Boolean) -> Unit = { newValue -> onClick(newValue) }
    RegularPreference(
        title = title,
        leadingIcon = leadingIcon,
        modifier = modifier,
        subtitle = subtitle,
        enabled = enabled,
        trailingContent = {
            Checkbox(
                checked = checked,
                onCheckedChange = { onValueUpdated(it) },
                enabled = enabled,
                colors = checkboxColors,
            )
        },
        onClick = { onValueUpdated(!checked) }
    )
}

@Composable
fun CheckboxPreference(
    checked: Boolean,
    title: String,
    leadingIcon: ImageVector,
    modifier: Modifier = Modifier,
    subtitle: @Composable (() -> Unit)? = null,
    enabled: Boolean = true,
    checkboxColors: CheckboxColors = CheckboxDefaults.colors(),
    onClick: (Boolean) -> Unit
) {
    CheckboxPreference(
        checked = checked,
        title = { PreferenceTitle(title = title) },
        leadingIcon = { PreferenceIcon(icon = leadingIcon) },
        modifier = modifier,
        subtitle = subtitle,
        enabled = enabled,
        checkboxColors = checkboxColors,
        onClick = onClick
    )
}

@PreviewLightDark
@Composable
private fun CheckboxPreferencePreview() {
    ComposePreview {
        var isChecked by remember { mutableStateOf(true) }
        CheckboxPreference(
            checked = isChecked,
            title = "Enable Something",
            subtitle = { PreferenceSubtitle(text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.") },
            leadingIcon = Icons.Default.Build
        ) { isChecked = it }
    }
}

@PreviewLightDark
@Composable
private fun CheckboxPreferenceDisabledPreview() {
    ComposePreview {
        var isChecked by remember { mutableStateOf(false) }
        CheckboxPreference(
            checked = isChecked,
            title = "Enable Something",
            subtitle = { PreferenceSubtitle(text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.") },
            leadingIcon = Icons.Default.Build,
            enabled = false
        ) { isChecked = it }
    }
}