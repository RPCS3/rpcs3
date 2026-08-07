package net.rpcs3.utils

import android.content.Context
import android.os.Build
import android.util.Log
import kotlinx.serialization.SerializationException
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

        // A map between the driver location and its metadata
        val driverMap = mutableMapOf<File, GpuDriverMetadata>()
        driverMap[File("/system/vendor")] = getSystemDriverMetadata()

        gpuDriverDir.listFiles()?.forEach { entry ->
            // Delete any files that aren't a directory
            if (!entry.isDirectory) {
                entry.delete()
                return@forEach
            }

            val metadataFile = File(entry.canonicalPath, GPU_DRIVER_META_FILE)
            // Delete entries without metadata
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

    private fun getSystemDriverMetadata(): GpuDriverMetadata {
        return GpuDriverMetadata(
            name = "Default",
            author = "",
            packageVersion = "",
            vendor = "",
            driverVersion = "",
            minApi = 0,
            description = "The driver provided by your device system",
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

        // Check that the metadata file exists
        val metadataFile = File(unpackDir, GPU_DRIVER_META_FILE)
        if (!metadataFile.isFile) {
            cleanup()
            return GpuDriverInstallResult.MissingMetadata
        }

        // Check that the driver metadata is valid
        val driverMetadata = try {
            GpuDriverMetadata.deserialize(metadataFile)
        } catch (e: SerializationException) {
            cleanup()
            return GpuDriverInstallResult.InvalidMetadata
        }

        // Check that the device satisfies the driver's minimum Android version requirements
        if (Build.VERSION.SDK_INT < driverMetadata.minApi) {
            cleanup()
            return GpuDriverInstallResult.UnsupportedAndroidVersion
        }

        // Check that the driver is not already installed
        val installedDrivers = getInstalledDrivers(context)
        val finalInstallDir = File(getDriversDirectory(context), driverMetadata.label)
        if (installedDrivers[finalInstallDir] != null) {
            cleanup()
            return GpuDriverInstallResult.AlreadyInstalled
        }

        // Move the driver files to the final location
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
            // Create the directory if it doesn't exist
            if (!isDirectory) {
                delete()
                mkdirs()
            }
        }

    fun resolveInstallResultToString(result: GpuDriverInstallResult) = when (result) {
        GpuDriverInstallResult.Success -> "Successfully installed GPU driver"
        GpuDriverInstallResult.InvalidArchive -> "Invalid GPU driver archive"
        GpuDriverInstallResult.MissingMetadata -> "Selected driver's metadata is missing"
        GpuDriverInstallResult.InvalidMetadata -> "Selected driver's metadata is invalid"
        GpuDriverInstallResult.UnsupportedAndroidVersion -> "Your android version doesn't support selected driver"
        GpuDriverInstallResult.AlreadyInstalled -> "Selected driver is already installed"
    }
}

enum class GpuDriverInstallResult {
    Success, InvalidArchive, MissingMetadata, InvalidMetadata, UnsupportedAndroidVersion, AlreadyInstalled,
}
