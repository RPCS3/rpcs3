package net.rpcs3.ui.settings.components.preference

import androidx.compose.material3.AlertDialog
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.res.stringResource
import net.rpcs3.R
import net.rpcs3.ui.settings.components.core.PreferenceSubtitle

@Composable
fun TextPreference(
    title: String,
    value: String,
    placeholder: String = "",
    leadingIcon: ImageVector? = null,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    onValueChange: (String) -> Unit,
    onLongClick: () -> Unit = {}
) {
    var dialogVisible by remember { mutableStateOf(false) }

    RegularPreference(
        title = title,
        leadingIcon = leadingIcon,
        modifier = modifier,
        subtitle = {
            PreferenceSubtitle(
                text = value.ifEmpty {
                    placeholder.ifEmpty { stringResource(R.string.preference_not_set) }
                }
            )
        },
        enabled = enabled,
        onClick = { dialogVisible = true },
        onLongClick = onLongClick
    )

    if (dialogVisible) {
        var draft by remember { mutableStateOf(value) }

        AlertDialog(
            onDismissRequest = { dialogVisible = false },
            title = { Text(text = title) },
            text = {
                OutlinedTextField(
                    value = draft,
                    onValueChange = { draft = it },
                    singleLine = true,
                    placeholder = if (placeholder.isNotEmpty()) {
                        { Text(text = placeholder) }
                    } else {
                        null
                    }
                )
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        dialogVisible = false
                        if (draft != value) {
                            onValueChange(draft)
                        }
                    }
                ) { Text(text = stringResource(R.string.action_save)) }
            },
            dismissButton = {
                TextButton(onClick = { dialogVisible = false }) {
                    Text(text = stringResource(R.string.action_cancel))
                }
            }
        )
    }
}
