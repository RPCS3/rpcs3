package net.rpcs3.ui.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.ui.Alignment
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp
import net.rpcs3.ui.theme.Rpcs
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
import net.rpcs3.ui.drivers.driverPathOf
import net.rpcs3.utils.DriverSelection
import net.rpcs3.utils.GpuDriverHelper

const val DriverCategory = "GPU Driver"

@Composable
fun GameDriverSettings(titleId: String) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val drivers = remember { GpuDriverHelper.getInstalledDrivers(context) }
    var selectedPath by remember(titleId) { mutableStateOf<String?>(null) }
    var usesGlobal by remember(titleId) { mutableStateOf(true) }

    LaunchedEffect(titleId) {
        val resolved = withContext(Dispatchers.IO) {
            DriverSelection.hasOverride(context, titleId) to DriverSelection.resolve(context, titleId)
        }
        usesGlobal = !resolved.first
        selectedPath = resolved.second
    }

    PaneSectionTitle(stringResource(R.string.settings_driver_per_game))

    GlobalDriverRow(
        selected = usesGlobal,
        onSelect = {
            usesGlobal = true
            scope.launch(Dispatchers.IO) {
                DriverSelection.clearOverride(context, titleId)
                val path = DriverSelection.globalPath(context)
                withContext(Dispatchers.Main) { selectedPath = path }
            }
        }
    )

    drivers.entries.toList().forEach { (file, metadata) ->
        val path = driverPathOf(file, metadata)

        DriverItem(
            file = file,
            metadata = metadata,
            isSelected = !usesGlobal && path == selectedPath,
            onSelect = {
                usesGlobal = false
                selectedPath = path
                scope.launch(Dispatchers.IO) {
                    DriverSelection.setForGame(context, titleId, path)
                }
            },
            onDelete = null
        )
    }

    Spacer(Modifier.height(18.dp))

    DriverFlagsSection(titleId)

    Spacer(Modifier.height(14.dp))
}

@Composable
private fun GlobalDriverRow(selected: Boolean, onSelect: () -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
            .background(
                if (selected) Rpcs.SelectionFill else Rpcs.SurfaceRaised,
                RoundedCornerShape(10.dp)
            )
            .border(
                1.dp,
                if (selected) Rpcs.SelectionBorder else Rpcs.Outline,
                RoundedCornerShape(10.dp)
            )
            .clickable(onClick = onSelect)
            .padding(horizontal = 12.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = stringResource(R.string.settings_driver_use_global),
                color = if (selected) Rpcs.Accent else Rpcs.TextPrimary,
                fontSize = 13.sp,
                fontWeight = FontWeight.Medium
            )
            Spacer(Modifier.height(2.dp))
            Text(
                text = stringResource(R.string.settings_driver_use_global_detail),
                color = Rpcs.TextSecondary,
                fontSize = 11.sp
            )
        }
    }
}
