package net.rpcs3.ui.diagnostics

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Info
import androidx.compose.material.icons.outlined.Memory
import androidx.compose.material.icons.outlined.Storage
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
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
import net.rpcs3.ui.components.PaneCard
import net.rpcs3.ui.components.PaneKeyValue
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.components.PaneTab
import net.rpcs3.ui.setup.hasStorageAccess
import net.rpcs3.ui.theme.Rpcs
import java.io.File

@Composable
fun DiagnosticsScreen(
    modifier: Modifier = Modifier,
    onClose: (() -> Unit)? = null
) {
    val context = LocalContext.current
    val firmware by FirmwareRepository.version

    var selected by remember { mutableIntStateOf(0) }
    var configWritable by remember { mutableStateOf<Boolean?>(null) }
    var customConfigs by remember { mutableIntStateOf(0) }
    var driver by remember { mutableStateOf("") }
    var systemInfo by remember { mutableStateOf("") }

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

            systemInfo = runCatching { RPCS3.instance.systemInfo() }.getOrDefault("")
        }
    }

    val tabs = listOf(
        PaneTab("Setup", Icons.Outlined.Info),
        PaneTab("Storage", Icons.Outlined.Storage),
        PaneTab("Graphics", Icons.Outlined.Memory)
    )

    PaneScaffold(
        title = "Diagnostics",
        tabs = tabs,
        selected = selected,
        onSelect = { selected = it },
        onBack = onClose,
        modifier = modifier
    ) {
        when (selected) {
            0 -> {
                PaneSectionTitle("Requirements")
                PaneCard {
                    val storage = hasStorageAccess()
                    PaneKeyValue(
                        "Storage access",
                        if (storage) "Granted" else "Not granted",
                        if (storage) Rpcs.Success else Rpcs.Danger
                    )
                    PaneKeyValue(
                        "Firmware",
                        firmware ?: "Not installed",
                        if (firmware != null) Rpcs.Success else Rpcs.Danger
                    )
                    val notifications = Permission.PostNotifications.checkPermission(context)
                    PaneKeyValue(
                        "Notifications",
                        if (notifications) "Granted" else "Not granted",
                        if (notifications) Rpcs.Success else Rpcs.Warning
                    )
                }
            }

            1 -> {
                PaneSectionTitle("Configuration")
                PaneCard {
                    PaneKeyValue(
                        "Config writable",
                        when (configWritable) {
                            true -> "Yes"
                            false -> "No"
                            null -> "Checking"
                        },
                        when (configWritable) {
                            true -> Rpcs.Success
                            false -> Rpcs.Danger
                            null -> Rpcs.TextDim
                        }
                    )
                    PaneKeyValue("Per-game configs", customConfigs.toString())
                }
                Spacer(Modifier.height(12.dp))
                PaneSectionTitle("Paths")
                PaneCard {
                    PaneKeyValue("Config directory", RPCS3.rootDirectory + "config")
                }
            }

            else -> {
                PaneSectionTitle("Renderer")
                PaneCard {
                    PaneKeyValue(
                        "Custom driver",
                        if (driver.contains("\"\"") || driver.isEmpty()) "System" else "Custom"
                    )
                }
                if (systemInfo.isNotEmpty()) {
                    Spacer(Modifier.height(12.dp))
                    PaneSectionTitle("System")
                    PaneCard {
                        systemInfo.lines().filter { it.isNotBlank() }.forEach { line ->
                            val parts = line.split(":", limit = 2)
                            if (parts.size == 2) {
                                PaneKeyValue(parts[0].trim(), parts[1].trim())
                            } else {
                                PaneKeyValue(line.trim(), "")
                            }
                        }
                    }
                }
            }
        }

        Spacer(Modifier.height(16.dp))
    }
}
