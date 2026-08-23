package net.rpcs3.ui.settings

import android.content.Intent
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Edit
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import net.rpcs3.R
import net.rpcs3.overlay.OverlayEditActivity
import net.rpcs3.overlay.OverlayPrefs
import net.rpcs3.ui.components.GhostButton
import net.rpcs3.ui.components.InfoRow
import net.rpcs3.ui.components.SectionLabel
import net.rpcs3.ui.components.SettingGroup
import net.rpcs3.ui.components.SettingSlider
import net.rpcs3.ui.components.SettingSwitch
import net.rpcs3.ui.components.ThinDivider
import net.rpcs3.ui.theme.Dimens

const val ControlsCategory = "Controls"

@Composable
fun ControlsSettings(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    var opacity by remember {
        mutableFloatStateOf(OverlayPrefs.getOpacity(context).toFloat())
    }
    var overlayEnabled by remember {
        mutableStateOf(OverlayPrefs.isEnabled(context))
    }

    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(Dimens.SectionGap)
    ) {
        SectionLabel(text = stringResource(R.string.controls_section))

        SettingGroup {
            SettingSwitch(
                label = stringResource(R.string.controls_show_overlay),
                subtitle = stringResource(R.string.controls_show_overlay_hint),
                checked = overlayEnabled,
                onCheckedChange = {
                    overlayEnabled = it
                    OverlayPrefs.setEnabled(context, it)
                }
            )

            ThinDivider()

            SettingSlider(
                label = stringResource(R.string.controls_button_transparency),
                value = opacity,
                valueRange = 0f..100f,
                valueText = stringResource(R.string.percent_value, opacity.toInt()),
                onValueChange = { opacity = it },
                onValueChangeFinished = {
                    OverlayPrefs.setOpacity(context, opacity.toInt())
                }
            )

            InfoRow(
                label = stringResource(R.string.controls_applies_to),
                value = stringResource(R.string.controls_applies_to_value)
            )

            GhostButton(
                label = stringResource(R.string.controls_edit_layout),
                icon = Icons.Outlined.Edit,
                onClick = {
                    context.startActivity(Intent(context, OverlayEditActivity::class.java))
                }
            )
        }
    }
}
