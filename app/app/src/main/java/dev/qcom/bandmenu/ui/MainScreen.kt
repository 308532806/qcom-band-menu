package dev.qcom.bandmenu.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.foundation.layout.size
import com.kyant.backdrop.backdrops.layerBackdrop
import com.kyant.backdrop.backdrops.rememberLayerBackdrop
import dev.qcom.bandmenu.ui.component.FloatingBottomBar
import dev.qcom.bandmenu.ui.component.FloatingBottomBarItem
import top.yukonga.miuix.kmp.basic.Icon
import top.yukonga.miuix.kmp.basic.Scaffold
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.Info
import top.yukonga.miuix.kmp.icon.extended.Phone
import top.yukonga.miuix.kmp.theme.MiuixTheme

@Composable
fun MainScreen(
    onApply: (Int, dev.qcom.bandmenu.SimState) -> Unit,
    onReset: (Int) -> Unit,
    refreshingSlots: Set<Int>,
    onRefresh: (Int) -> Unit,
    refreshKey0: Int,
    refreshKey1: Int,
    modemState: dev.qcom.bandmenu.ModemState?,
    isLoading: Boolean,
    showRootDeniedDialog: Boolean,
    onRootRetry: () -> Unit,
    onDismissRootDialog: () -> Unit,
    showErrorDialog: Boolean,
    errorDialogTitle: String,
    errorDialogMessage: String,
    onDismissErrorDialog: () -> Unit,
    snackbarHostState: top.yukonga.miuix.kmp.basic.SnackbarHostState,
    snackbarMessage: String?,
    onSnackbarShown: () -> Unit,
    debugEnabled: Boolean,
    onDebugToggle: () -> Unit
) {
    var selectedIndex by remember { mutableIntStateOf(0) }
    val pagerState = rememberPagerState(pageCount = { 3 })

    LaunchedEffect(selectedIndex) {
        if (pagerState.targetPage != selectedIndex) {
            pagerState.animateScrollToPage(selectedIndex)
        }
    }
    LaunchedEffect(pagerState.targetPage) {
        selectedIndex = pagerState.targetPage
    }

    val surfaceColor = MiuixTheme.colorScheme.surface
    val backdrop = rememberLayerBackdrop(onDraw = {
        drawRect(surfaceColor)
        drawContent()
    })

    Scaffold(
        modifier = Modifier.fillMaxSize()
    ) { innerPadding ->
        val topPadding = PaddingValues(top = innerPadding.calculateTopPadding())

        Box(modifier = Modifier.fillMaxSize()) {
            HorizontalPager(
                state = pagerState,
                beyondViewportPageCount = 2,
                userScrollEnabled = true,
                modifier = Modifier.fillMaxSize().layerBackdrop(backdrop)
            ) { page ->
                when (page) {
                    0 -> BandLockScreen(
                        slot = 0,
                        modemState = modemState,
                        isLoading = isLoading,
                        isRefreshing = refreshingSlots.contains(0),
                        onRefresh = { onRefresh(0) },
                        refreshKey = refreshKey0,
                        onApply = { state -> onApply(0, state) },
                        onReset = { onReset(0) },
                        contentPadding = topPadding,
                        snackbarHostState = snackbarHostState,
                        backdrop = backdrop
                    )
                    1 -> BandLockScreen(
                        slot = 1,
                        modemState = modemState,
                        isLoading = isLoading,
                        isRefreshing = refreshingSlots.contains(1),
                        onRefresh = { onRefresh(1) },
                        refreshKey = refreshKey1,
                        onApply = { state -> onApply(1, state) },
                        onReset = { onReset(1) },
                        contentPadding = topPadding,
                        snackbarHostState = snackbarHostState,
                        backdrop = backdrop
                    )
                    else -> InfoScreen(
                        contentPadding = topPadding,
                        debugEnabled = debugEnabled,
                        onDebugToggle = onDebugToggle
                    )
                }
            }

            // Navbar overlay (floating on top of content, edge-to-edge)
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .align(Alignment.BottomCenter)
                    .navigationBarsPadding()
                    .padding(bottom = 16.dp),
                contentAlignment = Alignment.BottomCenter
            ) {
                FloatingBottomBar(
                    selectedIndex = { selectedIndex },
                    onSelected = { selectedIndex = it },
                    backdrop = backdrop,
                    tabsCount = 3
                ) {
                    FloatingBottomBarItem(onClick = { selectedIndex = 0 }) {
                        Icon(imageVector = MiuixIcons.Phone, contentDescription = "SIM 1",
                            tint = MiuixTheme.colorScheme.onBackground, modifier = Modifier.size(18.dp))
                        Text("SIM 1", style = MiuixTheme.textStyles.body2.copy(fontSize = 12.sp))
                    }
                    FloatingBottomBarItem(onClick = { selectedIndex = 1 }) {
                        Icon(imageVector = MiuixIcons.Phone, contentDescription = "SIM 2",
                            tint = MiuixTheme.colorScheme.onBackground, modifier = Modifier.size(18.dp))
                        Text("SIM 2", style = MiuixTheme.textStyles.body2.copy(fontSize = 12.sp))
                    }
                    FloatingBottomBarItem(onClick = { selectedIndex = 2 }) {
                        Icon(imageVector = MiuixIcons.Info, contentDescription = "Info",
                            tint = MiuixTheme.colorScheme.onBackground, modifier = Modifier.size(18.dp))
                        Text("Info", style = MiuixTheme.textStyles.body2.copy(fontSize = 12.sp))
                    }
                }
            }

            // Snackbar overlay (above the floating navbar)
            val density = LocalDensity.current
            val navInset = WindowInsets.navigationBars.asPaddingValues(density).calculateBottomPadding()
            val navbarHeightDp = with(density) { 128f.toDp() }
            top.yukonga.miuix.kmp.basic.SnackbarHost(
                state = snackbarHostState,
                modifier = Modifier
                    .align(Alignment.BottomCenter)
                    .navigationBarsPadding()
                    .padding(bottom = navbarHeightDp + 16.dp + navInset + 50.dp + 8.dp)
            ) { data ->
                top.yukonga.miuix.kmp.basic.Snackbar(
                    data = data,
                    colors = top.yukonga.miuix.kmp.basic.SnackbarDefaults.snackbarColors(
                        containerColor = androidx.compose.ui.graphics.Color.White,
                        contentColor = androidx.compose.ui.graphics.Color.Black,
                    )
                )
            }

            // Full-screen loading overlay (covers all pages until data is ready)
            if (isLoading) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(MiuixTheme.colorScheme.background),
                    contentAlignment = Alignment.Center
                ) {
                    top.yukonga.miuix.kmp.basic.CircularProgressIndicator()
                }
            }

            // Status bar fade overlay
            val statusBarHeight = with(density) { WindowInsets.statusBars.getTop(density).toDp() }
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(statusBarHeight + 24.dp)
                    .align(Alignment.TopStart)
                    .background(
                        Brush.verticalGradient(
                            colors = listOf(surfaceColor, surfaceColor.copy(alpha = 0f))
                        )
                    )
            )

            // Error dialogs
            if (showRootDeniedDialog) {
                top.yukonga.miuix.kmp.window.WindowDialog(
                    show = true,
                    title = "Root Access Required",
                    summary = "This app requires root access to communicate with the Qualcomm modem. Please grant root access and retry.",
                    onDismissRequest = onDismissRootDialog,
                    content = {
                        top.yukonga.miuix.kmp.basic.TextButton(
                            text = "Retry",
                            onClick = onRootRetry,
                            modifier = Modifier.fillMaxWidth(),
                            colors = top.yukonga.miuix.kmp.basic.ButtonDefaults.textButtonColorsPrimary()
                        )
                    }
                )
            }

            if (showErrorDialog) {
                top.yukonga.miuix.kmp.window.WindowDialog(
                    show = true,
                    title = errorDialogTitle,
                    summary = errorDialogMessage,
                    onDismissRequest = onDismissErrorDialog,
                    content = {
                        top.yukonga.miuix.kmp.basic.TextButton(
                            text = "OK",
                            onClick = onDismissErrorDialog,
                            modifier = Modifier.fillMaxWidth()
                        )
                    }
                )
            }

            // Snackbar
            LaunchedEffect(snackbarMessage) {
                if (snackbarMessage != null) {
                    snackbarHostState.showSnackbar(snackbarMessage)
                    onSnackbarShown()
                }
            }
        }
    }
}
