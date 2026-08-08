package net.rpcs3.ui.games

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Album
import androidx.compose.material.icons.outlined.Download
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import net.rpcs3.ui.theme.SettingsStyle

@Composable
fun IsoChoiceDialog(
    onDirectBoot: () -> Unit,
    onInstall: () -> Unit,
    onDismiss: () -> Unit
) {
    Dialog(onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(18.dp))
                .background(SettingsStyle.BgDeep)
                .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(18.dp))
                .padding(18.dp)
        ) {
            Text(
                text = "Add disc image",
                color = SettingsStyle.TextPrimary,
                fontSize = 15.sp,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(Modifier.height(4.dp))
            Text(
                text = "Both options add the game to your library with full settings.",
                color = SettingsStyle.TextSecondary,
                fontSize = 12.sp
            )
            Spacer(Modifier.height(16.dp))

            IsoChoice(
                icon = Icons.Outlined.Album,
                title = "Direct boot",
                detail = "Runs from the disc image. Nothing is copied.",
                onClick = onDirectBoot
            )
            Spacer(Modifier.height(10.dp))
            IsoChoice(
                icon = Icons.Outlined.Download,
                title = "Install",
                detail = "Extracts the game to storage first.",
                onClick = onInstall
            )

            Spacer(Modifier.height(14.dp))
            Box(
                modifier = Modifier
                    .align(Alignment.End)
                    .clip(RoundedCornerShape(10.dp))
                    .clickable(onClick = onDismiss)
                    .padding(horizontal = 14.dp, vertical = 8.dp)
            ) {
                Text(text = "Cancel", color = SettingsStyle.TextSecondary, fontSize = 13.sp)
            }
        }
    }
}

@Composable
private fun IsoChoice(
    icon: ImageVector,
    title: String,
    detail: String,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(14.dp))
            .background(SettingsStyle.CardSurface)
            .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(14.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 14.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.Start
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = SettingsStyle.AccentBlue,
            modifier = Modifier.size(22.dp)
        )
        Spacer(Modifier.width(12.dp))
        Column {
            Text(
                text = title,
                color = SettingsStyle.TextPrimary,
                fontSize = 14.sp,
                fontWeight = FontWeight.SemiBold
            )
            Text(text = detail, color = SettingsStyle.TextSecondary, fontSize = 11.sp)
        }
    }
}
