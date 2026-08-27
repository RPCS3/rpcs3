package net.rpcs3.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeight
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CheckboxDefaults
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchColors
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.SettingsStyle

@Composable
fun rpcsSwitchColors(): SwitchColors =
    SwitchDefaults.colors(
        checkedThumbColor = Color.White,
        checkedTrackColor = SettingsStyle.AccentBlue,
        checkedBorderColor = Color.Transparent,
        uncheckedThumbColor = SettingsStyle.TextSecondary,
        uncheckedTrackColor = Rpcs.SurfaceRaised,
        uncheckedBorderColor = Rpcs.Outline,
        disabledCheckedThumbColor = Rpcs.TextDim,
        disabledCheckedTrackColor = Rpcs.SurfaceInset,
        disabledCheckedBorderColor = Rpcs.Outline,
        disabledUncheckedThumbColor = Rpcs.TextDim,
        disabledUncheckedTrackColor = Rpcs.SurfaceInset,
        disabledUncheckedBorderColor = Rpcs.OutlineSoft,
    )

@Composable
fun SettingGroup(
    modifier: Modifier = Modifier,
    verticalPadding: Dp = Dimens.GroupPadding,
    content: @Composable () -> Unit
) {
    Column(
        modifier = modifier
            .fillMaxWidth()
            .background(SettingsStyle.CardSurface, RoundedCornerShape(Dimens.GroupCorner))
            .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(Dimens.GroupCorner))
            .padding(horizontal = Dimens.GroupPadding, vertical = verticalPadding),
        verticalArrangement = Arrangement.spacedBy(Dimens.ItemGap)
    ) { content() }
}

@Composable
fun SectionLabel(text: String, modifier: Modifier = Modifier) {
    Text(
        text = text,
        modifier = modifier,
        color = SettingsStyle.TextSecondary,
        fontSize = Dimens.SectionLabelSize,
        fontWeight = FontWeight.SemiBold,
        letterSpacing = 0.8.sp
    )
}

@Composable
fun ThinDivider(modifier: Modifier = Modifier) {
    Box(
        modifier
            .fillMaxWidth()
            .height(1.dp)
            .background(SettingsStyle.Divider)
    )
}

@Composable
private fun FieldLabel(text: String, trailing: (@Composable () -> Unit)? = null) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(Dimens.LabelRowHeight),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = text,
            modifier = Modifier.weight(1f),
            color = SettingsStyle.TextSecondary,
            fontSize = Dimens.LabelSize,
            fontWeight = FontWeight.Medium,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
        trailing?.invoke()
    }
}

@Composable
fun SettingSwitch(
    label: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
    subtitle: String? = null,
    enabled: Boolean = true,
    onLongClick: (() -> Unit)? = null
) {
    val interaction = remember { MutableInteractionSource() }
    Row(
        modifier = modifier
            .fillMaxWidth()
            .background(Color.Transparent, RoundedCornerShape(Dimens.FieldCorner))
            .clickable(
                interactionSource = interaction,
                indication = null,
                enabled = enabled
            ) { onCheckedChange(!checked) }
            .padding(vertical = Dimens.TightGap),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier
            .weight(1f)
            .padding(end = 8.dp)) {
            Text(
                text = label,
                color = if (enabled) SettingsStyle.TextPrimary else SettingsStyle.TextDim,
                fontSize = Dimens.ValueSize,
                maxLines = 3,
                overflow = TextOverflow.Ellipsis
            )
            if (subtitle != null) {
                Text(
                    text = subtitle,
                    color = SettingsStyle.TextSecondary,
                    fontSize = Dimens.LabelSize,
                    lineHeight = 14.sp,
                    maxLines = 4,
                    overflow = TextOverflow.Ellipsis
                )
            }
        }
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange,
            enabled = enabled,
            colors = rpcsSwitchColors()
        )
    }
}

@Composable
fun SettingCheckbox(
    label: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .clickable(enabled = enabled) { onCheckedChange(!checked) }
            .padding(vertical = Dimens.TightGap),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Checkbox(
            checked = checked,
            onCheckedChange = onCheckedChange,
            enabled = enabled,
            modifier = Modifier.size(20.dp),
            colors = CheckboxDefaults.colors(
                checkedColor = SettingsStyle.AccentBlue,
                uncheckedColor = SettingsStyle.CheckBorder,
                checkmarkColor = Color.White
            )
        )
        Spacer(Modifier.width(10.dp))
        Text(
            text = label,
            color = if (enabled) SettingsStyle.TextPrimary else SettingsStyle.TextDim,
            fontSize = Dimens.ValueSize
        )
    }
}

@Composable
fun SettingInlineDropdown(
    label: String,
    entries: List<String>,
    selectedIndex: Int,
    onSelected: (Int) -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    highlighted: Boolean = false,
    fieldWidth: Dp = Dimens.InlineFieldWidth
) {
    var expanded by remember { mutableStateOf(false) }
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()
    val alpha = if (enabled) 1f else 0.4f

    Row(
        modifier = modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            modifier = Modifier
                .weight(1f)
                .padding(end = 10.dp),
            color = if (highlighted) SettingsStyle.AccentBlue else SettingsStyle.TextPrimary,
            fontSize = Dimens.ValueSize,
            fontWeight = if (highlighted) FontWeight.Medium else FontWeight.Normal,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )

        Box {
            Row(
                modifier = Modifier
                    .width(fieldWidth)
                    .background(SettingsStyle.InputSurface, RoundedCornerShape(Dimens.FieldCorner))
                    .border(
                        if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                        when {
                            focused -> Rpcs.FocusBorder
                            highlighted -> SettingsStyle.AccentBlue.copy(alpha = 0.5f)
                            else -> SettingsStyle.InputBorder
                        },
                        RoundedCornerShape(Dimens.FieldCorner)
                    )
                    .clickable(
                        interactionSource = interaction,
                        indication = null,
                        enabled = enabled
                    ) { expanded = true }
                    .padding(
                        horizontal = Dimens.FieldHorizontalPadding,
                        vertical = Dimens.TightGap
                    ),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = entries.getOrNull(selectedIndex) ?: "",
                    modifier = Modifier.weight(1f),
                    color = if (highlighted) {
                        SettingsStyle.AccentBlue.copy(alpha = alpha)
                    } else {
                        SettingsStyle.TextSecondary.copy(alpha = alpha)
                    },
                    fontSize = Dimens.ValueSize,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
                Icon(
                    imageVector = Icons.Default.KeyboardArrowDown,
                    contentDescription = null,
                    tint = SettingsStyle.TextDim.copy(alpha = alpha),
                    modifier = Modifier.size(Dimens.ControlIconSize)
                )
            }

            DropdownMenu(
                expanded = expanded,
                onDismissRequest = { expanded = false },
                shape = RoundedCornerShape(Dimens.FieldCorner),
                containerColor = SettingsStyle.CardSurface
            ) {
                entries.forEachIndexed { index, entry ->
                    DropdownMenuItem(
                        modifier = Modifier.background(
                            if (index == selectedIndex) {
                                SettingsStyle.AccentBlue.copy(alpha = 0.06f)
                            } else {
                                Color.Transparent
                            }
                        ),
                        text = {
                            Text(
                                text = entry,
                                color = if (index == selectedIndex) {
                                    SettingsStyle.AccentBlue
                                } else {
                                    SettingsStyle.TextPrimary
                                },
                                fontSize = Dimens.ValueSize
                            )
                        },
                        onClick = {
                            expanded = false
                            if (index != selectedIndex) onSelected(index)
                        }
                    )
                }
            }
        }
    }
}

@Composable
fun SettingTextField(
    label: String,
    value: String,
    onValueChange: (String) -> Unit,
    modifier: Modifier = Modifier,
    placeholder: String = "",
    enabled: Boolean = true,
    keyboardOptions: KeyboardOptions = KeyboardOptions.Default
) {
    Column(modifier = modifier.fillMaxWidth()) {
        FieldLabel(label)
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .background(SettingsStyle.InputSurface, RoundedCornerShape(Dimens.FieldCorner))
                .border(1.dp, SettingsStyle.InputBorder, RoundedCornerShape(Dimens.FieldCorner))
                .padding(
                    horizontal = Dimens.FieldHorizontalPadding,
                    vertical = Dimens.FieldVerticalPadding
                )
        ) {
            if (value.isEmpty() && placeholder.isNotEmpty()) {
                Text(
                    text = placeholder,
                    color = SettingsStyle.TextDim,
                    fontSize = Dimens.ValueSize
                )
            }
            BasicTextField(
                value = value,
                onValueChange = onValueChange,
                enabled = enabled,
                singleLine = true,
                keyboardOptions = keyboardOptions,
                cursorBrush = SolidColor(SettingsStyle.AccentBlue),
                textStyle = MaterialTheme.typography.bodyMedium.copy(
                    color = SettingsStyle.TextPrimary,
                    fontSize = Dimens.ValueSize
                ),
                modifier = Modifier.fillMaxWidth()
            )
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingSlider(
    label: String,
    value: Float,
    valueRange: ClosedFloatingPointRange<Float>,
    onValueChange: (Float) -> Unit,
    modifier: Modifier = Modifier,
    valueText: String = value.toInt().toString(),
    steps: Int = 0,
    enabled: Boolean = true,
    onValueChangeFinished: (() -> Unit)? = null
) {
    Column(modifier = modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = label,
                modifier = Modifier.weight(1f),
                color = SettingsStyle.TextSecondary,
                fontSize = Dimens.LabelSize,
                fontWeight = FontWeight.Medium
            )
            Text(
                text = valueText,
                modifier = Modifier
                    .background(
                        SettingsStyle.AccentBlue.copy(alpha = 0.1f),
                        RoundedCornerShape(Dimens.BadgeCorner)
                    )
                    .padding(horizontal = 7.dp, vertical = 2.dp),
                color = SettingsStyle.AccentBlue,
                fontSize = Dimens.LabelSize,
                fontWeight = FontWeight.SemiBold
            )
        }
        Spacer(Modifier.height(Dimens.TightGap))
        val accent = if (enabled) SettingsStyle.AccentBlue else SettingsStyle.TextDim
        Slider(
            value = value,
            onValueChange = onValueChange,
            onValueChangeFinished = onValueChangeFinished,
            valueRange = valueRange,
            steps = steps,
            enabled = enabled,
            modifier = Modifier
                .fillMaxWidth()
                .requiredHeight(Dimens.SliderHeight),
            thumb = {
                Box(
                    modifier = Modifier
                        .size(Dimens.SliderThumbSize)
                        .clip(CircleShape)
                        .background(accent)
                )
            },
            track = { state ->
                val span = state.valueRange.endInclusive - state.valueRange.start
                val fraction = if (span <= 0f) {
                    0f
                } else {
                    ((state.value - state.valueRange.start) / span).coerceIn(0f, 1f)
                }
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(Dimens.SliderTrackHeight)
                        .clip(RoundedCornerShape(Dimens.SliderTrackHeight / 2))
                        .background(SettingsStyle.SliderInactive)
                ) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth(fraction)
                            .fillMaxHeight()
                            .clip(RoundedCornerShape(Dimens.SliderTrackHeight / 2))
                            .background(accent)
                    )
                }
            }
        )
    }
}

@Composable
fun SettingChip(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    detail: String? = null,
    enabled: Boolean = true
) {
    val alpha = if (enabled) 1f else 0.4f
    val background by animateColorAsState(
        if (selected) SettingsStyle.AccentBlue.copy(alpha = 0.15f) else SettingsStyle.ChipSurface,
        label = "chipBackground"
    )
    val border by animateColorAsState(
        if (selected) SettingsStyle.AccentBlue.copy(alpha = 0.4f) else SettingsStyle.ChipBorder,
        label = "chipBorder"
    )

    Column(
        modifier = modifier
            .background(background, RoundedCornerShape(Dimens.ChipCorner))
            .border(1.dp, border, RoundedCornerShape(Dimens.ChipCorner))
            .clickable(enabled = enabled) { onClick() }
            .padding(horizontal = 10.dp, vertical = 7.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = label,
            color = (if (selected) SettingsStyle.AccentBlue else SettingsStyle.TextPrimary)
                .copy(alpha = alpha),
            fontSize = Dimens.ValueSize,
            fontWeight = FontWeight.SemiBold,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
        if (detail != null) {
            Text(
                text = detail,
                color = SettingsStyle.TextDim.copy(alpha = alpha),
                fontSize = Dimens.CaptionSize,
                lineHeight = 12.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }
    }
}

@Composable
fun SettingsSection(
    title: String,
    modifier: Modifier = Modifier,
    trailing: (@Composable () -> Unit)? = null,
    content: @Composable ColumnScope.() -> Unit
) {
    Column(
        modifier = modifier.fillMaxWidth(),
        verticalArrangement = Arrangement.spacedBy(Dimens.TightGap)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(start = 2.dp, bottom = 2.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            SectionLabel(text = title.uppercase(), modifier = Modifier.weight(1f))
            trailing?.invoke()
        }
        content()
    }
}

@Composable
fun SettingsHint(text: String, modifier: Modifier = Modifier) {
    Text(
        text = text,
        modifier = modifier.padding(horizontal = 2.dp),
        color = SettingsStyle.TextDim,
        fontSize = Dimens.LabelSize,
        lineHeight = 15.sp
    )
}

@Composable
fun CollapsibleSection(
    title: String,
    modifier: Modifier = Modifier,
    initiallyExpanded: Boolean = false,
    content: @Composable ColumnScope.() -> Unit
) {
    var expanded by remember(title) { mutableStateOf(initiallyExpanded) }
    val rotation by animateFloatAsState(
        targetValue = if (expanded) 180f else 0f,
        label = "collapsibleArrow"
    )

    Column(
        modifier = modifier
            .fillMaxWidth()
            .background(SettingsStyle.CardSurface, RoundedCornerShape(Dimens.GroupCorner))
            .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(Dimens.GroupCorner))
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .clickable { expanded = !expanded }
                .padding(horizontal = Dimens.GroupPadding, vertical = 10.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = title,
                modifier = Modifier.weight(1f),
                color = if (expanded) SettingsStyle.AccentBlue else SettingsStyle.TextPrimary,
                fontSize = Dimens.ValueSize,
                fontWeight = FontWeight.SemiBold,
                maxLines = 2,
                overflow = TextOverflow.Ellipsis
            )
            Icon(
                imageVector = Icons.Default.KeyboardArrowDown,
                contentDescription = null,
                tint = if (expanded) SettingsStyle.AccentBlue else SettingsStyle.TextDim,
                modifier = Modifier
                    .size(Dimens.IconSize)
                    .rotate(rotation)
            )
        }

        AnimatedVisibility(visible = expanded) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(
                        start = Dimens.GroupPadding,
                        end = Dimens.GroupPadding,
                        bottom = Dimens.GroupPadding
                    ),
                verticalArrangement = Arrangement.spacedBy(Dimens.ItemGap)
            ) {
                ThinDivider(Modifier.padding(bottom = Dimens.TightGap))
                content()
            }
        }
    }
}

@Composable
fun GhostButton(
    label: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    icon: ImageVector? = null,
    enabled: Boolean = true,
    accent: Boolean = false,
    tint: Color = Rpcs.Accent,
    horizontalPadding: Dp = 12.dp
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Row(
        modifier = modifier
            .background(
                tint.copy(alpha = if (accent) 0.18f else 0.10f),
                RoundedCornerShape(Dimens.GhostCorner)
            )
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                when {
                    focused -> Rpcs.FocusBorder
                    accent -> tint.copy(alpha = 0.55f)
                    else -> tint.copy(alpha = 0.35f)
                },
                RoundedCornerShape(Dimens.GhostCorner)
            )
            .clickable(interactionSource = interaction, indication = null, enabled = enabled) { onClick() }
            .padding(horizontal = horizontalPadding, vertical = 9.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Center
    ) {
        if (icon != null) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = tint,
                modifier = Modifier.size(Dimens.IconSize)
            )
            Spacer(Modifier.width(8.dp))
        }
        Text(
            text = label,
            color = tint,
            fontSize = Dimens.ValueSize,
            fontWeight = FontWeight.SemiBold,
            maxLines = 1,
            softWrap = false,
            overflow = TextOverflow.Ellipsis
        )
    }
}

@Composable
fun InfoRow(
    label: String,
    value: String,
    modifier: Modifier = Modifier,
    singleLineValue: Boolean = false
) {
    val clamp = singleLineValue || value.contains('/') || value.contains('\\') || value.contains(".so")
    Column(modifier = modifier.padding(vertical = Dimens.TightGap)) {
        Text(
            text = label,
            color = SettingsStyle.TextSecondary,
            fontSize = Dimens.LabelSize,
            fontWeight = FontWeight.Medium,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
        Spacer(Modifier.height(2.dp))
        Text(
            text = value,
            color = SettingsStyle.TextPrimary,
            fontSize = Dimens.ValueSize,
            fontWeight = FontWeight.Medium,
            maxLines = if (clamp) 1 else Int.MAX_VALUE,
            softWrap = !clamp,
            overflow = TextOverflow.Ellipsis
        )
    }
}
