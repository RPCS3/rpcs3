package net.rpcs3.ui.settings.util

import androidx.compose.foundation.layout.sizeIn
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.Dp


fun Modifier.sizeIn(
    minSize: Dp = Dp.Unspecified,
    maxSize: Dp = Dp.Unspecified
) = this.sizeIn(minWidth = minSize, minHeight = minSize, maxWidth = maxSize, maxHeight = maxSize)