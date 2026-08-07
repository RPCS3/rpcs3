package net.rpcs3.ui.settings.components.core

import androidx.compose.material3.Icon
import androidx.compose.material3.LocalContentColor
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.unit.dp
import net.rpcs3.ui.settings.components.LocalPreferenceState
import net.rpcs3.ui.settings.util.preferenceColor
import net.rpcs3.ui.settings.util.sizeIn

/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Wed, Mar 05, 2025
 * Time: 1:32 am
 */

@Composable
fun PreferenceIcon(
    icon: ImageVector?,
    modifier: Modifier = Modifier,
    enabled: Boolean = LocalPreferenceState.current,
    contentDescription: String? = null,
    tint: Color = preferenceColor(enabled, LocalContentColor.current),
) {
    if (icon != null) {
        Icon(
            imageVector = icon,
            modifier = modifier.sizeIn(minSize = 24.dp, maxSize = 48.dp),
            contentDescription = contentDescription,
            tint = tint
        )
    }
}

@Composable
fun PreferenceIcon(
    icon: Painter?,
    modifier: Modifier = Modifier,
    enabled: Boolean = LocalPreferenceState.current,
    contentDescription: String? = null,
    tint: Color = preferenceColor(enabled, LocalContentColor.current),
) {
    if (icon != null) {
        Icon(
            painter = icon,
            modifier = modifier.sizeIn(minSize = 24.dp, maxSize = 48.dp),
            contentDescription = contentDescription,
            tint = tint
        )
    }
}
