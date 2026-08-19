package dev.qcom.bandmenu.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.ui.state.ToggleableState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.unit.dp
import com.kyant.backdrop.Backdrop
import dev.qcom.bandmenu.BandConstants
import dev.qcom.bandmenu.HardwareBands
import dev.qcom.bandmenu.ModemState
import dev.qcom.bandmenu.NrMode
import dev.qcom.bandmenu.RatType
import dev.qcom.bandmenu.SimState
import top.yukonga.miuix.kmp.basic.Button
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.Checkbox
import top.yukonga.miuix.kmp.basic.CircularProgressIndicator
import top.yukonga.miuix.kmp.basic.DropdownEntry
import top.yukonga.miuix.kmp.basic.DropdownItem
import top.yukonga.miuix.kmp.basic.PullToRefresh
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.SmallTopAppBar
import top.yukonga.miuix.kmp.basic.SnackbarHostState
import top.yukonga.miuix.kmp.basic.TabRowWithContour
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.basic.TextButton
import top.yukonga.miuix.kmp.basic.TopAppBarDefaults
import top.yukonga.miuix.kmp.preference.WindowDropdownPreference
import top.yukonga.miuix.kmp.theme.MiuixTheme
import kotlinx.coroutines.delay

private class SlotBandState {
    val ratChecked = mutableStateMapOf<RatType, Boolean>()
    val gsmChecked = mutableStateMapOf<Int, Boolean>()
    val wcdmaChecked = mutableStateMapOf<Int, Boolean>()
    val lteChecked = mutableStateMapOf<Int, Boolean>()
    val nrNsaChecked = mutableStateMapOf<Int, Boolean>()
    val nrSaChecked = mutableStateMapOf<Int, Boolean>()
    val nrChecked = mutableStateMapOf<Int, Boolean>()
    var nrMode by mutableStateOf(NrMode.BOTH)
}

@Composable
fun BandLockScreen(
    modemState: ModemState?,
    isLoading: Boolean,
    refreshingSlots: Set<Int>,
    onRefresh: (Int) -> Unit,
    refreshKey0: Int,
    refreshKey1: Int,
    onApply: (Int, SimState) -> Unit,
    onReset: (Int) -> Unit,
    nrIndependentSupported: Boolean? = null,
    contentPadding: PaddingValues = PaddingValues(),
    snackbarHostState: SnackbarHostState,
    backdrop: Backdrop? = null
) {
    val density = LocalDensity.current
    val navbarHeightDp = 64.dp
    val navInset = WindowInsets.navigationBars.asPaddingValues(density).calculateBottomPadding()
    val navbarSpace = navbarHeightDp + 16.dp + navInset
    val applyResetSpace = 72.dp
    val statusBarInset = WindowInsets.statusBars.asPaddingValues(density).calculateTopPadding()
    val topBarHeight = statusBarInset + TopAppBarDefaults.CollapsedHeight

    val hapticFeedback = LocalHapticFeedback.current

    val hardware = modemState?.hardware
    val useIndependentLock = nrIndependentSupported == true

    var selectedSim by remember { mutableIntStateOf(0) }
    val pagerState = rememberPagerState(pageCount = { 2 })
    val slotStates = remember { arrayOf(SlotBandState(), SlotBandState()) }

    LaunchedEffect(selectedSim) {
        if (pagerState.targetPage != selectedSim) {
            pagerState.animateScrollToPage(selectedSim)
        }
    }
    LaunchedEffect(pagerState.targetPage) {
        selectedSim = pagerState.targetPage
    }

    Box(modifier = Modifier.fillMaxSize().padding(contentPadding)) {
        if (isLoading || hardware == null) {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                CircularProgressIndicator()
            }
        } else {
            Box(modifier = Modifier.fillMaxSize()) {
                PullToRefresh(
                    isRefreshing = refreshingSlots.contains(selectedSim),
                    onRefresh = { onRefresh(selectedSim) },
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = PaddingValues(top = topBarHeight)
                ) {
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .verticalScroll(rememberScrollState())
                            .padding(top = topBarHeight)
                            .padding(bottom = navbarSpace + applyResetSpace)
                    ) {
                        TabRowWithContour(
                            tabs = listOf("SIM 1", "SIM 2"),
                            selectedTabIndex = selectedSim,
                            onTabSelected = {
                                hapticFeedback.performHapticFeedback(HapticFeedbackType.Confirm)
                                selectedSim = it
                            },
                            modifier = Modifier.fillMaxWidth()
                        )

                        Spacer(modifier = Modifier.height(16.dp))

                        HorizontalPager(
                            state = pagerState,
                            beyondViewportPageCount = 1,
                            userScrollEnabled = true,
                            modifier = Modifier.fillMaxWidth()
                        ) { page ->
                            val simState = if (page == 0) modemState!!.sim1 else modemState!!.sim2
                            val refreshKey = if (page == 0) refreshKey0 else refreshKey1
                            SimBandLockPage(
                                state = slotStates[page],
                                simState = simState,
                                hardware = hardware,
                                useIndependentLock = useIndependentLock,
                                refreshKey = refreshKey
                            )
                        }
                    }
                }

                if (backdrop != null) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(Color.Black.copy(alpha = 0.6f))
                    ) {
                        SmallTopAppBar(title = "频段", color = Color.Transparent)
                    }
                } else {
                    SmallTopAppBar(title = "频段")
                }
            }

            Row(
                modifier = Modifier
                    .align(Alignment.BottomCenter)
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp)
                    .padding(bottom = navbarSpace + 24.dp),
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                TextButton(
                    text = "重置",
                    onClick = {
                        hapticFeedback.performHapticFeedback(HapticFeedbackType.Confirm)
                        onReset(selectedSim)
                    },
                    modifier = Modifier
                        .weight(1f)
                        .border(1.dp, MiuixTheme.colorScheme.outline, RoundedCornerShape(16.dp))
                )
                Button(
                    onClick = {
                        hapticFeedback.performHapticFeedback(HapticFeedbackType.Confirm)
                        val s = slotStates[selectedSim]
                        val nrBands = s.nrChecked.filterValues { it }.keys
                        val state = SimState(
                            ratMask = s.ratChecked.filterValues { it }.keys,
                            gsmBands = s.gsmChecked.filterValues { it }.keys,
                            wcdmaBands = s.wcdmaChecked.filterValues { it }.keys,
                            lteBands = s.lteChecked.filterValues { it }.keys,
                            nrNsaBands = if (useIndependentLock) s.nrNsaChecked.filterValues { it }.keys else nrBands,
                            nrSaBands = if (useIndependentLock) s.nrSaChecked.filterValues { it }.keys else nrBands,
                            nrMode = s.nrMode
                        )
                        onApply(selectedSim, state)
                    },
                    modifier = Modifier
                        .weight(1f)
                        .border(1.dp, MiuixTheme.colorScheme.outline, RoundedCornerShape(16.dp)),
                    colors = ButtonDefaults.buttonColorsPrimary()
                ) {
                    Text("应用")
                }
            }
        }
    }
}

@Composable
private fun SimBandLockPage(
    state: SlotBandState,
    simState: SimState?,
    hardware: HardwareBands,
    useIndependentLock: Boolean,
    refreshKey: Int
) {
    val hapticFeedback = LocalHapticFeedback.current

    LaunchedEffect(simState, refreshKey) {
        simState?.let { s ->
            state.ratChecked.clear()
            BandConstants.ALL_RAT_TYPES.forEach { rt ->
                state.ratChecked[rt] = s.ratMask.contains(rt)
            }
            state.gsmChecked.clear()
            s.gsmBands.forEach { state.gsmChecked[it] = true }
            state.wcdmaChecked.clear()
            s.wcdmaBands.forEach { state.wcdmaChecked[it] = true }
            state.lteChecked.clear()
            s.lteBands.forEach { state.lteChecked[it] = true }
            state.nrNsaChecked.clear()
            s.nrNsaBands.forEach { state.nrNsaChecked[it] = true }
            state.nrSaChecked.clear()
            s.nrSaBands.forEach { state.nrSaChecked[it] = true }
            state.nrChecked.clear()
            (s.nrNsaBands + s.nrSaBands).forEach { state.nrChecked[it] = true }
            state.nrMode = s.nrMode
        }
    }

    Column(modifier = Modifier.fillMaxWidth()) {
        SmallTitle("网络制式锁定")
        val supportedRats = BandConstants.ALL_RAT_TYPES.filter { rt ->
            when (rt) {
                RatType.GSM -> hardware.gsm.isNotEmpty()
                RatType.WCDMA -> hardware.wcdma.isNotEmpty()
                RatType.LTE -> hardware.lte.isNotEmpty()
                RatType.NR -> hardware.nr.isNotEmpty()
            }
        }
        val isAuto = supportedRats.all { state.ratChecked[it] == true }
        val ratSummary = if (isAuto && supportedRats.isNotEmpty()) "自动（全部制式）"
            else if (state.ratChecked.values.all { it != true } || supportedRats.isEmpty()) "无"
            else supportedRats.filter { state.ratChecked[it] == true }.joinToString(", ") { it.name }
        Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
            WindowDropdownPreference(
                entries = listOf(
                    DropdownEntry(items = listOf(
                        DropdownItem(
                            text = "自动（全部制式）",
                            selected = isAuto,
                            onClick = {
                                val newAuto = !isAuto
                                supportedRats.forEach { state.ratChecked[it] = newAuto }
                            }
                        )
                    )),
                    DropdownEntry(items = supportedRats.map { rt ->
                        DropdownItem(
                            text = rt.name,
                            selected = state.ratChecked[rt] == true,
                            onClick = {
                                state.ratChecked[rt] = !(state.ratChecked[rt] == true)
                            }
                        )
                    })
                ),
                title = "网络制式锁定",
                summary = ratSummary,
                showValue = false,
                collapseOnSelection = false
            )
        }

        if (hardware.nr.isNotEmpty()) {
            Spacer(modifier = Modifier.height(16.dp))
            SmallTitle("NR 模式")
            val nrModeIndex = when (state.nrMode) {
                NrMode.BOTH, NrMode.UNKNOWN -> 0
                NrMode.SA -> 1
                NrMode.NSA -> 2
            }
            val nrModeEnabled = useIndependentLock
            TabRowWithContour(
                tabs = listOf("SA/NSA", "SA", "NSA"),
                selectedTabIndex = nrModeIndex,
                onTabSelected = { index ->
                    if (nrModeEnabled) {
                        hapticFeedback.performHapticFeedback(HapticFeedbackType.Confirm)
                        state.nrMode = when (index) {
                            0 -> NrMode.BOTH
                            1 -> NrMode.SA
                            2 -> NrMode.NSA
                            else -> NrMode.BOTH
                        }
                    }
                },
                modifier = Modifier
                    .fillMaxWidth()
                    .then(if (nrModeEnabled) Modifier else Modifier.alpha(0.4f))
            )
        }

        Spacer(modifier = Modifier.height(16.dp))
        SmallTitle("频段锁定")
        Spacer(modifier = Modifier.height(4.dp))

        val allNrEnabled = if (useIndependentLock)
            hardware.nr.all { state.nrNsaChecked[it] == true } && hardware.nr.all { state.nrSaChecked[it] == true }
        else
            hardware.nr.all { state.nrChecked[it] == true }
        val allNsaEnabled = if (useIndependentLock) hardware.nr.all { state.nrNsaChecked[it] == true } else allNrEnabled
        val allSaEnabled = if (useIndependentLock) hardware.nr.all { state.nrSaChecked[it] == true } else allNrEnabled
        val allLteEnabled = hardware.lte.all { state.lteChecked[it] == true }
        val allWcdmaEnabled = hardware.wcdma.all { state.wcdmaChecked[it] == true }
        val allGsmEnabled = hardware.gsm.all { state.gsmChecked[it] == true }
        val all5gEnabled = allNrEnabled
        val allBandsEnabled = allGsmEnabled && allWcdmaEnabled && allLteEnabled && allNrEnabled

        var recentlyClicked by remember { mutableStateOf<Pair<Int, Boolean>?>(null) }
        LaunchedEffect(recentlyClicked) {
            if (recentlyClicked != null) {
                delay(2000)
                recentlyClicked = null
            }
        }
        fun emojiFor(index: Int): String {
            val rc = recentlyClicked ?: return ""
            if (rc.first != index) return ""
            return if (rc.second) " ✓" else " ✗"
        }

        val hasNrHardware = hardware.nr.isNotEmpty()
        val quickItems = if (useIndependentLock) {
            if (hasNrHardware) {
                listOf(
                    "全部频段（所有制式）" to 0,
                    "全部 5G 频段（NSA+SA）" to 1,
                    "全部 NR-SA 频段" to 2,
                    "全部 NR-NSA 频段" to 3,
                    "全部 LTE 频段" to 4,
                    "全部 WCDMA 频段" to 5,
                    "全部 GSM 频段" to 6
                )
            } else {
                listOf(
                    "全部频段（所有制式）" to 0,
                    "全部 LTE 频段" to 4,
                    "全部 WCDMA 频段" to 5,
                    "全部 GSM 频段" to 6
                )
            }
        } else {
            if (hasNrHardware) {
                listOf(
                    "全部频段（所有制式）" to 0,
                    "全部 NR 频段" to 1,
                    "全部 LTE 频段" to 4,
                    "全部 WCDMA 频段" to 5,
                    "全部 GSM 频段" to 6
                )
            } else {
                listOf(
                    "全部频段（所有制式）" to 0,
                    "全部 LTE 频段" to 4,
                    "全部 WCDMA 频段" to 5,
                    "全部 GSM 频段" to 6
                )
            }
        }

        val quickGroups = listOf(
            quickItems.filter { it.second == 0 },
            quickItems.filter { it.second in 1..3 },
            quickItems.filter { it.second in 4..6 }
        ).filter { it.isNotEmpty() }

        Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
            WindowDropdownPreference(
                entries = quickGroups.map { group ->
                    DropdownEntry(items = group.map { (label, idx) ->
                        DropdownItem(text = "$label${emojiFor(idx)}", onClick = {
                            when (idx) {
                                0 -> {
                                    val newState = !allBandsEnabled
                                    hardware.gsm.forEach { state.gsmChecked[it] = newState }
                                    hardware.wcdma.forEach { state.wcdmaChecked[it] = newState }
                                    hardware.lte.forEach { state.lteChecked[it] = newState }
                                    if (useIndependentLock) {
                                        hardware.nr.forEach {
                                            state.nrNsaChecked[it] = newState; state.nrSaChecked[it] = newState
                                        }
                                    } else {
                                        hardware.nr.forEach { state.nrChecked[it] = newState }
                                    }
                                    recentlyClicked = idx to newState
                                }
                                1 -> {
                                    val newState = !all5gEnabled
                                    if (useIndependentLock) {
                                        hardware.nr.forEach {
                                            state.nrNsaChecked[it] = newState; state.nrSaChecked[it] = newState
                                        }
                                    } else {
                                        hardware.nr.forEach { state.nrChecked[it] = newState }
                                    }
                                    recentlyClicked = idx to newState
                                }
                                2 -> {
                                    val newState = !allSaEnabled
                                    hardware.nr.forEach { state.nrSaChecked[it] = newState }
                                    recentlyClicked = idx to newState
                                }
                                3 -> {
                                    val newState = !allNsaEnabled
                                    hardware.nr.forEach { state.nrNsaChecked[it] = newState }
                                    recentlyClicked = idx to newState
                                }
                                4 -> {
                                    val newState = !allLteEnabled
                                    hardware.lte.forEach { state.lteChecked[it] = newState }
                                    recentlyClicked = idx to newState
                                }
                                5 -> {
                                    val newState = !allWcdmaEnabled
                                    hardware.wcdma.forEach { state.wcdmaChecked[it] = newState }
                                    recentlyClicked = idx to newState
                                }
                                6 -> {
                                    val newState = !allGsmEnabled
                                    hardware.gsm.forEach { state.gsmChecked[it] = newState }
                                    recentlyClicked = idx to newState
                                }
                            }
                        })
                    })
                },
                title = "快速选择",
                summary = "切换频段组",
                showValue = false,
                collapseOnSelection = false
            )
        }

        Spacer(modifier = Modifier.height(8.dp))

        if (hardware.nr.isNotEmpty()) {
            if (useIndependentLock) {
                SmallTitle("NR-SA")
                Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    BandCheckboxGrid(hardware.nr.sorted(), state.nrSaChecked, "n")
                }
                SmallTitle("NR-NSA")
                Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    BandCheckboxGrid(hardware.nr.sorted(), state.nrNsaChecked, "n")
                }
            } else {
                SmallTitle("NR")
                Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    BandCheckboxGrid(hardware.nr.sorted(), state.nrChecked, "n")
                }
            }
        }
        if (hardware.lte.isNotEmpty()) {
            SmallTitle("LTE")
            Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                BandCheckboxGrid(hardware.lte.sorted(), state.lteChecked, "B")
            }
        }
        if (hardware.wcdma.isNotEmpty()) {
            SmallTitle("WCDMA")
            Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                BandCheckboxGrid(hardware.wcdma.sorted(), state.wcdmaChecked, "B")
            }
        }
        if (hardware.gsm.isNotEmpty()) {
            SmallTitle("GSM")
            Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                BandCheckboxGrid(hardware.gsm.sorted(), state.gsmChecked, "")
            }
        }
        Spacer(modifier = Modifier.height(16.dp))
    }
}

@Composable
private fun BandCheckboxGrid(
    bands: List<Int>,
    checked: MutableMap<Int, Boolean>,
    prefix: String
) {
    val density = LocalDensity.current
    val rowMargin = with(density) { 3f.toDp() }
    val rowCount = (bands.size + 3) / 4
    Column(modifier = Modifier.padding(horizontal = 8.dp, vertical = 8.dp)) {
        bands.chunked(4).forEachIndexed { index, group ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(
                        top = if (index == 0) 0.dp else rowMargin,
                        bottom = if (index == rowCount - 1) 0.dp else rowMargin
                    ),
                horizontalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                group.forEach { band ->
                    Row(
                        modifier = Modifier.weight(1f),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        val isChecked = checked[band] == true
                        Checkbox(
                            state = if (isChecked) ToggleableState.On else ToggleableState.Off,
                            onClick = { checked[band] = !isChecked }
                        )
                        Text(
                            text = "$prefix$band",
                            style = MiuixTheme.textStyles.body1,
                            color = MiuixTheme.colorScheme.onBackground
                        )
                    }
                }
                repeat(4 - group.size) {
                    Spacer(modifier = Modifier.weight(1f))
                }
            }
        }
    }
}
