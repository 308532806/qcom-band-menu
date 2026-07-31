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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.state.ToggleableState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.unit.dp
import com.kyant.backdrop.Backdrop
import com.kyant.backdrop.drawBackdrop
import com.kyant.backdrop.effects.blur
import dev.qcom.bandmenu.BandConstants
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

@Composable
fun BandLockScreen(
    slot: Int,
    modemState: ModemState?,
    isLoading: Boolean,
    isRefreshing: Boolean,
    onRefresh: () -> Unit,
    refreshKey: Int,
    onApply: (SimState) -> Unit,
    onReset: () -> Unit,
    contentPadding: PaddingValues = PaddingValues(),
    snackbarHostState: SnackbarHostState,
    backdrop: Backdrop? = null
) {
    val simNumber = slot + 1
    val scrollState = rememberScrollState()
    val density = LocalDensity.current
    val navbarHeightDp = with(density) { 128f.toDp() }
    val navInset = WindowInsets.navigationBars.asPaddingValues(density).calculateBottomPadding()
    val navbarSpace = navbarHeightDp + 16.dp + navInset
    val applyResetSpace = 72.dp
    val statusBarInset = WindowInsets.statusBars.asPaddingValues(density).calculateTopPadding()
    val topBarHeight = statusBarInset + TopAppBarDefaults.CollapsedHeight

    val currentSimState = if (slot == 0) modemState?.sim1 else modemState?.sim2
    val hardware = modemState?.hardware

    val ratChecked = remember { mutableStateMapOf<RatType, Boolean>() }
    val gsmChecked = remember { mutableStateMapOf<Int, Boolean>() }
    val wcdmaChecked = remember { mutableStateMapOf<Int, Boolean>() }
    val lteChecked = remember { mutableStateMapOf<Int, Boolean>() }
    val nrNsaChecked = remember { mutableStateMapOf<Int, Boolean>() }
    val nrSaChecked = remember { mutableStateMapOf<Int, Boolean>() }
    var nrMode by remember { mutableStateOf(NrMode.BOTH) }

    LaunchedEffect(currentSimState, refreshKey) {
        currentSimState?.let { state ->
            ratChecked.clear()
            BandConstants.ALL_RAT_TYPES.forEach { rt ->
                ratChecked[rt] = state.ratMask.contains(rt)
            }
            gsmChecked.clear()
            state.gsmBands.forEach { gsmChecked[it] = true }
            wcdmaChecked.clear()
            state.wcdmaBands.forEach { wcdmaChecked[it] = true }
            lteChecked.clear()
            state.lteBands.forEach { lteChecked[it] = true }
            nrNsaChecked.clear()
            state.nrNsaBands.forEach { nrNsaChecked[it] = true }
            nrSaChecked.clear()
            state.nrSaBands.forEach { nrSaChecked[it] = true }
            nrMode = state.nrMode
        }
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
                    isRefreshing = isRefreshing,
                    onRefresh = onRefresh,
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = PaddingValues(top = topBarHeight)
                ) {
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .verticalScroll(scrollState)
                            .padding(top = topBarHeight)
                            .padding(bottom = navbarSpace + applyResetSpace)
                    ) {

                SmallTitle("RAT lock")
                val supportedRats = BandConstants.ALL_RAT_TYPES.filter { rt ->
                    when (rt) {
                        RatType.GSM -> hardware.gsm.isNotEmpty()
                        RatType.WCDMA -> hardware.wcdma.isNotEmpty()
                        RatType.LTE -> hardware.lte.isNotEmpty()
                        RatType.NR -> hardware.nr.isNotEmpty()
                    }
                }
                val isAuto = supportedRats.all { ratChecked[it] == true }
                val ratSummary = if (isAuto && supportedRats.isNotEmpty()) "AUTO (All RATs)"
                    else if (ratChecked.values.all { it != true } || supportedRats.isEmpty()) "None"
                    else supportedRats.filter { ratChecked[it] == true }.joinToString(", ") { it.name }
                Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    WindowDropdownPreference(
                        entries = listOf(
                            DropdownEntry(items = listOf(
                                DropdownItem(
                                    text = "AUTO (All RATs)",
                                    selected = isAuto,
                                    onClick = {
                                        val newAuto = !isAuto
                                        supportedRats.forEach { ratChecked[it] = newAuto }
                                    }
                                )
                            )),
                            DropdownEntry(items = supportedRats.map { rt ->
                                DropdownItem(
                                    text = rt.name,
                                    selected = ratChecked[rt] == true,
                                    onClick = {
                                        ratChecked[rt] = !(ratChecked[rt] == true)
                                    }
                                )
                            })
                        ),
                        title = "RAT lock",
                        summary = ratSummary,
                        showValue = false,
                        collapseOnSelection = false
                    )
                }

                if (hardware.nr.isNotEmpty()) {
                    Spacer(modifier = Modifier.height(16.dp))
                    SmallTitle("NR mode")
                    val nrModeIndex = when (nrMode) {
                        NrMode.BOTH, NrMode.UNKNOWN -> 0
                        NrMode.SA -> 1
                        NrMode.NSA -> 2
                    }
                    TabRowWithContour(
                        tabs = listOf("SA/NSA", "SA", "NSA"),
                        selectedTabIndex = nrModeIndex,
                        onTabSelected = { index ->
                            nrMode = when (index) {
                                0 -> NrMode.BOTH
                                1 -> NrMode.SA
                                2 -> NrMode.NSA
                                else -> NrMode.BOTH
                            }
                        },
                        modifier = Modifier.fillMaxWidth()
                    )
                }

                Spacer(modifier = Modifier.height(16.dp))
                SmallTitle("Band lock")
                Spacer(modifier = Modifier.height(4.dp))

                val allNsaEnabled = hardware.nr.all { nrNsaChecked[it] == true }
                val allSaEnabled = hardware.nr.all { nrSaChecked[it] == true }
                val allLteEnabled = hardware.lte.all { lteChecked[it] == true }
                val allWcdmaEnabled = hardware.wcdma.all { wcdmaChecked[it] == true }
                val allGsmEnabled = hardware.gsm.all { gsmChecked[it] == true }
                val all5gEnabled = allNsaEnabled && allSaEnabled
                val allBandsEnabled = allGsmEnabled && allWcdmaEnabled && allLteEnabled && allNsaEnabled && allSaEnabled

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
                Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    WindowDropdownPreference(
                        entries = listOf(
                            DropdownEntry(items = listOf(
                                DropdownItem(text = "All bands (all RATs)${emojiFor(0)}", onClick = {
                                    val newState = !allBandsEnabled
                                    hardware.gsm.forEach { gsmChecked[it] = newState }
                                    hardware.wcdma.forEach { wcdmaChecked[it] = newState }
                                    hardware.lte.forEach { lteChecked[it] = newState }
                                    hardware.nr.forEach {
                                        nrNsaChecked[it] = newState; nrSaChecked[it] = newState
                                    }
                                    recentlyClicked = 0 to newState
                                }),
                                DropdownItem(text = "All 5G bands (NSA+SA)${emojiFor(1)}", onClick = {
                                    val newState = !all5gEnabled
                                    hardware.nr.forEach { nrNsaChecked[it] = newState; nrSaChecked[it] = newState }
                                    recentlyClicked = 1 to newState
                                }),
                                DropdownItem(text = "All NR-SA bands${emojiFor(2)}", onClick = {
                                    val newState = !allSaEnabled
                                    hardware.nr.forEach { nrSaChecked[it] = newState }
                                    recentlyClicked = 2 to newState
                                }),
                                DropdownItem(text = "All NR-NSA bands${emojiFor(3)}", onClick = {
                                    val newState = !allNsaEnabled
                                    hardware.nr.forEach { nrNsaChecked[it] = newState }
                                    recentlyClicked = 3 to newState
                                }),
                                DropdownItem(text = "All LTE bands${emojiFor(4)}", onClick = {
                                    val newState = !allLteEnabled
                                    hardware.lte.forEach { lteChecked[it] = newState }
                                    recentlyClicked = 4 to newState
                                }),
                                DropdownItem(text = "All WCDMA bands${emojiFor(5)}", onClick = {
                                    val newState = !allWcdmaEnabled
                                    hardware.wcdma.forEach { wcdmaChecked[it] = newState }
                                    recentlyClicked = 5 to newState
                                }),
                                DropdownItem(text = "All GSM bands${emojiFor(6)}", onClick = {
                                    val newState = !allGsmEnabled
                                    hardware.gsm.forEach { gsmChecked[it] = newState }
                                    recentlyClicked = 6 to newState
                                })
                            ))
                        ),
                        title = "Quick selections",
                        summary = "Toggle band groups",
                        showValue = false,
                        collapseOnSelection = false
                    )
                }

                Spacer(modifier = Modifier.height(8.dp))

                if (hardware.nr.isNotEmpty()) {
                    SmallTitle("NR-SA")
                    Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                        BandCheckboxGrid(hardware.nr.sorted(), nrSaChecked, "n")
                    }
                    SmallTitle("NR-NSA")
                    Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                        BandCheckboxGrid(hardware.nr.sorted(), nrNsaChecked, "n")
                    }
                }
                if (hardware.lte.isNotEmpty()) {
                    SmallTitle("LTE")
                    Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                        BandCheckboxGrid(hardware.lte.sorted(), lteChecked, "B")
                    }
                }
                if (hardware.wcdma.isNotEmpty()) {
                    SmallTitle("WCDMA")
                    Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                        BandCheckboxGrid(hardware.wcdma.sorted(), wcdmaChecked, "B")
                    }
                }
                if (hardware.gsm.isNotEmpty()) {
                    SmallTitle("GSM")
                    Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                        BandCheckboxGrid(hardware.gsm.sorted(), gsmChecked, "")
                    }
                }
                Spacer(modifier = Modifier.height(16.dp))
                }
                }
                if (backdrop != null) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(Color.Black.copy(alpha = 0.6f))
                    ) {
                        SmallTopAppBar(title = "SIM $simNumber", color = Color.Transparent)
                    }
                } else {
                    SmallTopAppBar(title = "SIM $simNumber")
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
                    text = "Reset",
                    onClick = onReset,
                    modifier = Modifier
                        .weight(1f)
                        .border(1.dp, MiuixTheme.colorScheme.outline, RoundedCornerShape(16.dp))
                )
                Button(
                    onClick = {
                        val state = SimState(
                            ratMask = ratChecked.filterValues { it }.keys,
                            gsmBands = gsmChecked.filterValues { it }.keys,
                            wcdmaBands = wcdmaChecked.filterValues { it }.keys,
                            lteBands = lteChecked.filterValues { it }.keys,
                            nrNsaBands = nrNsaChecked.filterValues { it }.keys,
                            nrSaBands = nrSaChecked.filterValues { it }.keys,
                            nrMode = nrMode
                        )
                        onApply(state)
                    },
                    modifier = Modifier
                        .weight(1f)
                        .border(1.dp, MiuixTheme.colorScheme.outline, RoundedCornerShape(16.dp)),
                    colors = ButtonDefaults.buttonColorsPrimary()
                ) {
                    Text("Apply")
                }
            }
        }
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
