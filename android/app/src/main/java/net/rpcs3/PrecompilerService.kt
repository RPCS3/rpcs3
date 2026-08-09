package net.rpcs3

import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.content.res.AssetFileDescriptor
import android.net.Uri
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat
import net.rpcs3.utils.PackageInspector
import kotlin.concurrent.thread

enum class PrecompilerServiceAction {
    InstallFirmware,
    Install,
    AddIso,
    InstallPackages
}

class PrecompilerService : Service() {
    companion object {
        const val NoProgress = -1L

        private fun modeOf(action: Int) = when (action) {
            PrecompilerServiceAction.InstallFirmware.ordinal -> Mode.Firmware
            PrecompilerServiceAction.AddIso.ordinal -> Mode.AddIso
            PrecompilerServiceAction.InstallPackages.ordinal -> Mode.PackageBatch
            else -> Mode.Package
        }

        private fun progressTitle(context: Context, mode: Mode, count: Int) = when (mode) {
            Mode.Firmware -> context.getString(R.string.progress_firmware_installation)
            Mode.AddIso -> context.getString(R.string.progress_adding_disc_image)
            Mode.PackageBatch -> if (count > 1) {
                context.getString(R.string.progress_installing_packages, count)
            } else {
                context.getString(R.string.progress_package_installation)
            }

            Mode.Package -> context.getString(R.string.progress_package_installation)
        }

        private fun launch(
            context: Context,
            action: PrecompilerServiceAction,
            count: Int,
            fill: (Intent) -> Unit
        ): Long {
            val progressId = ProgressRepository.create(
                context.applicationContext,
                progressTitle(context, modeOf(action.ordinal), count)
            )

            val intent = Intent(context, PrecompilerService::class.java)
            intent.putExtra("action", action.ordinal)
            intent.putExtra("progressId", progressId)
            fill(intent)

            try {
                context.startForegroundService(intent)
            } catch (e: Exception) {
                e.printStackTrace()
                ProgressRepository.onProgressEvent(progressId, -1, 0)
                return NoProgress
            }

            return progressId
        }

        fun start(context: Context, action: PrecompilerServiceAction, uri: Uri?): Long {
            if (uri == null) {
                return NoProgress
            }

            return launch(context, action, 1) { it.putExtra("uri", uri) }
        }

        fun start(context: Context, action: PrecompilerServiceAction, batch: ArrayList<Uri>): Long {
            if (batch.isEmpty()) {
                return NoProgress
            }

            return launch(context, action, batch.size) { it.putExtra("batch", batch) }
        }
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    override fun onCreate() {
        super.onCreate()
    }

    fun install(isFw: Boolean, uri: Uri, installProgress: Long): Boolean =
        install(if (isFw) Mode.Firmware else Mode.Package, uri, installProgress)

    enum class Mode { Firmware, Package, AddIso, PackageBatch }

    private fun installBatch(uris: List<Uri>, installProgress: Long): Boolean {
        val descriptors = ArrayList<AssetFileDescriptor>(uris.size)
        val fds = ArrayList<Int>(uris.size)
        val names = ArrayList<String>(uris.size)

        try {
            uris.forEach { uri ->
                val descriptor = try {
                    contentResolver.openAssetFileDescriptor(uri, "r")
                } catch (e: Exception) {
                    e.printStackTrace()
                    null
                }

                val fd = descriptor?.parcelFileDescriptor?.fd

                if (descriptor != null && fd != null) {
                    descriptors += descriptor
                    fds += fd
                    names += PackageInspector.displayNameOf(this, uri)
                } else {
                    try {
                        descriptor?.close()
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            }

            if (fds.isEmpty()) {
                ProgressRepository.onProgressEvent(installProgress, -1, 0)
                return false
            }

            if (!RPCS3.instance.installPackages(
                    fds.toIntArray(), names.toTypedArray(), installProgress
                )
            ) {
                try {
                    ProgressRepository.onProgressEvent(installProgress, -1, 0)
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        } finally {
            descriptors.forEach {
                try {
                    it.close()
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }

        return true
    }

    fun install(mode: Mode, uri: Uri, installProgress: Long): Boolean {
        val parcel = if (mode == Mode.AddIso) {
            contentResolver.openFileDescriptor(uri, "r")
        } else {
            null
        }
        val descriptor = if (parcel == null) {
            contentResolver.openAssetFileDescriptor(uri, "r")
        } else {
            null
        }
        val fd = parcel?.fd ?: descriptor?.parcelFileDescriptor?.fd

        if (fd == null) {
            try {
                parcel?.close()
                descriptor?.close()
            } catch (e: Exception) {
                e.printStackTrace()
            }

            return false
        }

        val installResult = when (mode) {
            Mode.Firmware -> RPCS3.instance.installFw(fd, installProgress)
            Mode.AddIso -> RPCS3.instance.addIsoEntry(fd, installProgress)
            Mode.Package, Mode.PackageBatch -> RPCS3.instance.install(fd, installProgress)
        }

        if (!installResult) {
            try {
                ProgressRepository.onProgressEvent(installProgress, -1, 0)
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

        try {
            parcel?.close()
            descriptor?.close()
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return true
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val batch = intent?.getParcelableArrayListExtra<Uri>("batch")
        val uri = intent?.getParcelableExtra<Uri>("uri")
        val action = intent?.getIntExtra("action", 0) ?: 0
        val isFwInstall = action == PrecompilerServiceAction.InstallFirmware.ordinal
        val mode = modeOf(action)
        val requestedProgress = intent?.getLongExtra("progressId", NoProgress) ?: NoProgress

        if (uri == null && batch == null) {
            if (ProgressRepository.exists(requestedProgress)) {
                ProgressRepository.onProgressEvent(requestedProgress, -1, 0)
            }

            stopSelf(startId)
            return START_NOT_STICKY
        }

        val installProgress = if (ProgressRepository.exists(requestedProgress)) {
            requestedProgress
        } else {
            ProgressRepository.create(this, progressTitle(this, mode, batch?.size ?: 1))
        }

        ProgressRepository.addListener(installProgress) { entry ->
            if (entry.isFinished()) {
                if (isFwInstall) {
                    FirmwareRepository.progressChannel.value = null
                }

                stopSelf(startId)
            }
        }

        if (isFwInstall) {
            FirmwareRepository.progressChannel.value = installProgress
        }

        try {
            ServiceCompat.startForeground(
                this,
                installProgress.toInt(),
                NotificationCompat.Builder(this, "rpcs3-progress").build(),
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE
                } else {
                    0
                }
            )
        } catch (e: Exception) {
            e.printStackTrace()
        }

        thread {
            val installResult = when {
                uri != null -> install(mode, uri, installProgress)
                mode == Mode.PackageBatch || mode == Mode.Package ->
                    installBatch(batch ?: arrayListOf(), installProgress)

                else -> {
                    batch?.forEach { install(mode, it, installProgress) }
                    true
                }
            }

            if (!installResult) {
                stopSelf(startId)
            }
        }

        return START_STICKY
    }
}