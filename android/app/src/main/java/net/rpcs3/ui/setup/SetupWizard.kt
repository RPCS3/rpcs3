package net.rpcs3.ui.setup

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.Settings
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.CheckCircle
import androidx.compose.material.icons.outlined.Folder
import androidx.compose.material.icons.outlined.Notifications
import androidx.compose.material.icons.outlined.SystemUpdate
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.compose.runtime.DisposableEffect
import net.rpcs3.FirmwareRepository
import net.rpcs3.Permission
import net.rpcs3.ui.theme.SettingsStyle

fun hasStorageAccess(): Boolean =
    Build.VERSION.SDK_INT < Build.VERSION_CODES.R || Environment.isExternalStorageManager()

fun hasFirmware(): Boolean = FirmwareRepository.version.value != null

fun setupComplete(context: Context): Boolean = hasStorageAccess() && hasFirmware()

fun requestStorageAccess(context: Context) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
        return
    }

    val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
        data = Uri.parse("package:" + context.packageName)
        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
    }

    runCatching { context.startActivity(intent) }.onFailure {
        runCatching {
            context.startActivity(
                Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            )
        }
    }
}

@Composable
fun SetupWizard(
    onInstallFirmware: () -> Unit,
    onFinish: () -> Unit,
    onSkip: (() -> Unit)? = null
) {
    val context = LocalContext.current
    var storageGranted by remember { mutableStateOf(hasStorageAccess()) }
    var notificationsGranted by remember {
        mutableStateOf(Permission.PostNotifications.checkPermission(context))
    }

    val firmwareVersion by FirmwareRepository.version
    val firmwareProgress by FirmwareRepository.progressChannel
    val firmwareReady = firmwareVersion != null

    val lifecycleOwner = LocalLifecycleOwner.current
    DisposableEffect(lifecycleOwner) {
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_RESUME) {
                storageGranted = hasStorageAccess()
                notificationsGranted = Permission.PostNotifications.checkPermission(context)
            }
        }
        lifecycleOwner.lifecycle.addObserver(observer)
        onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(SettingsStyle.BgDeep)
            .windowInsetsPadding(WindowInsets.systemBars)
            .verticalScroll(rememberScrollState())
            .padding(horizontal = 24.dp, vertical = 28.dp)
    ) {
        Text(
            text = "Welcome to PS3Native",
            color = SettingsStyle.TextPrimary,
            fontSize = 24.sp,
            fontWeight = FontWeight.Bold
        )
        Spacer(Modifier.height(6.dp))
        Text(
            text = "Two things are needed before you can play. This takes a minute.",
            color = SettingsStyle.TextSecondary,
            fontSize = 13.sp
        )

        Spacer(Modifier.height(24.dp))

        SetupStep(
            index = 1,
            icon = Icons.Outlined.Folder,
            title = "Storage access",
            detail = "Lets PS3Native read games and disc images from your storage.",
            requirement = "Required",
            done = storageGranted,
            actionLabel = "Allow",
            onAction = { requestStorageAccess(context) }
        )

        Spacer(Modifier.height(12.dp))

        SetupStep(
            index = 2,
            icon = Icons.Outlined.SystemUpdate,
            title = "PS3 firmware",
            detail = firmwareVersion?.let { "Installed: $it" }
                ?: "Select the official PS3UPDAT.PUP file. Most games need it.",
            requirement = "Required",
            done = firmwareReady,
            busy = firmwareProgress != null,
            actionLabel = "Install",
            onAction = onInstallFirmware
        )

        Spacer(Modifier.height(12.dp))

        SetupStep(
            index = 3,
            icon = Icons.Outlined.Notifications,
            title = "Notifications",
            detail = "Shows install and compilation progress.",
            requirement = "Optional",
            done = notificationsGranted,
            actionLabel = "Allow",
            onAction = {
                (context as? Activity)?.let { Permission.PostNotifications.requestPermission(it) }
            }
        )

        Spacer(Modifier.height(28.dp))

        val ready = storageGranted && firmwareReady

        Box(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(14.dp))
                .background(if (ready) SettingsStyle.AccentBlue else SettingsStyle.CardSurface)
                .border(
                    1.dp,
                    if (ready) Color.Transparent else SettingsStyle.CardBorder,
                    RoundedCornerShape(14.dp)
                )
                .clickable(enabled = ready, onClick = onFinish)
                .padding(vertical = 14.dp),
            contentAlignment = Alignment.Center
        ) {
            Text(
                text = "Next",
                color = if (ready) Color.White else SettingsStyle.TextDim,
                fontSize = 15.sp,
                fontWeight = FontWeight.SemiBold
            )
        }

        if (onSkip != null) {
            Spacer(Modifier.height(10.dp))
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .clip(RoundedCornerShape(12.dp))
                    .clickable(onClick = onSkip)
                    .padding(vertical = 10.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = "Skip for now",
                    color = SettingsStyle.TextSecondary,
                    fontSize = 13.sp
                )
            }
        }
    }
}

@Composable
private fun SetupStep(
    index: Int,
    icon: ImageVector,
    title: String,
    detail: String,
    requirement: String,
    done: Boolean,
    actionLabel: String,
    onAction: () -> Unit,
    busy: Boolean = false
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(16.dp))
            .background(SettingsStyle.CardSurface)
            .border(
                1.dp,
                if (done) SettingsStyle.AccentBlue.copy(alpha = 0.5f) else SettingsStyle.CardBorder,
                RoundedCornerShape(16.dp)
            )
            .padding(16.dp),
        verticalAlignment = Alignment.Top
    ) {
        Box(
            modifier = Modifier
                .size(38.dp)
                .clip(RoundedCornerShape(11.dp))
                .background(SettingsStyle.InputSurface),
            contentAlignment = Alignment.Center
        ) {
            Icon(
                imageVector = if (done) Icons.Outlined.CheckCircle else icon,
                contentDescription = null,
                tint = if (done) SettingsStyle.AccentBlue else SettingsStyle.TextSecondary,
                modifier = Modifier.size(20.dp)
            )
        }

        Spacer(Modifier.width(14.dp))

        Column(modifier = Modifier.weight(1f)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    text = "$index. $title",
                    color = SettingsStyle.TextPrimary,
                    fontSize = 14.sp,
                    fontWeight = FontWeight.SemiBold
                )
                Spacer(Modifier.width(8.dp))
                Text(
                    text = requirement,
                    color = if (requirement == "Required") {
                        SettingsStyle.TextSecondary
                    } else {
                        SettingsStyle.TextDim
                    },
                    fontSize = 10.sp
                )
            }
            Spacer(Modifier.height(3.dp))
            Text(text = detail, color = SettingsStyle.TextSecondary, fontSize = 12.sp)
        }

        Spacer(Modifier.width(12.dp))

        Box(
            modifier = Modifier
                .clip(RoundedCornerShape(10.dp))
                .background(if (done) Color.Transparent else SettingsStyle.InputSurface)
                .border(
                    1.dp,
                    if (done) Color.Transparent else SettingsStyle.InputBorder,
                    RoundedCornerShape(10.dp)
                )
                .clickable(enabled = !done && !busy, onClick = onAction)
                .padding(horizontal = 14.dp, vertical = 8.dp),
            contentAlignment = Alignment.Center
        ) {
            when {
                busy -> CircularProgressIndicator(
                    modifier = Modifier.size(16.dp),
                    color = SettingsStyle.AccentBlue,
                    strokeWidth = 2.dp
                )

                done -> Text(
                    text = "Done",
                    color = SettingsStyle.AccentBlue,
                    fontSize = 12.sp,
                    fontWeight = FontWeight.SemiBold
                )

                else -> Text(
                    text = actionLabel,
                    color = SettingsStyle.TextPrimary,
                    fontSize = 12.sp,
                    fontWeight = FontWeight.SemiBold
                )
            }
        }
    }
}

@Composable
fun StorageAccessDialog(onGrant: () -> Unit, onDismiss: () -> Unit) {
    androidx.compose.ui.window.Dialog(onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(18.dp))
                .background(SettingsStyle.BgDeep)
                .border(1.dp, SettingsStyle.CardBorder, RoundedCornerShape(18.dp))
                .padding(18.dp)
        ) {
            Text(
                text = "Storage access needed",
                color = SettingsStyle.TextPrimary,
                fontSize = 15.sp,
                fontWeight = FontWeight.SemiBold
            )
            Spacer(Modifier.height(6.dp))
            Text(
                text = "Booting a disc image reads it straight from your storage, " +
                    "so PS3Native needs all-files access.",
                color = SettingsStyle.TextSecondary,
                fontSize = 12.sp
            )
            Spacer(Modifier.height(16.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.End
            ) {
                Box(
                    modifier = Modifier
                        .clip(RoundedCornerShape(10.dp))
                        .clickable(onClick = onDismiss)
                        .padding(horizontal = 14.dp, vertical = 8.dp)
                ) {
                    Text("Not now", color = SettingsStyle.TextSecondary, fontSize = 13.sp)
                }
                Spacer(Modifier.width(6.dp))
                Box(
                    modifier = Modifier
                        .clip(RoundedCornerShape(10.dp))
                        .background(SettingsStyle.AccentBlue)
                        .clickable(onClick = onGrant)
                        .padding(horizontal = 16.dp, vertical = 8.dp)
                ) {
                    Text("Open settings", color = Color.White, fontSize = 13.sp)
                }
            }
        }
    }
}
