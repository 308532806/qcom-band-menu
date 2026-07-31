package dev.qcom.bandmenu

import org.json.JSONArray
import org.json.JSONObject

enum class RatType { GSM, WCDMA, LTE, NR }

enum class NrMode { SA, NSA, BOTH, UNKNOWN }

data class SimState(
    val ratMask: Set<RatType> = emptySet(),
    val gsmBands: Set<Int> = emptySet(),
    val wcdmaBands: Set<Int> = emptySet(),
    val lteBands: Set<Int> = emptySet(),
    val nrNsaBands: Set<Int> = emptySet(),
    val nrSaBands: Set<Int> = emptySet(),
    val nrMode: NrMode = NrMode.BOTH
)

data class HardwareBands(
    val gsm: Set<Int> = emptySet(),
    val wcdma: Set<Int> = emptySet(),
    val lte: Set<Int> = emptySet(),
    val nr: Set<Int> = emptySet()
)

data class ModemState(
    val sim1: SimState = SimState(),
    val sim2: SimState = SimState(),
    val hardware: HardwareBands = HardwareBands(),
    val binaryInstalled: Boolean = false
)

object BandConstants {
    val GSM_BANDS = setOf(850, 900, 1800, 1900)
    val WCDMA_RANGE = 1..19
    val LTE_RANGE = 1..256
    val NR_RANGE = 1..512
    val ALL_RAT_TYPES = setOf(RatType.GSM, RatType.WCDMA, RatType.LTE, RatType.NR)
}

data class DaemonResponse(
    val id: Int?,
    val cmd: String,
    val ok: Boolean,
    val error: DaemonError?,
    val simState: SimState?,
    val hardware: HardwareBands?,
    val sim: Int,
    val status: String
)

data class DaemonError(
    val stage: String,
    val message: String,
    val result: Int?,
    val code: Int?,
    val label: String?,
    val rejectedBands: Set<Int>?
)

object JsonRequestBuilder {

    fun query(): JSONObject = JSONObject().put("cmd", "query")

    fun refresh(): JSONObject = JSONObject().put("cmd", "refresh")

    fun simSet(sim: Int): JSONObject = JSONObject().put("cmd", "sim_set").put("sim", sim)

    fun ratSet(rats: Set<RatType>): JSONObject {
        val ratStr = if (rats == BandConstants.ALL_RAT_TYPES) "auto"
            else rats.sortedBy { it.ordinal }.joinToString(",") { it.name.lowercase() }
        return JSONObject().put("cmd", "rat_set").put("rat", ratStr)
    }

    fun gsmSet(bands: Set<Int>): JSONObject = bandSet("gsm_set", bands)
    fun wcdmaSet(bands: Set<Int>): JSONObject = bandSet("wcdma_set", bands)
    fun lteSet(bands: Set<Int>): JSONObject = bandSet("lte_set", bands)
    fun nrSaSet(bands: Set<Int>): JSONObject = bandSet("nr_sa_set", bands)
    fun nrNsaSet(bands: Set<Int>): JSONObject = bandSet("nr_nsa_set", bands)

    private fun bandSet(cmd: String, bands: Set<Int>): JSONObject {
        val req = JSONObject().put("cmd", cmd)
        if (bands.isEmpty()) {
            req.put("bands", "none")
        } else {
            val arr = JSONArray()
            bands.sorted().forEach { arr.put(it) }
            req.put("bands", arr)
        }
        return req
    }

    fun modeSet(mode: NrMode): JSONObject {
        val modeStr = when (mode) {
            NrMode.SA -> "sa"
            NrMode.NSA -> "nsa"
            NrMode.BOTH, NrMode.UNKNOWN -> "both"
        }
        return JSONObject().put("cmd", "mode_set").put("mode", modeStr)
    }

    fun reset(): JSONObject = JSONObject().put("cmd", "reset")

    fun shutdown(): JSONObject = JSONObject().put("cmd", "shutdown")

    fun verboseSet(verbose: Boolean): JSONObject =
        JSONObject().put("cmd", "verbose_set").put("verbose", verbose)
}

object JsonStateParser {

    fun parseResponse(response: JSONObject): DaemonResponse {
        val stateJson = response.optJSONObject("state")
        val errorJson = response.optJSONObject("error")
        return DaemonResponse(
            id = if (response.isNull("id")) null else response.optInt("id", 0),
            cmd = response.optString("cmd", ""),
            ok = response.optBoolean("ok", false),
            error = if (errorJson != null) parseError(errorJson) else null,
            simState = if (stateJson != null) parseSimState(stateJson) else null,
            hardware = if (stateJson != null) parseHardware(stateJson) else null,
            sim = if (stateJson != null) stateJson.optInt("sim", 1) else 1,
            status = if (stateJson != null) stateJson.optString("status", "") else ""
        )
    }

    fun parseSimState(state: JSONObject): SimState {
        if (!state.optBoolean("valid", false)) return SimState()

        val ratObj = state.optJSONObject("rat")
        val ratMask = if (ratObj != null) {
            val mask = mutableSetOf<RatType>()
            if (ratObj.optBoolean("gsm", false)) mask.add(RatType.GSM)
            if (ratObj.optBoolean("wcdma", false)) mask.add(RatType.WCDMA)
            if (ratObj.optBoolean("lte", false)) mask.add(RatType.LTE)
            if (ratObj.optBoolean("nr", false)) mask.add(RatType.NR)
            mask.toSet()
        } else emptySet()

        val nrMode = when (state.optString("nr_mode", "both")) {
            "sa" -> NrMode.SA
            "nsa" -> NrMode.NSA
            "unknown" -> NrMode.UNKNOWN
            else -> NrMode.BOTH
        }

        return SimState(
            ratMask = ratMask,
            gsmBands = parseIntArray(state, "gsm"),
            wcdmaBands = parseIntArray(state, "wcdma"),
            lteBands = parseIntArray(state, "lte"),
            nrNsaBands = parseIntArray(state, "nr_nsa"),
            nrSaBands = parseIntArray(state, "nr_sa"),
            nrMode = nrMode
        )
    }

    fun parseHardware(state: JSONObject): HardwareBands {
        val hw = state.optJSONObject("hardware") ?: return HardwareBands()
        return HardwareBands(
            gsm = parseIntArray(hw, "gsm"),
            wcdma = parseIntArray(hw, "wcdma"),
            lte = parseIntArray(hw, "lte"),
            nr = parseIntArray(hw, "nr")
        )
    }

    private fun parseError(error: JSONObject): DaemonError {
        return DaemonError(
            stage = error.optString("stage", "daemon"),
            message = error.optString("message", "Unknown error"),
            result = if (error.has("result") && !error.isNull("result")) error.optInt("result") else null,
            code = if (error.has("code") && !error.isNull("code")) error.optInt("code") else null,
            label = if (error.has("label") && !error.isNull("label")) error.optString("label") else null,
            rejectedBands = if (error.has("rejected_bands") && !error.isNull("rejected_bands"))
                parseIntArray(error, "rejected_bands") else null
        )
    }

    private fun parseIntArray(json: JSONObject, key: String): Set<Int> {
        if (json.isNull(key)) return emptySet()
        val arr = json.optJSONArray(key) ?: return emptySet()
        val result = mutableSetOf<Int>()
        for (i in 0 until arr.length()) {
            val v = arr.optInt(i, -1)
            if (v > 0) result.add(v)
        }
        return result.toSet()
    }
}

object BandValidator {

    fun validateGsm(bands: Set<Int>, hw: HardwareBands): Set<Int> = bands.intersect(hw.gsm)

    fun validateWcdma(bands: Set<Int>, hw: HardwareBands): Set<Int> = bands.intersect(hw.wcdma)

    fun validateLte(bands: Set<Int>, hw: HardwareBands): Set<Int> = bands.intersect(hw.lte)

    fun validateNr(bands: Set<Int>, hw: HardwareBands): Set<Int> = bands.intersect(hw.nr)

    fun validateSimState(state: SimState, hw: HardwareBands): SimState =
        state.copy(
            gsmBands = validateGsm(state.gsmBands, hw),
            wcdmaBands = validateWcdma(state.wcdmaBands, hw),
            lteBands = validateLte(state.lteBands, hw),
            nrNsaBands = validateNr(state.nrNsaBands, hw),
            nrSaBands = validateNr(state.nrSaBands, hw)
        )
}
