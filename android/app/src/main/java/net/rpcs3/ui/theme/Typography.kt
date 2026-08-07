package net.rpcs3.ui.theme

import androidx.compose.material3.Typography
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import net.rpcs3.R

val RpcsFontFamily =
    FontFamily(
        Font(R.font.inter_medium, FontWeight.Normal),
        Font(R.font.inter_medium, FontWeight.Medium),
        Font(R.font.inter_medium, FontWeight.SemiBold),
        Font(R.font.inter_medium, FontWeight.Bold),
    )

private val Base = Typography()

val RpcsTypography =
    Typography(
        displayLarge = Base.displayLarge.copy(fontFamily = RpcsFontFamily),
        displayMedium = Base.displayMedium.copy(fontFamily = RpcsFontFamily),
        displaySmall = Base.displaySmall.copy(fontFamily = RpcsFontFamily),
        headlineLarge = Base.headlineLarge.copy(fontFamily = RpcsFontFamily),
        headlineMedium = Base.headlineMedium.copy(fontFamily = RpcsFontFamily),
        headlineSmall = Base.headlineSmall.copy(fontFamily = RpcsFontFamily),
        titleLarge = Base.titleLarge.copy(fontFamily = RpcsFontFamily),
        titleMedium = Base.titleMedium.copy(fontFamily = RpcsFontFamily),
        titleSmall = Base.titleSmall.copy(fontFamily = RpcsFontFamily),
        bodyLarge = Base.bodyLarge.copy(fontFamily = RpcsFontFamily),
        bodyMedium = Base.bodyMedium.copy(fontFamily = RpcsFontFamily),
        bodySmall = Base.bodySmall.copy(fontFamily = RpcsFontFamily),
        labelLarge = Base.labelLarge.copy(fontFamily = RpcsFontFamily),
        labelMedium = Base.labelMedium.copy(fontFamily = RpcsFontFamily),
        labelSmall = Base.labelSmall.copy(fontFamily = RpcsFontFamily),
    )
