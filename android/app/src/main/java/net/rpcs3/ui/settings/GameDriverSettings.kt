package net.rpcs3.ui.settings

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.rpcs3.R
import net.rpcs3.RPCS3
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.drivers.DriverItem
import net.rpcs3.utils.GpuDriverHelper

const val DriverCategory = "GPU Driver"

private const val DriverPathKey = "Video@@Vulkan@@Custom Driver@@Path"
private const val DriverDataDirKey = "Video@@Vulkan@@Custom Driver@@Internal Data Directory"

@Composable
fun GameDriverSettings(titleId: String) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val drivers = remember { GpuDriverHelper.getInstalledDrivers(context) }
    var selectedPath by remember(titleId) { mutableStateOf<String?>(null) }

    LaunchedEffect(titleId) {
        selectedPath = withContext(Dispatchers.IO) {
            runCatching {
                RPCS3.instance.settingsGet(DriverPathKey, titleId).trim().trim('"')
            }.getOrDefault("")
        }
    }

    PaneSectionTitle(stringResource(R.string.settings_driver_per_game))

    drivers.entries.toList().forEach { (file, metadata) ->
        val path = if (metadata.name == "Default") "" else file.path

        DriverItem(
            file = file,
            metadata = metadata,
            isSelected = selectedPath != null && path == selectedPath,
            onSelect = {
                selectedPath = path
                scope.launch(Dispatchers.IO) {
                    RPCS3.instance.settingsSet(DriverPathKey, "\"" + path + "\"", titleId)
                    RPCS3.instance.settingsSet(
                        DriverDataDirKey, "\"" + context.filesDir + "\"", titleId
                    )
                }
            },
            onDelete = null
        )
    }

    Spacer(Modifier.height(14.dp))
}
