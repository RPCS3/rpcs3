package net.rpcs3.ui.hud

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.view.View

private const val MAX_SAMPLES = 60
private const val CEILING_MS = 66.6f
private const val RANGE_MS = 40f

class FrametimeGraphView(context: Context, lineColor: Int) : View(context) {
    private val history = FloatArray(MAX_SAMPLES)
    private var historyIndex = 0
    private var historySize = 0
    private val path = Path()

    private val paintLine = Paint().apply {
        color = lineColor
        strokeWidth = 1.5f
        style = Paint.Style.STROKE
        isAntiAlias = true
    }

    init {
        setBackgroundColor(Color.TRANSPARENT)
    }

    fun addFrame(ms: Float) {
        if (ms <= 0f) {
            return
        }

        history[historyIndex] = minOf(ms, CEILING_MS)
        historyIndex = (historyIndex + 1) % MAX_SAMPLES
        if (historySize < MAX_SAMPLES) {
            historySize++
        }

        invalidate()
    }

    fun reset() {
        historyIndex = 0
        historySize = 0
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        if (historySize < 2) {
            return
        }

        val w = width.toFloat()
        val h = height.toFloat()
        val step = w / (MAX_SAMPLES - 1)
        val start = (historyIndex - historySize + MAX_SAMPLES) % MAX_SAMPLES

        path.reset()
        path.moveTo(0f, maxOf(0f, h - history[start] / RANGE_MS * h))

        for (i in 1 until historySize) {
            val index = (start + i) % MAX_SAMPLES
            val y = h - history[index] / RANGE_MS * h
            path.lineTo(i * step, maxOf(0f, y))
        }

        canvas.drawPath(path, paintLine)
    }
}
