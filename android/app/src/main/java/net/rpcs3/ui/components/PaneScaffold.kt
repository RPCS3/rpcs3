package net.rpcs3.ui.components

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsFocusedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.outlined.ArrowBack
import androidx.compose.material3.Icon
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.R
import net.rpcs3.ui.theme.Dims
import net.rpcs3.ui.theme.Rpcs

data class PaneTab(
    val label: String,
    val icon: ImageVector
)

@Composable
fun PaneScaffold(
    title: String,
    tabs: List<PaneTab>,
    selected: Int,
    onSelect: (Int) -> Unit,
    onBack: (() -> Unit)?,
    modifier: Modifier = Modifier,
    scrollableContent: Boolean = true,
    contentMaxWidth: Dp = Dims.ContentMaxWidth,
    content: @Composable () -> Unit
) {
    Row(
        modifier = modifier
            .fillMaxSize()
            .background(Rpcs.Background)
            .windowInsetsPadding(WindowInsets.systemBars)
    ) {
        Column(
            modifier = Modifier
                .width(Dims.SidebarWidth)
                .fillMaxHeight()
                .background(Rpcs.Surface)
                .padding(top = 12.dp, bottom = 12.dp)
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(start = 8.dp, end = 12.dp, bottom = 10.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                if (onBack != null) {
                    PaneIconButton(
                        icon = Icons.AutoMirrored.Outlined.ArrowBack,
                        contentDescription = stringResource(R.string.action_back),
                        onClick = onBack
                    )
                    Spacer(Modifier.width(2.dp))
                }
                Text(
                    text = title,
                    color = Rpcs.TextPrimary,
                    fontSize = 13.sp,
                    fontWeight = FontWeight.SemiBold,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
            }

            PaneDivider()
            Spacer(Modifier.height(8.dp))

            Column(
                modifier = Modifier
                    .weight(1f)
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(2.dp)
            ) {
                tabs.forEachIndexed { index, tab ->
                    PaneTabItem(
                        tab = tab,
                        isSelected = index == selected,
                        onClick = { onSelect(index) }
                    )
                }
            }
        }

        Box(
            Modifier
                .width(1.dp)
                .fillMaxHeight()
                .background(Rpcs.OutlineSoft)
        )

        Box(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight()
                .background(Rpcs.Background)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxHeight()
                    .widthIn(max = contentMaxWidth)
                    .padding(Dims.ScreenPadding)
                    .then(
                        if (scrollableContent) {
                            Modifier.verticalScroll(rememberScrollState())
                        } else {
                            Modifier
                        }
                    )
            ) {
                content()
            }
        }
    }
}

@Composable
fun PaneDivider(modifier: Modifier = Modifier) {
    Box(
        modifier
            .padding(horizontal = 12.dp)
            .fillMaxWidth()
            .height(1.dp)
            .background(Rpcs.OutlineSoft)
    )
}

@Composable
private fun PaneTabItem(
    tab: PaneTab,
    isSelected: Boolean,
    onClick: () -> Unit
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 8.dp)
            .background(
                if (isSelected) Rpcs.SelectionFill else Color.Transparent,
                RoundedCornerShape(Dims.RowCorner)
            )
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                when {
                    focused -> Rpcs.FocusBorder
                    isSelected -> Rpcs.SelectionBorder
                    else -> Color.Transparent
                },
                RoundedCornerShape(Dims.RowCorner)
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick)
            .padding(horizontal = 12.dp, vertical = 10.dp)
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(
                imageVector = tab.icon,
                contentDescription = null,
                tint = if (isSelected || focused) Rpcs.Accent else Rpcs.TextDim,
                modifier = Modifier.size(18.dp)
            )
            Spacer(Modifier.width(10.dp))
            Text(
                text = tab.label,
                color = if (isSelected || focused) Rpcs.TextPrimary else Rpcs.TextSecondary,
                fontSize = 13.sp,
                fontWeight = if (isSelected) FontWeight.SemiBold else FontWeight.Normal,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }
    }
}

@Composable
fun PaneIconButton(
    icon: ImageVector,
    contentDescription: String?,
    onClick: () -> Unit,
    tint: Color = Rpcs.TextSecondary
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Box(
        modifier = Modifier
            .size(36.dp)
            .background(Color.Transparent, RoundedCornerShape(Dims.RowCorner))
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else Color.Transparent,
                RoundedCornerShape(Dims.RowCorner)
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Icon(
            imageVector = icon,
            contentDescription = contentDescription,
            tint = if (focused) Rpcs.Accent else tint,
            modifier = Modifier.size(20.dp)
        )
    }
}

@Composable
fun PaneActionButton(
    label: String,
    icon: ImageVector,
    enabled: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Row(
        modifier = modifier
            .fillMaxWidth()
            .background(
                if (enabled) Rpcs.SelectionFill else Rpcs.SurfaceRaised,
                RoundedCornerShape(Dims.RowCorner)
            )
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                when {
                    focused -> Rpcs.FocusBorder
                    enabled -> Rpcs.SelectionBorder
                    else -> Rpcs.Outline
                },
                RoundedCornerShape(Dims.RowCorner)
            )
            .then(
                if (enabled) {
                    Modifier.clickable(
                        interactionSource = interaction,
                        indication = null,
                        onClick = onClick
                    )
                } else {
                    Modifier
                }
            )
            .padding(horizontal = 14.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = if (enabled) Rpcs.Accent else Rpcs.TextDim,
            modifier = Modifier.size(18.dp)
        )
        Spacer(Modifier.width(10.dp))
        Text(
            text = label,
            color = if (enabled) Rpcs.Accent else Rpcs.TextDim,
            fontSize = 13.sp,
            fontWeight = FontWeight.SemiBold
        )
    }
}

@Composable
fun PaneNavRow(
    title: String,
    description: String,
    icon: ImageVector,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Row(
        modifier = modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(Dims.RowCorner))
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else Rpcs.Outline,
                RoundedCornerShape(Dims.RowCorner)
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick)
            .padding(horizontal = 14.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = if (focused) Rpcs.Accent else Rpcs.TextSecondary,
            modifier = Modifier.size(20.dp)
        )
        Spacer(Modifier.width(12.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                color = Rpcs.TextPrimary,
                fontSize = 13.sp,
                fontWeight = FontWeight.Medium
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = description,
                color = Rpcs.TextSecondary,
                fontSize = 11.sp
            )
        }
    }
}

@Composable
fun PaneEntryRow(
    title: String,
    subtitle: String,
    actionIcon: ImageVector,
    actionDescription: String,
    onAction: () -> Unit,
    modifier: Modifier = Modifier
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .padding(vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                color = Rpcs.TextPrimary,
                fontSize = 13.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = subtitle,
                color = Rpcs.TextSecondary,
                fontSize = 11.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }
        Spacer(Modifier.width(10.dp))
        PaneIconButton(
            icon = actionIcon,
            contentDescription = actionDescription,
            tint = Rpcs.Danger,
            onClick = onAction
        )
    }
}

@Composable
fun PaneProgressOverlay(
    title: String,
    message: String?,
    value: Long,
    max: Long,
    onHide: () -> Unit,
    modifier: Modifier = Modifier
) {
    val fraction = if (max > 0L) {
        (value.toFloat() / max.toFloat()).coerceIn(0f, 1f)
    } else {
        0f
    }
    val animated by animateFloatAsState(targetValue = fraction, label = "paneProgress")

    Box(
        modifier = modifier
            .fillMaxSize()
            .background(Rpcs.Background.copy(alpha = 0.72f))
            .clickable(
                interactionSource = remember { MutableInteractionSource() },
                indication = null,
                onClick = {}
            )
            .windowInsetsPadding(WindowInsets.systemBars)
            .padding(Dims.ScreenPadding),
        contentAlignment = Alignment.Center
    ) {
        Column(
            modifier = Modifier
                .widthIn(max = Dims.OverlayWidth)
                .fillMaxWidth()
                .background(Rpcs.SurfaceRaised, RoundedCornerShape(Dims.CardCorner))
                .border(Dims.BorderWidth, Rpcs.Outline, RoundedCornerShape(Dims.CardCorner))
                .padding(14.dp)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = title,
                    color = Rpcs.TextPrimary,
                    fontSize = 13.sp,
                    fontWeight = FontWeight.SemiBold,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f)
                )
                Spacer(Modifier.width(10.dp))
                Text(
                    text = if (max > 0L) {
                        stringResource(R.string.percent_value, (fraction * 100f).toInt())
                    } else {
                        stringResource(R.string.pane_working)
                    },
                    color = Rpcs.Accent,
                    fontSize = 12.sp,
                    fontWeight = FontWeight.SemiBold
                )
            }

            if (!message.isNullOrEmpty()) {
                Spacer(Modifier.height(4.dp))
                Text(
                    text = message,
                    color = Rpcs.TextSecondary,
                    fontSize = 11.sp,
                    maxLines = 2,
                    overflow = TextOverflow.Ellipsis
                )
            }

            Spacer(Modifier.height(10.dp))

            if (max > 0L) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(Dims.ProgressBarHeight)
                        .background(Rpcs.SurfaceInset, RoundedCornerShape(Dims.ProgressBarHeight))
                ) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth(animated)
                            .fillMaxHeight()
                            .background(Rpcs.Accent, RoundedCornerShape(Dims.ProgressBarHeight))
                    )
                }
            } else {
                LinearProgressIndicator(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(Dims.ProgressBarHeight),
                    color = Rpcs.Accent,
                    trackColor = Rpcs.SurfaceInset
                )
            }

            Spacer(Modifier.height(12.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.End
            ) {
                PaneCompactButton(
                    label = stringResource(R.string.action_hide),
                    onClick = onHide
                )
            }
        }
    }
}

@Composable
fun PaneCompactButton(
    label: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    val interaction = remember { MutableInteractionSource() }
    val focused by interaction.collectIsFocusedAsState()

    Box(
        modifier = modifier
            .background(Rpcs.SelectionFill, RoundedCornerShape(Dims.RowCorner))
            .border(
                if (focused) Dims.FocusBorderWidth else Dims.BorderWidth,
                if (focused) Rpcs.FocusBorder else Rpcs.SelectionBorder,
                RoundedCornerShape(Dims.RowCorner)
            )
            .clickable(interactionSource = interaction, indication = null, onClick = onClick)
            .padding(horizontal = 18.dp, vertical = 9.dp)
    ) {
        Text(
            text = label,
            color = if (focused) Rpcs.AccentBright else Rpcs.Accent,
            fontSize = 13.sp,
            fontWeight = FontWeight.SemiBold
        )
    }
}

@Composable
fun PaneSectionTitle(text: String) {
    Text(
        text = text,
        color = Rpcs.TextSecondary,
        fontSize = 11.sp,
        fontWeight = FontWeight.SemiBold,
        modifier = Modifier.padding(bottom = 8.dp)
    )
}

@Composable
fun PaneCard(
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit
) {
    Column(
        modifier = modifier
            .fillMaxWidth()
            .background(Rpcs.SurfaceRaised, RoundedCornerShape(Dims.CardCorner))
            .border(Dims.BorderWidth, Rpcs.Outline, RoundedCornerShape(Dims.CardCorner))
            .padding(14.dp)
    ) {
        content()
    }
}

@Composable
fun PaneKeyValue(label: String, value: String, valueColor: Color = Rpcs.TextPrimary) {
    if (value.length > 28) {
        Column(modifier = Modifier.fillMaxWidth().padding(vertical = 7.dp)) {
            Text(text = label, color = Rpcs.TextSecondary, fontSize = 12.sp)
            Spacer(Modifier.height(3.dp))
            Text(
                text = value,
                color = valueColor,
                fontSize = 11.sp,
                fontWeight = FontWeight.Medium
            )
        }
        return
    }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 7.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            color = Rpcs.TextSecondary,
            fontSize = 12.sp,
            modifier = Modifier.weight(1f)
        )
        Spacer(Modifier.width(12.dp))
        Text(
            text = value,
            color = valueColor,
            fontSize = 12.sp,
            fontWeight = FontWeight.Medium,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
    }
}
