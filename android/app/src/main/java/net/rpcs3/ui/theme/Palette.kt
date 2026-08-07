package net.rpcs3.ui.theme

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

val RpcsBackground = Color(0xFF18181D)
val RpcsSurface = Color(0xFF1C1C2A)
val RpcsSurfaceAlt = Color(0xFF21212A)
val RpcsPanel = Color(0xFF161622)
val RpcsOutline = Color(0xFF2A2A3A)
val RpcsAccent = Color(0xFF1A9FFF)
val RpcsTextPrimary = Color(0xFFF0F4FF)
val RpcsTextSecondary = Color(0xFF7A8FA8)
val RpcsDanger = Color(0xFFFF7A88)

object DrawerStyle {
    const val SheetAlpha = 0.86f
    const val SurfaceAlpha = 0.72f
    const val PressedAlpha = 0.88f

    val Accent = Color(0xFF2196F3)
    val ActiveAccent = Color(0xFF29B6F6)
    val FocusFill = Color(0xFF0E2438)
    val TextPrimary = RpcsTextPrimary.copy(alpha = 0.88f)
    val TextSecondary = RpcsTextSecondary.copy(alpha = 0.82f)
    val Outline = RpcsOutline
    val Background = RpcsBackground.copy(alpha = SheetAlpha)
    val PaneSurface = RpcsBackground.copy(alpha = SheetAlpha)
    val PaneSurfacePressed = Color(0xFF232B3A).copy(alpha = PressedAlpha)
    val TopRailSurface = RpcsSurface.copy(alpha = SheetAlpha)
    val TileResting = Color(0xFF20283A).copy(alpha = SurfaceAlpha)
    val TileExitResting = Color(0xFF3A2125).copy(alpha = SurfaceAlpha)
    val TileExitPressed = Color(0xFF4A2A30).copy(alpha = PressedAlpha)
    val PaneInnerResting = RpcsPanel.copy(alpha = SurfaceAlpha)
    val PaneInnerPressed = Color(0xFF242B3A).copy(alpha = PressedAlpha)
    val RestingCardBorder = RpcsOutline.copy(alpha = 0.72f)
    val DisabledCardBorder = Color(0xFF202033).copy(alpha = 0.58f)
    val ActiveCardBorder = ActiveAccent
    val Divider = RpcsOutline.copy(alpha = 0.6f)

    val Width = 320.dp
    val StartPadding = 6.dp
    val VerticalPadding = 6.dp
    val RailTileSize = 64.dp
    val CornerRadius = 14.dp
    const val PaneScaleMin = 0.78f
    const val PaneScaleReferenceHeightDp = 520f
}

object SettingsStyle {
    val BgDeep = Color(0xFF11111C)
    val SidebarBg = Color(0xFF11111C)
    val ContentBg = Color(0xFF11111C)
    val CardSurface = RpcsSurface
    val CardBorder = RpcsOutline
    val InputSurface = Color(0xFF171722)
    val InputBorder = RpcsOutline
    val AccentBlue = RpcsAccent
    val TextPrimary = RpcsTextPrimary
    val TextSecondary = RpcsTextSecondary
    val TextDim = Color(0xFF6E7681)
    val Divider = RpcsOutline
    val CheckBorder = RpcsOutline
    val SliderInactive = RpcsSurfaceAlt
    val ChipSurface = Color(0xFF171722)
    val ChipBorder = RpcsOutline
    val DangerRed = Color(0xFFFF6B6B)
    val WarningAmber = Color(0xFFFFB74D)
    val NavHighlight = Color(0xFF4FC3F7)

    val CardCorner = 14.dp
    val InputCorner = 10.dp
    val SidebarWidth = 208.dp
    val ContentPadding = 16.dp
    val RowSpacing = 10.dp
}
