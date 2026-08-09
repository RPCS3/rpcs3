package net.rpcs3.overlay

import android.content.Context
import android.content.SharedPreferences
import android.content.res.Resources
import android.graphics.Bitmap
import android.graphics.drawable.BitmapDrawable
import android.view.MotionEvent
import kotlin.math.roundToInt

class PadOverlayButton(private val context: Context, resources: Resources, image: Bitmap, private val digital1: Int, private val digital2: Int, private val pressedArt: Bitmap? = null) : BitmapDrawable(resources, image) {
    private var pressed = false

    override fun draw(canvas: android.graphics.Canvas) {
        val art = pressedArt
        if (pressed && art != null) {
            val paint = android.graphics.Paint(android.graphics.Paint.FILTER_BITMAP_FLAG).apply {
                alpha = this@PadOverlayButton.alpha
            }
            canvas.drawBitmap(art, null, bounds, paint)
            return
        }
        super.draw(canvas)
    }
    private var locked = -1
    private var origAlpha = alpha
    var dragging = false
    private var offsetX = 0
    private var offsetY = 0
    private var scaleFactor = 0.5f
    private var opacity = alpha
    var defaultSize: Pair<Int, Int> = Pair(-1, -1)
    lateinit var defaultPosition: Pair<Int, Int>
    private val prefs: SharedPreferences by lazy { context.getSharedPreferences("PadOverlayPrefs", Context.MODE_PRIVATE) }
    
    fun contains(x: Int, y: Int) = bounds.contains(x, y)

    fun contains(x: Int, y: Int, slop: Int) =
        x >= bounds.left - slop && x <= bounds.right + slop &&
            y >= bounds.top - slop && y <= bounds.bottom + slop

    fun onTouch(event: MotionEvent, pointerIndex: Int, padState: State): Boolean {
        val action = event.actionMasked
        var hit = false
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if (locked == -1) {
                locked = event.getPointerId(pointerIndex)
                pressed = true
                origAlpha = alpha
                alpha = 255
                hit = true
            }
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_CANCEL) {
            if (locked != -1 && (action == MotionEvent.ACTION_CANCEL || event.getPointerId(pointerIndex) == locked)) {
                pressed = false
                locked = -1
                alpha = origAlpha
                hit = true
            }
        }

        if (pressed) {
            padState.digital[0] = padState.digital[0] or digital1
            padState.digital[1] = padState.digital[1] or digital2
        } else {
            padState.digital[0] = padState.digital[0] and digital1.inv()
            padState.digital[1] = padState.digital[1] and digital2.inv()
        }

        return hit
    }

    fun startDragging(startX: Int, startY: Int) {
        dragging = true
        offsetX = startX - bounds.left
        offsetY = startY - bounds.top
    }

    fun updatePosition(x: Int, y: Int, force: Boolean = false) {
        if (dragging) {
            setBounds(x - offsetX, y - offsetY, x - offsetX + bounds.width(), y - offsetY + bounds.height())
            prefs.edit()
                .putInt("button_${digital1}_${digital2}_x", x - offsetX)
                .putInt("button_${digital1}_${digital2}_y", y - offsetY)
                .apply()
        } else if (force) {
            setBounds(x, y, x + bounds.width(), y + bounds.height())
            prefs.edit()
                .putInt("button_${digital1}_${digital2}_x", x)
                .putInt("button_${digital1}_${digital2}_y", y)
                .apply()
        }
    }

    fun stopDragging() {
        dragging = false
    }

    fun setScale(percent: Int) {
        scaleFactor = percent / 100f
        val newWidth = (intrinsicWidth * scaleFactor).roundToInt()
        val newHeight = (intrinsicHeight * scaleFactor).roundToInt()
        setBounds(bounds.left, bounds.top, bounds.left + newWidth, bounds.top + newHeight)
        prefs.edit().putInt("button_${digital1}_${digital2}_scale", percent).apply()
    }

    fun setOpacity(percent: Int) {
        opacity = (255 * (percent / 100f)).roundToInt()
        alpha = opacity
        prefs.edit().putInt("button_${digital1}_${digital2}_opacity", percent).apply()
    }

    fun measureDefaultScale(): Int {
        if (defaultSize.second <= 0 || defaultSize.first <= 0) return 100
        val widthScale = defaultSize.second.toFloat() / intrinsicWidth * 100
        val heightScale = defaultSize.first.toFloat() / intrinsicHeight * 100
        return minOf(widthScale, heightScale).roundToInt()
    }

    fun resetConfigs() {
        setOpacity(50)
        setBounds(defaultPosition.first, defaultPosition.second, defaultPosition.first + defaultSize.second, defaultPosition.second + defaultSize.first)
        prefs.edit()
            .remove("button_${digital1}_${digital2}_scale")
            .remove("button_${digital1}_${digital2}_opacity")
            .remove("button_${digital1}_${digital2}_x")
            .remove("button_${digital1}_${digital2}_y")
            .apply()
    }

    fun getInfo(): Triple<String, Int, Int> {
        return Triple("${digital1}_${digital2}", prefs.getInt("button_${digital1}_${digital2}_scale", measureDefaultScale()), prefs.getInt("button_${digital1}_${digital2}_opacity", 50))
    }
}
