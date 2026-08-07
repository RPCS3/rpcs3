package net.rpcs3.ui.settings.util

import androidx.compose.foundation.layout.sizeIn
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.Dp

/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Wed, Mar 05, 2025
 * Time: 1:34 am
 */

/**
 * Constrains the size of the element to be within given bounds.
 *
 * The element's size will be at least [minSize] and at most [maxSize] in both dimensions.
 * If only one constraint is specified, the other dimension will be determined by the content.
 *
 * @param minSize The minimum size of the layout element in both dimensions.
 * @param maxSize The maximum size of the layout element in both dimensions.
 */
fun Modifier.sizeIn(
    minSize: Dp = Dp.Unspecified,
    maxSize: Dp = Dp.Unspecified
) = this.sizeIn(minWidth = minSize, minHeight = minSize, maxWidth = maxSize, maxHeight = maxSize)