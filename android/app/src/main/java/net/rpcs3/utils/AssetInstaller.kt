package net.rpcs3.utils

import android.content.res.AssetManager
import android.util.Log
import java.io.File

private const val TAG = "AssetInstaller"

private fun copyAssetTree(assets: AssetManager, assetPath: String, target: File): Int {
    val children = try {
        assets.list(assetPath)
    } catch (e: Exception) {
        Log.e(TAG, "Failed to list '$assetPath'", e)
        return 0
    }

    if (children.isNullOrEmpty()) {
        return try {
            target.parentFile?.mkdirs()
            assets.open(assetPath).use { input ->
                target.outputStream().use { output -> input.copyTo(output) }
            }
            1
        } catch (e: Exception) {
            Log.e(TAG, "Failed to copy '$assetPath'", e)
            0
        }
    }

    target.mkdirs()

    var count = 0
    for (child in children) {
        count += copyAssetTree(assets, "$assetPath/$child", File(target, child))
    }
    return count
}

fun installBundledPatches(assets: AssetManager, rootDirectory: String) {
    val target = File(rootDirectory + "config/patches/patch.yml")
    val stamp = File(rootDirectory + "config/.patches-stamp")

    val bundled = try {
        assets.open("patches/patch.yml").use { it.readBytes() }
    } catch (e: Exception) {
        Log.e(TAG, "No bundled patch set present", e)
        return
    }

    val bundledHash = PatchUpdater.sha256Hex(bundled)
    val seededHash = if (stamp.isFile) runCatching { stamp.readText().trim() }.getOrNull() else null

    if (target.isFile && seededHash == bundledHash) {
        return
    }

    if (target.isFile) {
        val currentHash = runCatching { PatchUpdater.sha256Hex(target.readBytes()) }.getOrNull()
        if (currentHash != seededHash) {
            Log.i(TAG, "Keeping user-managed patch.yml; bundled set not applied")
            return
        }
    }

    val installed = runCatching {
        target.parentFile?.mkdirs()
        val staging = File(target.parentFile, "patch.yml.seed")
        staging.writeBytes(bundled)
        staging.renameTo(target) || staging.copyTo(target, overwrite = true).isFile
    }.getOrElse { e ->
        Log.e(TAG, "Failed to seed bundled patches", e)
        false
    }

    if (installed) {
        Log.i(TAG, "Seeded bundled patch set (${bundled.size} bytes) to ${target.path}")
        runCatching {
            stamp.parentFile?.mkdirs()
            stamp.writeText(bundledHash)
        }
    }
}

fun installBundledAssets(assets: AssetManager, rootDirectory: String) {
    val target = File(rootDirectory + "config/Icons")
    val stamp = File(rootDirectory + "config/.icons-stamp")
    val version = try {
        assets.list("Icons/ui")?.size ?: 0
    } catch (e: Exception) {
        0
    }.toString()

    if (target.isDirectory && stamp.isFile && stamp.readText() == version) {
        return
    }

    target.deleteRecursively()
    val copied = copyAssetTree(assets, "Icons", target)
    Log.i(TAG, "Installed $copied overlay resources to ${target.path}")

    runCatching {
        stamp.parentFile?.mkdirs()
        stamp.writeText(version)
    }
}
