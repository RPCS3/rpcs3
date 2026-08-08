package net.rpcs3

import androidx.activity.ComponentActivity
import android.os.Handler
import android.os.Looper
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.InputDevice
import android.view.View
import androidx.compose.runtime.mutableStateOf
import net.rpcs3.ui.drawer.InGameDrawer
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.ViewGroup.MarginLayoutParams
import android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.core.view.isInvisible
import androidx.core.view.updateLayoutParams
import net.rpcs3.databinding.ActivityRpcs3Binding
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.overlay.State
import kotlin.concurrent.thread
import kotlin.math.abs

class RPCS3Activity : ComponentActivity() {
    private lateinit var binding: ActivityRpcs3Binding
    private lateinit var unregisterUsbEventListener: () -> Unit
    private var isoDescriptor: android.os.ParcelFileDescriptor? = null
    private var gamePadState: State = State()
    private var usesAxisL2 = false
    private var usesAxisR2 = false
    private var bootThread: Thread? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityRpcs3Binding.inflate(layoutInflater)
        setContentView(binding.root)

        unregisterUsbEventListener = listenUsbEvents(this)
        enableFullScreenImmersive()

        RPCS3.onEmulationStopped = {
            runOnUiThread {
                if (!isFinishing && !isDestroyed) {
                    finish()
                }
            }
        }

        binding.drawerHost.setContent {
            RPCS3Theme {
                InGameDrawer(
                    visible = drawerVisible.value,
                    titleId = gameTitleId(intent.getStringExtra("path").orEmpty()),
                    paused = drawerPaused.value,
                    onDismiss = { setDrawerVisible(false) },
                    onTogglePause = {
                        val wasPaused = drawerPaused.value
                        drawerPaused.value = !wasPaused
                        if (wasPaused) {
                            setDrawerVisible(false)
                            thread { RPCS3.instance.resume() }
                        } else {
                            thread { RPCS3.instance.pause() }
                        }
                    },
                    onExit = {
                        setDrawerVisible(false)
                        thread {
                            RPCS3.instance.kill()

                            var waited = 0
                            while (RPCS3.getState() != EmulatorState.Stopped && waited < 15000) {
                                Thread.sleep(100)
                                waited += 100
                            }

                            RPCS3.activeGame.value = null
                            RPCS3.state.value = EmulatorState.Stopped
                            runOnUiThread { finish() }
                        }
                    }
                )
            }
        }

        binding.oscToggle.setOnClickListener {
            binding.padOverlay.isInvisible = !binding.padOverlay.isInvisible
            binding.oscToggle.setImageResource(if (binding.padOverlay.isInvisible) R.drawable.ic_osc_off else R.drawable.ic_show_osc)
            wakeToggle()
        }

        binding.oscToggle.setOnTouchListener { _, _ ->
            wakeToggle()
            false
        }

        binding.oscToggle.post { placeToggleBesideR3() }

        val isoUriString = intent.getStringExtra("isoUri")
        val gamePath = intent.getStringExtra("path") ?: isoUriString!!

        if (isoUriString != null) {
            isoDescriptor = runCatching {
                contentResolver.openFileDescriptor(Uri.parse(isoUriString), "r")
            }.getOrNull()

            if (isoDescriptor == null) {
                AlertDialogQueue.showDialog("Boot Failed", "Could not open the selected disc image")
                finish()
                return
            }
        }


        bootThread = thread {
            if (RPCS3.getState() != EmulatorState.Stopped) {
                val state = RPCS3.getState()
                Log.w("RPCS3 State", state.name)

                if (state == EmulatorState.Paused && RPCS3.activeGame.value == gamePath) {
                    RPCS3.instance.resume()
                    return@thread
                }

                if (RPCS3.getState() != EmulatorState.Stopping && RPCS3.getState() != EmulatorState.Stopped) {
                    RPCS3.instance.kill()

                    while (RPCS3.getState() != EmulatorState.Stopped) {
                        Thread.sleep(300)
                        if (Thread.interrupted()) {
                            return@thread
                        }
                    }
                }
            }

            Log.w("RPCS3 State", RPCS3.getState().name)
            RPCS3.activeGame.value = gamePath

            val descriptor = isoDescriptor
            val bootResult = if (descriptor != null) {
                BootResult.fromInt(RPCS3.instance.bootIso(descriptor.fd))
            } else {
                RPCS3.boot(gamePath)
            }
            if (bootResult != BootResult.NoErrors) {
                AlertDialogQueue.showDialog("Boot Failed", "Error: ${bootResult.name}")
                finish()
            }
        }
    }

    private val toggleHandler = Handler(Looper.getMainLooper())
    private val dimToggle = Runnable {
        binding.oscToggle.animate().alpha(0.25f).setDuration(400).start()
    }

    private fun wakeToggle() {
        toggleHandler.removeCallbacks(dimToggle)
        binding.oscToggle.animate().alpha(1f).setDuration(120).start()
        toggleHandler.postDelayed(dimToggle, 3000)
    }

    private fun placeToggleBesideR3() {
        val r3 = binding.padOverlay.r3Bounds()
        if (r3.width() <= 0) {
            binding.oscToggle.postDelayed({ placeToggleBesideR3() }, 200)
            return
        }

        val toggle = binding.oscToggle
        val gap = (r3.width() * 0.22f).toInt()
        toggle.x = (r3.left - gap - toggle.width).toFloat()
        toggle.y = (r3.centerY() - toggle.height / 2).toFloat()
        wakeToggle()
    }

    private val drawerVisible = mutableStateOf(false)
    private val drawerPaused = mutableStateOf(false)

    private fun setDrawerVisible(visible: Boolean) {
        if (visible) {
            drawerPaused.value = RPCS3.getState() == EmulatorState.Paused
            binding.drawerHost.visibility = View.VISIBLE
            drawerVisible.value = true
            return
        }

        drawerVisible.value = false
        binding.drawerHost.postDelayed({
            if (!drawerVisible.value) {
                binding.drawerHost.visibility = View.GONE
            }
        }, 260)
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() {
        if (drawerVisible.value) {
            setDrawerVisible(false)
            return
        }

        if (RPCS3.getState() != EmulatorState.Stopped) {
            setDrawerVisible(true)
            return
        }

        super.onBackPressed()
    }

    override fun onPause() {
        super.onPause()
        RPCS3.instance.settingsFlush()
    }

    override fun onDestroy() {
        super.onDestroy()
        RPCS3.onEmulationStopped = null
        if (RPCS3.getState() != EmulatorState.Stopped) {
            RPCS3.state.value = EmulatorState.Paused
        } else {
            RPCS3.activeGame.value = null
        }
        unregisterUsbEventListener()
        bootThread?.interrupt()
        bootThread?.join()
        runCatching { isoDescriptor?.close() }
        isoDescriptor = null
    }


    private fun keyCodeToPadBit(keyCode: Int): Pair<Int, Int> {
        when (keyCode) {
            KeyEvent.KEYCODE_DPAD_UP -> return Pair(Digital1Flags.CELL_PAD_CTRL_UP.bit, 0)
            KeyEvent.KEYCODE_DPAD_DOWN -> return Pair(Digital1Flags.CELL_PAD_CTRL_DOWN.bit, 0)
            KeyEvent.KEYCODE_DPAD_LEFT -> return Pair(Digital1Flags.CELL_PAD_CTRL_LEFT.bit, 0)
            KeyEvent.KEYCODE_DPAD_RIGHT -> return Pair(Digital1Flags.CELL_PAD_CTRL_RIGHT.bit, 0)
            KeyEvent.KEYCODE_BUTTON_A -> return Pair(Digital2Flags.CELL_PAD_CTRL_CROSS.bit, 1)
            KeyEvent.KEYCODE_BUTTON_B -> return Pair(Digital2Flags.CELL_PAD_CTRL_CIRCLE.bit, 1)
            KeyEvent.KEYCODE_BUTTON_X -> return Pair(Digital2Flags.CELL_PAD_CTRL_SQUARE.bit, 1)
            KeyEvent.KEYCODE_BUTTON_Y -> return Pair(Digital2Flags.CELL_PAD_CTRL_TRIANGLE.bit, 1)
            KeyEvent.KEYCODE_BUTTON_L1 -> return Pair(Digital2Flags.CELL_PAD_CTRL_L1.bit, 1)
            KeyEvent.KEYCODE_BUTTON_R1 -> return Pair(Digital2Flags.CELL_PAD_CTRL_R1.bit, 1)
            KeyEvent.KEYCODE_BUTTON_L2 -> return if (usesAxisL2) Pair(
                0,
                0
            ) else Pair(Digital2Flags.CELL_PAD_CTRL_L2.bit, 1)

            KeyEvent.KEYCODE_BUTTON_R2 -> return if (usesAxisR2) Pair(
                0,
                0
            ) else Pair(Digital2Flags.CELL_PAD_CTRL_R2.bit, 1)

            KeyEvent.KEYCODE_BUTTON_START -> return Pair(Digital1Flags.CELL_PAD_CTRL_START.bit, 0)
            KeyEvent.KEYCODE_BUTTON_SELECT -> return Pair(Digital1Flags.CELL_PAD_CTRL_SELECT.bit, 0)
            KeyEvent.KEYCODE_BUTTON_THUMBL -> return Pair(Digital1Flags.CELL_PAD_CTRL_L3.bit, 0)
            KeyEvent.KEYCODE_BUTTON_THUMBR -> return Pair(Digital1Flags.CELL_PAD_CTRL_R3.bit, 0)
        }

        return Pair(0, 0)
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        if (event == null || (event.source and (InputDevice.SOURCE_GAMEPAD or InputDevice.SOURCE_JOYSTICK or InputDevice.SOURCE_DPAD)) == 0 || event.repeatCount != 0) {
            return super.onKeyDown(keyCode, event)
        }
        val padBit = keyCodeToPadBit(keyCode)
        if (padBit.first == 0) {
            return super.onKeyDown(keyCode, event)
        }

        gamePadState.digital[padBit.second] = gamePadState.digital[padBit.second] or padBit.first
        sendGamepadData()
        return true
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent?): Boolean {
        if (event == null || event.source and (InputDevice.SOURCE_GAMEPAD or InputDevice.SOURCE_JOYSTICK or InputDevice.SOURCE_DPAD) == 0) {
            return super.onKeyUp(keyCode, event)
        }

        val padBit = keyCodeToPadBit(keyCode)
        if (padBit.first == 0) {
            return super.onKeyUp(keyCode, event)
        }

        gamePadState.digital[padBit.second] =
            gamePadState.digital[padBit.second] and padBit.first.inv()
        sendGamepadData()
        return true
    }

    override fun onGenericMotionEvent(event: MotionEvent?): Boolean {
        if (event == null || event.source and InputDevice.SOURCE_JOYSTICK != InputDevice.SOURCE_JOYSTICK || event.action != MotionEvent.ACTION_MOVE) {
            return super.onGenericMotionEvent(event)
        }

        if (event.getAxisValue(MotionEvent.AXIS_LTRIGGER) > 0.1) {
            gamePadState.digital[1] =
                gamePadState.digital[1] or Digital2Flags.CELL_PAD_CTRL_L2.bit
            usesAxisL2 = true
        } else if (usesAxisL2) {
            usesAxisL2 = false
            gamePadState.digital[1] =
                gamePadState.digital[1] and Digital2Flags.CELL_PAD_CTRL_L2.bit.inv()
        }

        if (event.getAxisValue(MotionEvent.AXIS_RTRIGGER) > 0.1) {
            gamePadState.digital[1] =
                gamePadState.digital[1] or Digital2Flags.CELL_PAD_CTRL_R2.bit
            usesAxisR2 = true
        } else if (usesAxisR2) {
            usesAxisR2 = false
            gamePadState.digital[1] =
                gamePadState.digital[1] and Digital2Flags.CELL_PAD_CTRL_R2.bit.inv()
        }

        val dpadX = event.getAxisValue(MotionEvent.AXIS_HAT_X)
        val dpadY = event.getAxisValue(MotionEvent.AXIS_HAT_Y)

        gamePadState.digital[0] =
            gamePadState.digital[0] and (Digital1Flags.CELL_PAD_CTRL_LEFT.bit or Digital1Flags.CELL_PAD_CTRL_RIGHT.bit or Digital1Flags.CELL_PAD_CTRL_UP.bit or Digital1Flags.CELL_PAD_CTRL_DOWN.bit).inv()
        if (abs(dpadX) > 0.1f) {
            if (dpadX < 0) {
                gamePadState.digital[0] =
                    gamePadState.digital[0] or Digital1Flags.CELL_PAD_CTRL_LEFT.bit
            } else {
                gamePadState.digital[0] =
                    gamePadState.digital[0] or Digital1Flags.CELL_PAD_CTRL_RIGHT.bit
            }
        }

        if (abs(dpadY) > 0.1f) {
            if (dpadY < 0) {
                gamePadState.digital[0] =
                    gamePadState.digital[0] or Digital1Flags.CELL_PAD_CTRL_UP.bit
            } else {
                gamePadState.digital[0] =
                    gamePadState.digital[0] or Digital1Flags.CELL_PAD_CTRL_DOWN.bit
            }
        }

        gamePadState.leftStickX = (event.getAxisValue(MotionEvent.AXIS_X) * 127 + 128).toInt()
        gamePadState.leftStickY = (event.getAxisValue(MotionEvent.AXIS_Y) * 127 + 128).toInt()
        gamePadState.rightStickX = (event.getAxisValue(MotionEvent.AXIS_Z) * 127 + 128).toInt()
        gamePadState.rightStickY = (event.getAxisValue(MotionEvent.AXIS_RZ) * 127 + 128).toInt()

        sendGamepadData()
        return true
    }

    private fun sendGamepadData() {
        RPCS3.instance.overlayPadData(
            gamePadState.digital[0],
            gamePadState.digital[1],
            gamePadState.leftStickX,
            gamePadState.leftStickY,
            gamePadState.rightStickX,
            gamePadState.rightStickY
        )
    }

    private fun enableFullScreenImmersive() {
        with(window) {
            WindowCompat.setDecorFitsSystemWindows(this, false)
            val insetsController = WindowInsetsControllerCompat(this, decorView)
            insetsController.apply {
                hide(WindowInsetsCompat.Type.systemBars())
                systemBarsBehavior =
                    WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
            attributes.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
        }
        applyInsetsToPadOverlay()
    }

    private fun applyInsetsToPadOverlay() {
        ViewCompat.setOnApplyWindowInsetsListener(binding.padOverlay) { view, windowInsets ->
            // I don't think we need `displayCutout` insets here as well
            // Since there is hardly any overlay overlapping with it
            val insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            view.updateLayoutParams<MarginLayoutParams> {
                leftMargin = insets.left
                rightMargin = insets.right
                topMargin = insets.top
                bottomMargin = insets.bottom
            }
            WindowInsetsCompat.CONSUMED
        }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) enableFullScreenImmersive()
    }
}
