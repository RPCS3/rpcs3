package net.rpcs3.ui.framegen

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
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
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.R
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.framegen.FrameGen
import net.rpcs3.framegen.FrameGenImportResult
import net.rpcs3.framegen.FrameGenMode
import net.rpcs3.framegen.FrameGenPrefs
import net.rpcs3.ui.theme.Rpcs

const val FrameGenCategory = "Frame Gen"

private val TargetRates = listOf(60, 90, 120, 144)
private val Multipliers = listOf(2, 3, 4)

@Composable
fun FrameGenPanel(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val prefs = remember { FrameGenPrefs.of(context) }
    val scope = rememberCoroutineScope()
    val state by FrameGen.state

    var enabled by remember { mutableStateOf(FrameGenPrefs.isEnabled(prefs)) }
    var mode by remember { mutableStateOf(FrameGenPrefs.mode(prefs)) }
    var multiplier by remember { mutableIntStateOf(FrameGenPrefs.multiplier(prefs)) }
    var targetRate by remember { mutableIntStateOf(FrameGenPrefs.targetRate(prefs)) }
    var flowScale by remember { mutableIntStateOf(FrameGenPrefs.flowScale(prefs)) }
    var importing by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) { FrameGen.refresh(context) }

    val picker = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument(),
        onResult = { uri: Uri? ->
            if (uri != null) {
                importing = true
                scope.launch {
                    val result = withContext(Dispatchers.IO) { FrameGen.import(context, uri) }
                    importing = false

                    AlertDialogQueue.showDialog(
                        context.getString(R.string.framegen_import_title),
                        context.getString(result.messageRes)
                    )

                    if (result == FrameGenImportResult.Ok) {
                        enabled = FrameGenPrefs.isEnabled(prefs)
                    }
                }
            }
        }
    )

    Column(modifier = modifier.fillMaxWidth()) {
        SourceCard(
            imported = state.imported,
            busy = importing,
            sourceName = state.sourceName,
            variant = state.variant,
            modules = state.modules,
            onPick = { picker.launch(arrayOf("*/*")) },
            onForget = {
                scope.launch {
                    withContext(Dispatchers.IO) { FrameGen.forget(context) }
                    enabled = false
                    FrameGenPrefs.setEnabled(prefs, false)
                }
            }
        )

        Spacer(Modifier.height(14.dp))

        FrameGenToggleRow(
            title = stringResource(R.string.framegen_enable),
            subtitle = stringResource(
                if (state.imported) R.string.framegen_enable_hint else R.string.framegen_enable_blocked
            ),
            checked = enabled && state.imported,
            enabled = state.imported,
            onCheckedChange = { wanted ->
                enabled = wanted
                FrameGenPrefs.setEnabled(prefs, wanted)
                FrameGen.push(context)
            }
        )

        Spacer(Modifier.height(14.dp))

        SectionCaption(stringResource(R.string.framegen_section_pacing))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            FrameGenMode.entries.forEach { candidate ->
                FrameGenChip(
                    label = stringResource(
                        if (candidate == FrameGenMode.Fixed) {
                            R.string.framegen_mode_fixed
                        } else {
                            R.string.framegen_mode_adaptive
                        }
                    ),
                    detail = stringResource(
                        if (candidate == FrameGenMode.Fixed) {
                            R.string.framegen_mode_fixed_detail
                        } else {
                            R.string.framegen_mode_adaptive_detail
                        }
                    ),
                    selected = candidate == mode,
                    modifier = Modifier.weight(1f),
                    onClick = {
                        mode = candidate
                        FrameGenPrefs.setMode(prefs, candidate)
                        FrameGen.push(context)
                    }
                )
            }
        }

        Spacer(Modifier.height(12.dp))

        if (mode == FrameGenMode.Fixed) {
            SectionCaption(stringResource(R.string.framegen_section_multiplier))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                Multipliers.forEach { candidate ->
                    FrameGenChip(
                        label = stringResource(R.string.framegen_multiplier_value, candidate),
                        detail = stringResource(R.string.framegen_multiplier_detail, candidate - 1),
                        selected = candidate == multiplier,
                        modifier = Modifier.weight(1f),
                        onClick = {
                            multiplier = candidate
                            FrameGenPrefs.setMultiplier(prefs, candidate)
                            FrameGen.push(context)
                        }
                    )
                }
            }
        } else {
            SectionCaption(stringResource(R.string.framegen_section_target))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                TargetRates.forEach { candidate ->
                    FrameGenChip(
                        label = stringResource(R.string.framegen_target_value, candidate),
                        detail = stringResource(R.string.framegen_target_detail),
                        selected = candidate == targetRate,
                        modifier = Modifier.weight(1f),
                        onClick = {
                            targetRate = candidate
                            FrameGenPrefs.setTargetRate(prefs, candidate)
                            FrameGen.push(context)
                        }
                    )
                }
            }
        }

        Spacer(Modifier.height(14.dp))

        SectionCaption(stringResource(R.string.framegen_section_flow))

        Text(
            text = stringResource(R.string.framegen_flow_value, flowScale),
            color = Rpcs.TextSecondary,
            fontSize = 11.sp
        )

        Slider(
            value = flowScale.toFloat(),
            onValueChange = { flowScale = it.toInt() },
            onValueChangeFinished = {
                FrameGenPrefs.setFlowScale(prefs, flowScale)
                FrameGen.push(context)
            },
            valueRange = 25f..100f,
            steps = 14,
            colors = SliderDefaults.colors(
                thumbColor = Rpcs.Accent,
                activeTrackColor = Rpcs.Accent
            )
        )

        Text(
            text = stringResource(R.string.framegen_flow_hint),
            color = Rpcs.TextDim,
            fontSize = 10.sp,
            lineHeight = 14.sp
        )

        Spacer(Modifier.height(14.dp))

        Text(
            text = when {
                state.unsupported -> stringResource(R.string.framegen_runtime_unsupported)
                state.ready -> stringResource(R.string.framegen_runtime_active, state.width, state.height)
                else -> stringResource(R.string.framegen_runtime_idle)
            },
            color = if (state.unsupported) Rpcs.TextDim else Rpcs.TextSecondary,
            fontSize = 11.sp,
            lineHeight = 15.sp
        )

        Spacer(Modifier.height(8.dp))

        Text(
            text = stringResource(R.string.framegen_latency_note),
            color = Rpcs.TextDim,
            fontSize = 10.sp,
            lineHeight = 14.sp
        )
    }
}

@Composable
private fun SourceCard(
    imported: Boolean,
    busy: Boolean,
    sourceName: String,
    variant: String,
    modules: Int,
    onPick: () -> Unit,
    onForget: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(10.dp))
            .border(1.dp, Rpcs.OutlineSoft, RoundedCornerShape(10.dp))
            .padding(12.dp)
    ) {
        Text(
            text = stringResource(R.string.framegen_source_title),
            color = Rpcs.TextPrimary,
            fontSize = 12.sp,
            fontWeight = FontWeight.SemiBold
        )

        Spacer(Modifier.height(4.dp))

        Text(
            text = when {
                busy -> stringResource(R.string.framegen_source_importing)
                imported && sourceName.isNotEmpty() ->
                    stringResource(R.string.framegen_source_loaded_named, sourceName, modules, variant)
                imported -> stringResource(R.string.framegen_source_loaded, modules, variant)
                else -> stringResource(R.string.framegen_source_missing)
            },
            color = if (imported) Rpcs.TextSecondary else Rpcs.TextDim,
            fontSize = 11.sp,
            lineHeight = 15.sp
        )

        Spacer(Modifier.height(10.dp))

        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            FrameGenButton(
                label = stringResource(
                    if (imported) R.string.framegen_source_replace else R.string.framegen_source_select
                ),
                enabled = !busy,
                modifier = Modifier.weight(1f),
                onClick = onPick
            )

            if (imported) {
                FrameGenButton(
                    label = stringResource(R.string.framegen_source_remove),
                    enabled = !busy,
                    modifier = Modifier.weight(1f),
                    onClick = onForget
                )
            }
        }

        Spacer(Modifier.height(8.dp))

        Text(
            text = stringResource(R.string.framegen_source_hint),
            color = Rpcs.TextDim,
            fontSize = 10.sp,
            lineHeight = 14.sp
        )
    }
}

@Composable
private fun SectionCaption(text: String) {
    Text(
        text = text,
        color = Rpcs.TextDim,
        fontSize = 10.sp,
        fontWeight = FontWeight.SemiBold
    )
    Spacer(Modifier.height(6.dp))
}

@Composable
private fun FrameGenButton(
    label: String,
    enabled: Boolean,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    Row(
        modifier = modifier
            .background(Rpcs.SelectionFill, RoundedCornerShape(10.dp))
            .border(1.dp, Rpcs.SelectionBorder, RoundedCornerShape(10.dp))
            .clickable(enabled = enabled, onClick = onClick)
            .padding(vertical = 9.dp, horizontal = 10.dp),
        horizontalArrangement = Arrangement.Center
    ) {
        Text(
            text = label,
            color = if (enabled) Rpcs.Accent else Rpcs.TextDim,
            fontSize = 11.sp,
            fontWeight = FontWeight.SemiBold
        )
    }
}

@Composable
private fun FrameGenToggleRow(
    title: String,
    subtitle: String?,
    checked: Boolean,
    enabled: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(10.dp))
            .border(1.dp, Rpcs.OutlineSoft, RoundedCornerShape(10.dp))
            .clickable(enabled = enabled) { onCheckedChange(!checked) }
            .padding(horizontal = 12.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                color = if (enabled) Rpcs.TextPrimary else Rpcs.TextDim,
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
            enabled = enabled,
            colors = SwitchDefaults.colors(
                checkedThumbColor = Rpcs.TextPrimary,
                checkedTrackColor = Rpcs.Accent
            )
        )
    }
}

@Composable
private fun FrameGenChip(
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
