package net.rpcs3.ui.common

import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import net.rpcs3.RPCS3Theme



@Composable
fun ComposePreview(
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit
) {
    RPCS3Theme {
        Surface(
            modifier = modifier
        ) {
            content()
        }
    }
}
