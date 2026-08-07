package net.rpcs3.ui.settings.util

import androidx.compose.ui.graphics.Color

/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Wed, Mar 05, 2025
 * Time: 1:34 am
 */

/**
 * A low level of alpha used to represent disabled components, such as text in a disabled Button.
 */
internal const val DisabledAlpha = 0.38f

internal const val MediumAlpha = 0.67f

/**
 * Returns a color to be used for preference title or leading/trailing icons.
 *
 * If the preference is enabled, the provided `contentColor` is returned as is.
 * If the preference is disabled, the `contentColor` is returned with its alpha
 * adjusted to represent a disabled state.
 *
 * @param enabled Whether the preference is enabled.
 * @param contentColor The base color to be used for the preference.
 * @return The color to be used for the preference title or icons.
 */
fun preferenceColor(enabled: Boolean, contentColor: Color) =
    if (!enabled) contentColor.copy(alpha = DisabledAlpha) else contentColor

/**
 * Returns a color to be used for preference subtitle.
 *
 * The returned color is based on the enabled state of the preference and the provided content color.
 * If the preference is disabled, the content color will be modified to have a lower alpha value
 * representing the disabled state. Otherwise, the content color will be modified to have a medium
 * alpha value.
 *
 * @param enabled Whether the preference is enabled.
 * @param contentColor The base content color of the preference subtitle.
 * @return The color to be used for the preference subtitle.
 */
fun preferenceSubtitleColor(enabled: Boolean, contentColor: Color) =
    if (!enabled) contentColor.copy(alpha = DisabledAlpha) else contentColor.copy(alpha = MediumAlpha)