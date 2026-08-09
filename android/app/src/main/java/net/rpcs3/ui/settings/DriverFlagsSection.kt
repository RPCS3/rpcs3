package net.rpcs3.ui.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Add
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.toMutableStateList
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import net.rpcs3.R
import net.rpcs3.ui.components.PaneSectionTitle
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.utils.DriverFlags

@Composable
fun DriverFlagsSection(titleId: String?) {
    val context = LocalContext.current
    val flags = remember(titleId) {
        DriverFlags.flagsFor(context, titleId).toMutableStateList()
    }
    var pending by remember(titleId) { mutableStateOf("") }

    fun persist() = DriverFlags.setFlags(context, titleId, flags.toList())

    PaneSectionTitle(stringResource(R.string.drivers_flags_title))

    Text(
        text = stringResource(R.string.drivers_flags_description),
        color = Rpcs.TextDim,
        fontSize = 12.sp,
        lineHeight = 16.sp,
        modifier = Modifier.padding(bottom = 8.dp)
    )

    flags.forEachIndexed { index, flag ->
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 3.dp)
                .clip(RoundedCornerShape(10.dp))
                .background(Rpcs.SurfaceRaised)
                .padding(start = 14.dp, end = 4.dp, top = 4.dp, bottom = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = flag,
                color = Rpcs.TextPrimary,
                fontSize = 14.sp,
                fontWeight = FontWeight.Medium,
                modifier = Modifier.weight(1f)
            )
            IconButton(onClick = {
                flags.removeAt(index)
                persist()
            }) {
                Icon(
                    imageVector = Icons.Outlined.Delete,
                    contentDescription = stringResource(R.string.drivers_flags_delete),
                    tint = Rpcs.Danger,
                    modifier = Modifier.size(20.dp)
                )
            }
        }
    }

    Spacer(Modifier.height(6.dp))

    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        OutlinedTextField(
            value = pending,
            onValueChange = { pending = it.filter { ch -> !ch.isWhitespace() && ch != ',' } },
            singleLine = true,
            label = { Text(stringResource(R.string.drivers_flags_add_hint)) },
            modifier = Modifier.weight(1f)
        )
        IconButton(
            onClick = {
                val value = pending.trim()
                if (value.isNotEmpty() && !flags.contains(value)) {
                    flags.add(value)
                    persist()
                }
                pending = ""
            },
            enabled = pending.isNotBlank()
        ) {
            Icon(
                imageVector = Icons.Outlined.Add,
                contentDescription = stringResource(R.string.drivers_flags_add),
                tint = Rpcs.FocusBorder
            )
        }
    }

    Column(modifier = Modifier.fillMaxWidth()) {
        TextButton(onClick = {
            flags.clear()
            flags.addAll(if (titleId.isNullOrEmpty()) DriverFlags.defaults else DriverFlags.globalFlags(context))
            persist()
        }) {
            Text(
                text = stringResource(R.string.drivers_flags_restore),
                color = Rpcs.FocusBorder,
                fontSize = 13.sp
            )
        }
    }

    Spacer(Modifier.height(10.dp))
}
