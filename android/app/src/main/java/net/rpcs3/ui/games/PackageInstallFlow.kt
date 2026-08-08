package net.rpcs3.ui.games

import android.content.Context
import android.net.Uri
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.rpcs3.PrecompilerService
import net.rpcs3.PrecompilerServiceAction
import net.rpcs3.RPCS3
import net.rpcs3.utils.PackageInfo
import net.rpcs3.utils.PackageInspector
import net.rpcs3.utils.PackageKind
import java.io.File

private fun freeSpaceBytes(): Long = runCatching {
    File(RPCS3.rootDirectory).usableSpace
}.getOrDefault(0L)

@Composable
fun PackageInstallFlow(
    uris: List<Uri>,
    onFinished: () -> Unit
) {
    val context = LocalContext.current
    var scanned by remember(uris) { mutableStateOf<List<PackageInfo>?>(null) }
    val freeSpace = remember(uris) { freeSpaceBytes() }

    LaunchedEffect(uris) {
        val result = withContext(Dispatchers.IO) {
            uris.mapNotNull { PackageInspector.read(context, it) }
        }

        val nonPackages = result.filter { it.kind != PackageKind.Pkg }

        if (nonPackages.isNotEmpty()) {
            nonPackages.forEach {
                PrecompilerService.start(context, PrecompilerServiceAction.Install, it.uri)
            }
        }

        val packages = result.filter { it.kind == PackageKind.Pkg }

        if (packages.isEmpty()) {
            onFinished()
        } else {
            scanned = packages
        }
    }

    val packages = scanned

    PackageInstallDialog(
        packages = packages ?: emptyList(),
        scanning = packages == null,
        freeSpace = freeSpace,
        onConfirm = { selection ->
            onFinished()
            startInstall(context, selection)
        },
        onDismiss = onFinished
    )
}

private fun startInstall(context: Context, selection: List<PackageInfo>) {
    if (selection.isEmpty()) {
        return
    }

    val ordered = PackageInspector.installOrder(selection)

    PrecompilerService.start(
        context,
        PrecompilerServiceAction.InstallPackages,
        ArrayList(ordered.map { it.uri })
    )
}
