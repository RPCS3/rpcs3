package net.rpcs3.ui.settings.util

import androidx.compose.ui.graphics.Color


internal const val DisabledAlpha = 0.38f

internal const val MediumAlpha = 0.67f

fun preferenceColor(enabled: Boolean, contentColor: Color) =
    if (!enabled) contentColor.copy(alpha = DisabledAlpha) else contentColor

fun preferenceSubtitleColor(enabled: Boolean, contentColor: Color) =
    if (!enabled) contentColor.copy(alpha = DisabledAlpha) else contentColor.copy(alpha = MediumAlpha)