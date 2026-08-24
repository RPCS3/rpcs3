package net.rpcs3.ui.framegen

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.AutoAwesomeMotion
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import net.rpcs3.R
import net.rpcs3.ui.components.PaneScaffold
import net.rpcs3.ui.components.PaneTab

@Composable
fun FrameGenScreen(
    modifier: Modifier = Modifier,
    onClose: (() -> Unit)? = null
) {
    PaneScaffold(
        title = stringResource(R.string.framegen_title),
        tabs = listOf(
            PaneTab(stringResource(R.string.framegen_tab), Icons.Outlined.AutoAwesomeMotion)
        ),
        selected = 0,
        onSelect = {},
        onBack = onClose,
        modifier = modifier
    ) {
        FrameGenPanel()
    }
}
