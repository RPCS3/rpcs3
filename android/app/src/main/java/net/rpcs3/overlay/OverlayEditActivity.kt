package net.rpcs3.overlay

import android.graphics.PointF
import android.os.Build
import android.os.Bundle
import android.content.Context
import android.view.ViewGroup.MarginLayoutParams
import android.view.ViewGroup
import android.view.WindowManager
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.core.view.updateLayoutParams
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowLeft
import androidx.compose.material.icons.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.viewinterop.AndroidView
import net.rpcs3.RPCS3Theme
import net.rpcs3.R
import kotlin.math.roundToInt

class OverlayEditActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableFullScreenImmersive(this)
        setContent {
            RPCS3Theme {
                OverlayEditScreen()
            }
        }
    }

    private fun enableFullScreenImmersive(activity: ComponentActivity) {
        val window = activity.window
        WindowCompat.setDecorFitsSystemWindows(window, false)
    
        val insetsController = WindowInsetsControllerCompat(window, window.decorView)
        insetsController.apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        val params = window.attributes
        params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
        window.attributes = params
    }
}

private fun applyInsetsToPadOverlay(padOverlay: PadOverlay) {
    ViewCompat.setOnApplyWindowInsetsListener(padOverlay) { view, windowInsets ->
        val insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
        if (view.layoutParams is MarginLayoutParams) {
            view.updateLayoutParams<MarginLayoutParams> {
                leftMargin = insets.left
                rightMargin = insets.right
                topMargin = insets.top
                bottomMargin = insets.bottom
            }
        }
        WindowInsetsCompat.CONSUMED
    }
}

@Composable
fun OverlayEditScreen() {
    var isPanelVisible by remember { mutableStateOf(true) }
    var scaleValue by remember { mutableStateOf(50f) }
    var opacityValue by remember { mutableStateOf(100f) }
    var isEnabled by remember { mutableStateOf(true) }
    val unknownButtonName = stringResource(R.string.overlay_unknown_button)
    var currentButtonName by remember { mutableStateOf(unknownButtonName) }
    var showResetDialog by remember { mutableStateOf(false) }
    val context = LocalContext.current
    var padOverlay: PadOverlay? by remember { mutableStateOf(null) }

    Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
        AndroidView(
            modifier = Modifier.fillMaxSize(),
            factory = { ctx: Context ->
                PadOverlay(ctx, null).also { padOverlay = it }
            },
            update = { padOverlay = it }
        )

        padOverlay?.layoutParams = MarginLayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT
        )
        padOverlay?.let { applyInsetsToPadOverlay(it) }
        padOverlay?.isEditing = true

        padOverlay?.onSelectedInputChange = { input ->
            val info = (input as? PadOverlayDpad)?.getInfo() ?: (input as? PadOverlayButton)?.getInfo()
            if (info != null) {
                currentButtonName = info.first.toString()
                scaleValue = info.second.toFloat()
                opacityValue = info.third.toFloat()
            }
        }

        if (isPanelVisible) {
            ControlPanel(
                scaleValue = scaleValue,
                onScaleChange = { 
                    scaleValue = it 
                    padOverlay?.setButtonScale(it.roundToInt())
                },
                opacityValue = opacityValue,
                onOpacityChange = { 
                    opacityValue = it 
                    padOverlay?.setButtonOpacity(it.roundToInt())
                },
                isEnabled = isEnabled,
                onEnableChange = { isEnabled = it },
                currentButtonName = currentButtonName,
                onResetClick = { showResetDialog = true },
                onCloseClick = { isPanelVisible = false },
                onMoveUp = { padOverlay?.moveButtonUp() },
                onMoveRight = { padOverlay?.moveButtonRight() },
                onMoveLeft = { padOverlay?.moveButtonLeft() },
                onMoveDown = { padOverlay?.moveButtonDown() }
            )
        }

        if (showResetDialog) {
            ResetDialog(
                buttonName = currentButtonName,
                onConfirm = { 
                    showResetDialog = false
                    padOverlay?.resetButtonConfigs() 
                },
                onDismiss = { showResetDialog = false }
            )
        }
    }
}

@Composable
fun ControlPanel(
    scaleValue: Float,
    onScaleChange: (Float) -> Unit,
    opacityValue: Float,
    onOpacityChange: (Float) -> Unit,
    isEnabled: Boolean,
    onEnableChange: (Boolean) -> Unit,
    currentButtonName: String,
    onResetClick: () -> Unit,
    onCloseClick: () -> Unit,
    onMoveUp: () -> Unit,
    onMoveRight: () -> Unit,
    onMoveLeft: () -> Unit,
    onMoveDown: () -> Unit
) {
    val configuration = LocalConfiguration.current
    val screenWidth = configuration.screenWidthDp
    val screenHeight = configuration.screenHeightDp

    val panelWidth = 336f
    val panelHeight = 200f
    
    var panelOffset by remember { 
        mutableStateOf(
            PointF(
                (screenWidth / 2f - panelWidth / 2f), 
                (screenHeight / 2f - panelHeight / 2f)
            )
        ) 
    }

    Box(
        modifier = Modifier
            .offset(panelOffset.x.dp, panelOffset.y.dp)
            .background(MaterialTheme.colorScheme.surface.copy(alpha = 0.88f), RoundedCornerShape(8.dp))
            .padding(10.dp)
            .width(336.dp)
            .pointerInput(Unit) {
                detectDragGestures { change, dragAmount ->
                    change.consume()
                    panelOffset = PointF(panelOffset.x + dragAmount.x, panelOffset.y + dragAmount.y)
                }
            }
    ) {
        Column(
            modifier = Modifier.padding(6.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(4.dp)
                        .background(MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f), RoundedCornerShape(50))
                )
                IconButton(onClick = onCloseClick) {
                    Icon(
                        Icons.Default.Close,
                        contentDescription = stringResource(R.string.action_close),
                        tint = MaterialTheme.colorScheme.error
                    )
                }
            }

            Spacer(modifier = Modifier.height(5.dp))
            
            Text(
                text = stringResource(R.string.overlay_editing, currentButtonName),
                style = MaterialTheme.typography.titleSmall,
                color = MaterialTheme.colorScheme.onSurface
            )

            Spacer(modifier = Modifier.height(6.dp))

            Column(
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                IconButton(onClick = onMoveUp) {
                    Icon(
                        imageVector = Icons.Default.KeyboardArrowUp,
                        contentDescription = stringResource(R.string.overlay_move_up),
                        tint = MaterialTheme.colorScheme.primary
                    )
                }

                Row(
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    IconButton(onClick = onMoveLeft) {
                        Icon(
                            imageVector = Icons.Default.KeyboardArrowLeft,
                            contentDescription = stringResource(R.string.overlay_move_left),
                            tint = MaterialTheme.colorScheme.primary
                        )
                    }

                    Checkbox(
                        checked = isEnabled,
                        onCheckedChange = onEnableChange,
                        modifier = Modifier.padding(4.dp),
                        colors = CheckboxDefaults.colors(
                            checkedColor = MaterialTheme.colorScheme.primary,
                            uncheckedColor = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
                        )
                    )

                    IconButton(onClick = onMoveRight) {
                        Icon(
                            imageVector = Icons.Default.KeyboardArrowRight,
                            contentDescription = stringResource(R.string.overlay_move_right),
                            tint = MaterialTheme.colorScheme.primary
                        )
                    }
                }

                IconButton(onClick = onMoveDown) {
                    Icon(
                        imageVector = Icons.Default.KeyboardArrowDown,
                        contentDescription = stringResource(R.string.overlay_move_down),
                        tint = MaterialTheme.colorScheme.primary
                    )
                }
            }

            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    SliderComponent(
                        stringResource(R.string.overlay_scale),
                        scaleValue,
                        onScaleChange
                    )
                    Spacer(modifier = Modifier.height(6.dp))
                    SliderComponent(
                        stringResource(R.string.overlay_opacity),
                        opacityValue,
                        onOpacityChange
                    )
                }

                Spacer(modifier = Modifier.width(8.dp))

                IconButton(
                    onClick = onResetClick,
                    modifier = Modifier.size(40.dp),
                    colors = IconButtonDefaults.filledTonalIconButtonColors()
                ) {
                    Icon(
                        painter = painterResource(id = R.drawable.ic_restore),
                        contentDescription = stringResource(R.string.action_reset),
                        tint = MaterialTheme.colorScheme.primary
                    )
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SliderComponent(label: String, value: Float, onValueChange: (Float) -> Unit) {
    Column {
        Text(
            text = stringResource(R.string.overlay_slider_value, label, value.roundToInt()),
            fontSize = 12.sp,
            color = MaterialTheme.colorScheme.onSurface
        )
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = 0f..100f,
            thumb = {
                Box(
                    modifier = Modifier
                        .size(12.dp)
                        .background(MaterialTheme.colorScheme.primary, shape = CircleShape)
                )
            },
            modifier = Modifier.padding(horizontal = 16.dp).height(20.dp)
        )
    }
}

@Composable
fun ResetDialog(buttonName: String, onConfirm: () -> Unit, onDismiss: () -> Unit) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(text = stringResource(R.string.overlay_reset_title, buttonName)) },
        text = { Text(text = stringResource(R.string.overlay_reset_message)) },
        confirmButton = {
            TextButton(onClick = onConfirm) {
                Text(text = stringResource(R.string.action_confirm))
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text(text = stringResource(R.string.action_cancel))
            }
        }
    )
}

@Preview
@Composable
fun PreviewOverlayEditScreen() {
    OverlayEditScreen()
}
