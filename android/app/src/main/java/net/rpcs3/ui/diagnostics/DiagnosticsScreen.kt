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
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.FirmwareRepository
import net.rpcs3.Permission
import net.rpcs3.R
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
        PaneTab(stringResource(R.string.diagnostics_tab_setup), Icons.Outlined.Info),
        PaneTab(stringResource(R.string.diagnostics_tab_storage), Icons.Outlined.Storage),
        PaneTab(stringResource(R.string.diagnostics_tab_graphics), Icons.Outlined.Memory)
    )

    PaneScaffold(
        title = stringResource(R.string.diagnostics_title),
        tabs = tabs,
        selected = selected,
        onSelect = { selected = it },
        onBack = onClose,
        modifier = modifier
    ) {
        when (selected) {
            0 -> {
                PaneSectionTitle(stringResource(R.string.diagnostics_requirements))
                PaneCard {
                    val storage = hasStorageAccess()
                    PaneKeyValue(
                        stringResource(R.string.diagnostics_storage_access),
                        stringResource(
                            if (storage) {
                                R.string.diagnostics_granted
                            } else {
                                R.string.diagnostics_not_granted
                            }
                        ),
                        if (storage) Rpcs.Success else Rpcs.Danger
                    )
                    PaneKeyValue(
                        stringResource(R.string.diagnostics_firmware),
                        firmware ?: stringResource(R.string.diagnostics_firmware_missing),
                        if (firmware != null) Rpcs.Success else Rpcs.Danger
                    )
                    val notifications = Permission.PostNotifications.checkPermission(context)
                    PaneKeyValue(
                        stringResource(R.string.diagnostics_notifications),
                        stringResource(
                            if (notifications) {
                                R.string.diagnostics_granted
                            } else {
                                R.string.diagnostics_not_granted
                            }
                        ),
                        if (notifications) Rpcs.Success else Rpcs.Warning
                    )
                }
            }

            1 -> {
                PaneSectionTitle(stringResource(R.string.diagnostics_configuration))
                PaneCard {
                    PaneKeyValue(
                        stringResource(R.string.diagnostics_config_writable),
                        stringResource(
                            when (configWritable) {
                                true -> R.string.diagnostics_yes
                                false -> R.string.diagnostics_no
                                null -> R.string.diagnostics_checking
                            }
                        ),
                        when (configWritable) {
                            true -> Rpcs.Success
                            false -> Rpcs.Danger
                            null -> Rpcs.TextDim
                        }
                    )
                    PaneKeyValue(
                        stringResource(R.string.diagnostics_per_game_configs),
                        customConfigs.toString()
                    )
                }
                Spacer(Modifier.height(12.dp))
                PaneSectionTitle(stringResource(R.string.diagnostics_paths))
                PaneCard {
                    PaneKeyValue(
                        stringResource(R.string.diagnostics_config_directory),
                        RPCS3.rootDirectory + "config"
                    )
                }
            }

            else -> {
                PaneSectionTitle(stringResource(R.string.diagnostics_renderer))
                PaneCard {
                    PaneKeyValue(
                        stringResource(R.string.diagnostics_custom_driver),
                        stringResource(
                            if (driver.contains("\"\"") || driver.isEmpty()) {
                                R.string.diagnostics_driver_system
                            } else {
                                R.string.diagnostics_driver_custom
                            }
                        )
                    )
                }
                if (systemInfo.isNotEmpty()) {
                    Spacer(Modifier.height(12.dp))
                    PaneSectionTitle(stringResource(R.string.diagnostics_system))
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
