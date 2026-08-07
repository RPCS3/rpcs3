package net.rpcs3.overlay

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Rect
import android.graphics.Paint
import android.graphics.drawable.BitmapDrawable
import android.graphics.drawable.VectorDrawable
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.SurfaceView
import androidx.core.content.ContextCompat
import androidx.core.graphics.drawable.toBitmap
import androidx.core.graphics.scale
import net.rpcs3.Digital1Flags
import net.rpcs3.Digital2Flags
import net.rpcs3.R
import net.rpcs3.RPCS3
import kotlin.math.min



data class State(
    val digital: IntArray = IntArray(2),
    var leftStickX: Int = 127,
    var leftStickY: Int = 127,
    var rightStickX: Int = 127,
    var rightStickY: Int = 127
)

class PadOverlay(context: Context?, attrs: AttributeSet?) : SurfaceView(context, attrs) {
    private val buttons: Array<PadOverlayButton>
    private val dpad: PadOverlayDpad
    private val triangleSquareCircleCross: PadOverlayDpad
    private val state = State()
    private val leftStick: PadOverlayStick
    private val rightStick: PadOverlayStick
    private val floatingSticks = arrayOf<PadOverlayStick?>(null, null)
    private val sticks = mutableListOf<PadOverlayStick>()
    private val prefs by lazy { context!!.getSharedPreferences("PadOverlayPrefs", Context.MODE_PRIVATE) }
    private var touchSlop = 0
    private var reservedZones: List<Rect> = emptyList()

    fun r3Bounds(): Rect = Rect(sticks.last().bounds)
    private val idleAlpha by lazy {
        (255 * OverlayPrefs.getOpacity(context!!) / 100).coerceIn(0, 255)
    }
    private var selectedInput: Any? = null
        set(value) {
            field = value
            onSelectedInputChange?.invoke(value!!)
        }
        
    var onSelectedInputChange: ((Any) -> Unit)? = null
    var isEditing = false
    
    private val outlinePaint = Paint().apply {
        color = Color.RED
        style = Paint.Style.STROKE
        strokeWidth = 5f
    }

    init {
        val metrics = context!!.resources.displayMetrics
        val totalWidth = metrics.widthPixels
        val totalHeight = metrics.heightPixels
        val sizeHint = min(totalHeight, totalWidth)
        val buttonSize = sizeHint / 10

        val snap = totalWidth / 100f
        val margin = snap * 2.5f
        val bottomGap = snap * 9f

        val trigW = (snap * 10.4f).toInt()
        val trigH = (snap * 5.2f).toInt()
        val trigGap = snap * 1.5f

        val faceRadius = snap * 3f
        val spread = snap * 5.5f
        val clusterCx = totalWidth - margin - faceRadius - spread
        val clusterCy = totalHeight - bottomGap - faceRadius - spread
        val faceHalf = (spread + faceRadius)
        val faceSpan = (faceHalf * 2f).toInt()
        val faceBtn = (faceRadius * 2f).toInt()

        val dpadRadius = snap * 7.5f
        val dpadCx = margin + dpadRadius
        val dpadCy = totalHeight - bottomGap - dpadRadius
        val dpadW = (dpadRadius * 2f).toInt()
        val dpadH = dpadW
        val dpadAreaX = (dpadCx - dpadRadius).toInt()
        val dpadAreaY = (dpadCy - dpadRadius).toInt()
        val dpadArm = (dpadRadius * 0.36f * 2f).toInt()

        val btnAreaX = (clusterCx - faceHalf).toInt()
        val btnAreaY = (clusterCy - faceHalf).toInt()

        val pillW = (snap * 6f).toInt()
        val pillH = (snap * 3f).toInt()
        val pillGap = (snap * 1.2f).toInt()
        val shoulderW = trigW
        val shoulderH = trigH

        val startSelectSize = pillH
        val btnStartY = (totalHeight - bottomGap * 0.55f - pillH).toInt()
        val btnStartX = (totalWidth / 2f + pillGap / 2f).toInt()
        val btnSelectX = (totalWidth / 2f - pillGap / 2f - pillW).toInt()
        val btnSelectY = btnStartY

        val btnL2X = margin.toInt()
        val btnL2Y = margin.toInt()
        val btnL1X = btnL2X
        val btnL1Y = (margin + trigH + trigGap).toInt()

        val btnR2X = (totalWidth - margin - trigW).toInt()
        val btnR2Y = btnL2Y
        val btnR1X = btnR2X
        val btnR1Y = btnL1Y

        val btnHomeX = totalWidth / 2 - buttonSize / 2
        val btnHomeY = btnStartY - buttonSize - (snap * 1.2f).toInt()

        touchSlop = (snap * 0.55f).toInt()
        reservedZones = listOf(
            Rect(btnAreaX, btnAreaY, btnAreaX + faceSpan, btnAreaY + faceSpan),
            Rect(dpadAreaX, dpadAreaY, dpadAreaX + dpadW, dpadAreaY + dpadH),
            Rect(btnL2X, btnL2Y, btnL2X + trigW, btnL1Y + trigH),
            Rect(btnR2X, btnR2Y, btnR2X + trigW, btnR1Y + trigH)
        ).map { zone ->
            Rect(
                zone.left - touchSlop,
                zone.top - touchSlop,
                zone.right + touchSlop,
                zone.bottom + touchSlop
            )
        }

        dpad = createDpad(
            "dpad", dpadAreaX, dpadAreaY, dpadW, dpadH,
            dpadArm,
            dpadH / 2,
            0,
            R.drawable.dpad_top,
            Digital1Flags.CELL_PAD_CTRL_UP.bit,
            R.drawable.dpad_left,
            Digital1Flags.CELL_PAD_CTRL_LEFT.bit,
            R.drawable.dpad_right,
            Digital1Flags.CELL_PAD_CTRL_RIGHT.bit,
            R.drawable.dpad_bottom,
            Digital1Flags.CELL_PAD_CTRL_DOWN.bit,
            false
        )

        triangleSquareCircleCross = createDpad(
            "triangleSquareCircleCross", btnAreaX, btnAreaY, faceSpan, faceSpan,
            faceBtn,
            faceBtn,
            1,
            R.drawable.triangle,
            Digital2Flags.CELL_PAD_CTRL_TRIANGLE.bit,
            R.drawable.square,
            Digital2Flags.CELL_PAD_CTRL_SQUARE.bit,
            R.drawable.circle,
            Digital2Flags.CELL_PAD_CTRL_CIRCLE.bit,
            R.drawable.cross,
            Digital2Flags.CELL_PAD_CTRL_CROSS.bit,
            true
        )

        leftStick = PadOverlayStick(
            resources,
            true,
            PadButtonArt.stickWell(buttonSize * 2),
            PadButtonArt.stickKnob(buttonSize * 2)
        )
        rightStick = PadOverlayStick(
            resources,
            false,
            PadButtonArt.stickWell(buttonSize * 2),
            PadButtonArt.stickKnob(buttonSize * 2)
        )

        leftStick.setBounds(0, 0, buttonSize * 2, buttonSize * 2)
        leftStick.alpha = idleAlpha
        rightStick.setBounds(0, 0, buttonSize * 2, buttonSize * 2)
        rightStick.alpha = idleAlpha


        val l3r3Size = (snap * 5f).toInt()
        val l3 = PadOverlayStick(
            resources,
            true,
            PadButtonArt.stickWell(l3r3Size),
            PadButtonArt.circleButton(l3r3Size, "L3"),
            pressDigitalIndex = 0,
            pressBit = Digital1Flags.CELL_PAD_CTRL_L3.bit
        )
        l3.alpha = idleAlpha
        l3.setBounds(
            buttonSize / 2,
            totalHeight - l3r3Size - buttonSize / 2,
            buttonSize / 2 + l3r3Size,
            totalHeight - buttonSize / 2
        )

        val r3 = PadOverlayStick(
            resources,
            false,
            PadButtonArt.stickWell(l3r3Size),
            PadButtonArt.circleButton(l3r3Size, "R3"),
            pressDigitalIndex = 0,
            pressBit = Digital1Flags.CELL_PAD_CTRL_R3.bit
        )
        r3.alpha = idleAlpha
        r3.setBounds(
            totalWidth - l3r3Size - buttonSize / 2,
            totalHeight - l3r3Size - buttonSize / 2,
            totalWidth - buttonSize / 2,
            totalHeight - buttonSize / 2
        )

        sticks += l3
        sticks += r3

        buttons = arrayOf(
            createButton(
                R.drawable.start,
                btnStartX,
                btnStartY,
                pillW,
                pillH,
                Digital1Flags.CELL_PAD_CTRL_START,
                Digital2Flags.None,
                art = PadButtonArt.pillButton(pillW, pillH, "START"),
                pressedArt = PadButtonArt.pillButton(pillW, pillH, "START", pressed = true)
            ),
            createButton(
                R.drawable.select,
                btnSelectX,
                btnSelectY,
                pillW,
                pillH,
                Digital1Flags.CELL_PAD_CTRL_SELECT,
                Digital2Flags.None,
                art = PadButtonArt.pillButton(pillW, pillH, "SELECT"),
                pressedArt = PadButtonArt.pillButton(pillW, pillH, "SELECT", pressed = true)
            ),

            createButton(
                R.mipmap.ic_launcher_round,
                btnHomeX,
                btnHomeY,
                buttonSize,
                buttonSize,
                Digital1Flags.CELL_PAD_CTRL_PS,
                Digital2Flags.None
            ),
            createButton(
                R.drawable.l1,
                btnL1X,
                btnL1Y,
                shoulderW,
                shoulderH,
                Digital1Flags.None,
                Digital2Flags.CELL_PAD_CTRL_L1,
                art = PadButtonArt.shoulderButton(shoulderW, shoulderH, "L1", true, true),
                pressedArt = PadButtonArt.shoulderButton(shoulderW, shoulderH, "L1", true, true, pressed = true)
            ),
            createButton(
                R.drawable.l2,
                btnL2X,
                btnL2Y,
                shoulderW,
                shoulderH,
                Digital1Flags.None,
                Digital2Flags.CELL_PAD_CTRL_L2,
                art = PadButtonArt.shoulderButton(shoulderW, shoulderH, "L2", true, false),
                pressedArt = PadButtonArt.shoulderButton(shoulderW, shoulderH, "L2", true, false, pressed = true)
            ),
            createButton(
                R.drawable.r1,
                btnR1X,
                btnR1Y,
                shoulderW,
                shoulderH,
                Digital1Flags.None,
                Digital2Flags.CELL_PAD_CTRL_R1,
                art = PadButtonArt.shoulderButton(shoulderW, shoulderH, "R1", false, true),
                pressedArt = PadButtonArt.shoulderButton(shoulderW, shoulderH, "R1", false, true, pressed = true)
            ),
            createButton(
                R.drawable.r2,
                btnR2X,
                btnR2Y,
                shoulderW,
                shoulderH,
                Digital1Flags.None,
                Digital2Flags.CELL_PAD_CTRL_R2,
                art = PadButtonArt.shoulderButton(shoulderW, shoulderH, "R2", false, false),
                pressedArt = PadButtonArt.shoulderButton(shoulderW, shoulderH, "R2", false, false, pressed = true)
            ),
        )

        setWillNotDraw(false)
        requestFocus()

        setOnTouchListener { _, motionEvent ->
            var hit = false

            val action = motionEvent.actionMasked
            val pointerIndex =
                if (action == MotionEvent.ACTION_POINTER_DOWN || action == MotionEvent.ACTION_POINTER_UP) motionEvent.actionIndex else 0
            val x = motionEvent.getX(pointerIndex).toInt()
            val y = motionEvent.getY(pointerIndex).toInt()

            if (isEditing) {
                when (action) {
                    MotionEvent.ACTION_DOWN -> {
                        buttons.forEach { button ->
                            if (button.contains(x, y)) {
                                selectedInput = button
                                button.startDragging(x, y)
                                hit = true
                            }
                        }
                        if (dpad.contains(x, y)) {
                            selectedInput = dpad
                            dpad.startDragging(x, y)
                            hit = true
                        }
                        if (triangleSquareCircleCross.contains(x, y)) {
                            selectedInput = triangleSquareCircleCross
                            triangleSquareCircleCross.startDragging(x, y)
                            hit = true
                        }
                    }
                    MotionEvent.ACTION_MOVE -> {
                        buttons.forEach { button ->
                            if (button.dragging) {
                                button.updatePosition(x, y)
                                hit = true
                            }
                        }
                        if (dpad.dragging) {
                            dpad.updatePosition(x, y)
                            hit = true
                        }
                        if (triangleSquareCircleCross.contains(x, y)) {
                            triangleSquareCircleCross.updatePosition(x, y)
                            hit = true
                        }
                    }
                    MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                        buttons.forEach { button ->
                            button.stopDragging()
                        }
                        dpad.stopDragging()
                        triangleSquareCircleCross.stopDragging()
                    }
                }
                if (hit) invalidate()
                return@setOnTouchListener true
            }
            
            val force =
                action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_CANCEL || action == MotionEvent.ACTION_MOVE
            if (force || dpad.contains(x, y)) {
                hit = dpad.onTouch(motionEvent, pointerIndex, state)
            }

            if (force || (!hit && triangleSquareCircleCross.contains(x, y))
            ) {
                hit = triangleSquareCircleCross.onTouch(motionEvent, pointerIndex, state)
            }

            buttons.forEach { button ->
                if (force || (!hit && button.contains(x, y, touchSlop))) {
                    hit = button.onTouch(motionEvent, pointerIndex, state)
                }
            }

            if (force || !hit) {
                for (i in sticks.indices) {
                    if (!force && (!sticks[i].contains(x, y) || floatingSticks[i] != null)) {
                        continue
                    }

                    val touchResult = sticks[i].onTouch(motionEvent, pointerIndex, state)
                    hit = if (touchResult < 0) {
                        true
                    } else {
                        touchResult == 1
                    }
                }
            }

            if (force || !hit) {
                for (i in floatingSticks.indices) {
                    val stick = floatingSticks[i] ?: continue
                    val touchResult = stick.onTouch(motionEvent, pointerIndex, state)
                    if (touchResult < 0) {
                        floatingSticks[i] = null
                        hit = true
                    } else {
                        hit = touchResult == 1
                    }
                }
            }

            RPCS3.instance.overlayPadData(
                state.digital[0],
                state.digital[1],
                state.leftStickX,
                state.leftStickY,
                state.rightStickX,
                state.rightStickY
            )

            if (!hit && (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN)) {
                val xInFloatingArea = x > buttonSize * 2 && x < totalWidth - buttonSize * 2
                val yInFloatingArea = y > buttonSize && y < totalHeight - buttonSize
                var inFloatingArea = xInFloatingArea && yInFloatingArea
                if (!inFloatingArea && yInFloatingArea) {
                    if (x > buttonSize && x <= buttonSize * 2) {
                        inFloatingArea = true
                    }

                    if (x <= totalWidth - buttonSize && x >= totalWidth - buttonSize * 2) {
                        inFloatingArea = true
                    }
                }

                if (inFloatingArea && reservedZones.none { it.contains(x, y) }) {
                    val stickIndex = if (x <= totalWidth / 2) 0 else 1
                    val stick = if (stickIndex == 0) leftStick else rightStick

                    if (floatingSticks[stickIndex] == null && !sticks[stickIndex].isActive()) {
                        floatingSticks[stickIndex] = stick
                        stick.onAdd(motionEvent, pointerIndex)
                        hit = true
                    }
                }
            }

            if (hit || force) {
                invalidate()
            }

            hit || performClick()
        }
    }

    override fun draw(canvas: Canvas) {
        super.draw(canvas)
        buttons.forEach { button -> button.draw(canvas) }
        dpad.draw(canvas)
        triangleSquareCircleCross.draw(canvas)
        sticks.forEach { it.draw(canvas) }
        floatingSticks.forEach { it?.draw(canvas) }

        if (isEditing && selectedInput != null) {
            val bounds = when (selectedInput) {
                is PadOverlayButton -> (selectedInput as PadOverlayButton).bounds
                is PadOverlayDpad -> (selectedInput as PadOverlayDpad).getBounds()
                else -> null
            }

            bounds?.let {
                val rect = Rect(it.left, it.top, it.right, it.bottom)
                canvas.drawRect(rect, outlinePaint)
            }
        }
    }

    private fun getBitmap(resourceId: Int, width: Int, height: Int): Bitmap {
        return when (val drawable = ContextCompat.getDrawable(context, resourceId)) {
            is BitmapDrawable -> {
                BitmapFactory.decodeResource(context.resources, resourceId).scale(width, height)
            }

            is VectorDrawable -> {
                drawable.toBitmap(width, height)
            }

            else -> {
                throw IllegalArgumentException("unexpected drawable type")
            }
        }
    }

    private fun createButton(
        resourceId: Int,
        x: Int,
        y: Int,
        width: Int,
        height: Int,
        digital1: Digital1Flags,
        digital2: Digital2Flags,
        art: Bitmap? = null,
        pressedArt: Bitmap? = null
    ): PadOverlayButton {
        val resources = context!!.resources
        val bitmap = art ?: getBitmap(resourceId, width, height)
        val result = PadOverlayButton(context!!, resources, bitmap, digital1.bit, digital2.bit, pressedArt)
        val scale = prefs.getInt("button_${digital1.bit}_${digital2.bit}_scale", 0)
        val alpha = prefs.getInt(OverlayPrefs.OPACITY_KEY, OverlayPrefs.DEFAULT_OPACITY)
        val savedX = prefs.getInt("button_${digital1.bit}_${digital2.bit}_x", x)
        val savedY = prefs.getInt("button_${digital1.bit}_${digital2.bit}_y", y)
        result.setBounds(savedX, savedY, savedX + width, savedY + height)
        result.defaultPosition = Pair(x, y)
        result.defaultSize = Pair(height, width)
        if (scale != 0) result.setScale(scale)
        result.setOpacity(alpha)
        return result
    }

    private fun createDpad(
        inputId: String,
        x: Int,
        y: Int,
        width: Int,
        height: Int,
        buttonWidth: Int,
        buttonHeight: Int,
        digital: Int,
        upResource: Int, upBit: Int,
        leftResource: Int, leftBit: Int,
        rightResource: Int, rightBit: Int,
        downResource: Int, downBit: Int,
        multitouch: Boolean
    ): PadOverlayDpad {
        val faceGroup = inputId == "triangleSquareCircleCross"
        val isDpad = inputId == "dpad"
        val area = Rect(x, y, x + width, y + height)
        val upBitmap = if (faceGroup) {
            PadButtonArt.faceButton(buttonWidth, PadButtonArt.TRIANGLE_COLOR, PadSymbol.Triangle)
        } else if (isDpad) {
            PadButtonArt.dpadPressHighlight(buttonWidth, buttonHeight, buttonWidth * 0.26f)
        } else {
            getBitmap(upResource, buttonWidth, buttonHeight)
        }
        val leftBitmap = if (faceGroup) {
            PadButtonArt.faceButton(buttonWidth, PadButtonArt.SQUARE_COLOR, PadSymbol.Square)
        } else if (isDpad) {
            PadButtonArt.dpadPressHighlight(buttonHeight, buttonWidth, buttonWidth * 0.26f)
        } else {
            getBitmap(leftResource, buttonHeight, buttonWidth)
        }
        val rightBitmap = if (faceGroup) {
            PadButtonArt.faceButton(buttonWidth, PadButtonArt.CIRCLE_COLOR, PadSymbol.Circle)
        } else if (isDpad) {
            PadButtonArt.dpadPressHighlight(buttonHeight, buttonWidth, buttonWidth * 0.26f)
        } else {
            getBitmap(rightResource, buttonHeight, buttonWidth)
        }
        val downBitmap = if (faceGroup) {
            PadButtonArt.faceButton(buttonWidth, PadButtonArt.CROSS_COLOR, PadSymbol.Cross)
        } else if (isDpad) {
            PadButtonArt.dpadPressHighlight(buttonWidth, buttonHeight, buttonWidth * 0.26f)
        } else {
            getBitmap(downResource, buttonWidth, buttonHeight)
        }

        val result = PadOverlayDpad(
            context, resources, buttonWidth, buttonHeight, inputId, Rect(x, y, x + width, y + height), digital,
            upBitmap, upBit,
            leftBitmap, leftBit,
            rightBitmap, rightBit,
            downBitmap, downBit,
            multitouch,
            if (isDpad) {
                PadButtonArt.dpadCross(area.width(), area.height(), buttonWidth.toFloat())
            } else {
                null
            },
            if (faceGroup) {
                PadButtonArt.faceButton(buttonWidth, PadButtonArt.TRIANGLE_COLOR, PadSymbol.Triangle, true)
            } else {
                null
            },
            if (faceGroup) {
                PadButtonArt.faceButton(buttonWidth, PadButtonArt.SQUARE_COLOR, PadSymbol.Square, true)
            } else {
                null
            },
            if (faceGroup) {
                PadButtonArt.faceButton(buttonWidth, PadButtonArt.CIRCLE_COLOR, PadSymbol.Circle, true)
            } else {
                null
            },
            if (faceGroup) {
                PadButtonArt.faceButton(buttonWidth, PadButtonArt.CROSS_COLOR, PadSymbol.Cross, true)
            } else {
                null
            },
            faceGroup
        )

        result.idleAlpha = idleAlpha
        return result
    }

    fun setButtonScale(value: Int) {
        (selectedInput as? PadOverlayDpad)?.setScale(value)
        ?: (selectedInput as? PadOverlayButton)?.setScale(value)
        invalidate()
    }

    fun setButtonOpacity(value: Int) {
        (selectedInput as? PadOverlayDpad)?.setOpacity(value)
        ?: (selectedInput as? PadOverlayButton)?.setOpacity(value)
        invalidate()
    }

    fun resetButtonConfigs() {
        (selectedInput as? PadOverlayDpad)?.resetConfigs()
        ?: (selectedInput as? PadOverlayButton)?.resetConfigs()
        invalidate()
        onSelectedInputChange?.invoke(selectedInput!!)
    }

    fun moveButtonLeft() {
        val bounds = (selectedInput as? PadOverlayDpad)?.getBounds() 
        ?: (selectedInput as? PadOverlayButton)?.bounds
        if (bounds != null) {
            (selectedInput as? PadOverlayDpad)?.updatePosition(bounds.left - 1, bounds.top, true)
            ?: (selectedInput as? PadOverlayButton)?.updatePosition(bounds.left - 1, bounds.top, true)
            invalidate()
        }
    }

    fun moveButtonRight() {
        val bounds = (selectedInput as? PadOverlayDpad)?.getBounds() 
        ?: (selectedInput as? PadOverlayButton)?.bounds
        if (bounds != null) {
            (selectedInput as? PadOverlayDpad)?.updatePosition(bounds.left + 1, bounds.top, true)
            ?: (selectedInput as? PadOverlayButton)?.updatePosition(bounds.left + 1, bounds.top, true)
            invalidate()
        }
    }

    fun moveButtonUp() {
        val bounds = (selectedInput as? PadOverlayDpad)?.getBounds() 
        ?: (selectedInput as? PadOverlayButton)?.bounds
        if (bounds != null) {
            (selectedInput as? PadOverlayDpad)?.updatePosition(bounds.left, bounds.top - 1, true)
            ?: (selectedInput as? PadOverlayButton)?.updatePosition(bounds.left, bounds.top - 1, true)
            invalidate()
        }
    }

    fun moveButtonDown() {
        val bounds = (selectedInput as? PadOverlayDpad)?.getBounds() 
        ?: (selectedInput as? PadOverlayButton)?.bounds
        if (bounds != null) {
            (selectedInput as? PadOverlayDpad)?.updatePosition(bounds.left, bounds.top + 1, true)
            ?: (selectedInput as? PadOverlayButton)?.updatePosition(bounds.left, bounds.top + 1, true)
            invalidate()
        }
    }
}
