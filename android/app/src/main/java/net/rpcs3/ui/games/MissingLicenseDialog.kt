package net.rpcs3.ui.games

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import net.rpcs3.R
import net.rpcs3.ui.components.GhostButton
import net.rpcs3.ui.theme.Rpcs
import net.rpcs3.ui.theme.SettingsStyle

@Composable
fun MissingLicenseDialog(
    onSelect: () -> Unit,
    onCancel: () -> Unit
) {
    Dialog(onDismissRequest = onCancel) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(18.dp))
                .background(SettingsStyle.BgDeep)
                .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(18.dp))
                .padding(18.dp)
        ) {
            Text(
                text = stringResource(R.string.games_missing_key_title),
                color = SettingsStyle.TextPrimary,
                fontSize = 15.sp,
                fontWeight = FontWeight.SemiBold,
                textAlign = TextAlign.Center
            )
            Spacer(Modifier.height(10.dp))
            Text(
                text = stringResource(R.string.games_missing_rap_message),
                color = SettingsStyle.TextSecondary,
                fontSize = 13.sp,
                textAlign = TextAlign.Center,
                modifier = Modifier.fillMaxWidth()
            )
            Spacer(Modifier.height(20.dp))
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(12.dp, Alignment.CenterHorizontally),
                modifier = Modifier.fillMaxWidth()
            ) {
                GhostButton(
                    label = stringResource(R.string.action_select),
                    accent = true,
                    tint = Rpcs.Success,
                    onClick = onSelect,
                    modifier = Modifier
                        .weight(1f, fill = false)
                        .widthIn(min = 112.dp)
                )
                GhostButton(
                    label = stringResource(R.string.action_cancel),
                    accent = true,
                    tint = Rpcs.Danger,
                    onClick = onCancel,
                    modifier = Modifier
                        .weight(1f, fill = false)
                        .widthIn(min = 112.dp)
                )
            }
        }
    }
}
