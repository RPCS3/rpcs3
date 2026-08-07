package net.rpcs3.overlay

import android.content.res.Resources
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.BitmapDrawable
import android.view.MotionEvent
import androidx.core.graphics.drawable.toDrawable
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.sin

class PadOverlayStick(
    resources: Resources,
    private val isLeft: Boolean,
    bg: Bitmap,
    stick: Bitmap,
    private val pressDigitalIndex: Int = 0,
    private val pressBit: Int = 0
) :
    BitmapDrawable(resources, stick) {
    private var bg = bg.toDrawable(resources)
    private var locked = -1
    private var pressX = -1
    private var pressY = -1
    private var bgOffsetX = 0
    private var bgOffsetY = 0
    fun contains(x: Int, y: Int) = bounds.contains(x, y)

    fun isActive(): Boolean {
        return locked != -1
    }

    fun onAdd(event: MotionEvent, pointerIndex: Int) {
        locked = event.getPointerId(pointerIndex)
        val x = event.getX(pointerIndex).toInt()
        val y = event.getY(pointerIndex).toInt()

        pressX = x
        pressY = y

        setBounds(
            x - bounds.width() / 2,
            y - bounds.height() / 2,
            x + bounds.width() / 2,
            y + bounds.height() / 2,
        )
    }

    fun onTouch(event: MotionEvent, pointerIndex: Int, padState: State): Int {
        val action = event.actionMasked

        if ((pressBit != 0 && (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN)) || (locked != -1 && action == MotionEvent.ACTION_MOVE)) {
            var activePointerIndex = pointerIndex

            if (action != MotionEvent.ACTION_MOVE) {
                if (locked == -1) {
                    locked = event.getPointerId(pointerIndex)
                    pressX = event.getX(pointerIndex).toInt()
                    pressY = event.getY(pointerIndex).toInt()
                    bgOffsetX = bg.bounds.centerX() - pressX
                    bgOffsetY = bg.bounds.centerY() - pressY

                    bg.setBounds(bg.bounds.left - bgOffsetX,bg.bounds.top - bgOffsetY, bg.bounds.right - bgOffsetX, bg.bounds.bottom - bgOffsetY)
                } else if (locked != event.getPointerId(pointerIndex)) {
                    return 0
                }
            } else {

                for (i in 0..<event.pointerCount) {
                    if (locked == event.getPointerId(i)) {
                        activePointerIndex = i
                        break
                    }
                }

                if (activePointerIndex == -1) {
                    return 0
                }
            }

            padState.digital[pressDigitalIndex] = padState.digital[pressDigitalIndex] or pressBit

            val bgCenterX = pressX
            val bgCenterY = pressY

            var x = event.getX(activePointerIndex)
            var y = event.getY(activePointerIndex)

            x -= bgCenterX
            y -= bgCenterY

            val bgR =
                hypot((bg.bounds.left - bgCenterX).toFloat(), (bg.bounds.top - bgCenterY).toFloat())
            val stickR = hypot(x, y)

            if (stickR > bgR) {
                val L = atan2(y, x)
                x = bgR * cos(L)
                y = bgR * sin(L)
            }

            val stickX = ((x / bgR) * 127 + 128).toInt()
            val stickY = ((y / bgR) * 127 + 128).toInt()

            if (isLeft) {
                padState.leftStickX = stickX
                padState.leftStickY = stickY
            } else {
                padState.rightStickX = stickX
                padState.rightStickY = stickY
            }

            x += bgCenterX
            y += bgCenterY

            super.setBounds(
                x.toInt() - bounds.width() / 2,
                y.toInt() - bounds.height() / 2,
                x.toInt() + bounds.width() / 2,
                y.toInt() + bounds.height() / 2,
            )

            return 1
        }

        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_CANCEL) {
            if (locked != -1 && (action == MotionEvent.ACTION_CANCEL || event.getPointerId(
                    pointerIndex
                ) == locked)
            ) {
                locked = -1

                bg.setBounds(bg.bounds.left + bgOffsetX,bg.bounds.top + bgOffsetY, bg.bounds.right + bgOffsetX, bg.bounds.bottom + bgOffsetY)
                bgOffsetY = 0
                bgOffsetX = 0

                padState.digital[pressDigitalIndex] =
                    padState.digital[pressDigitalIndex] and pressBit.inv()

                super.setBounds(bg.bounds)

                if (isLeft) {
                    padState.leftStickX = 127
                    padState.leftStickY = 127
                } else {
                    padState.rightStickX = 127
                    padState.rightStickY = 127
                }
                return -1
            }
        }

        return 0
    }

    override fun setBounds(left: Int, top: Int, right: Int, bottom: Int) {
        super.setBounds(left, top, right, bottom)
        bg.setBounds(left, top, right, bottom)
    }

    override fun setAlpha(alpha: Int) {
        super.setAlpha(alpha)
        bg.alpha = alpha
    }

    override fun draw(canvas: Canvas) {
        bg.draw(canvas)
        super.draw(canvas)
    }
}
