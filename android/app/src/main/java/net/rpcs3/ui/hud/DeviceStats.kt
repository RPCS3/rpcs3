package net.rpcs3.ui.hud

import android.app.ActivityManager
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.BatteryManager
import java.io.File

private val GPU_LOAD_NODES = listOf(
    "/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage",
    "/sys/class/kgsl/kgsl-3d0/devfreq/gpu_load",
    "/sys/class/misc/mali0/device/utilisation",
    "/sys/devices/platform/gpusysfs/gpu_busy"
)

private val CURRENT_NODES = listOf(
    "/sys/class/power_supply/battery/current_now",
    "/sys/class/power_supply/bms/current_now"
)

private fun readNode(path: String): String? = runCatching {
    val file = File(path)
    if (file.canRead()) file.readText().trim() else null
}.getOrNull()

private fun parseLeadingInt(raw: String?): Int? {
    if (raw.isNullOrEmpty()) {
        return null
    }

    val digits = raw.takeWhile { it.isDigit() || it == '-' }
    return digits.toIntOrNull()
}

class DeviceStatsReader(context: Context) {
    private val appContext = context.applicationContext
    private val activityManager =
        appContext.getSystemService(Context.ACTIVITY_SERVICE) as? ActivityManager
    private val batteryManager =
        appContext.getSystemService(Context.BATTERY_SERVICE) as? BatteryManager

    private var lastCpuTotal = 0L
    private var lastCpuIdle = 0L

    fun read(): HudSample = HudSample(
        cpuPercent = readCpuPercent(),
        gpuPercent = readGpuPercent(),
        ramUsedMb = readRamUsedMb(),
        ramTotalMb = readRamTotalMb(),
        batteryPercent = readBatteryPercent(),
        watts = readWatts(),
        temperatureC = readTemperatureC()
    )

    private fun readCpuPercent(): Int {
        val stat = readNode("/proc/stat") ?: return readCpuPercentFromClocks()
        val line = stat.lineSequence().firstOrNull { it.startsWith("cpu ") }
            ?: return readCpuPercentFromClocks()

        val fields = line.split(Regex("\\s+")).drop(1).mapNotNull { it.toLongOrNull() }
        if (fields.size < 4) {
            return readCpuPercentFromClocks()
        }

        val idle = fields[3] + (fields.getOrNull(4) ?: 0L)
        val total = fields.sum()
        val deltaTotal = total - lastCpuTotal
        val deltaIdle = idle - lastCpuIdle

        lastCpuTotal = total
        lastCpuIdle = idle

        if (deltaTotal <= 0L) {
            return -1
        }

        return (100.0 * (deltaTotal - deltaIdle) / deltaTotal).toInt().coerceIn(0, 100)
    }

    private fun readCpuPercentFromClocks(): Int {
        var current = 0L
        var maximum = 0L
        var cpu = 0

        while (cpu < 32) {
            val base = "/sys/devices/system/cpu/cpu$cpu/cpufreq"
            val max = parseLeadingInt(readNode("$base/cpuinfo_max_freq")) ?: break
            val now = parseLeadingInt(readNode("$base/scaling_cur_freq")) ?: 0

            current += now.toLong()
            maximum += max.toLong()
            cpu++
        }

        if (maximum <= 0L) {
            return -1
        }

        return (100.0 * current / maximum).toInt().coerceIn(0, 100)
    }

    private fun readGpuPercent(): Int {
        for (node in GPU_LOAD_NODES) {
            val raw = readNode(node) ?: continue

            if (node.endsWith("gpu_load") && raw.contains('%')) {
                parseLeadingInt(raw)?.let { return it.coerceIn(0, 100) }
                continue
            }

            parseLeadingInt(raw)?.let { return it.coerceIn(0, 100) }
        }

        val busy = readNode("/sys/class/kgsl/kgsl-3d0/gpubusy") ?: return -1
        val parts = busy.split(Regex("\\s+")).mapNotNull { it.toLongOrNull() }

        if (parts.size < 2 || parts[1] <= 0L) {
            return -1
        }

        return (100.0 * parts[0] / parts[1]).toInt().coerceIn(0, 100)
    }

    private fun memoryInfo(): ActivityManager.MemoryInfo? {
        val manager = activityManager ?: return null
        val info = ActivityManager.MemoryInfo()
        return runCatching {
            manager.getMemoryInfo(info)
            info
        }.getOrNull()
    }

    private fun readRamUsedMb(): Int {
        val info = memoryInfo() ?: return -1
        return ((info.totalMem - info.availMem) / (1024L * 1024L)).toInt()
    }

    private fun readRamTotalMb(): Int {
        val info = memoryInfo() ?: return -1
        return (info.totalMem / (1024L * 1024L)).toInt()
    }

    private fun readBatteryPercent(): Int {
        val manager = batteryManager ?: return -1
        val value = runCatching {
            manager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)
        }.getOrDefault(-1)

        return if (value in 0..100) value else -1
    }

    private fun batteryIntent(): Intent? = runCatching {
        appContext.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
    }.getOrNull()

    private fun readWatts(): Float {
        val microAmps = runCatching {
            batteryManager?.getIntProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_NOW) ?: 0
        }.getOrDefault(0).let { if (it != 0) it else CURRENT_NODES.firstNotNullOfOrNull { node ->
            parseLeadingInt(readNode(node))
        } ?: 0 }

        if (microAmps == 0) {
            return Float.NaN
        }

        val milliVolts = batteryIntent()
            ?.getIntExtra(BatteryManager.EXTRA_VOLTAGE, 0)
            ?.takeIf { it > 0 }
            ?: return Float.NaN

        val amps = kotlin.math.abs(microAmps) / 1_000_000f
        return amps * (milliVolts / 1000f)
    }

    private fun readTemperatureC(): Int {
        val tenths = batteryIntent()?.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, Int.MIN_VALUE)

        if (tenths != null && tenths != Int.MIN_VALUE && tenths > 0) {
            return tenths / 10
        }

        val thermal = parseLeadingInt(readNode("/sys/class/thermal/thermal_zone0/temp")) ?: return -1
        return if (thermal > 1000) thermal / 1000 else thermal
    }
}
