package net.rpcs3

import android.app.Activity
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowInsetsControllerCompat
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.RpcsTypography

private val RpcsColorScheme =
    darkColorScheme(
        primary = Rpcs.Accent,
        onPrimary = Rpcs.TextPrimary,
        primaryContainer = Rpcs.SelectionFill,
        onPrimaryContainer = Rpcs.Accent,
        inversePrimary = Rpcs.AccentBright,
        secondary = Rpcs.AccentBright,
        onSecondary = Rpcs.TextPrimary,
        secondaryContainer = Rpcs.SelectionFill,
        onSecondaryContainer = Rpcs.AccentBright,
        tertiary = Rpcs.Accent,
        onTertiary = Rpcs.TextPrimary,
        tertiaryContainer = Rpcs.SelectionFill,
        onTertiaryContainer = Rpcs.Accent,
        errorContainer = Rpcs.Danger.copy(alpha = 0.16f),
        onErrorContainer = Rpcs.Danger,
        inverseSurface = Rpcs.TextPrimary,
        inverseOnSurface = Rpcs.Background,
        scrim = Rpcs.Background,
        background = Rpcs.Background,
        onBackground = Rpcs.TextPrimary,
        surface = Rpcs.Surface,
        onSurface = Rpcs.TextPrimary,
        surfaceVariant = Rpcs.SurfaceRaised,
        onSurfaceVariant = Rpcs.TextSecondary,
        surfaceContainerLowest = Rpcs.Background,
        surfaceContainerLow = Rpcs.Surface,
        surfaceContainer = Rpcs.Surface,
        surfaceContainerHigh = Rpcs.SurfaceRaised,
        surfaceContainerHighest = Rpcs.SurfaceRaised,
        outline = Rpcs.Outline,
        outlineVariant = Rpcs.OutlineSoft,
        error = Rpcs.Danger,
        onError = Rpcs.TextPrimary,
    )

@Composable
fun RPCS3Theme(content: @Composable () -> Unit) {
    val view = LocalView.current
    val activity = view.context as? Activity

    SideEffect {
        activity?.window?.apply {
            statusBarColor = android.graphics.Color.TRANSPARENT
            navigationBarColor = android.graphics.Color.TRANSPARENT
            isNavigationBarContrastEnforced = false
            val insetsController = WindowInsetsControllerCompat(this, decorView)
            insetsController.isAppearanceLightNavigationBars = false
            insetsController.isAppearanceLightStatusBars = false
        }
    }

    MaterialTheme(
        colorScheme = RpcsColorScheme,
        typography = RpcsTypography,
        content = content,
    )
}
