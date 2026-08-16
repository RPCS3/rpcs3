package net.rpcs3.ui.hud

import android.graphics.Canvas
import android.graphics.ColorFilter
import android.graphics.Paint
import android.graphics.Path
import android.graphics.PixelFormat
import android.graphics.drawable.Drawable

private val ARROW_OUTLINE = floatArrayOf(
    5f, 9f,
    6.41f, 10.41f,
    11f, 5.83f,
    11f, 22f,
    13f, 22f,
    13f, 5.83f,
    17.59f, 10.41f,
    19f, 9f,
    12f, 2f
)

class AnchorArrowDrawable(private val bearingDegrees: Float, tint: Int) : Drawable() {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = tint
        style = Paint.Style.FILL
    }
    private val path = Path()

    override fun draw(canvas: Canvas) {
        val side = minOf(bounds.width(), bounds.height()).toFloat()

        if (side <= 0f) {
            return
        }

        val unit = side / 24f
        path.reset()

        var index = 0
        while (index < ARROW_OUTLINE.size) {
            val px = bounds.left + ARROW_OUTLINE[index] * unit
            val py = bounds.top + ARROW_OUTLINE[index + 1] * unit

            if (index == 0) {
                path.moveTo(px, py)
            } else {
                path.lineTo(px, py)
            }

            index += 2
        }

        path.close()

        val saved = canvas.save()
        canvas.rotate(bearingDegrees, bounds.exactCenterX(), bounds.exactCenterY())
        canvas.drawPath(path, paint)
        canvas.restoreToCount(saved)
    }

    override fun setAlpha(alpha: Int) {
        paint.alpha = alpha
    }

    override fun setColorFilter(colorFilter: ColorFilter?) {
        paint.colorFilter = colorFilter
    }

    @Deprecated("Deprecated in Java", ReplaceWith("PixelFormat.TRANSLUCENT"))
    override fun getOpacity() = PixelFormat.TRANSLUCENT
}
