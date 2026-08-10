package net.rpcs3

import android.view.Surface
import androidx.annotation.Keep
import androidx.compose.runtime.mutableStateOf

enum class Digital1Flags(val bit: Int)
{
    None(0),
    CELL_PAD_CTRL_SELECT(0x00000001),
    CELL_PAD_CTRL_L3(0x00000002),
    CELL_PAD_CTRL_R3(0x00000004),
    CELL_PAD_CTRL_START(0x00000008),
    CELL_PAD_CTRL_UP(0x00000010),
    CELL_PAD_CTRL_RIGHT(0x00000020),
    CELL_PAD_CTRL_DOWN(0x00000040),
    CELL_PAD_CTRL_LEFT(0x00000080),
    CELL_PAD_CTRL_PS(0x00000100),
}

enum class Digital2Flags(val bit: Int)
{
    None(0),
    CELL_PAD_CTRL_L2(0x00000001),
    CELL_PAD_CTRL_R2(0x00000002),
    CELL_PAD_CTRL_L1(0x00000004),
    CELL_PAD_CTRL_R1(0x00000008),
    CELL_PAD_CTRL_TRIANGLE(0x00000010),
    CELL_PAD_CTRL_CIRCLE(0x00000020),
    CELL_PAD_CTRL_CROSS(0x00000040),
    CELL_PAD_CTRL_SQUARE(0x00000080),
};

enum class EmulatorState {
    Stopped,
    Loading,
    Stopping,
    Running,
    Paused,
    Frozen,
    Ready,
    Starting,
    Compiling;

    val isEmulationActive: Boolean
        get() = this != Stopped && this != Compiling

    companion object {
        fun fromInt(value: Int) = EmulatorState.entries.firstOrNull { it.ordinal == value } ?: Stopped
    }
}

enum class BootResult
{
    NoErrors,
    GenericError,
    NothingToBoot,
    WrongDiscLocation,
    InvalidFileOrFolder,
    InvalidBDvdFolder,
    InstallFailed,
    DecryptionError,
    FileCreationError,
    FirmwareMissing,
    UnsupportedDiscType,
    SavestateCorrupted,
    SavestateVersionUnsupported,
    StillRunning,
    AlreadyAdded,
    CurrentlyRestricted;

    companion object {
        fun fromInt(value: Int) = entries.first { it.ordinal == value }
    }
};

class RPCS3 {
    external fun initialize(rootDir: String): Boolean
    external fun installFw(fd: Int, progressId: Long): Boolean
    external fun install(fd: Int, progressId: Long, name: String): Boolean
    external fun installPackages(fds: IntArray, names: Array<String>, progressId: Long): Boolean
    external fun pkgInfo(fd: Int): String
    external fun installKey(fd: Int, requestId: Long, gamePath: String): Boolean
    external fun boot(path: String): Int
    external fun surfaceEvent(surface: Surface, event: Int): Boolean
    external fun usbDeviceEvent(fd: Int, vendorId: Int, productId: Int, event: Int): Boolean
    external fun processCompilationQueue(): Boolean
    external fun startMainThreadProcessor(): Boolean
    external fun overlayPadData(digital1: Int, digital2: Int, leftStickX: Int, leftStickY: Int, rightStickX: Int, rightStickY: Int): Boolean
    external fun collectGameInfo(rootDir: String, progressId: Long): Boolean
    external fun systemInfo(): String
    external fun takeSettingsSaveFailure(): String?
    external fun bootIso(fd: Int): Int
    external fun addIsoEntry(fd: Int, progressId: Long): Boolean

    external fun patchesGet(titleId: String): String
    external fun patchesAll(): String
    external fun patchFiles(): String
    external fun patchImport(fd: Int, name: String): Boolean
    external fun patchFileDelete(name: String): Boolean
    external fun gameDetails(path: String): String
    external fun installedUpdates(titleId: String): String
    external fun uninstallUpdate(path: String): Boolean
    external fun patchSet(
        titleId: String,
        hash: String,
        description: String,
        title: String,
        serial: String,
        appVersion: String,
        enabled: Boolean
    ): Boolean

    external fun settingsGet(path: String, titleId: String): String
    external fun settingsSet(path: String, value: String, titleId: String): Boolean
    external fun settingsFlush()
    external fun logChannels(): String
    external fun getState() : Int
    external fun kill()
    external fun resume()
    external fun pause()
    external fun openHomeMenu()
    external fun getTitleId(): String
    external fun supportsCustomDriverLoading() : Boolean
    external fun isInstallableFile(fd: Int) : Boolean
    external fun getDirInstallPath(sfoFd: Int) : String?


    companion object {
        val initialized = mutableStateOf(false)
        val instance = RPCS3()
        var rootDirectory: String = ""
        var internalDirectory: String = ""
        var activeGame = mutableStateOf<String?>(null)
        var state = mutableStateOf(EmulatorState.Stopped)

        fun boot(path: String): BootResult {
            return BootResult.fromInt(instance.boot(path))
        }

        fun updateState() {
            val newState = EmulatorState.fromInt(instance.getState())
            if (newState != state.value) {
                state.value = newState
            }
        }

        fun getState(): EmulatorState {
            updateState()
            return state.value
        }

        val settingsTitleId = mutableStateOf("")

        @Volatile
        var onEmulationStopped: (() -> Unit)? = null

        @Keep
        @JvmStatic
        fun notifyEmulationStopped() {
            onEmulationStopped?.invoke()
        }

        init {
            System.loadLibrary("rpcs3-android")
        }
    }
}
