package net.rpcs3.ui.settings.components.preference

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Settings
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.tooling.preview.PreviewLightDark
import net.rpcs3.ui.common.ComposePreview
import net.rpcs3.ui.settings.components.base.BasePreference
import net.rpcs3.ui.settings.components.core.PreferenceIcon
import net.rpcs3.ui.settings.components.core.PreferenceSubtitle
import net.rpcs3.ui.settings.components.core.PreferenceTitle


/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Wed, Mar 05, 2025
 * Time: 1:01 am
 */

/**
 * A regular preference item.
 * This is a simple preference item with a title, subtitle, leading icon, and trailing content.
 * This can also be called a simple TextPreference.
 * which is just a preference item, meant to show something to the user.
 * or be used to navigate the user to another screen.
 */

@Composable
fun RegularPreference(
    title: @Composable () -> Unit,
    leadingIcon: @Composable (() -> Unit) = {},
    modifier: Modifier = Modifier,
    subtitle: @Composable (() -> Unit)? = null,
    value: @Composable (() -> Unit)? = null,
    trailingContent: @Composable (() -> Unit)? = null,
    enabled: Boolean = true,
    onClick: () -> Unit,
    onLongClick: () -> Unit = {}
) {
    BasePreference(
        title = title,
        modifier = modifier,
        subContent = subtitle,
        value = value,
        leadingContent = leadingIcon,
        trailingContent = trailingContent,
        enabled = enabled,
        onClick = onClick,
        onLongClick = onLongClick
    )
}

@Composable
fun RegularPreference(
    title: String,
    leadingIcon: ImageVector? = null,
    modifier: Modifier = Modifier,
    subtitle: @Composable (() -> Unit)? = null,
    value: @Composable (() -> Unit)? = null,
    trailingContent: @Composable (() -> Unit)? = null,
    enabled: Boolean = true,
    onClick: () -> Unit,
    onLongClick: () -> Unit = {}
) {
    RegularPreference(
        title = { PreferenceTitle(title = title) },
        leadingIcon = { PreferenceIcon(icon = leadingIcon) },
        modifier = modifier,
        subtitle = subtitle,
        value = value,
        trailingContent = trailingContent,
        enabled = enabled,
        onClick = onClick,
        onLongClick = onLongClick
    )
}

@PreviewLightDark
@Composable
private fun RegularPreferencePreview() {
    ComposePreview {
        RegularPreference(
            title = "Install Firmware",
            leadingIcon = Icons.Default.Settings,
            subtitle = { PreferenceSubtitle(text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ullamcorper tempor imperdiet. Tempor magna proident pariatur nonumy iusto, sint laborum possim accumsan, elit nonummy facer enim autem eiusmod lobortis reprehenderit molestie vel esse aliquyam cupiditat velit nisi aliquid ipsum. Erat accusam reprehenderit. Feugiat aliquyam iure. Nisi ex officia.") },
            trailingContent = { PreferenceIcon(icon = Icons.AutoMirrored.Default.KeyboardArrowRight) },
            onClick = { }
        )
    }
}

@PreviewLightDark
@Composable
private fun RegularPreferenceDisabledPreview() {
    ComposePreview {
        RegularPreference(
            title = "Advanced Settings",
            leadingIcon = Icons.Default.Settings,
            subtitle = { PreferenceSubtitle(text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ullamcorper tempor imperdiet. Tempor magna proident pariatur nonumy iusto, sint laborum possim accumsan, elit nonummy facer enim autem eiusmod lobortis reprehenderit molestie vel esse aliquyam cupiditat velit nisi aliquid ipsum. Erat accusam reprehenderit. Feugiat aliquyam iure. Nisi ex officia.") },
            trailingContent = { PreferenceIcon(icon = Icons.AutoMirrored.Default.KeyboardArrowRight) },
            enabled = false,
            onClick = { }
        )
    }
}
