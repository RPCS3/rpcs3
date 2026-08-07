
package net.rpcs3.ui.settings.components.core

import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.updateTransition
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.animateDp
import androidx.compose.animation.animateColor
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.SwitchDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.Alignment
import androidx.compose.ui.draw.shadow

@Composable
fun MaterialSwitch(
    checked: Boolean = true,
    enabled: Boolean = true,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier
) {
    val colors = SwitchDefaults.colors()
    val transition = updateTransition(targetState = checked, label = "switchTransition")

    val thumbOffset by transition.animateFloat(
        transitionSpec = { tween(300, easing = FastOutSlowInEasing) },
        label = "thumbOffset"
    ) { if (it) 24f else 6f }

    val thumbSize by transition.animateDp(
        transitionSpec = { tween(300) },
        label = "thumbSize"
    ) { if (it) 24.dp else 15.dp }

    val trackColor by transition.animateColor(
        transitionSpec = { tween(300) },
        label = "trackColor"
    ) { if (it) if (enabled) colors.checkedTrackColor else colors.disabledCheckedTrackColor
      else if (enabled) colors.uncheckedTrackColor else colors.disabledUncheckedTrackColor
    }

    val thumbColor by transition.animateColor(
        transitionSpec = { tween(300) },
        label = "thumbColor"
    ) { if (it) if (enabled) colors.checkedThumbColor else colors.disabledCheckedThumbColor
      else if (enabled) colors.uncheckedThumbColor else colors.disabledUncheckedThumbColor
    }

    val borderColor by transition.animateColor(
        transitionSpec = { tween(300) },
        label = "borderColor"
    ) { if (it) if (enabled) colors.checkedBorderColor else colors.disabledCheckedBorderColor
      else if (enabled) colors.uncheckedBorderColor else colors.disabledUncheckedBorderColor
    }

    Box(
        modifier = modifier
            .width(52.dp)
            .height(32.dp)
            .clip(RoundedCornerShape(16.dp))
            .border(if (!checked) 2.dp else 0.dp, borderColor, RoundedCornerShape(16.dp))
            .background(trackColor)
            .clickable(enabled) { onCheckedChange(!checked) },
        contentAlignment = Alignment.CenterStart
    ) {
        Box(
            modifier = Modifier
                .size(thumbSize)
                .offset(x = thumbOffset.dp)
                .shadow(4.dp, CircleShape)
                .clip(CircleShape)
                .background(thumbColor)
        )
    }
}

@Preview
@Composable
fun MaterialSwitchPreview() {
    var switchState = true
    MaterialSwitch(
        checked = switchState,
        onCheckedChange = { switchState = it }
    )
}
