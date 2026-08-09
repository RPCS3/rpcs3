package net.rpcs3.utils

import android.content.Context
import android.os.Build
import android.util.Log
import kotlinx.serialization.SerializationException
import net.rpcs3.R
import java.io.File
import java.io.IOException
import java.io.InputStream

private const val GPU_DRIVER_DIRECTORY = "gpu_drivers"
private const val GPU_DRIVER_FILE_REDIRECT_DIR = "gpu/vk_file_redirect"
private const val GPU_DRIVER_INSTALL_TEMP_DIR = "driver_temp"
private const val GPU_DRIVER_META_FILE = "meta.json"
private const val TAG = "GPUDriverHelper"

object GpuDriverHelper {
    fun getInstalledDrivers(context: Context): Map<File, GpuDriverMetadata> {
        val gpuDriverDir = getDriversDirectory(context)

        val driverMap = mutableMapOf<File, GpuDriverMetadata>()
        driverMap[File("/system/vendor")] = getSystemDriverMetadata(context)

        gpuDriverDir.listFiles()?.forEach { entry ->
            if (!entry.isDirectory) {
                entry.delete()
                return@forEach
            }

            val metadataFile = File(entry.canonicalPath, GPU_DRIVER_META_FILE)
            if (!metadataFile.exists()) {
                entry.delete()
                return@forEach
            }

            try {
                driverMap[entry] = GpuDriverMetadata.deserialize(metadataFile)
            } catch (e: SerializationException) {
                Log.w(
                    TAG,
                    "Failed to load gpu driver metadata for ${entry.name}, skipping\n${e.message}"
                )
            }
        }

        return driverMap
    }

    private fun getSystemDriverMetadata(context: Context): GpuDriverMetadata {
        return GpuDriverMetadata(
            name = "Default",
            author = "",
            packageVersion = "",
            vendor = "",
            driverVersion = "",
            minApi = 0,
            description = context.getString(R.string.drivers_default_description),
            libraryName = ""
        )
    }

    fun installDriver(context: Context, stream: InputStream): GpuDriverInstallResult {
        val installTempDir =
            File(context.cacheDir.canonicalPath, GPU_DRIVER_INSTALL_TEMP_DIR).apply {
                deleteRecursively()
            }

        try {
            ZipUtil.unzip(stream, installTempDir)
        } catch (e: Exception) {
            e.printStackTrace()
            installTempDir.deleteRecursively()
            return GpuDriverInstallResult.InvalidArchive
        }

        return installUnpackedDriver(context, installTempDir)
    }

    fun installDriver(context: Context, file: File): GpuDriverInstallResult {
        val installTempDir =
            File(context.cacheDir.canonicalPath, GPU_DRIVER_INSTALL_TEMP_DIR).apply {
                deleteRecursively()
            }

        try {
            ZipUtil.unzip(file, installTempDir)
        } catch (e: Exception) {
            e.printStackTrace()
            installTempDir.deleteRecursively()
            return GpuDriverInstallResult.InvalidArchive
        }

        return installUnpackedDriver(context, installTempDir)
    }

    private fun installUnpackedDriver(context: Context, unpackDir: File): GpuDriverInstallResult {
        val cleanup = {
            unpackDir.deleteRecursively()
        }

        val metadataFile = File(unpackDir, GPU_DRIVER_META_FILE)
        if (!metadataFile.isFile) {
            cleanup()
            return GpuDriverInstallResult.MissingMetadata
        }

        val driverMetadata = try {
            GpuDriverMetadata.deserialize(metadataFile)
        } catch (e: SerializationException) {
            cleanup()
            return GpuDriverInstallResult.InvalidMetadata
        }

        if (Build.VERSION.SDK_INT < driverMetadata.minApi) {
            cleanup()
            return GpuDriverInstallResult.UnsupportedAndroidVersion
        }

        val installedDrivers = getInstalledDrivers(context)
        val finalInstallDir = File(getDriversDirectory(context), driverMetadata.label)
        if (installedDrivers[finalInstallDir] != null) {
            cleanup()
            return GpuDriverInstallResult.AlreadyInstalled
        }

        if (!unpackDir.renameTo(finalInstallDir)) {
            cleanup()
            throw IOException("Failed to create directory ${finalInstallDir.name}")
        }

        return GpuDriverInstallResult.Success
    }

    fun getLibraryName(context: Context, driverLabel: String): String {
        val driverDir = File(getDriversDirectory(context), driverLabel)
        val metadataFile = File(driverDir, GPU_DRIVER_META_FILE)
        return try {
            GpuDriverMetadata.deserialize(metadataFile).libraryName
        } catch (e: SerializationException) {
            Log.w(
                TAG,
                "Failed to load library name for driver ${driverLabel}, driver may not exist or have invalid metadata"
            )
            ""
        }
    }

    fun ensureFileRedirectDir(context: Context) {
        File(context.getExternalFilesDir(null), GPU_DRIVER_FILE_REDIRECT_DIR).apply {
            if (!isDirectory) {
                delete()
                mkdirs()
            }
        }
    }

    private fun getDriversDirectory(context: Context) =
        File(context.filesDir.canonicalPath, GPU_DRIVER_DIRECTORY).apply {
            if (!isDirectory) {
                delete()
                mkdirs()
            }
        }

    fun resolveInstallResultToString(context: Context, result: GpuDriverInstallResult): String =
        context.getString(
            when (result) {
                GpuDriverInstallResult.Success -> R.string.drivers_install_success
                GpuDriverInstallResult.InvalidArchive -> R.string.drivers_install_invalid_archive
                GpuDriverInstallResult.MissingMetadata -> R.string.drivers_install_missing_metadata
                GpuDriverInstallResult.InvalidMetadata -> R.string.drivers_install_invalid_metadata
                GpuDriverInstallResult.UnsupportedAndroidVersion -> {
                    R.string.drivers_install_unsupported_android
                }

                GpuDriverInstallResult.AlreadyInstalled -> {
                    R.string.drivers_install_already_installed
                }
            }
        )
}

enum class GpuDriverInstallResult {
    Success, InvalidArchive, MissingMetadata, InvalidMetadata, UnsupportedAndroidVersion, AlreadyInstalled,
}
