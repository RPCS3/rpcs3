package net.rpcs3.ui.hud

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
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
import net.rpcs3.ui.components.SettingChip
import net.rpcs3.ui.components.SettingGroup
import net.rpcs3.ui.components.SettingSlider
import net.rpcs3.ui.components.SettingSwitch
import net.rpcs3.ui.components.SettingsHint
import net.rpcs3.ui.components.SettingsSection
import net.rpcs3.ui.components.ThinDivider
import net.rpcs3.ui.theme.Dimens

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

    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(Dimens.SectionGap)
    ) {
        SettingsSection(title = stringResource(R.string.hud_section_overlay)) {
            SettingGroup {
                SettingSwitch(
                    label = stringResource(R.string.hud_show),
                    subtitle = stringResource(R.string.hud_show_hint),
                    checked = enabled,
                    onCheckedChange = {
                        enabled = it
                        HudPrefs.setEnabled(prefs, it)
                    }
                )
            }
        }

        SettingsSection(title = stringResource(R.string.hud_section_layout)) {
            SettingGroup {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(Dimens.TightGap)
                ) {
                    HudMode.entries.forEach { candidate ->
                        SettingChip(
                            label = stringResource(candidate.labelRes),
                            detail = stringResource(candidate.detailRes),
                            selected = candidate == mode,
                            modifier = Modifier.weight(1f),
                            onClick = {
                                mode = candidate
                                HudPrefs.setMode(prefs, candidate)
                            }
                        )
                    }
                }

                ThinDivider()

                SettingSlider(
                    label = stringResource(R.string.hud_text_size),
                    value = scale,
                    valueRange = 0.6f..2f,
                    valueText = stringResource(R.string.percent_value, (scale * 100).toInt()),
                    onValueChange = { scale = it },
                    onValueChangeFinished = { HudPrefs.setScale(prefs, scale) }
                )
            }

            SettingsHint(text = stringResource(R.string.hud_gesture_hint))
        }

        SettingsSection(title = stringResource(R.string.hud_section_readouts)) {
            SettingGroup {
                HudElement.entries.forEachIndexed { index, element ->
                    if (index > 0) {
                        ThinDivider()
                    }
                    SettingSwitch(
                        label = stringResource(element.labelRes),
                        checked = active.contains(element),
                        onCheckedChange = { wanted ->
                            HudPrefs.setElementEnabled(prefs, element, wanted)
                            active = HudPrefs.enabledElements(prefs)
                        }
                    )
                }

                ThinDivider()

                SettingSwitch(
                    label = stringResource(R.string.hud_frametime_numeric),
                    subtitle = stringResource(
                        if (numericFrametime) {
                            R.string.hud_frametime_numeric_on
                        } else {
                            R.string.hud_frametime_numeric_off
                        }
                    ),
                    checked = numericFrametime,
                    enabled = active.contains(HudElement.Frametime),
                    onCheckedChange = {
                        numericFrametime = it
                        HudPrefs.setFrametimeNumeric(prefs, it)
                    }
                )
            }
        }
    }
}
