package net.rpcs3.ui.library

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.SystemClock
import android.view.MotionEvent
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.compose.animation.core.animateIntAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.wrapContentHeight
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.outlined.ArrowBack
import androidx.compose.material.icons.outlined.Close
import androidx.compose.material.icons.outlined.Home
import androidx.compose.material.icons.outlined.Refresh
import androidx.compose.material3.Icon
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import net.rpcs3.ui.theme.SettingsStyle

const val CompatibilityHomeUrl = "https://rpcs3.net/compatibility"

private const val HeaderHideTravel = 96
private const val HeaderShowHoldMillis = 500L
private const val HeaderHideHoldMillis = 180L
private const val HeaderStickyBelowAnchor = 320
private const val UpScrollGapMillis = 140L

private const val AnchorToSearchScript = """
(function(){
  if (window.__rpcs3Anchored) return;
  window.__rpcs3Anchored = true;
  var cancelled = false;
  window.addEventListener('touchstart', function(){ cancelled = true; }, { once: true, passive: true });
  function unblock(){
    var stuck = document.querySelectorAll('.mini-menu-con-dimmer.object-show');
    for (var i = 0; i < stuck.length; i++) stuck[i].classList.remove('object-show');
  }
  function anchor(){
    if (cancelled) return;
    unblock();
    var box = document.querySelector('#game-search') || document.querySelector('input[name="g"]');
    if (!box) return;
    var nav = document.querySelector('.menu-con-container');
    var pinned = nav ? nav.getBoundingClientRect().height : 0;
    var y = box.getBoundingClientRect().top + window.pageYOffset - pinned - 10;
    window.scrollTo(0, Math.max(0, y));
  }
  if (document.readyState === 'complete') {
    setTimeout(anchor, 80);
  } else {
    window.addEventListener('load', function(){ setTimeout(anchor, 80); }, { once: true });
  }
})();
"""

@SuppressLint("SetJavaScriptEnabled", "ClickableViewAccessibility")
@Composable
fun CompatibilityScreen(
    modifier: Modifier = Modifier,
    header: @Composable () -> Unit
) {
    var webView by remember { mutableStateOf<WebView?>(null) }
    var canGoBack by remember { mutableStateOf(false) }
    var loading by remember { mutableStateOf(false) }
    var currentUrl by remember { mutableStateOf(CompatibilityHomeUrl) }
    var headerHeight by remember { mutableIntStateOf(0) }
    var headerVisible by remember { mutableStateOf(true) }

    val slide by animateIntAsState(
        targetValue = if (headerVisible) 0 else -headerHeight,
        label = "compatibilityHeader"
    )

    BoxWithConstraints(
        modifier = modifier
            .fillMaxSize()
            .clipToBounds()
            .background(SettingsStyle.BgDeep)
    ) {
        val overscan = with(LocalDensity.current) { headerHeight.toDp() }

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .wrapContentHeight(align = Alignment.Top, unbounded = true)
                .height(maxHeight + overscan)
                .offset { IntOffset(0, slide) }
        ) {
            Box(modifier = Modifier.onSizeChanged { headerHeight = it.height }) {
                header()
            }

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(SettingsStyle.SidebarBg)
                    .padding(horizontal = 8.dp, vertical = 6.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(2.dp)
            ) {
                NavIcon(
                    icon = Icons.AutoMirrored.Outlined.ArrowBack,
                    contentDescription = "Back",
                    enabled = canGoBack,
                    onClick = { webView?.goBack() }
                )
                NavIcon(
                    icon = if (loading) Icons.Outlined.Close else Icons.Outlined.Refresh,
                    contentDescription = if (loading) "Stop" else "Refresh",
                    onClick = {
                        if (loading) webView?.stopLoading() else webView?.reload()
                    }
                )
                NavIcon(
                    icon = Icons.Outlined.Home,
                    contentDescription = "Home",
                    onClick = { webView?.loadUrl(CompatibilityHomeUrl) }
                )

                Text(
                    text = currentUrl,
                    modifier = Modifier
                        .weight(1f)
                        .padding(horizontal = 8.dp),
                    color = SettingsStyle.TextSecondary,
                    fontSize = 11.sp,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
            }

            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(2.dp)
                    .background(SettingsStyle.Divider)
            ) {
                if (loading) {
                    LinearProgressIndicator(
                        modifier = Modifier.fillMaxSize(),
                        color = SettingsStyle.AccentBlue,
                        trackColor = SettingsStyle.SliderInactive
                    )
                }
            }

            AndroidView(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
                factory = { context ->
                    var touched = false
                    var travel = 0
                    var anchorY = 0
                    var upSince = 0L
                    var lastUp = 0L
                    var downSince = 0L
                    var lastDown = 0L
                    WebView(context).apply {
                        settings.javaScriptEnabled = true
                        settings.domStorageEnabled = true
                        settings.useWideViewPort = true
                        settings.loadWithOverviewMode = true
                        settings.builtInZoomControls = true
                        settings.displayZoomControls = false
                        overScrollMode = WebView.OVER_SCROLL_NEVER
                        webViewClient = object : WebViewClient() {
                            override fun onPageStarted(
                                view: WebView?,
                                url: String?,
                                favicon: Bitmap?
                            ) {
                                loading = true
                                touched = false
                                travel = 0
                                anchorY = 0
                                upSince = 0L
                                downSince = 0L
                                headerVisible = true
                                if (url != null) currentUrl = url
                                canGoBack = view?.canGoBack() == true
                            }

                            override fun onPageFinished(view: WebView?, url: String?) {
                                loading = false
                                if (url != null) currentUrl = url
                                canGoBack = view?.canGoBack() == true
                                if (url != null && url.contains("/compatibility")) {
                                    view?.evaluateJavascript(AnchorToSearchScript, null)
                                }
                            }
                        }
                        setOnTouchListener { view, event ->
                            when (event.actionMasked) {
                                MotionEvent.ACTION_DOWN -> {
                                    touched = true
                                    view.parent?.requestDisallowInterceptTouchEvent(true)
                                }

                                MotionEvent.ACTION_UP,
                                MotionEvent.ACTION_CANCEL ->
                                    view.parent?.requestDisallowInterceptTouchEvent(false)
                            }
                            false
                        }
                        setOnScrollChangeListener { _, _, scrollY, _, oldScrollY ->
                            if (!touched) {
                                anchorY = scrollY
                            } else {
                                val delta = scrollY - oldScrollY
                                val now = SystemClock.uptimeMillis()
                                if (scrollY <= anchorY) {
                                    travel = 0
                                    upSince = 0L
                                    downSince = 0L
                                    headerVisible = true
                                } else if (delta > 0 && scrollY > anchorY + HeaderStickyBelowAnchor) {
                                    upSince = 0L
                                    if (downSince == 0L || now - lastDown > UpScrollGapMillis) {
                                        downSince = now
                                        travel = 0
                                    }
                                    lastDown = now
                                    travel += delta
                                    if (travel > HeaderHideTravel &&
                                        now - downSince >= HeaderHideHoldMillis
                                    ) {
                                        travel = 0
                                        headerVisible = false
                                    }
                                } else if (delta < 0) {
                                    travel = 0
                                    downSince = 0L
                                    if (upSince == 0L || now - lastUp > UpScrollGapMillis) {
                                        upSince = now
                                    }
                                    lastUp = now
                                    if (now - upSince >= HeaderShowHoldMillis) {
                                        headerVisible = true
                                    }
                                }
                            }
                        }
                        loadUrl(CompatibilityHomeUrl)
                        webView = this
                    }
                }
            )
        }
    }
}

@Composable
private fun NavIcon(
    icon: ImageVector,
    contentDescription: String,
    onClick: () -> Unit,
    enabled: Boolean = true
) {
    Box(
        modifier = Modifier
            .size(38.dp)
            .clip(RoundedCornerShape(10.dp))
            .clickable(enabled = enabled, onClick = onClick),
        contentAlignment = Alignment.Center
    ) {
        Icon(
            imageVector = icon,
            contentDescription = contentDescription,
            tint = if (enabled) SettingsStyle.TextPrimary else SettingsStyle.TextDim,
            modifier = Modifier.size(20.dp)
        )
    }
}
