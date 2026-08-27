package net.rpcs3

import androidx.activity.ComponentActivity
import android.hardware.display.DisplayManager
import android.os.Handler
import android.os.Looper
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.InputDevice
import android.view.View
import androidx.compose.runtime.mutableStateOf
import net.rpcs3.framegen.FrameGen
import net.rpcs3.framegen.FrameGenPrefs
import net.rpcs3.ui.drawer.InGameDrawer
import net.rpcs3.ui.hud.DeviceStatsReader
import net.rpcs3.ui.hud.HudPrefs
import net.rpcs3.overlay.PS_HOLD_MS
import org.json.JSONObject
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.ViewGroup.MarginLayoutParams
import android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.core.view.isVisible
import androidx.core.view.updateLayoutParams
import net.rpcs3.databinding.ActivityRpcs3Binding
import net.rpcs3.dialogs.AlertDialogQueue
import net.rpcs3.overlay.OverlayPrefs
import net.rpcs3.overlay.State
import net.rpcs3.utils.DriverFlags
import net.rpcs3.utils.DriverSelection
import net.rpcs3.utils.GameDetailsReader
import kotlin.concurrent.thread
import kotlin.math.abs

private const val HUD_REFRESH_MS = 500L
private const val HUD_GRAPH_MS = 50L

class RPCS3Activity : ComponentActivity() {
    private lateinit var binding: ActivityRpcs3Binding
    private lateinit var unregisterUsbEventListener: () -> Unit
    private var isoDescriptor: android.os.ParcelFileDescriptor? = null
    private var gamePadState: State = State()
    private var overlayPadState: State = State()
    private var usesAxisL2 = false
    private var usesAxisR2 = false
    private var bootThread: Thread? = null
    private val hudHandler = Handler(Looper.getMainLooper())
    private var hudPump: Runnable? = null
    private var hudGraphPump: Runnable? = null
    private var hudPrefsListener:
        android.content.SharedPreferences.OnSharedPreferenceChangeListener? = null
    private var overlayPrefsListener:
        android.content.SharedPreferences.OnSharedPreferenceChangeListener? = null
    private var framegenPrefsListener:
        android.content.SharedPreferences.OnSharedPreferenceChangeListener? = null
    private var displayListener: DisplayManager.DisplayListener? = null
    private val modeHoldRunnable = Runnable {
        setDrawerVisible(!drawerVisible.value)
    }
    private val stopHandler = Handler(Looper.getMainLooper())
    private var pendingStopCheck: Runnable? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityRpcs3Binding.inflate(layoutInflater)
        setContentView(binding.root)

        unregisterUsbEventListener = listenUsbEvents(this)
        enableFullScreenImmersive()

        FrameGen.sync(this)
        startFrameGenDisplay()

        RPCS3.onEmulationStopped = {
            runOnUiThread { scheduleStopCheck() }
        }

        binding.drawerHost.setContent {
            RPCS3Theme {
                InGameDrawer(
                    visible = drawerVisible.value,
                    titleId = drawerTitleId.value,
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
                            while (RPCS3.getState().isEmulationActive && waited < 15000) {
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

        binding.padOverlay.onPsHold = {
            runOnUiThread {
                if (!drawerVisible.value) {
                    setDrawerVisible(true)
                }
            }
        }

        binding.padOverlay.onPadState = { state ->
            overlayPadState = state
            sendGamepadData()
        }

        startHud()
        startOverlay()

        binding.oscToggle.setOnClickListener {
            OverlayPrefs.setEnabled(this, !OverlayPrefs.isEnabled(this))
            applyOverlayPreferences()
            wakeToggle()
        }

        binding.oscToggle.setOnTouchListener { _, _ ->
            wakeToggle()
            false
        }

        binding.oscToggle.post { placeToggleBesideR3() }

        val isoUriString = intent.getStringExtra("isoUri")
        val gamePath = intent.getStringExtra("path") ?: isoUriString!!
        val bootPath = intent.getStringExtra("bootPath") ?: gamePath

        if (isoUriString != null) {
            isoDescriptor = runCatching {
                contentResolver.openFileDescriptor(Uri.parse(isoUriString), "r")
            }.getOrNull()

            if (isoDescriptor == null) {
                AlertDialogQueue.showDialog(
                    getString(R.string.boot_failed_title),
                    getString(R.string.boot_failed_open_image)
                )
                finish()
                return
            }
        }


        bootThread = thread {
            val state = RPCS3.getState()
            if (state.isEmulationActive) {
                Log.w("RPCS3 State", state.name)

                if (state == EmulatorState.Paused && RPCS3.activeGame.value == gamePath) {
                    RPCS3.instance.resume()
                    return@thread
                }

                val current = RPCS3.getState()
                if (current.isEmulationActive && current != EmulatorState.Stopping) {
                    RPCS3.instance.kill()

                    while (RPCS3.getState().isEmulationActive) {
                        Thread.sleep(300)
                        if (Thread.interrupted()) {
                            return@thread
                        }
                    }
                }
            }

            Log.w("RPCS3 State", RPCS3.getState().name)
            RPCS3.activeGame.value = gamePath

            val bootTitleId = GameDetailsReader.read(bootPath).titleId
            DriverSelection.apply(this@RPCS3Activity, bootTitleId)
            DriverFlags.apply(this@RPCS3Activity, bootTitleId)

            val descriptor = isoDescriptor
            val bootResult = if (descriptor != null) {
                BootResult.fromInt(RPCS3.instance.bootIso(descriptor.fd))
            } else {
                RPCS3.boot(bootPath)
            }
            if (bootResult != BootResult.NoErrors) {
                AlertDialogQueue.showDialog(
                    getString(R.string.boot_failed_title),
                    getString(R.string.boot_failed_error, bootResult.name)
                )
                finish()
            }
        }
    }

    private fun scheduleStopCheck() {
        pendingStopCheck?.let { stopHandler.removeCallbacks(it) }

        val check = Runnable {
            pendingStopCheck = null
            if (!isFinishing && !isDestroyed && !RPCS3.getState().isEmulationActive) {
                finish()
            }
        }

        pendingStopCheck = check
        stopHandler.postDelayed(check, EMULATION_STOP_GRACE_MS)
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
    private val drawerTitleId = mutableStateOf("")

    private fun resolveTitleId(): String {
        val running = runCatching { RPCS3.instance.getTitleId() }.getOrNull().orEmpty()
        if (running.isNotEmpty()) {
            return running
        }

        val path = intent.getStringExtra("path").orEmpty()
        val fromDetails = runCatching { GameDetailsReader.read(path).titleId }.getOrNull().orEmpty()
        if (fromDetails.isNotEmpty()) {
            return fromDetails
        }

        return gameTitleId(path)
    }

    private fun releaseAllPadInput() {
        gamePadState.digital[0] = 0
        gamePadState.digital[1] = 0
        gamePadState.leftStickX = 127
        gamePadState.leftStickY = 127
        gamePadState.rightStickX = 127
        gamePadState.rightStickY = 127
        usesAxisL2 = false
        usesAxisR2 = false
        binding.padOverlay.releaseAllInput()
        sendGamepadData()
    }

    private fun startOverlay() {
        val prefs = OverlayPrefs.of(this)

        overlayPrefsListener =
            android.content.SharedPreferences.OnSharedPreferenceChangeListener { _, key ->
                if (key == OverlayPrefs.ENABLED_KEY) {
                    runOnUiThread { applyOverlayPreferences() }
                }
            }
        prefs.registerOnSharedPreferenceChangeListener(overlayPrefsListener)

        applyOverlayPreferences()
    }

    private fun stopOverlay() {
        overlayPrefsListener?.let {
            OverlayPrefs.of(this).unregisterOnSharedPreferenceChangeListener(it)
        }
        overlayPrefsListener = null
    }

    private fun applyOverlayPreferences() {
        val enabled = OverlayPrefs.isEnabled(this)

        if (binding.padOverlay.isVisible != enabled) {
            binding.padOverlay.isVisible = enabled
        }

        binding.oscToggle.setImageResource(
            if (enabled) R.drawable.ic_show_osc else R.drawable.ic_osc_off
        )

        if (enabled) {
            binding.padOverlay.invalidate()
        }
    }

    private fun startFrameGenDisplay() {
        val prefs = FrameGenPrefs.of(this)

        framegenPrefsListener =
            android.content.SharedPreferences.OnSharedPreferenceChangeListener { _, _ ->
                runOnUiThread { applyFrameGenDisplayMode() }
            }
        prefs.registerOnSharedPreferenceChangeListener(framegenPrefsListener)

        val listener = object : DisplayManager.DisplayListener {
            override fun onDisplayAdded(displayId: Int) {}

            override fun onDisplayRemoved(displayId: Int) {}

            override fun onDisplayChanged(displayId: Int) {
                if (displayId == display?.displayId) {
                    FrameGen.pushRefreshRate(display?.refreshRate ?: 0f)
                }
            }
        }

        getSystemService(DisplayManager::class.java)?.registerDisplayListener(listener, hudHandler)
        displayListener = listener

        applyFrameGenDisplayMode()
    }

    private fun stopFrameGenDisplay() {
        framegenPrefsListener?.let {
            FrameGenPrefs.of(this).unregisterOnSharedPreferenceChangeListener(it)
        }
        framegenPrefsListener = null

        displayListener?.let {
            getSystemService(DisplayManager::class.java)?.unregisterDisplayListener(it)
        }
        displayListener = null
    }

    private fun applyFrameGenDisplayMode() {
        val active = display?.mode
        val enabled = FrameGenPrefs.isEnabled(FrameGenPrefs.of(this)) && FrameGen.state.value.imported

        val wanted = if (active == null || !enabled) {
            0
        } else {
            display?.supportedModes
                ?.filter {
                    it.physicalWidth == active.physicalWidth && it.physicalHeight == active.physicalHeight
                }
                ?.maxByOrNull { it.refreshRate }
                ?.takeIf { it.refreshRate > active.refreshRate }
                ?.modeId ?: 0
        }

        if (window.attributes.preferredDisplayModeId != wanted) {
            window.attributes = window.attributes.also { it.preferredDisplayModeId = wanted }
        }

        FrameGen.pushRefreshRate(display?.refreshRate ?: 0f)
    }

    private fun applyHudPreferences() {
        val prefs = HudPrefs.of(this)

        if (!HudPrefs.isEnabled(prefs)) {
            binding.hudView.visibility = View.GONE
            stopPump()
            return
        }

        val wasHidden = binding.hudView.visibility != View.VISIBLE
        binding.hudView.visibility = View.VISIBLE
        binding.hudView.setMode(HudPrefs.mode(prefs))
        binding.hudView.setFrametimeNumeric(HudPrefs.frametimeNumeric(prefs))
        binding.hudView.setElements(HudPrefs.enabledElements(prefs))
        binding.hudView.setScale(HudPrefs.scale(prefs))

        if (wasHidden) {
            binding.hudView.restorePosition()
        }

        startPump()
    }

    private fun startHud() {
        val prefs = HudPrefs.of(this)

        hudPrefsListener =
            android.content.SharedPreferences.OnSharedPreferenceChangeListener { _, _ ->
                runOnUiThread { applyHudPreferences() }
            }
        prefs.registerOnSharedPreferenceChangeListener(hudPrefsListener)

        applyHudPreferences()
    }

    private fun startPump() {
        if (hudPump != null) {
            return
        }

        val reader = DeviceStatsReader(this)

        var lastPresented = -1L
        var lastGenerated = 0L
        var lastSampleAt = 0L

        hudPump = object : Runnable {
            override fun run() {
                val device = reader.read()
                val emu = runCatching { JSONObject(RPCS3.instance.perfMetrics()) }.getOrNull()

                val presented = emu?.optLong("presented", 0L) ?: 0L
                val generated = emu?.optLong("generated", 0L) ?: 0L
                val now = android.os.SystemClock.elapsedRealtime()

                val outputFps = if (lastPresented >= 0 && now > lastSampleAt && generated > lastGenerated) {
                    (presented - lastPresented) * 1000f / (now - lastSampleAt)
                } else {
                    0f
                }

                lastPresented = presented
                lastGenerated = generated
                lastSampleAt = now

                binding.hudView.submit(
                    device.copy(
                        fps = emu?.optDouble("fps", 0.0)?.toFloat() ?: 0f,
                        outputFps = outputFps,
                        frametimeMs = emu?.optDouble("frametime", 0.0)?.toFloat() ?: 0f,
                        renderer = emu?.optString("renderer").orEmpty(),
                        gpuPercent = if (device.gpuPercent >= 0) {
                            device.gpuPercent
                        } else {
                            emu?.optInt("rsxLoad", -1) ?: -1
                        }
                    )
                )

                hudHandler.postDelayed(this, HUD_REFRESH_MS)
            }
        }

        hudGraphPump = object : Runnable {
            override fun run() {
                val ms = runCatching { RPCS3.instance.frameTimeMs() }.getOrDefault(0f)
                if (ms > 0f) {
                    binding.hudView.addFrameSample(ms)
                }
                hudHandler.postDelayed(this, HUD_GRAPH_MS)
            }
        }

        hudHandler.post(hudPump!!)
        hudHandler.post(hudGraphPump!!)
    }

    private fun stopPump() {
        hudPump?.let { hudHandler.removeCallbacks(it) }
        hudGraphPump?.let { hudHandler.removeCallbacks(it) }
        hudPump = null
        hudGraphPump = null
    }

    private fun stopHud() {
        stopPump()
        hudPrefsListener?.let {
            HudPrefs.of(this).unregisterOnSharedPreferenceChangeListener(it)
        }
        hudPrefsListener = null
    }

    private fun setDrawerVisible(visible: Boolean) {
        if (visible) {
            releaseAllPadInput()
            drawerTitleId.value = resolveTitleId()
            drawerPaused.value = RPCS3.getState() == EmulatorState.Paused
            binding.drawerHost.visibility = View.VISIBLE
            binding.drawerHost.requestFocus()
            drawerVisible.value = true
            return
        }

        drawerVisible.value = false
        binding.drawerHost.postDelayed({
            if (!drawerVisible.value) {
                binding.drawerHost.visibility = View.GONE
                applyOverlayPreferences()
            }
        }, 260)
    }

    @Suppress("DEPRECATION")
    override fun onBackPressed() {
        if (drawerVisible.value) {
            setDrawerVisible(false)
            return
        }

        if (RPCS3.getState().isEmulationActive) {
            setDrawerVisible(true)
            return
        }

        super.onBackPressed()
    }

    override fun onResume() {
        super.onResume()
        applyOverlayPreferences()
        applyFrameGenDisplayMode()
    }

    override fun onPause() {
        super.onPause()
        releaseAllPadInput()
        RPCS3.instance.settingsFlush()
    }

    override fun onDestroy() {
        super.onDestroy()
        stopHud()
        stopOverlay()
        stopFrameGenDisplay()
        pendingStopCheck?.let { stopHandler.removeCallbacks(it) }
        pendingStopCheck = null
        RPCS3.onEmulationStopped = null
        if (RPCS3.getState().isEmulationActive) {
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

        if (keyCode == KeyEvent.KEYCODE_BUTTON_MODE) {
            hudHandler.removeCallbacks(modeHoldRunnable)
            hudHandler.postDelayed(modeHoldRunnable, PS_HOLD_MS)
            gamePadState.digital[0] =
                gamePadState.digital[0] or Digital1Flags.CELL_PAD_CTRL_PS.bit
            sendGamepadData()
            return true
        }

        if (drawerVisible.value) {
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

        if (keyCode == KeyEvent.KEYCODE_BUTTON_MODE) {
            hudHandler.removeCallbacks(modeHoldRunnable)
            gamePadState.digital[0] =
                gamePadState.digital[0] and Digital1Flags.CELL_PAD_CTRL_PS.bit.inv()
            sendGamepadData()
            return true
        }

        if (drawerVisible.value) {
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

        if (drawerVisible.value) {
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

    private fun mergeAxis(pad: Int, overlay: Int) = if (overlay != 127) overlay else pad

    private fun sendGamepadData() {
        RPCS3.instance.overlayPadData(
            gamePadState.digital[0] or overlayPadState.digital[0],
            gamePadState.digital[1] or overlayPadState.digital[1],
            mergeAxis(gamePadState.leftStickX, overlayPadState.leftStickX),
            mergeAxis(gamePadState.leftStickY, overlayPadState.leftStickY),
            mergeAxis(gamePadState.rightStickX, overlayPadState.rightStickX),
            mergeAxis(gamePadState.rightStickY, overlayPadState.rightStickY)
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
        if (hasFocus) {
            enableFullScreenImmersive()
            applyOverlayPreferences()
        } else {
            releaseAllPadInput()
        }
    }

    private companion object {
        const val EMULATION_STOP_GRACE_MS = 1500L
    }
}
