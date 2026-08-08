package net.rpcs3.utils

import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import net.rpcs3.RPCS3
import org.json.JSONObject

enum class PackageKind {
    Pkg,
    Rap,
    Edat,
    Iso,
    Pup,
    Unknown;

    companion object {
        fun fromTag(tag: String) = when (tag) {
            "pkg" -> Pkg
            "rap" -> Rap
            "edat" -> Edat
            "iso" -> Iso
            "pup" -> Pup
            else -> Unknown
        }
    }
}

data class PackageInfo(
    val uri: Uri,
    val displayName: String,
    val kind: PackageKind,
    val valid: Boolean,
    val title: String,
    val titleId: String,
    val category: String,
    val appVer: String,
    val targetAppVer: String,
    val dataSize: Long,
    val isUpdate: Boolean,
    val isDlc: Boolean
) {
    val label: String
        get() = title.ifEmpty { displayName }

    val typeLabel: String
        get() = when {
            !valid -> "Corrupted"
            isUpdate -> "Update"
            isDlc -> "DLC"
            category.isNotEmpty() -> category
            else -> "Package"
        }

    val versionOrder: Double
        get() = appVer.toDoubleOrNull() ?: 0.0
}

object PackageInspector {
    fun read(context: Context, uri: Uri): PackageInfo? {
        val name = displayName(context, uri)
        val descriptor = runCatching {
            context.contentResolver.openAssetFileDescriptor(uri, "r")
        }.getOrNull() ?: return null

        return descriptor.use {
            val fd = it.parcelFileDescriptor?.fd ?: return@use null
            val raw = runCatching { RPCS3.instance.pkgInfo(fd) }.getOrNull() ?: return@use null
            parse(uri, name, raw)
        }
    }

    private fun parse(uri: Uri, displayName: String, raw: String): PackageInfo? = runCatching {
        val json = JSONObject(raw)
        PackageInfo(
            uri = uri,
            displayName = displayName,
            kind = PackageKind.fromTag(json.optString("kind", "unknown")),
            valid = json.optBoolean("valid", false),
            title = json.optString("title"),
            titleId = json.optString("titleId"),
            category = json.optString("category"),
            appVer = json.optString("appVer"),
            targetAppVer = json.optString("targetAppVer"),
            dataSize = json.optLong("dataSize", 0L),
            isUpdate = json.optBoolean("isUpdate", false),
            isDlc = json.optBoolean("isDlc", false)
        )
    }.getOrNull()

    fun displayNameOf(context: Context, uri: Uri): String = displayName(context, uri)

    private fun displayName(context: Context, uri: Uri): String {
        val fromProvider = runCatching {
            context.contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)
                ?.use { cursor ->
                    if (cursor.moveToFirst()) cursor.getString(0) else null
                }
        }.getOrNull()

        return fromProvider ?: uri.lastPathSegment?.substringAfterLast('/') ?: "package.pkg"
    }

    fun installOrder(packages: List<PackageInfo>): List<PackageInfo> =
        packages.sortedWith(
            compareBy<PackageInfo> { it.titleId }
                .thenBy { if (it.isUpdate) 1 else 0 }
                .thenBy { it.versionOrder }
                .thenBy { it.displayName }
        )

    fun formatSize(bytes: Long): String {
        if (bytes <= 0) return "unknown size"
        val units = arrayOf("B", "KB", "MB", "GB", "TB")
        var value = bytes.toDouble()
        var unit = 0
        while (value >= 1024.0 && unit < units.lastIndex) {
            value /= 1024.0
            unit++
        }
        return if (unit == 0) "$bytes ${units[unit]}" else String.format("%.1f %s", value, units[unit])
    }
}
