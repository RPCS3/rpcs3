package net.rpcs3.ui.settings.components.preference

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderColors
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.PreviewLightDark
import androidx.compose.ui.unit.dp
import net.rpcs3.ui.common.ComposePreview
import net.rpcs3.ui.settings.components.core.PreferenceIcon
import net.rpcs3.ui.settings.components.core.PreferenceSubtitle
import net.rpcs3.ui.settings.components.core.PreferenceTitle


/**
 * Created using Android Studio
 * User: Muhammad Ashhal
 * Date: Sat, Mar 08, 2025
 * Time: 10:35 pm
 */

@Composable
fun SliderPreference(
    value: Float,
    onValueChange: (Float) -> Unit,
    title: String,
    modifier: Modifier = Modifier,
    leadingIcon: ImageVector? = null,
    subtitle: String? = null,
    enabled: Boolean = true,
    valueRange: ClosedFloatingPointRange<Float> = 0f..1f,
    steps: Int = 0,
    valueContent: @Composable (() -> Unit)? = null,
    sliderColors: SliderColors = SliderDefaults.colors(),
    onLongClick: () -> Unit = {}
) {
    var showDialog by remember { mutableStateOf(false) }
    var tempValue by remember { mutableFloatStateOf(value) }
    var textValue by remember { mutableStateOf(value.toInt().toString()) }
    var isError by remember { mutableStateOf(false) }

    val stepSize = 1 

    fun isStepNotAligned(input: Float): Boolean {
       return input % stepSize != 0f
    }

    RegularPreference(
        modifier = modifier,
        title = { PreferenceTitle(title = title) },
        leadingIcon = { PreferenceIcon(icon = leadingIcon) },
        subtitle = { subtitle?.let { PreferenceSubtitle(text = it) } },
        enabled = enabled,
        onClick = { showDialog = true },
        value = valueContent,
        onLongClick = onLongClick
    )

    if (showDialog) {
        AlertDialog(
            onDismissRequest = { showDialog = false },
            title = { Text(title) },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(16.dp)) {
                    OutlinedTextField(
                        value = textValue,
                        onValueChange = { input ->
                            textValue = input
                            val parsedValue = input.toFloatOrNull()
                            if (parsedValue != null && parsedValue in valueRange) {
                                isError = isStepNotAligned(parsedValue)
                                if (!isError) tempValue = parsedValue
                            } else {
                                isError = true
                            }
                        },
                        keyboardOptions = KeyboardOptions.Default.copy(
                            keyboardType = KeyboardType.Number
                        ),
                        isError = isError,
                        label = { Text("Value") },
                        supportingText = {
                            if (isError) {
                                Text("Value must be a multiple of step size $stepSize within ${valueRange.start} to ${valueRange.endInclusive}")
                            }
                        }
                    )

                    if ((valueRange.endInclusive - valueRange.start) < 1000) {
                        Slider(
                            value = tempValue,
                            onValueChange = { newValue ->
                                tempValue = newValue
                                textValue = newValue.toInt().toString()
                            },
                            valueRange = valueRange,
                            steps = steps,
                            colors = sliderColors
                        )
                    }
                }
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        if (!isError) {
                            onValueChange(tempValue)
                            showDialog = false
                        }
                    },
                    enabled = !isError
                ) {
                    Text("OK")
                }
            },
            dismissButton = {
                TextButton(onClick = { 
                    showDialog = false
                    tempValue = value
                    textValue = value.toInt().toString()
                }) {
                    Text("Cancel")
                }
            }
        )
    }
}

@PreviewLightDark
@Composable
private fun SliderPreferencePreview() {
    ComposePreview {
        var value by remember { mutableFloatStateOf(0.5f) }
        SliderPreference(
            value = value,
            onValueChange = { value = it },
            title = "Refresh Duration",
            leadingIcon = Icons.Default.Refresh,
            subtitle = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit."
        )
    }
}

@PreviewLightDark
@Composable
private fun SliderPreferenceDisabledPreview() {
    ComposePreview {
        var value by remember { mutableFloatStateOf(0.5f) }
        SliderPreference(
            value = value,
            onValueChange = { value = it },
            title = "Refresh Duration",
            leadingIcon = Icons.Default.Refresh,
            subtitle = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
            enabled = false
        )
    }
}
