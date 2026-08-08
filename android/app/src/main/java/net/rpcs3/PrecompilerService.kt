package net.rpcs3

import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.net.Uri
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat
import kotlin.concurrent.thread

enum class PrecompilerServiceAction {
    InstallFirmware,
    Install,
    AddIso
}

class PrecompilerService : Service() {
    companion object {
        fun start(context: Context, action: PrecompilerServiceAction, uri: Uri?) {
            val intent = Intent(context, PrecompilerService::class.java)
            intent.putExtra("action", action.ordinal)
            intent.putExtra("uri", uri)

            try {
                context.startForegroundService(intent)
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }

        fun start(context: Context, action: PrecompilerServiceAction, batch: ArrayList<Uri>) {
            if (batch.isEmpty()) {
                return
            }

            if (batch.size == 1) {
                start(context, action, batch[0])
                return
            }

            val intent = Intent(context, PrecompilerService::class.java)
            intent.putExtra("action", action.ordinal)
            intent.putExtra("batch", batch)

            try {
                context.startForegroundService(intent)
            } catch (e: Exception) {
                e.printStackTrace()
            }
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

    enum class Mode { Firmware, Package, AddIso }

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
            Mode.Package -> RPCS3.instance.install(fd, installProgress)
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
        val action = intent?.getIntExtra("action", 0)
        val isFwInstall = action == PrecompilerServiceAction.InstallFirmware.ordinal
        val mode = when (action) {
            PrecompilerServiceAction.InstallFirmware.ordinal -> Mode.Firmware
            PrecompilerServiceAction.AddIso.ordinal -> Mode.AddIso
            else -> Mode.Package
        }

        if (uri == null && batch == null) {
            stopSelf(startId)
            return START_NOT_STICKY
        }

        val installProgress =
            ProgressRepository.create(
                this,
                when (mode) {
                    Mode.Firmware -> "Firmware Installation"
                    Mode.AddIso -> "Adding disc image"
                    Mode.Package -> "Package Installation"
                }
            ) { entry ->
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
            var installResult = false
            if (uri != null) {
                installResult = install(mode, uri, installProgress)
            } else batch?.forEach { uri ->
                // FIXME: create child progress
                if (install(mode, uri, installProgress)) {
                    installResult = true
                }
            }

            if (!installResult) {
                stopSelf(startId)
            }
        }

        return START_STICKY
    }
}