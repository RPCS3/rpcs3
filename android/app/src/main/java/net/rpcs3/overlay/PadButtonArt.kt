package net.rpcs3.overlay

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.CornerPathEffect
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.PorterDuff
import android.graphics.PorterDuffXfermode
import android.graphics.RadialGradient
import android.graphics.RectF
import android.graphics.Shader
import androidx.core.graphics.createBitmap

enum class PadSymbol { Triangle, Circle, Cross, Square }

object PadButtonArt {
    const val TRIANGLE_COLOR = 0xFF3DBE6E.toInt()
    const val CIRCLE_COLOR = 0xFFE03A3A.toInt()
    const val CROSS_COLOR = 0xFF7B8FE8.toInt()
    const val SQUARE_COLOR = 0xFFE86FC0.toInt()
    const val PILL_COLOR = 0xFF6B6B75.toInt()

    private fun lighten(color: Int, amount: Float): Int {
        val r = Color.red(color) + ((255 - Color.red(color)) * amount)
        val g = Color.green(color) + ((255 - Color.green(color)) * amount)
        val b = Color.blue(color) + ((255 - Color.blue(color)) * amount)
        return Color.argb(Color.alpha(color), r.toInt(), g.toInt(), b.toInt())
    }

    private fun darken(color: Int, amount: Float): Int {
        val factor = 1f - amount
        return Color.argb(
            Color.alpha(color),
            (Color.red(color) * factor).toInt(),
            (Color.green(color) * factor).toInt(),
            (Color.blue(color) * factor).toInt()
        )
    }

    fun faceButton(size: Int, symbolColor: Int, symbol: PadSymbol, pressed: Boolean = false): Bitmap {
        val bitmap = createBitmap(size, size)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val cx = size / 2f
        val radius = size * 0.44f
        val rim = size * 0.035f
        val cy = size / 2f + if (pressed) radius * 0.085f else 0f

        paint.style = Paint.Style.FILL
        paint.shader = RadialGradient(
            cx,
            cy + radius * 0.16f,
            radius * 1.12f,
            Color.argb(90, 0, 0, 0),
            Color.TRANSPARENT,
            Shader.TileMode.CLAMP
        )
        canvas.drawCircle(cx, cy + radius * 0.16f, radius * 1.12f, paint)

        paint.shader = RadialGradient(
            cx - radius * 0.42f,
            cy - radius * 0.46f,
            radius * 1.75f,
            if (pressed) {
                intArrayOf(0xFF474750.toInt(), 0xFF35353D.toInt(), 0xFF26262D.toInt())
            } else {
                intArrayOf(0xFF5A5A63.toInt(), 0xFF43434B.toInt(), 0xFF303038.toInt())
            },
            floatArrayOf(0f, 0.52f, 1f),
            Shader.TileMode.CLAMP
        )
        canvas.drawCircle(cx, cy, radius, paint)
        paint.shader = null

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = rim
        paint.color = 0xFF23232B.toInt()
        canvas.drawCircle(cx, cy, radius - rim * 0.5f, paint)

        drawSymbol(canvas, paint, cx, cy, radius, symbol, symbolColor)
        return bitmap
    }

    private fun drawSymbol(
        canvas: Canvas,
        paint: Paint,
        cx: Float,
        cy: Float,
        radius: Float,
        symbol: PadSymbol,
        symbolColor: Int
    ) {
        val r = radius * 0.44f
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = radius * 0.085f
        paint.strokeCap = Paint.Cap.BUTT
        paint.strokeJoin = Paint.Join.MITER
        paint.pathEffect = null
        paint.color = symbolColor

        when (symbol) {
            PadSymbol.Circle -> canvas.drawCircle(cx, cy, r * 0.80f, paint)

            PadSymbol.Cross -> {
                val d = r * 0.78f
                paint.strokeCap = Paint.Cap.ROUND
                canvas.drawLine(cx - d, cy - d, cx + d, cy + d, paint)
                canvas.drawLine(cx + d, cy - d, cx - d, cy + d, paint)
                paint.strokeCap = Paint.Cap.BUTT
            }

            PadSymbol.Square -> {
                val d = r * 0.76f
                canvas.drawRect(cx - d, cy - d, cx + d, cy + d, paint)
            }

            PadSymbol.Triangle -> {
                val path = Path().apply {
                    moveTo(cx, cy - r * 0.92f)
                    lineTo(cx + r * 0.86f, cy + r * 0.66f)
                    lineTo(cx - r * 0.86f, cy + r * 0.66f)
                    close()
                }
                canvas.drawPath(path, paint)
            }
        }
    }

    const val BODY_COLOR = 0xFF33333A.toInt()

    private fun shadeBody(
        canvas: Canvas,
        paint: Paint,
        cx: Float,
        cy: Float,
        radius: Float,
        color: Int,
        drawShape: (Paint) -> Unit
    ) {
        val strokeWidth = radius * 0.09f

        paint.style = Paint.Style.FILL
        paint.shader = RadialGradient(
            cx,
            cy + radius * 0.12f,
            radius * 1.18f,
            Color.argb(95, 0, 0, 0),
            Color.TRANSPARENT,
            Shader.TileMode.CLAMP
        )
        drawShape(paint)

        paint.shader = RadialGradient(
            cx - radius * 0.35f,
            cy - radius * 0.45f,
            radius * 1.9f,
            intArrayOf(lighten(color, 0.30f), color, darken(color, 0.16f)),
            floatArrayOf(0f, 0.55f, 1f),
            Shader.TileMode.CLAMP
        )
        drawShape(paint)
        paint.shader = null

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = strokeWidth
        paint.color = darken(color, 0.40f)
        drawShape(paint)

        paint.style = Paint.Style.FILL
        paint.shader = RadialGradient(
            cx - radius * 0.34f,
            cy - radius * 0.40f,
            radius * 0.62f,
            Color.argb(60, 255, 255, 255),
            Color.TRANSPARENT,
            Shader.TileMode.CLAMP
        )
        drawShape(paint)
        paint.shader = null
    }

    private fun label(
        canvas: Canvas,
        paint: Paint,
        cx: Float,
        cy: Float,
        size: Float,
        text: String,
        maxWidth: Float
    ) {
        paint.style = Paint.Style.FILL
        paint.shader = null
        paint.color = Color.argb(235, 255, 255, 255)
        paint.textAlign = Paint.Align.CENTER
        paint.isFakeBoldText = true
        paint.textSize = size

        val measured = paint.measureText(text)
        if (measured > maxWidth) {
            paint.textSize = size * maxWidth / measured
        }

        val y = cy - (paint.descent() + paint.ascent()) * 0.5f
        canvas.drawText(text, cx, y, paint)
        paint.isFakeBoldText = false
    }

    fun shoulderButton(
        width: Int,
        height: Int,
        text: String,
        slantLeft: Boolean,
        flipVertical: Boolean,
        pressed: Boolean = false
    ): Bitmap {
        val bitmap = createBitmap(width, height)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val padH = height * 0.10f + if (pressed) height * 0.055f else 0f
        val left = height * 0.06f
        val right = width - height * 0.06f
        val top = padH
        val bottom = height - padH * 1.9f
        val slant = (right - left) * 0.22f
        val corner = (bottom - top) * 0.30f

        fun buildPath(dy: Float): Path {
            val path = Path()
            val narrowTop = !flipVertical
            if (slantLeft) {
                if (narrowTop) {
                    path.moveTo(left + slant, top + dy)
                    path.lineTo(right, top + dy)
                    path.lineTo(right, bottom + dy)
                    path.lineTo(left, bottom + dy)
                } else {
                    path.moveTo(left, top + dy)
                    path.lineTo(right, top + dy)
                    path.lineTo(right, bottom + dy)
                    path.lineTo(left + slant, bottom + dy)
                }
            } else {
                if (narrowTop) {
                    path.moveTo(left, top + dy)
                    path.lineTo(right - slant, top + dy)
                    path.lineTo(right, bottom + dy)
                    path.lineTo(left, bottom + dy)
                } else {
                    path.moveTo(left, top + dy)
                    path.lineTo(right, top + dy)
                    path.lineTo(right - slant, bottom + dy)
                    path.lineTo(left, bottom + dy)
                }
            }
            path.close()
            return path
        }

        paint.pathEffect = CornerPathEffect(corner)

        paint.style = Paint.Style.FILL
        paint.shader = null
        paint.color = Color.argb(120, 0, 0, 0)
        canvas.drawPath(buildPath(padH * 0.9f), paint)

        paint.shader = LinearGradient(
            0f,
            top,
            0f,
            bottom,
            if (pressed) {
                intArrayOf(0xFF57575F.toInt(), 0xFF3E3E46.toInt(), 0xFF313139.toInt())
            } else {
                intArrayOf(0xFF6E6E77.toInt(), 0xFF4C4C55.toInt(), 0xFF3C3C44.toInt())
            },
            floatArrayOf(0f, 0.62f, 1f),
            Shader.TileMode.CLAMP
        )
        canvas.drawPath(buildPath(0f), paint)
        paint.shader = null

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = height * 0.055f
        paint.color = 0xFF23232B.toInt()
        canvas.drawPath(buildPath(0f), paint)
        paint.pathEffect = null

        label(canvas, paint, (left + right) / 2f, (top + bottom) / 2f, (bottom - top) * 0.52f, text, (right - left) * 0.62f)
        return bitmap
    }

    fun pillButton(width: Int, height: Int, text: String, pressed: Boolean = false): Bitmap {
        val bitmap = createBitmap(width, height)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val stroke = height * 0.06f
        val rect = RectF(stroke, stroke, width - stroke, height - stroke)
        val r = rect.height() * 0.5f

        paint.shader = null
        paint.style = Paint.Style.FILL
        paint.color = if (pressed) 0xF22B3446.toInt() else 0xF21C222C.toInt()
        canvas.drawRoundRect(rect, r, r, paint)

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = stroke
        paint.color = 0xFF3A4354.toInt()
        canvas.drawRoundRect(rect, r, r, paint)

        paint.style = Paint.Style.FILL
        paint.color = 0xFFF2F5FA.toInt()
        paint.textAlign = Paint.Align.CENTER
        paint.isFakeBoldText = true
        paint.textSize = rect.height() * 0.42f
        val measured = paint.measureText(text)
        val maxWidth = rect.width() * 0.84f
        if (measured > maxWidth) paint.textSize = paint.textSize * maxWidth / measured
        val textY = rect.centerY() - (paint.descent() + paint.ascent()) * 0.5f
        canvas.drawText(text, rect.centerX(), textY, paint)
        paint.isFakeBoldText = false
        return bitmap
    }

    fun circleButton(size: Int, text: String, color: Int = BODY_COLOR): Bitmap {
        val bitmap = createBitmap(size, size)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)
        val cx = size / 2f
        val radius = size * 0.40f

        shadeBody(canvas, paint, cx, cx, radius, color) { p ->
            canvas.drawCircle(cx, cx, radius, p)
        }
        label(canvas, paint, cx, cx, radius * 0.72f, text, radius * 1.5f)
        return bitmap
    }

    private fun crossPath(w: Float, h: Float, armW: Float, armH: Float): Path {
        val x0 = (w - armW) / 2f
        val x1 = (w + armW) / 2f
        val y0 = (h - armW) / 2f
        val y1 = (h + armW) / 2f
        return Path().apply {
            moveTo(x0, 0f)
            lineTo(x1, 0f)
            lineTo(x1, y0)
            lineTo(w, y0)
            lineTo(w, y1)
            lineTo(x1, y1)
            lineTo(x1, h)
            lineTo(x0, h)
            lineTo(x0, y1)
            lineTo(0f, y1)
            lineTo(0f, y0)
            lineTo(x0, y0)
            close()
        }
    }

    fun dpadCross(width: Int, height: Int, armThickness: Float): Bitmap {
        val bitmap = createBitmap(width, height)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val color = BODY_COLOR
        val cx = width / 2f
        val cy = height / 2f
        val r = minOf(width, height) / 2f * 0.97f
        val arm = armThickness / 2f
        val corner = arm * 0.5f
        val depth = r * 0.05f
        val stroke = (r * 0.045f).coerceAtLeast(2f)

        val shadow = Path().apply {
            addRoundRect(cx - r, cy - arm + depth, cx + r, cy + arm + depth, corner, corner, Path.Direction.CW)
            addRoundRect(cx - arm, cy - r + depth, cx + arm, cy + r + depth, corner, corner, Path.Direction.CW)
        }
        paint.style = Paint.Style.FILL
        paint.color = Color.argb(80, 0, 0, 0)
        canvas.drawPath(shadow, paint)

        val body = Path().apply {
            addRoundRect(cx - r, cy - arm, cx + r, cy + arm, corner, corner, Path.Direction.CW)
            addRoundRect(cx - arm, cy - r, cx + arm, cy + r, corner, corner, Path.Direction.CW)
        }
        paint.shader = LinearGradient(
            cx,
            cy - r,
            cx,
            cy + r,
            intArrayOf(lighten(color, 0.2f), color, darken(color, 0.2f)),
            floatArrayOf(0f, 0.5f, 1f),
            Shader.TileMode.CLAMP
        )
        canvas.drawPath(body, paint)
        paint.shader = null

        val outline = Path().apply {
            addRoundRect(cx - r, cy - arm, cx + r, cy + arm, corner, corner, Path.Direction.CW)
        }
        val vertical = Path().apply {
            addRoundRect(cx - arm, cy - r, cx + arm, cy + r, corner, corner, Path.Direction.CW)
        }
        outline.op(vertical, Path.Op.UNION)
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = stroke
        paint.color = darken(color, 0.45f)
        canvas.drawPath(outline, paint)

        paint.style = Paint.Style.FILL
        paint.shader = RadialGradient(
            cx,
            cy,
            arm * 0.85f,
            darken(color, 0.3f),
            color,
            Shader.TileMode.CLAMP
        )
        canvas.drawCircle(cx, cy, arm * 0.85f, paint)
        paint.shader = null

        paint.color = 0xFFF3F4F7.toInt()
        val tip = arm * 0.5f
        val reach = r * 0.62f
        drawArrow(canvas, paint, cx, cy - reach, tip, 0f)
        drawArrow(canvas, paint, cx, cy + reach, tip, 180f)
        drawArrow(canvas, paint, cx - reach, cy, tip, 270f)
        drawArrow(canvas, paint, cx + reach, cy, tip, 90f)
        return bitmap
    }

    private fun drawArrow(
        canvas: Canvas,
        paint: Paint,
        cx: Float,
        cy: Float,
        size: Float,
        rotation: Float
    ) {
        canvas.save()
        canvas.rotate(rotation, cx, cy)
        val path = Path().apply {
            moveTo(cx, cy - size)
            lineTo(cx + size * 0.92f, cy + size * 0.66f)
            lineTo(cx - size * 0.92f, cy + size * 0.66f)
            close()
        }
        canvas.drawPath(path, paint)
        canvas.restore()
    }

    fun dpadPressHighlight(width: Int, height: Int, corner: Float): Bitmap {
        val bitmap = createBitmap(width, height)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.FILL
            color = Color.argb(90, 255, 255, 255)
        }
        canvas.drawRoundRect(0f, 0f, width.toFloat(), height.toFloat(), corner, corner, paint)
        return bitmap
    }

    fun stickWell(size: Int): Bitmap {
        val bitmap = createBitmap(size, size)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)
        val cx = size / 2f
        val radius = size * 0.46f
        val well = darken(BODY_COLOR, 0.16f)

        paint.style = Paint.Style.FILL
        paint.shader = RadialGradient(
            cx,
            cx,
            radius,
            intArrayOf(darken(well, 0.32f), well, lighten(well, 0.10f)),
            floatArrayOf(0f, 0.78f, 1f),
            Shader.TileMode.CLAMP
        )
        canvas.drawCircle(cx, cx, radius, paint)
        paint.shader = null

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = radius * 0.07f
        paint.color = darken(well, 0.40f)
        canvas.drawCircle(cx, cx, radius, paint)
        return bitmap
    }

    fun stickKnob(size: Int, text: String = ""): Bitmap {
        val bitmap = createBitmap(size, size)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)
        val cx = size / 2f
        val r = size * 0.40f
        val cap = BODY_COLOR

        paint.style = Paint.Style.FILL
        paint.shader = RadialGradient(
            cx,
            cx + r * 0.18f,
            r * 1.3f,
            Color.argb(100, 0, 0, 0),
            Color.TRANSPARENT,
            Shader.TileMode.CLAMP
        )
        canvas.drawCircle(cx, cx + r * 0.18f, r * 1.3f, paint)

        paint.shader = RadialGradient(
            cx - r * 0.28f,
            cx - r * 0.32f,
            r * 1.9f,
            intArrayOf(lighten(cap, 0.28f), cap, darken(cap, 0.30f)),
            floatArrayOf(0f, 0.5f, 1f),
            Shader.TileMode.CLAMP
        )
        canvas.drawCircle(cx, cx, r, paint)

        paint.shader = RadialGradient(
            cx,
            cx + r * 0.08f,
            r * 0.72f,
            intArrayOf(darken(cap, 0.24f), darken(cap, 0.12f), lighten(cap, 0.18f)),
            floatArrayOf(0f, 0.75f, 1f),
            Shader.TileMode.CLAMP
        )
        canvas.drawCircle(cx, cx, r * 0.72f, paint)
        paint.shader = null

        paint.style = Paint.Style.STROKE
        paint.strokeWidth = r * 0.08f
        paint.color = darken(cap, 0.42f)
        canvas.drawCircle(cx, cx, r, paint)
        paint.strokeWidth = r * 0.056f
        paint.color = lighten(cap, 0.08f)
        canvas.drawCircle(cx, cx, r * 0.72f, paint)

        if (text.isNotEmpty()) {
            label(canvas, paint, cx, cx, r * 0.62f, text, r * 1.3f)
        }
        return bitmap
    }
}
