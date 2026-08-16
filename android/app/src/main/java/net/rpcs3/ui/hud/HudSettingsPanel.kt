package net.rpcs3.ui.hud

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.ui.theme.Rpcs

const val HudCategory = "HUD"

@Composable
fun HudSettingsPanel(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val prefs = remember { HudPrefs.of(context) }

    var enabled by remember { mutableStateOf(HudPrefs.isEnabled(prefs)) }
    var active by remember { mutableStateOf(HudPrefs.enabledElements(prefs)) }
    var scale by remember { mutableFloatStateOf(HudPrefs.scale(prefs)) }
    var mode by remember { mutableStateOf(HudPrefs.mode(prefs)) }
    var numericFrametime by remember { mutableStateOf(HudPrefs.frametimeNumeric(prefs)) }

    Column(modifier = modifier.fillMaxWidth()) {
        HudToggleRow(
            title = "Show HUD",
            subtitle = "Overlay performance readouts on the game",
            checked = enabled,
            onCheckedChange = {
                enabled = it
                HudPrefs.setEnabled(prefs, it)
            }
        )

        Spacer(Modifier.height(14.dp))

        Text(
            text = "MONITORED",
            color = Rpcs.TextDim,
            fontSize = 10.sp,
            fontWeight = FontWeight.SemiBold
        )
        Spacer(Modifier.height(6.dp))

        HudElement.entries.forEach { element ->
            HudToggleRow(
                title = element.label,
                subtitle = null,
                checked = active.contains(element),
                onCheckedChange = { wanted ->
                    HudPrefs.setElementEnabled(prefs, element, wanted)
                    active = HudPrefs.enabledElements(prefs)
                }
            )
            Spacer(Modifier.height(4.dp))
        }

        Spacer(Modifier.height(10.dp))

        HudToggleRow(
            title = "Frame time as number",
            subtitle = if (numericFrametime) {
                "Showing milliseconds as text"
            } else {
                "Showing the frame time line graph"
            },
            checked = numericFrametime,
            onCheckedChange = {
                numericFrametime = it
                HudPrefs.setFrametimeNumeric(prefs, it)
            }
        )

        Spacer(Modifier.height(14.dp))

        Text(
            text = "LAYOUT",
            color = Rpcs.TextDim,
            fontSize = 10.sp,
            fontWeight = FontWeight.SemiBold
        )
        Spacer(Modifier.height(6.dp))

        Text(
            text = "Tap the HUD to cycle layouts, drag to move, hold to snap it to an edge.",
            color = Rpcs.TextSecondary,
            fontSize = 11.sp,
            lineHeight = 15.sp
        )

        Spacer(Modifier.height(10.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            HudMode.entries.forEach { candidate ->
                HudModeChip(
                    label = if (candidate.horizontal) "Row" else "Stack",
                    detail = if (candidate.backdrop) "shaded" else "plain",
                    selected = candidate == mode,
                    modifier = Modifier.weight(1f),
                    onClick = {
                        mode = candidate
                        HudPrefs.setMode(prefs, candidate)
                    }
                )
            }
        }

        Spacer(Modifier.height(14.dp))

        Text(
            text = "Text size  ${(scale * 100).toInt()}%",
            color = Rpcs.TextSecondary,
            fontSize = 11.sp
        )

        Slider(
            value = scale,
            onValueChange = { scale = it },
            onValueChangeFinished = { HudPrefs.setScale(prefs, scale) },
            valueRange = 0.6f..2f,
            colors = SliderDefaults.colors(
                thumbColor = Rpcs.Accent,
                activeTrackColor = Rpcs.Accent
            )
        )
    }
}

@Composable
private fun HudToggleRow(
    title: String,
    subtitle: String?,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(10.dp))
            .border(1.dp, Rpcs.OutlineSoft, RoundedCornerShape(10.dp))
            .clickable { onCheckedChange(!checked) }
            .padding(horizontal = 12.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                color = Rpcs.TextPrimary,
                fontSize = 12.sp,
                fontWeight = FontWeight.Medium
            )
            if (subtitle != null) {
                Text(
                    text = subtitle,
                    color = Rpcs.TextDim,
                    fontSize = 10.sp,
                    lineHeight = 13.sp
                )
            }
        }

        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange,
            colors = SwitchDefaults.colors(
                checkedThumbColor = Rpcs.TextPrimary,
                checkedTrackColor = Rpcs.Accent
            )
        )
    }
}

@Composable
private fun HudModeChip(
    label: String,
    detail: String,
    selected: Boolean,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    Column(
        modifier = modifier
            .background(
                if (selected) Rpcs.SelectionFill else Rpcs.SurfaceRaised,
                RoundedCornerShape(10.dp)
            )
            .border(
                1.dp,
                if (selected) Rpcs.SelectionBorder else Rpcs.OutlineSoft,
                RoundedCornerShape(10.dp)
            )
            .clickable(onClick = onClick)
            .padding(vertical = 8.dp, horizontal = 6.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = label,
            color = if (selected) Rpcs.Accent else Rpcs.TextPrimary,
            fontSize = 11.sp,
            fontWeight = FontWeight.SemiBold
        )
        Text(
            text = detail,
            color = Rpcs.TextDim,
            fontSize = 9.sp
        )
    }
}
