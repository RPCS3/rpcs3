package net.rpcs3.ui.hud

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.AttributeSet
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import kotlin.math.abs

private const val TAP_SLOP_PX = 20f
private const val LONG_PRESS_MS = 500L
private const val TAP_MAX_MS = 400L
private const val BACKDROP_COLOR = 0xA6000000.toInt()

private const val COLOR_FPS = 0xFF76FF03.toInt()
private const val COLOR_FRAMETIME = 0xFFB2FF59.toInt()
private const val COLOR_RENDERER = 0xFF90A4AE.toInt()
private const val COLOR_GPU = 0xFFE040FB.toInt()
private const val COLOR_CPU = 0xFFFF8200.toInt()
private const val COLOR_RAM = 0xFF26C6DA.toInt()
private const val COLOR_BATTERY = 0xFFE03A94.toInt()
private const val COLOR_POWER = 0xFF00BCD4.toInt()
private const val COLOR_TEMP = 0xFFE53935.toInt()
private const val COLOR_TEMP_WARM = 0xFFFFC107.toInt()
private const val COLOR_TEMP_HOT = 0xFFFF1744.toInt()
private const val COLOR_SEPARATOR = 0xFF616161.toInt()
private const val COLOR_VALUE = 0xFFFFFFFF.toInt()

private const val READOUT_SP = 10f
private const val SCALE_BASE = 1.2f

private const val POPUP_SURFACE = 0xFF1C1C2A.toInt()
private const val POPUP_EDGE = 0xFF2A2A3A.toInt()
private const val POPUP_TEXT = 0xFFF0F4FF.toInt()
private const val POPUP_ACCENT = 0xFF1A9FFF.toInt()
private const val POPUP_RIPPLE = 0x33A0C8FF

private class SmallRaisedSpan(private val ratio: Float) :
    android.text.style.MetricAffectingSpan() {

    private fun apply(paint: android.text.TextPaint) {
        val fullAscent = paint.ascent()
        paint.textSize = paint.textSize * ratio
        paint.baselineShift += (fullAscent - paint.ascent()).toInt()
    }

    override fun updateDrawState(tp: android.text.TextPaint) = apply(tp)

    override fun updateMeasureState(tp: android.text.TextPaint) = apply(tp)
}

class HudView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : LinearLayout(context, attrs, defStyleAttr) {

    private val prefs = HudPrefs.of(context)
    private val handler = Handler(Looper.getMainLooper())
    private val readouts = LinkedHashMap<HudElement, TextView>()
    private val separators = ArrayList<TextView>()
    private val backdrop = GradientDrawable().apply {
        setColor(BACKDROP_COLOR)
        cornerRadius = dp(6f)
    }

    private var graph: FrametimeGraphView? = null
    private var frametimeNumeric = HudPrefs.frametimeNumeric(prefs)
    private var mode = HudPrefs.mode(prefs)
    private var active = HudPrefs.enabledElements(prefs)
    private var scale = HudPrefs.scale(prefs)
    private var sample = HudSample()

    private var positionPopup: android.widget.PopupWindow? = null

    var onModeChanged: ((HudMode) -> Unit)? = null

    init {
        isClickable = true
        isFocusable = false
        clipChildren = false
        buildChildren()
        applyMode()
        applyElements()
        installTouchHandling()
        render()
    }

    private fun dp(value: Float) = value * resources.displayMetrics.density

    private fun buildChildren() {
        removeAllViews()
        readouts.clear()
        separators.clear()

        HudElement.entries.forEachIndexed { index, element ->
            if (index > 0) {
                val separator = TextView(context).apply {
                    text = " | "
                    setTextColor(COLOR_SEPARATOR)
                    textSize = READOUT_SP
                    setTypeface(android.graphics.Typeface.DEFAULT, android.graphics.Typeface.BOLD)
                    includeFontPadding = false
                }
                separators.add(separator)
                addView(
                    separator,
                    LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT)
                )
            }

            val readout = TextView(context).apply {
                includeFontPadding = false
                textSize = READOUT_SP
                setTypeface(android.graphics.Typeface.MONOSPACE, android.graphics.Typeface.BOLD)
                setTextColor(colorOf(element))
                setShadowLayer(1f, 1f, 1f, Color.BLACK)
            }
            readouts[element] = readout
            addView(readout, LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT))

            if (element == HudElement.Frametime) {
                val view = FrametimeGraphView(context, COLOR_FRAMETIME)
                graph = view
                addView(view, LayoutParams(dp(50f).toInt(), dp(14f).toInt()))
            }
        }

        applyScale()
    }

    private fun applyScale() {
        pivotX = 0f
        pivotY = 0f
        scaleX = scale * SCALE_BASE
        scaleY = scale * SCALE_BASE
    }

    private fun colorOf(element: HudElement) = when (element) {
        HudElement.Fps -> COLOR_FPS
        HudElement.Frametime -> COLOR_FRAMETIME
        HudElement.Renderer -> COLOR_RENDERER
        HudElement.Gpu -> COLOR_GPU
        HudElement.Cpu -> COLOR_CPU
        HudElement.Ram -> COLOR_RAM
        HudElement.Battery -> COLOR_BATTERY
        HudElement.Power -> COLOR_POWER
        HudElement.Temperature -> COLOR_TEMP
    }

    val currentMode: HudMode
        get() = mode

    fun setMode(value: HudMode) {
        if (mode == value) {
            return
        }

        mode = value
        HudPrefs.setMode(prefs, value)
        applyMode()
        onModeChanged?.invoke(value)
    }

    fun cycleMode() {
        val next = HudMode.entries[(mode.ordinal + 1) % HudMode.entries.size]
        setMode(next)
    }

    fun setFrametimeNumeric(value: Boolean) {
        frametimeNumeric = value
        applyElements()
        requestLayout()
        reanchorIfUnpinned()
    }

    fun setElements(elements: Set<HudElement>) {
        active = elements
        applyElements()
        render()
        requestLayout()
        reanchorIfUnpinned()
    }

    fun setScale(value: Float) {
        scale = value
        applyScale()
        requestLayout()
        reanchorIfUnpinned()
    }

    fun submit(value: HudSample) {
        sample = value
        render()
    }

    fun addFrameSample(ms: Float) {
        graph?.addFrame(ms)
    }

    private fun applyMode() {
        orientation = if (mode.horizontal) HORIZONTAL else VERTICAL
        gravity = if (mode.horizontal) Gravity.CENTER_VERTICAL else Gravity.START
        background = if (mode.backdrop) backdrop else null

        val padH = if (mode.backdrop) dp(8f).toInt() else dp(2f).toInt()
        val padV = if (mode.backdrop) dp(5f).toInt() else dp(2f).toInt()
        setPadding(padH, padV, padH, padV)

        applyElements()
        requestLayout()
        reanchorIfUnpinned()
    }

    private fun reanchorIfUnpinned() {
        if (HudPrefs.hasPosition(prefs)) {
            return
        }

        applyAnchor(HudPrefs.anchor(prefs), persist = false)
    }

    private fun applyElements() {
        readouts.forEach { (element, view) ->
            view.visibility = if (active.contains(element)) View.VISIBLE else View.GONE
        }

        val frametimeOn = active.contains(HudElement.Frametime)
        readouts[HudElement.Frametime]?.visibility =
            if (frametimeOn && frametimeNumeric) View.VISIBLE else View.GONE
        graph?.visibility = if (frametimeOn && !frametimeNumeric) View.VISIBLE else View.GONE

        if (!mode.horizontal) {
            separators.forEach { it.visibility = View.GONE }
            return
        }

        val visible = HudElement.entries.filter { active.contains(it) }
        separators.forEach { it.visibility = View.GONE }

        if (visible.size < 2) {
            return
        }

        var previousVisibleIndex = -1
        HudElement.entries.forEachIndexed { index, element ->
            if (!active.contains(element)) {
                return@forEachIndexed
            }

            if (previousVisibleIndex >= 0) {
                separators.getOrNull(index - 1)?.visibility = View.VISIBLE
            }

            previousVisibleIndex = index
        }
    }

    private fun labelled(label: String, labelColor: Int, value: String): CharSequence {
        val builder = android.text.SpannableStringBuilder()
        builder.append(label)
        builder.setSpan(
            android.text.style.ForegroundColorSpan(labelColor),
            0,
            label.length,
            android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE
        )

        val start = builder.length
        builder.append(value)
        builder.setSpan(
            android.text.style.ForegroundColorSpan(COLOR_VALUE),
            start,
            builder.length,
            android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE
        )

        return builder
    }

    private fun percentValue(value: Int) = if (value >= 0) "$value%" else "N/A"

    private fun render() {
        readouts[HudElement.Fps]?.text = labelled(
            "FPS ",
            COLOR_FPS,
            if (sample.fps > 0f) String.format(java.util.Locale.US, "%.0f", sample.fps) else "0"
        )

        readouts[HudElement.Frametime]?.text = if (sample.frametimeMs > 0f) {
            String.format(java.util.Locale.US, "%.1f ms", sample.frametimeMs)
        } else {
            "0.0 ms"
        }

        readouts[HudElement.Renderer]?.text = sample.renderer.ifEmpty { "API" }

        readouts[HudElement.Gpu]?.text =
            labelled("GPU ", COLOR_GPU, percentValue(sample.gpuPercent))
        readouts[HudElement.Cpu]?.text =
            labelled("CPU ", COLOR_CPU, percentValue(sample.cpuPercent))

        readouts[HudElement.Ram]?.text = labelled(
            "RAM ",
            COLOR_RAM,
            when {
                sample.ramUsedMb < 0 -> "N/A"
                sample.ramTotalMb > 0 -> "${sample.ramUsedMb}/${sample.ramTotalMb}"
                else -> "${sample.ramUsedMb}"
            }
        )

        readouts[HudElement.Battery]?.text = labelled(
            "BAT ",
            COLOR_BATTERY,
            if (sample.batteryPercent >= 0) "${sample.batteryPercent}%" else "N/A"
        )

        readouts[HudElement.Power]?.text = labelled(
            "PWR ",
            COLOR_POWER,
            if (!sample.watts.isNaN()) {
                String.format(java.util.Locale.US, "%.1fw", sample.watts)
            } else {
                "N/A"
            }
        )

        val temp = readouts[HudElement.Temperature]
        if (temp != null) {
            if (sample.temperatureC < 0) {
                temp.text = labelled("TMP ", COLOR_TEMP, "N/A")
            } else {
                val valueColor = when {
                    sample.temperatureC >= 45 -> COLOR_TEMP_HOT
                    sample.temperatureC >= 40 -> COLOR_TEMP_WARM
                    else -> COLOR_VALUE
                }

                val builder = android.text.SpannableStringBuilder()
                builder.append("TMP ")
                builder.setSpan(
                    android.text.style.ForegroundColorSpan(COLOR_TEMP),
                    0,
                    builder.length,
                    android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE
                )

                val start = builder.length
                builder.append("${sample.temperatureC}°")
                val unitStart = builder.length
                builder.append("C")
                builder.setSpan(
                    android.text.style.ForegroundColorSpan(valueColor),
                    start,
                    builder.length,
                    android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE
                )
                builder.setSpan(
                    SmallRaisedSpan(0.7f),
                    unitStart,
                    builder.length,
                    android.text.Spannable.SPAN_EXCLUSIVE_EXCLUSIVE
                )

                temp.text = builder
            }
        }
    }

    fun applyAnchor(anchor: HudAnchor, persist: Boolean = true) {
        val parent = parent as? ViewGroup ?: return

        post {
            val shownW = (width * scaleX).toInt()
            val shownH = (height * scaleY).toInt()
            val maxX = (parent.width - shownW).coerceAtLeast(0).toFloat()
            val maxY = (parent.height - shownH).coerceAtLeast(0).toFloat()
            val margin = dp(12f)

            val targetX = when (anchor) {
                HudAnchor.TopLeft, HudAnchor.BottomLeft, HudAnchor.LeftCenter -> margin
                HudAnchor.TopCenter, HudAnchor.BottomCenter -> maxX / 2f
                HudAnchor.TopRight, HudAnchor.BottomRight, HudAnchor.RightCenter -> maxX - margin
            }.coerceIn(0f, maxX)

            val targetY = when (anchor) {
                HudAnchor.TopLeft, HudAnchor.TopCenter, HudAnchor.TopRight -> margin
                HudAnchor.LeftCenter, HudAnchor.RightCenter -> maxY / 2f
                HudAnchor.BottomLeft, HudAnchor.BottomCenter, HudAnchor.BottomRight -> maxY - margin
            }.coerceIn(0f, maxY)

            x = targetX
            y = targetY

            if (persist) {
                HudPrefs.setAnchor(prefs, anchor)
            }
        }
    }

    fun restorePosition() {
        post {
            if (HudPrefs.hasPosition(prefs)) {
                val (savedX, savedY) = HudPrefs.position(prefs)
                x = savedX
                y = savedY
                clampToParent()
                return@post
            }

            applyAnchor(HudPrefs.anchor(prefs), persist = false)
        }
    }

    private fun clampToParent() {
        val parent = parent as? ViewGroup ?: return
        val shownW = (width * scaleX).toInt()
        val shownH = (height * scaleY).toInt()
        x = x.coerceIn(0f, (parent.width - shownW).coerceAtLeast(0).toFloat())
        y = y.coerceIn(0f, (parent.height - shownH).coerceAtLeast(0).toFloat())
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun installTouchHandling() {
        setOnTouchListener(object : View.OnTouchListener {
            private var pointerId = -1
            private var offsetX = 0f
            private var offsetY = 0f
            private var downX = 0f
            private var downY = 0f
            private var downAt = 0L
            private var dragging = false
            private var longPressFired = false

            private val longPress = Runnable {
                if (!dragging && pointerId != -1) {
                    longPressFired = true
                    showPositionMenu()
                }
            }

            override fun onTouch(view: View, event: MotionEvent): Boolean {
                if (event.pointerCount > 1) {
                    pointerId = -1
                    handler.removeCallbacks(longPress)
                    return false
                }

                when (event.actionMasked) {
                    MotionEvent.ACTION_DOWN -> {
                        pointerId = event.getPointerId(0)
                        offsetX = view.x - event.rawX
                        offsetY = view.y - event.rawY
                        downX = event.rawX
                        downY = event.rawY
                        downAt = SystemClock.elapsedRealtime()
                        dragging = false
                        longPressFired = false
                        view.bringToFront()
                        handler.removeCallbacks(longPress)
                        handler.postDelayed(longPress, LONG_PRESS_MS)
                        return true
                    }

                    MotionEvent.ACTION_MOVE -> {
                        if (pointerId == -1) {
                            return false
                        }

                        if (abs(event.rawX - downX) > TAP_SLOP_PX ||
                            abs(event.rawY - downY) > TAP_SLOP_PX
                        ) {
                            dragging = true
                            handler.removeCallbacks(longPress)
                        }

                        if (dragging && !longPressFired) {
                            view.x = event.rawX + offsetX
                            view.y = event.rawY + offsetY
                            clampToParent()
                        }

                        return true
                    }

                    MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                        handler.removeCallbacks(longPress)

                        if (pointerId == -1) {
                            return false
                        }

                        val elapsed = SystemClock.elapsedRealtime() - downAt

                        if (!longPressFired && !dragging && elapsed < TAP_MAX_MS) {
                            cycleMode()
                        } else if (dragging) {
                            clampToParent()
                            HudPrefs.setPosition(prefs, view.x, view.y)
                        }

                        pointerId = -1
                        return true
                    }
                }

                return false
            }
        })
    }

    fun dismissPositionMenu() {
        runCatching { positionPopup?.dismiss() }
        positionPopup = null
    }

    private fun anchorCell(anchor: HudAnchor?, cellSize: Int, margin: Int): View {
        val params = android.widget.GridLayout.LayoutParams().apply {
            width = cellSize
            height = cellSize
            setMargins(margin, margin, margin, margin)
        }

        if (anchor == null) {
            return View(context).apply { layoutParams = params }
        }

        return android.widget.ImageView(context).apply {
            layoutParams = params
            val inset = dp(7f).toInt()
            setPadding(inset, inset, inset, inset)
            setImageDrawable(AnchorArrowDrawable(anchor.bearingDegrees, POPUP_ACCENT))
            background = rippleFor(POPUP_RIPPLE)
            isClickable = true
            isFocusable = true
            contentDescription = anchor.label
            setOnClickListener {
                applyAnchor(anchor)
                dismissPositionMenu()
            }
        }
    }

    private fun rippleFor(color: Int): android.graphics.drawable.Drawable {
        val mask = GradientDrawable().apply {
            setColor(Color.WHITE)
            cornerRadius = dp(8f)
        }

        return android.graphics.drawable.RippleDrawable(
            android.content.res.ColorStateList.valueOf(color),
            null,
            mask
        )
    }

    private fun showPositionMenu() {
        if (!isAttachedToWindow) {
            return
        }

        dismissPositionMenu()

        val container = LinearLayout(context).apply {
            orientation = VERTICAL
            background = GradientDrawable().apply {
                setColor(POPUP_SURFACE)
                cornerRadius = dp(10f)
                setStroke(dp(1f).toInt(), POPUP_EDGE)
            }
            val pad = dp(5f).toInt()
            setPadding(pad, pad, pad, pad)
            elevation = dp(8f)
        }

        container.addView(
            TextView(context).apply {
                text = "Snap position"
                setTextColor(POPUP_TEXT)
                alpha = 0.7f
                textSize = 10f
                gravity = Gravity.CENTER_HORIZONTAL
                setPadding(dp(4f).toInt(), dp(1f).toInt(), dp(4f).toInt(), dp(4f).toInt())
            },
            LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT)
        )

        val grid = android.widget.GridLayout(context).apply {
            columnCount = 3
            rowCount = 3
        }

        val cellSize = dp(36f).toInt()
        val cellMargin = dp(1f).toInt()

        listOf(
            HudAnchor.TopLeft, HudAnchor.TopCenter, HudAnchor.TopRight,
            HudAnchor.LeftCenter, null, HudAnchor.RightCenter,
            HudAnchor.BottomLeft, HudAnchor.BottomCenter, HudAnchor.BottomRight
        ).forEach { grid.addView(anchorCell(it, cellSize, cellMargin)) }

        container.addView(grid)

        val popup = android.widget.PopupWindow(
            container,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            true
        ).apply {
            isOutsideTouchable = true
            elevation = dp(8f)
            setBackgroundDrawable(android.graphics.drawable.ColorDrawable(Color.TRANSPARENT))
        }

        positionPopup = popup

        container.measure(
            View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
            View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED)
        )

        val popupWidth = container.measuredWidth
        val popupHeight = container.measuredHeight
        val host = parent as? View
        val hostWidth = host?.width ?: popupWidth
        val hostHeight = host?.height ?: popupHeight
        val origin = IntArray(2)
        host?.getLocationOnScreen(origin)

        val edge = dp(8f).toInt()
        val centreX = (x + width * scaleX / 2f).toInt()
        val centreY = (y + height * scaleY / 2f).toInt()
        val posX = (centreX - popupWidth / 2).coerceIn(edge, (hostWidth - popupWidth - edge).coerceAtLeast(edge))
        val posY = (centreY - popupHeight / 2).coerceIn(edge, (hostHeight - popupHeight - edge).coerceAtLeast(edge))

        popup.showAtLocation(
            host ?: this,
            Gravity.NO_GRAVITY,
            origin[0] + posX,
            origin[1] + posY
        )
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        handler.removeCallbacksAndMessages(null)
        dismissPositionMenu()
    }
}
