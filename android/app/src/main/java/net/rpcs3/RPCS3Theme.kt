package net.rpcs3

import android.app.Activity
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowInsetsControllerCompat
import net.rpcs3.ui.theme.RpcsAccent
import net.rpcs3.ui.theme.RpcsBackground
import net.rpcs3.ui.theme.RpcsDanger
import net.rpcs3.ui.theme.RpcsOutline
import net.rpcs3.ui.theme.RpcsPanel
import net.rpcs3.ui.theme.RpcsSurface
import net.rpcs3.ui.theme.RpcsSurfaceAlt
import net.rpcs3.ui.theme.RpcsTextPrimary
import net.rpcs3.ui.theme.RpcsTextSecondary
import net.rpcs3.ui.theme.RpcsTypography

private val RpcsColorScheme =
    darkColorScheme(
        primary = RpcsAccent,
        onPrimary = RpcsTextPrimary,
        primaryContainer = RpcsSurface,
        onPrimaryContainer = RpcsTextPrimary,
        secondary = RpcsAccent,
        onSecondary = RpcsTextPrimary,
        secondaryContainer = RpcsSurfaceAlt,
        onSecondaryContainer = RpcsTextPrimary,
        tertiary = RpcsAccent,
        onTertiary = RpcsTextPrimary,
        tertiaryContainer = RpcsSurfaceAlt,
        onTertiaryContainer = RpcsTextPrimary,
        errorContainer = RpcsPanel,
        onErrorContainer = RpcsDanger,
        inverseSurface = RpcsTextPrimary,
        inverseOnSurface = RpcsBackground,
        scrim = RpcsBackground,
        background = RpcsBackground,
        onBackground = RpcsTextPrimary,
        surface = RpcsSurface,
        onSurface = RpcsTextPrimary,
        surfaceVariant = RpcsSurfaceAlt,
        onSurfaceVariant = RpcsTextSecondary,
        surfaceContainer = RpcsPanel,
        surfaceContainerHigh = RpcsSurfaceAlt,
        surfaceContainerLow = RpcsPanel,
        outline = RpcsOutline,
        outlineVariant = RpcsOutline,
        error = RpcsDanger,
        onError = RpcsTextPrimary,
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
