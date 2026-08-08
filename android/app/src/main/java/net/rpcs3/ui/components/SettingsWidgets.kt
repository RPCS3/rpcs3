package net.rpcs3.ui.components

import net.rpcs3.ui.theme.Rpcs

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CheckboxDefaults
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
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
fun SettingDropdown(
    label: String,
    entries: List<String>,
    selectedIndex: Int,
    onSelected: (Int) -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true
) {
    var expanded by remember { mutableStateOf(false) }
    val alpha = if (enabled) 1f else 0.4f

    Column(modifier = modifier.fillMaxWidth()) {
        FieldLabel(label)
        Box {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(SettingsStyle.InputSurface, RoundedCornerShape(Dimens.FieldCorner))
                    .border(1.dp, SettingsStyle.InputBorder, RoundedCornerShape(Dimens.FieldCorner))
                    .clickable(enabled = enabled) { expanded = true }
                    .padding(
                        horizontal = Dimens.FieldHorizontalPadding,
                        vertical = Dimens.FieldVerticalPadding
                    ),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = entries.getOrNull(selectedIndex) ?: "",
                    modifier = Modifier.weight(1f),
                    color = SettingsStyle.TextPrimary.copy(alpha = alpha),
                    fontSize = Dimens.ValueSize,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
                Icon(
                    imageVector = Icons.Default.KeyboardArrowDown,
                    contentDescription = null,
                    tint = SettingsStyle.TextDim.copy(alpha = alpha),
                    modifier = Modifier.size(Dimens.IconSize)
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
        Slider(
            value = value,
            onValueChange = onValueChange,
            onValueChangeFinished = onValueChangeFinished,
            valueRange = valueRange,
            steps = steps,
            enabled = enabled,
            modifier = Modifier
                .fillMaxWidth()
                .height(Dimens.SliderHeight),
            colors = SliderDefaults.colors(
                thumbColor = SettingsStyle.AccentBlue,
                activeTrackColor = SettingsStyle.AccentBlue,
                inactiveTrackColor = SettingsStyle.SliderInactive,
                activeTickColor = Color.Transparent,
                inactiveTickColor = Color.Transparent
            )
        )
    }
}

@Composable
fun SettingChip(
    label: String,
    selected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true
) {
    val background by animateColorAsState(
        if (selected) SettingsStyle.AccentBlue.copy(alpha = 0.15f) else SettingsStyle.ChipSurface,
        label = "chipBackground"
    )
    val border by animateColorAsState(
        if (selected) SettingsStyle.AccentBlue.copy(alpha = 0.4f) else SettingsStyle.ChipBorder,
        label = "chipBorder"
    )

    Box(
        modifier = modifier
            .background(background, RoundedCornerShape(Dimens.ChipCorner))
            .border(1.dp, border, RoundedCornerShape(Dimens.ChipCorner))
            .clickable(enabled = enabled) { onClick() }
            .padding(horizontal = 12.dp, vertical = 6.dp)
    ) {
        Text(
            text = label,
            color = if (selected) SettingsStyle.AccentBlue else SettingsStyle.TextDim,
            fontSize = Dimens.ValueSize,
            fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Normal
        )
    }
}

@Composable
fun GhostButton(
    label: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    icon: ImageVector? = null,
    enabled: Boolean = true
) {
    Row(
        modifier = modifier
            .background(
                SettingsStyle.AccentBlue.copy(alpha = 0.08f),
                RoundedCornerShape(Dimens.GhostCorner)
            )
            .border(
                1.dp,
                SettingsStyle.AccentBlue.copy(alpha = 0.2f),
                RoundedCornerShape(Dimens.GhostCorner)
            )
            .clickable(enabled = enabled) { onClick() }
            .padding(horizontal = 12.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        if (icon != null) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = SettingsStyle.AccentBlue,
                modifier = Modifier.size(Dimens.IconSize)
            )
            Spacer(Modifier.width(8.dp))
        }
        Text(
            text = label,
            color = SettingsStyle.AccentBlue,
            fontSize = Dimens.ValueSize,
            fontWeight = FontWeight.Medium
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
