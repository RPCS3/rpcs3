package net.rpcs3.ui.settings.components.preference

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.selection.selectable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Star
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.tooling.preview.PreviewLightDark
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import net.rpcs3.ui.common.ComposePreview
import net.rpcs3.ui.settings.components.base.BaseDialogPreference
import net.rpcs3.ui.settings.components.core.PreferenceIcon
import net.rpcs3.ui.settings.components.core.PreferenceTitle
import net.rpcs3.ui.settings.components.core.PreferenceValue

@Composable
fun <T> SingleSelectionDialog(
    onValueChange: (T) -> Unit,
    values: List<T>,
    title: @Composable () -> Unit,
    modifier: Modifier = Modifier,
    icon: @Composable () -> Unit = {},
    currentValue: T? = null,
    enabled: Boolean = true,
    subtitle: @Composable (() -> Unit)? = null,
    trailingContent: @Composable (() -> Unit)? = {},
    valueToText: (T) -> String = { it.toString() },
    key: ((T) -> Any)? = null,
    onLongClick: () -> Unit = {},
    item: @Composable (value: T, currentValue: T?, onClick: () -> Unit) -> Unit =
        ListPreferenceItem(valueToText)
) {
    require(currentValue in values) {
        "Currently selected item $currentValue is not in the provided values."
    }

    var showDialog by remember { mutableStateOf(false) }

    RegularPreference(
        modifier = modifier,
        title = title,
        leadingIcon = icon,
        enabled = enabled,
        subtitle = subtitle,
        value = { PreferenceValue(text = currentValue.toString()) },
        trailingContent = trailingContent,
        onLongClick = onLongClick,
        onClick = {
           showDialog = true
        }
    )

    if (!showDialog) return

    val coroutineScope = rememberCoroutineScope()

    val onSelected: (T) -> Unit = { selectedItem ->
        coroutineScope.launch {
            onValueChange(selectedItem)
            delay(50)
            showDialog = false
        }
    }

    BaseDialogPreference(
        onDismissRequest = { showDialog = false },
        icon = icon,
        title = title,
        content = {
            LazyColumn(
                modifier = Modifier.fillMaxWidth()
            ) {
                items(
                    items = values,
                    key = key
                ) {
                    item(it, currentValue) {
                        onSelected(it)
                    }
                }
            }
        }
    )
}

@Composable
fun <T> SingleSelectionDialog(
    currentValue: T?,
    onValueChange: (T) -> Unit,
    values: List<T>,
    title: String,
    modifier: Modifier = Modifier,
    icon: ImageVector? = null,
    enabled: Boolean = true,
    subtitle: @Composable (() -> Unit)? = null,
    trailingContent: @Composable (() -> Unit)? = {},
    valueToText: (T) -> String = { it.toString() },
    key: ((T) -> Any)? = null,
    onLongClick: () -> Unit = {},
    item: @Composable (value: T, currentValue: T?, onClick: () -> Unit) -> Unit =
        ListPreferenceItem(valueToText)
) {
    SingleSelectionDialog(
        title = { PreferenceTitle(title = title) },
        icon = { PreferenceIcon(icon = icon) },
        currentValue = currentValue,
        onValueChange = onValueChange,
        values = values,
        modifier = modifier,
        enabled = enabled,
        subtitle = subtitle,
        trailingContent = trailingContent,
        valueToText = valueToText,
        key = key,
        onLongClick = onLongClick,
        item = item
    )
}

fun <T> ListPreferenceItem(
    valueToText: (T) -> String
): @Composable (T, T?, () -> Unit) -> Unit = { value, currentValue, onClick ->
    DialogPreferenceItem(
        value = value,
        currentValue = currentValue,
        valueToText = valueToText,
        onClick = onClick
    )
}

@Composable
private fun <T> DialogPreferenceItem(
    value: T,
    currentValue: T?,
    valueToText: (T) -> String,
    onClick: () -> Unit
) {
    val isSelected by remember(value, currentValue) {
        derivedStateOf { value == currentValue }
    }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .heightIn(min = 48.dp)
            .selectable(
                selected = isSelected,
                enabled = true,
                role = Role.RadioButton,
                onClick = onClick
            ),
        verticalAlignment = Alignment.CenterVertically
    ) {
        RadioButton(
            selected = isSelected,
            onClick = null // Since onClick is handled by the row
        )

        Spacer(modifier = Modifier.width(16.dp))

        Text(
            text = valueToText(value),
            style = MaterialTheme.typography.bodyLarge
        )
    }
}

/**
 * Use Interactive Mode inside preview to preview
 * dialog selection and interaction.
 */
@PreviewLightDark
@Composable
private fun SingleSelectionDialogPreview() {
    ComposePreview {
        var currentValue by remember { mutableIntStateOf(4) }

        Column(
            verticalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            Text("Selected Value: $currentValue")
            SingleSelectionDialog(
                currentValue = currentValue,
                onValueChange = { currentValue = it },
                values = (1..10).toList(),
                title = { PreferenceTitle("Choose a number") },
                icon = { PreferenceIcon(Icons.Default.Star) }
            )
        }
    }
}

@PreviewLightDark
@Composable
private fun SingleSelectionDialogDisabledPreview() {
    ComposePreview {
        var currentValue by remember { mutableIntStateOf(4) }

        Column(
            verticalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            Text("Selected Value: $currentValue")
            SingleSelectionDialog(
                currentValue = currentValue,
                onValueChange = { currentValue = it },
                values = (1..10).toList(),
                title = { PreferenceTitle("Choose a number") },
                icon = { PreferenceIcon(Icons.Default.Star) },
                enabled = false
            )
        }
    }
}
