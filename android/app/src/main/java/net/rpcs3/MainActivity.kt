package net.rpcs3

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.core.view.WindowInsetsControllerCompat
import androidx.core.view.WindowInsetsCompat
import java.io.File
import androidx.core.view.WindowCompat
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import net.rpcs3.ui.debug.applyDefaultLogLevels
import net.rpcs3.ui.navigation.AppNavHost
import net.rpcs3.utils.installBundledAssets
import kotlin.concurrent.thread

private const val ACTION_USB_PERMISSION = "net.rpcs3.USB_PERMISSION"

@Composable
private fun StartupScreen() {
    Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
        Column(
            modifier = Modifier.fillMaxSize(),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            CircularProgressIndicator()
            Text(
                text = "Starting RPCS3",
                style = MaterialTheme.typography.bodyMedium,
                modifier = Modifier.padding(top = 16.dp)
            )
        }
    }
}

class MainActivity : ComponentActivity() {
    private var unregisterUsbEventListener: (() -> Unit)? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, window.decorView).apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        setContent {
            RPCS3Theme {
                if (RPCS3.initialized.value) {
                    AppNavHost()
                } else {
                    StartupScreen()
                }
            }
        }

        RPCS3.rootDirectory = applicationContext.getExternalFilesDir(null).toString()
        if (!RPCS3.rootDirectory.endsWith("/")) {
            RPCS3.rootDirectory += "/"
        }

        Permission.PostNotifications.requestPermission(this)

        with(getSystemService(NOTIFICATION_SERVICE) as NotificationManager) {
            val channel = NotificationChannel(
                "rpcs3-progress",
                "Installation progress",
                NotificationManager.IMPORTANCE_DEFAULT
            ).apply {
                setShowBadge(false)
                lockscreenVisibility = Notification.VISIBILITY_PUBLIC
            }

            createNotificationChannel(channel)
        }

        if (!RPCS3.initialized.value) {
            val nativeLibraryDir = packageManager.getApplicationInfo(packageName, 0).nativeLibraryDir

            lifecycleScope.launch {
                GameRepository.load()
                FirmwareRepository.load()
            }

            thread(name = "rpcs3-startup") {
                installBundledAssets(assets, RPCS3.rootDirectory)
                RPCS3.instance.initialize(RPCS3.rootDirectory)
                applyDefaultLogLevels()
                val hookDirectory = "\"" + nativeLibraryDir + "\""
                RPCS3.instance.settingsSet(
                    "Video@@Vulkan@@Custom Driver@@Hook Directory",
                    hookDirectory,
                    ""
                )

                File(RPCS3.rootDirectory + "/config/custom_configs")
                    .listFiles { file ->
                        file.name.startsWith("config_") && file.name.endsWith(".yml")
                    }
                    ?.forEach { file ->
                        val id = file.name.removePrefix("config_").removeSuffix(".yml")
                        RPCS3.instance.settingsSet(
                            "Video@@Vulkan@@Custom Driver@@Hook Directory",
                            hookDirectory,
                            id
                        )
                    }

                thread(name = "rpcs3-main-processor") {
                    RPCS3.instance.startMainThreadProcessor()
                }

                thread(name = "rpcs3-compilation-queue") {
                    RPCS3.instance.processCompilationQueue()
                }

                lifecycleScope.launch {
                    RPCS3.initialized.value = true
                    unregisterUsbEventListener = listenUsbEvents(this@MainActivity)
                }
            }
        } else {
            unregisterUsbEventListener = listenUsbEvents(this@MainActivity)
        }
    }

    override fun onPause() {
        super.onPause()
        RPCS3.instance.settingsFlush()
    }

    override fun onDestroy() {
        super.onDestroy()
        unregisterUsbEventListener?.invoke()
        unregisterUsbEventListener = null
    }
}
