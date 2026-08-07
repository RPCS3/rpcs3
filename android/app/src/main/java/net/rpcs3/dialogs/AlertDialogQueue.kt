
package net.rpcs3.dialogs

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.BasicAlertDialog
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.Divider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.derivedStateOf
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

object AlertDialogQueue {
    val dialogs = mutableStateListOf<DialogData>()

    fun showDialog(
        title: String,
        message: String,
        onConfirm: () -> Unit = {},
        onDismiss: (() -> Unit)? = null,
        confirmText: String = "OK",
        dismissText: String = "Cancel"
    ) {
        dialogs.add(DialogData(title, message, onConfirm, onDismiss, confirmText, dismissText))
    }

    private fun dismissDialog() {
        if (dialogs.isNotEmpty()) {
            dialogs.removeAt(0)
        }
    }

    @OptIn(ExperimentalMaterial3Api::class)
    @Composable
    fun AlertDialog() {
        if (dialogs.isEmpty()) {
            return
        }

        val dialog = dialogs.first()
        val onDismiss = dialog.onDismiss

        val scrollState = rememberScrollState()
        val hasScrolled = remember { derivedStateOf { scrollState.value > 0 } }

        BasicAlertDialog(
            onDismissRequest = { 
                onDismiss?.invoke() 
                dismissDialog()
            },
            content = {
                Surface(
                    shape = RoundedCornerShape(28.dp),
                    tonalElevation = 6.dp,
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(vertical = 16.dp)
                ) {
                    Column(modifier = Modifier.padding(vertical = 16.dp)) {
                        Text(
                            dialog.title, 
                            modifier = Modifier.padding(horizontal = 16.dp), 
                            style = MaterialTheme.typography.headlineSmall,
                            color = MaterialTheme.colorScheme.onSurface
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        
                        if (hasScrolled.value) {
                            Divider()
                        }
                        
                        Text(
                            text = dialog.message,
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            modifier = Modifier
                                .heightIn(max = 200.dp)
                                .verticalScroll(scrollState)
                                .padding(vertical = 4.dp, horizontal = 16.dp)
                        )

                        if (hasScrolled.value) {
                            Divider()
                        }
                        
                        Spacer(modifier = Modifier.height(16.dp))
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.End
                        ) {
                            TextButton(
                                onClick = { 
                                    onDismiss?.invoke() 
                                    dismissDialog()
                                }
                            ) {
                                Text(text = dialog.dismissText)
                            }
                            Spacer(modifier = Modifier.width(8.dp))
                            TextButton(
                                onClick = {
                                    dialog.onConfirm()
                                    onDismiss?.invoke()
                                    dismissDialog()
                                }, 
                                modifier = Modifier.padding(end = 16.dp)
                            ) {
                                Text(text = dialog.confirmText)
                            }
                        }
                    }
                }
            }
        )
    }
}

data class DialogData(
    val title: String,
    val message: String,
    val onConfirm: () -> Unit,
    val onDismiss: (() -> Unit)?,
    val confirmText: String,
    val dismissText: String
)
