package net.rpcs3.ui.components

import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.SweepGradient
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.draw.drawWithCache
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

fun Modifier.chasingBorder(
    isFocused: Boolean = true,
    cornerRadius: Dp = 12.dp,
    borderWidth: Dp = 3.dp,
    animationDurationMs: Int = 5000
): Modifier = composed {
    if (!isFocused) return@composed this

    val density = LocalDensity.current.density
    val cornerRadiusPx = cornerRadius.value * density
    val borderWidthPx = borderWidth.value * density

    val transition = rememberInfiniteTransition(label = "chasingBorder")
    val rotation = transition.animateFloat(
        initialValue = 0f,
        targetValue = 360f,
        animationSpec = infiniteRepeatable(
            animation = tween(durationMillis = animationDurationMs, easing = LinearEasing),
            repeatMode = RepeatMode.Restart
        ),
        label = "borderRotation"
    )

    val colors = remember {
        intArrayOf(
            0xFF2196F3.toInt(),
            0xFF29B6F6.toInt(),
            0xFF00E5FF.toInt(),
            0xFF29B6F6.toInt(),
            0xFF2196F3.toInt()
        )
    }
    val stops = remember { floatArrayOf(0f, 0.25f, 0.50f, 0.75f, 1f) }

    drawWithCache {
        val w = size.width
        val h = size.height

        if (w <= 0f || h <= 0f) {
            onDrawWithContent { drawContent() }
        } else {
            val inset = borderWidthPx / 2f
            val rect = RectF(inset, inset, w - inset, h - inset)
            val strokeCorner = (cornerRadiusPx - inset).coerceAtLeast(0f)
            val shader = SweepGradient(w / 2f, h / 2f, colors, stops)
            val paint = Paint().apply {
                isAntiAlias = true
                style = Paint.Style.STROKE
                strokeWidth = borderWidthPx
                this.shader = shader
            }
            val matrix = Matrix()

            onDrawWithContent {
                drawContent()
                matrix.setRotate(rotation.value, w / 2f, h / 2f)
                shader.setLocalMatrix(matrix)
                drawIntoCanvas { canvas ->
                    canvas.nativeCanvas.drawRoundRect(rect, strokeCorner, strokeCorner, paint)
                }
            }
        }
    }
}
