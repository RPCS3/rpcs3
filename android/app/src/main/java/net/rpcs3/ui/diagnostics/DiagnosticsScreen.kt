package net.rpcs3.ui.diagnostics

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.FirmwareRepository
import net.rpcs3.Permission
import net.rpcs3.RPCS3
import net.rpcs3.ui.components.InfoRow
import net.rpcs3.ui.components.SectionLabel
import net.rpcs3.ui.components.SettingGroup
import net.rpcs3.ui.setup.hasStorageAccess
import net.rpcs3.ui.theme.Dimens
import net.rpcs3.ui.theme.SettingsStyle
import java.io.File

@Composable
fun DiagnosticsScreen(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val firmware by FirmwareRepository.version

    var configWritable by remember { mutableStateOf<Boolean?>(null) }
    var customConfigs by remember { mutableStateOf(0) }
    var driver by remember { mutableStateOf("") }

    LaunchedEffect(Unit) {
        withContext(Dispatchers.IO) {
            val configDir = File(RPCS3.rootDirectory + "/config")
            val probe = File(configDir, ".write-probe")
            configWritable = runCatching {
                configDir.mkdirs()
                probe.writeText("x")
                probe.delete()
                true
            }.getOrDefault(false)

            customConfigs = File(configDir, "custom_configs")
                .listFiles { f -> f.name.endsWith(".yml") }?.size ?: 0

            driver = runCatching {
                RPCS3.instance.settingsGet("Video@@Vulkan@@Custom Driver@@Path", "")
            }.getOrDefault("")
        }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(SettingsStyle.BgDeep)
            .windowInsetsPadding(WindowInsets.systemBars)
            .verticalScroll(rememberScrollState())
            .padding(horizontal = 20.dp, vertical = 16.dp),
        verticalArrangement = Arrangement.spacedBy(Dimens.SectionGap)
    ) {
        SectionLabel(text = "Setup")
        SettingGroup {
            InfoRow(
                label = "Storage access",
                value = if (hasStorageAccess()) "Granted" else "Not granted"
            )
            InfoRow(label = "Firmware", value = firmware ?: "Not installed")
            InfoRow(
                label = "Notifications",
                value = if (Permission.PostNotifications.checkPermission(context)) {
                    "Granted"
                } else {
                    "Not granted"
                }
            )
        }

        SectionLabel(text = "Storage")
        SettingGroup {
            InfoRow(
                label = "Config writable",
                value = when (configWritable) {
                    true -> "Yes"
                    false -> "No"
                    null -> "Checking"
                }
            )
            InfoRow(label = "Per-game configs", value = customConfigs.toString())
            InfoRow(label = "Config directory", value = RPCS3.rootDirectory + "config")
        }

        SectionLabel(text = "Graphics")
        SettingGroup {
            InfoRow(
                label = "Custom driver",
                value = if (driver.contains("\"\"") || driver.isEmpty()) "System" else "Custom"
            )
        }

        Spacer(Modifier.height(Dimens.SectionGap))
    }
}
