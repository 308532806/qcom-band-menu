package dev.qcom.bandmenu

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.datastore.preferences.preferencesDataStore
import dev.qcom.bandmenu.ui.MainScreen
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import top.yukonga.miuix.kmp.basic.SnackbarHostState
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.theme.darkColorScheme
import java.io.File

private val ComponentActivity.bandDataStore by preferencesDataStore(name = "band_prefs")

class MainActivity : ComponentActivity() {

    companion object {
        private const val TAG = "QcomBand"
    }

    private val daemonManager by lazy { DaemonManager(applicationContext) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            MiuixTheme(colors = darkColorScheme()) {
                val snackbarHostState = remember { SnackbarHostState() }

                var modemState by remember { mutableStateOf<ModemState?>(null) }
                var isLoading by remember { mutableStateOf(true) }
                var showRootDenied by remember { mutableStateOf(false) }
                var showErrorDialog by remember { mutableStateOf(false) }
                var errorTitle by remember { mutableStateOf("") }
                var errorMessage by remember { mutableStateOf("") }
                var snackbarMessage by remember { mutableStateOf<String?>(null) }
                var refreshingSlots by remember { mutableStateOf(emptySet<Int>()) }
                var refreshKey0 by remember { mutableIntStateOf(0) }
                var refreshKey1 by remember { mutableIntStateOf(0) }
                var debugEnabled by remember { mutableStateOf(false) }
                val scope = rememberCoroutineScope()
                val modemLock = remember { Mutex() }

                LaunchedEffect(Unit) {
                    BandPreferences.getDebugLogging(bandDataStore).collectLatest { enabled ->
                        debugEnabled = enabled
                        AppLog.debugEnabled = enabled
                    }
                }

                LaunchedEffect(Unit) {
                    daemonManager.start(
                        onDenied = {
                            showRootDenied = true
                            isLoading = false
                        },
                        onLaunchFailed = { msg ->
                            errorTitle = "Daemon Launch Failed"
                            errorMessage = msg
                            showErrorDialog = true
                            isLoading = false
                        }
                    )
                }

                LaunchedEffect(daemonManager.isReady.value) {
                    if (daemonManager.isReady.value) {
                        scope.launch {
                            isLoading = true
                            var initModemState: ModemState? = null
                            var initErrorTitle: String? = null
                            var initError: String? = null
                            withContext(Dispatchers.IO) {
                                try {
                                    // C5: Explicitly select SIM 1 before query
                                    val simSet1Parsed = JsonStateParser.parseResponse(daemonManager.simSet(1))
                                    if (!simSet1Parsed.ok) {
                                        initErrorTitle = "Initialization Failed"
                                        initError = "Failed to select SIM 1: ${simSet1Parsed.error?.message ?: "unknown"}"
                                        return@withContext
                                    }

                                    // I8: Check ok on each response
                                    val sim1Resp = daemonManager.query()
                                    val sim1Parsed = JsonStateParser.parseResponse(sim1Resp)
                                    if (!sim1Parsed.ok) {
                                        initErrorTitle = "Initialization Failed"
                                        initError = "Failed to query SIM 1: ${sim1Parsed.error?.message ?: "unknown"}"
                                        return@withContext
                                    }
                                    val sim1State = sim1Parsed.simState ?: SimState()
                                    val hardware = sim1Parsed.hardware ?: HardwareBands()

                                    val sim2Resp = daemonManager.simSet(2)
                                    val sim2Parsed = JsonStateParser.parseResponse(sim2Resp)
                                    if (!sim2Parsed.ok) {
                                        initErrorTitle = "Initialization Failed"
                                        initError = "Failed to select SIM 2: ${sim2Parsed.error?.message ?: "unknown"}"
                                        return@withContext
                                    }
                                    val sim2State = sim2Parsed.simState ?: SimState()

                                    // Switch back to SIM 1 as default
                                    daemonManager.simSet(1)

                                    initModemState = ModemState(sim1State, sim2State, hardware, true)

                                    // I6: Enable verbose logging on startup if debug is enabled
                                    if (debugEnabled) {
                                        daemonManager.verboseSet(true)
                                    }

                                    AppLog.i(TAG, "Init: success")
                                } catch (e: Exception) {
                                    AppLog.e(TAG, "Init: failed", e)
                                    initErrorTitle = "Initialization Failed"
                                    initError = "Failed to query modem state: ${e.message}"
                                }
                            }
                            // C4: Back on Main dispatcher - update Compose state
                            if (initModemState != null) {
                                modemState = initModemState
                            }
                            if (initError != null) {
                                errorTitle = initErrorTitle ?: "Initialization Failed"
                                errorMessage = initError
                                showErrorDialog = true
                            }
                            isLoading = false
                        }
                    }
                }

                MainScreen(
                    onApply = { slot, state ->
                        scope.launch {
                            modemLock.withLock {
                                val sim = slot + 1
                                var firstError: DaemonError? = null
                                var newState: SimState? = null
                                var errorMsg: String? = null

                                withContext(Dispatchers.IO) {
                                    try {
                                        // I3: Validate bands against hardware before sending
                                        val hw = modemState?.hardware
                                        val validatedState = if (hw != null)
                                            BandValidator.validateSimState(state, hw) else state

                                        val simResp = JsonStateParser.parseResponse(daemonManager.simSet(sim))
                                        // C2: If simSet fails, abort early
                                        if (!simResp.ok) {
                                            errorMsg = simResp.error?.message ?: "Failed to select SIM $sim"
                                            return@withContext
                                        }

                                        val hasNrHardware = hw != null && hw.nr.isNotEmpty()
                                        val commands = buildList {
                                            add(JsonRequestBuilder.gsmSet(validatedState.gsmBands))
                                            add(JsonRequestBuilder.wcdmaSet(validatedState.wcdmaBands))
                                            add(JsonRequestBuilder.lteSet(validatedState.lteBands))
                                            if (hasNrHardware) {
                                                add(JsonRequestBuilder.nrNsaSet(validatedState.nrNsaBands))
                                                add(JsonRequestBuilder.nrSaSet(validatedState.nrSaBands))
                                                add(JsonRequestBuilder.modeSet(validatedState.nrMode))
                                            }
                                            add(JsonRequestBuilder.ratSet(validatedState.ratMask))
                                        }

                                        // C2: Check every response's ok field, collect first error
                                        var lastResp: DaemonResponse = simResp
                                        for (cmd in commands) {
                                            lastResp = JsonStateParser.parseResponse(daemonManager.sendRequest(cmd))
                                            if (!lastResp.ok && firstError == null) {
                                                firstError = lastResp.error
                                            }
                                        }
                                        newState = lastResp.simState
                                    } catch (e: Exception) {
                                        errorMsg = "Apply failed: ${e.message}"
                                        AppLog.e(TAG, "Apply SIM $sim: error", e)
                                    }
                                }
                                // C4: Back on Main dispatcher - update Compose state
                                // Always increment refreshKey to force LaunchedEffect re-sync,
                                // even when the modem state is unchanged (e.g. command rejected).
                                // Without this, SimState data-class equality suppresses the
                                // re-sync and checkboxes stay in the user's desired state.
                                if (newState != null) {
                                    modemState = if (slot == 0)
                                        modemState?.copy(sim1 = newState) ?: ModemState(sim1 = newState)
                                    else
                                        modemState?.copy(sim2 = newState) ?: ModemState(sim2 = newState)
                                }
                                if (slot == 0) refreshKey0++ else refreshKey1++
                                snackbarMessage = if (errorMsg != null) errorMsg
                                    else if (firstError != null) {
                                        val msg = firstError.message
                                        val rejected = firstError.rejectedBands
                                        if (rejected != null && rejected.isNotEmpty())
                                            "$msg (rejected: $rejected)" else msg
                                    } else "Settings applied for SIM $sim"
                                if (errorMsg == null && firstError == null) {
                                    AppLog.i(TAG, "Apply SIM $sim: success")
                                }
                            }
                        }
                    },
                    onReset = { slot ->
                        scope.launch {
                            modemLock.withLock {
                                val sim = slot + 1
                                var newState: SimState? = null
                                var errorMsg: String? = null
                                var firstError: DaemonError? = null

                                withContext(Dispatchers.IO) {
                                    try {
                                        val simResp = JsonStateParser.parseResponse(daemonManager.simSet(sim))
                                        if (!simResp.ok) {
                                            errorMsg = simResp.error?.message ?: "Failed to select SIM $sim"
                                            return@withContext
                                        }

                                        val resp = daemonManager.reset()
                                        val parsed = JsonStateParser.parseResponse(resp)
                                        if (!parsed.ok) {
                                            firstError = parsed.error
                                        }
                                        newState = parsed.simState
                                    } catch (e: Exception) {
                                        errorMsg = "Reset failed: ${e.message}"
                                        AppLog.e(TAG, "Reset SIM $sim: error", e)
                                    }
                                }
                                // C4: Back on Main dispatcher - update Compose state
                                if (newState != null) {
                                    modemState = if (slot == 0)
                                        modemState?.copy(sim1 = newState) ?: ModemState(sim1 = newState)
                                    else
                                        modemState?.copy(sim2 = newState) ?: ModemState(sim2 = newState)
                                }
                                if (slot == 0) refreshKey0++ else refreshKey1++
                                snackbarMessage = if (errorMsg != null) errorMsg
                                    else if (firstError != null) firstError.message
                                    else "Reset to hardware defaults for SIM $sim"
                                if (errorMsg == null && firstError == null) {
                                    AppLog.i(TAG, "Reset SIM $sim: success")
                                }
                            }
                        }
                    },
                    refreshingSlots = refreshingSlots,
                    onRefresh = { slot ->
                        scope.launch {
                            refreshingSlots = refreshingSlots + slot
                            val sim = slot + 1
                            try {
                                modemLock.withLock {
                                    var newState: SimState? = null
                                    var errorMsg: String? = null
                                    var success = false

                                    withContext(Dispatchers.IO) {
                                        try {
                                            // I11: Try simSet first, fall back to refresh
                                            var resp = daemonManager.simSet(sim)
                                            var parsed = JsonStateParser.parseResponse(resp)
                                            if (!parsed.ok) {
                                                resp = daemonManager.refresh()
                                                parsed = JsonStateParser.parseResponse(resp)
                                            }
                                            if (!parsed.ok) {
                                                errorMsg = parsed.error?.message ?: "Refresh failed"
                                                return@withContext
                                            }
                                            newState = parsed.simState
                                            success = true
                                        } catch (e: Exception) {
                                            errorMsg = "Refresh failed: ${e.message}"
                                            AppLog.e(TAG, "Refresh SIM $sim: error", e)
                                        }
                                    }
                                    // C4/I9: Back on Main dispatcher - update Compose state
                                    if (success && newState != null) {
                                        modemState = if (slot == 0)
                                            modemState?.copy(sim1 = newState) ?: ModemState(sim1 = newState)
                                        else
                                            modemState?.copy(sim2 = newState) ?: ModemState(sim2 = newState)
                                        if (slot == 0) refreshKey0++ else refreshKey1++
                                        AppLog.i(TAG, "Refresh SIM $sim: success")
                                    } else {
                                        snackbarMessage = errorMsg ?: "Refresh failed"
                                        AppLog.i(TAG, "Refresh SIM $sim: failed")
                                    }
                                }
                            } finally {
                                refreshingSlots = refreshingSlots - slot
                            }
                        }
                    },
                    refreshKey0 = refreshKey0,
                    refreshKey1 = refreshKey1,
                    modemState = modemState,
                    isLoading = isLoading,
                    showRootDeniedDialog = showRootDenied,
                    onRootRetry = {
                        showRootDenied = false
                        isLoading = true
                        daemonManager.retry()
                    },
                    onDismissRootDialog = { showRootDenied = false },
                    showErrorDialog = showErrorDialog,
                    errorDialogTitle = errorTitle,
                    errorDialogMessage = errorMessage,
                    onDismissErrorDialog = { showErrorDialog = false },
                    snackbarHostState = snackbarHostState,
                    snackbarMessage = snackbarMessage,
                    onSnackbarShown = { snackbarMessage = null },
                    debugEnabled = debugEnabled,
                    onDebugToggle = {
                        val newVal = !debugEnabled
                        debugEnabled = newVal
                        AppLog.debugEnabled = newVal
                        scope.launch {
                            BandPreferences.setDebugLogging(bandDataStore, newVal)
                            withContext(Dispatchers.IO) {
                                try { daemonManager.verboseSet(newVal) } catch (_: Exception) {}
                            }
                        }
                    }
                )
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        daemonManager.stop()
    }
}
