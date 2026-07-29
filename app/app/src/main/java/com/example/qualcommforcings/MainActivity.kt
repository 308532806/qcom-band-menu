package com.example.qualcommforcings

import android.app.Activity
import android.app.AlertDialog
import android.graphics.Typeface
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.InputType
import android.view.Gravity
import android.view.View
import android.widget.*
import java.io.File
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

class MainActivity : Activity() {
    private val executor = Executors.newSingleThreadExecutor()
    private val mainHandler = Handler(Looper.getMainLooper())

    private lateinit var simButton: Button
    private lateinit var hardwareSummary: TextView
    private lateinit var gsmInput: EditText
    private lateinit var wcdmaInput: EditText
    private lateinit var lteInput: EditText
    private lateinit var nrInput: EditText
    private lateinit var ratGsm: CheckBox
    private lateinit var ratWcdma: CheckBox
    private lateinit var ratLte: CheckBox
    private lateinit var ratNr: CheckBox
    private lateinit var nrModeGroup: RadioGroup
    private lateinit var progress: ProgressBar

    private val actionButtons = mutableListOf<Button>()
    private val prefs by lazy { getSharedPreferences("forcings_v2", MODE_PRIVATE) }
    private val helperAssetName = "qcom-band-menu"
    private val privateHelper by lazy { File(filesDir, helperAssetName) }
    private val rootHelper = "/data/local/tmp/qcom-band-menu-app"

    private var selectedSim = 1
    private var hardwareBands: HardwareBands? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        selectedSim = prefs.getInt("selected_sim", 1).coerceIn(1, 2)
        setContentView(buildUi())
        restoreInputs()
        updateSimButton()

        if (!prefs.getBoolean("warning_acknowledged", false)) {
            showFirstRunWarning()
        } else {
            checkHardwareBands(automatic = true)
        }
    }

    override fun onDestroy() {
        executor.shutdownNow()
        super.onDestroy()
    }

    private fun buildUi(): View {
        val density = resources.displayMetrics.density
        fun dp(value: Int) = (value * density).toInt()

        val page = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(16), dp(16), dp(12))
        }

        val header = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        header.addView(TextView(this).apply {
            text = "Qualcomm Forcings"
            textSize = 25f
            setTypeface(typeface, Typeface.BOLD)
        }, LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f))
        simButton = Button(this).apply {
            minWidth = dp(92)
            setOnClickListener { switchSim() }
        }
        header.addView(simButton)
        page.addView(header)

        progress = ProgressBar(this).apply {
            visibility = View.GONE
            isIndeterminate = true
        }
        page.addView(progress, LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        ).apply { gravity = Gravity.CENTER_HORIZONTAL })

        val controlsScroll = ScrollView(this)
        val controls = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(0, 0, 0, dp(12))
        }
        controlsScroll.addView(controls)
        page.addView(controlsScroll, LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f
        ))

        hardwareSummary = TextView(this).apply {
            text = "Not checked yet."
            textSize = 13f
            setPadding(dp(10), dp(8), dp(10), dp(8))
            setBackgroundColor(0xFF20242A.toInt())
            setTextColor(0xFFE9ECEF.toInt())
            setTextIsSelectable(true)
            visibility = View.GONE
        }
        controls.addView(hardwareSummary)
        val hardwareRow = horizontalRow()
        hardwareRow.addView(actionButton("CURRENT BAND") { refreshCurrentBands() }, weighted())
        hardwareRow.addView(actionButton("CHECK HARDWARE") { checkHardwareBands(false) }, weighted())
        controls.addView(hardwareRow)

        controls.addView(gap(dp(14)))
        controls.addView(sectionTitle("RAT preference"))
        val ratChecks = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        ratNr = CheckBox(this).apply { text = "NR5G"; isChecked = true }
        ratLte = CheckBox(this).apply { text = "LTE"; isChecked = true }
        ratWcdma = CheckBox(this).apply { text = "WCDMA" }
        ratGsm = CheckBox(this).apply { text = "GSM" }
        listOf(ratNr, ratLte, ratWcdma, ratGsm).forEach { ratChecks.addView(it) }
        controls.addView(ratChecks)
        controls.addView(actionButton("APPLY") { applyRat() })

        controls.addView(gap(dp(14)))
        controls.addView(sectionTitle("NR mode"))
        nrModeGroup = RadioGroup(this).apply {
            orientation = RadioGroup.HORIZONTAL
            addView(radio("NSA + SA", ID_NR_BOTH, true))
            addView(radio("NSA", ID_NR_NSA))
            addView(radio("SA", ID_NR_SA))
        }
        controls.addView(nrModeGroup)
        controls.addView(actionButton("APPLY") { applyNrMode() })

        controls.addView(gap(dp(14)))
        controls.addView(sectionTitle("NR bands"))
        nrInput = bandInput("1,28,41,78,258")
        controls.addView(nrInput)
        controls.addView(actionButton("APPLY") { applyNr() })

        controls.addView(gap(dp(14)))
        controls.addView(sectionTitle("LTE bands"))
        lteInput = bandInput("1,3,8,28,66,71")
        controls.addView(lteInput)
        controls.addView(actionButton("APPLY") { applyLte() })

        controls.addView(gap(dp(14)))
        controls.addView(sectionTitle("WCDMA bands"))
        wcdmaInput = bandInput("1,2,4,5,6,8,19")
        controls.addView(wcdmaInput)
        controls.addView(actionButton("APPLY") { applyWcdma() })

        controls.addView(gap(dp(14)))
        controls.addView(sectionTitle("GSM bands"))
        gsmInput = bandInput("850,900,1800,1900")
        controls.addView(gsmInput)
        controls.addView(actionButton("APPLY") { applyGsm() })

        return page
    }

    private fun restoreInputs() {
        // Prefilling disabled as requested. Hints will provide example text.
    }

    private fun sectionTitle(value: String) = TextView(this).apply {
        text = value
        textSize = 18f
        setTypeface(typeface, Typeface.BOLD)
        setPadding(0, 0, 0, 4)
    }

    private fun gap(height: Int) = Space(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, height)
    }

    private fun horizontalRow() = LinearLayout(this).apply {
        orientation = LinearLayout.HORIZONTAL
    }

    private fun weighted() = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)

    private fun radio(label: String, idValue: Int, checked: Boolean = false) = RadioButton(this).apply {
        id = idValue
        text = label
        isChecked = checked
    }

    private fun bandInput(hintValue: String) = EditText(this).apply {
        hint = hintValue
        inputType = InputType.TYPE_CLASS_TEXT
        isSingleLine = true
    }

    private fun actionButton(label: String, action: () -> Unit) = Button(this).apply {
        text = label
        setOnClickListener { action() }
        actionButtons += this
    }

    private fun showFirstRunWarning() {
        AlertDialog.Builder(this)
            .setTitle("Root and modem warning")
            .setMessage(
                "This app requires root and directly changes modem RAT and band preferences. " +
                    "Unsupported or unsuitable settings may temporarily remove mobile service."
            )
            .setCancelable(false)
            .setPositiveButton("I understand") { _, _ ->
                prefs.edit().putBoolean("warning_acknowledged", true).apply()
                checkHardwareBands(automatic = true)
            }
            .setNegativeButton("Exit") { _, _ -> finish() }
            .show()
    }

    private fun switchSim() {
        selectedSim = if (selectedSim == 1) 2 else 1
        prefs.edit().putInt("selected_sim", selectedSim).apply()
        updateSimButton()
        hardwareBands = null
        hardwareSummary.visibility = View.GONE
        runCommands(emptyList(), "Switch to SIM$selectedSim")
    }

    private fun updateSimButton() {
        simButton.text = "SIM $selectedSim ⇄"
    }

    private fun applyRat() {
        val tokens = mutableListOf<String>()
        if (ratGsm.isChecked) tokens += "gsm"
        if (ratWcdma.isChecked) tokens += "wcdma"
        if (ratLte.isChecked) tokens += "lte"
        if (ratNr.isChecked) tokens += "nr"
        if (tokens.isEmpty()) {
            Toast.makeText(this, "Select at least one RAT", Toast.LENGTH_SHORT).show()
            return
        }
        runCommands(listOf("rat ${tokens.joinToString(",")}"), "Apply RAT on SIM$selectedSim")
    }

    private fun applyGsm() {
        val bands = parseGsm(gsmInput.text.toString()) ?: return
        if (!validateHardware("GSM", bands, hardwareBands?.gsm)) return
        prefs.edit().putString("gsm_bands", bands.joinToString(",")).apply()
        runCommands(listOf("gsm ${bands.joinToString(",")}"), "Apply GSM on SIM$selectedSim")
        gsmInput.text.clear()
    }

    private fun applyWcdma() {
        val bands = parseBands(wcdmaInput.text.toString(), 1, 19, "WCDMA") ?: return
        if (!validateHardware("WCDMA", bands, hardwareBands?.wcdma)) return
        prefs.edit().putString("wcdma_bands", bands.joinToString(",")).apply()
        runCommands(listOf("wcdma ${bands.joinToString(",")}"), "Apply WCDMA on SIM$selectedSim")
        wcdmaInput.text.clear()
    }

    private fun applyLte() {
        val bands = parseBands(lteInput.text.toString(), 1, 256, "LTE") ?: return
        if (!validateHardware("LTE", bands, hardwareBands?.lte)) return
        prefs.edit().putString("lte_bands", bands.joinToString(",")).apply()
        runCommands(listOf("lte ${bands.joinToString(",")}"), "Apply LTE on SIM$selectedSim")
        lteInput.text.clear()
    }

    private fun applyNr() {
        val bands = parseBands(nrInput.text.toString(), 1, 512, "NR") ?: return
        if (!validateHardware("NR", bands, hardwareBands?.nr)) return
        prefs.edit().putString("nr_bands", bands.joinToString(",")).apply()
        runCommands(listOf("nr ${bands.joinToString(",")}"), "Apply NR on SIM$selectedSim")
        nrInput.text.clear()
    }

    private fun applyNrMode() {
        val mode = when (nrModeGroup.checkedRadioButtonId) {
            ID_NR_BOTH -> "both"
            ID_NR_NSA -> "nsa"
            ID_NR_SA -> "sa"
            else -> return
        }
        runCommands(listOf("mode $mode"), "Apply NR mode on SIM$selectedSim")
    }

    private fun checkHardwareBands(automatic: Boolean) {
        runCommands(listOf("hardware"), if (automatic) "Initial hardware check" else "Check hardware bands") { output ->
            val parsed = parseHardware(output)
            if (parsed != null) {
                hardwareBands = parsed
                hardwareSummary.visibility = View.VISIBLE
                hardwareSummary.text = buildString {
                    append("SUPPORTED BANDS:\n")
                    append("GSM: ${parsed.gsm.joinToString(",")}\n")
                    append("WCDMA: ${parsed.wcdma.joinToString(",") { "B$it" }}\n")
                    append("LTE: ${parsed.lte.joinToString(",") { "B$it" }}\n")
                    append("NR: ${parsed.nr.joinToString(",") { "n$it" }}")
                }
            } else if (!automatic) {
                hardwareSummary.visibility = View.VISIBLE
                hardwareSummary.text = "Hardware query did not return a parseable supported-band list. Check system logs for details."
            }
        }
    }

    private fun refreshCurrentBands() {
        runCommands(listOf("refresh"), "Query current bands") { output ->
            val block = parseCurrentBandsBlock(output)
            if (block != null) {
                hardwareSummary.visibility = View.VISIBLE
                hardwareSummary.text = block
            } else {
                hardwareSummary.visibility = View.VISIBLE
                hardwareSummary.text = "Could not parse current modem state. See logs."
            }
        }
    }

    private fun parseCurrentBandsBlock(output: String): String? {
        val clean = stripAnsi(output)
        val anchor = clean.lastIndexOf("CURRENT BANDS (SIM")
        if (anchor < 0) return null
        val endAnchor = clean.indexOf("====", anchor + 20)
        return if (endAnchor > anchor) {
            clean.substring(anchor, endAnchor).trim()
        } else {
            clean.substring(anchor).trim()
        }
    }

    private fun parseGsm(raw: String): List<Int>? {
        val values = parseBands(raw, 1, 2000, "GSM") ?: return null
        val allowed = setOf(850, 900, 1800, 1900)
        val invalid = values.filterNot { it in allowed }
        if (invalid.isNotEmpty()) {
            appendConsole("Validation: invalid GSM band(s): ${invalid.joinToString(",")}. Use 850,900,1800,1900.\n")
            return null
        }
        return values
    }

    private fun parseBands(raw: String, min: Int, max: Int, label: String): List<Int>? {
        val cleaned = raw.trim()
        if (cleaned.isEmpty()) {
            appendConsole("Validation: $label band list cannot be empty.\n")
            return null
        }
        val values = sortedSetOf<Int>()
        for (token in cleaned.split(Regex("[\\s,]+"))) {
            val normalized = token.removePrefix("B").removePrefix("b").removePrefix("N").removePrefix("n")
            val value = normalized.toIntOrNull()
            if (value == null || value !in min..max) {
                appendConsole("Validation: invalid $label band '$token' (range $min-$max).\n")
                return null
            }
            values += value
        }
        return values.toList()
    }

    private fun validateHardware(label: String, requested: List<Int>, supported: Set<Int>?): Boolean {
        if (supported == null) {
            appendConsole("Validation: hardware bands are not loaded. Run Check hardware first.\n")
            return false
        }
        val unsupported = requested.filterNot { it in supported }
        if (unsupported.isNotEmpty()) {
            appendConsole("Validation: unsupported $label band(s): ${unsupported.joinToString(",")}. Command not sent.\n")
            return false
        }
        return true
    }

    private fun runCommands(
        commands: List<String>,
        title: String,
        onComplete: ((String) -> Unit)? = null
    ) {
        setBusy(true)
        appendConsole("\n> $title\n")
        executor.execute {
            val result = try {
                installRootHelper()
                executeInteractive(commands)
            } catch (e: Exception) {
                CommandResult(-1, "${e.javaClass.simpleName}: ${e.message ?: "Unknown error"}")
            }

            mainHandler.post {
                setBusy(false)
                val clean = stripAnsi(result.output).trim()
                val display = when {
                    result.timedOut -> "Command timed out."
                    clean.isNotEmpty() -> clean
                    result.exitCode == 0 -> "Command completed."
                    else -> "Command failed with exit code ${result.exitCode}."
                }
                appendConsole("$display\n")
                onComplete?.invoke(clean)
            }
        }
    }

    private fun installRootHelper() {
        assets.open(helperAssetName).use { input ->
            privateHelper.outputStream().use { output -> input.copyTo(output) }
        }
        privateHelper.setReadable(true, true)
        val install = runSu(
            "cp ${shellQuote(privateHelper.absolutePath)} ${shellQuote(rootHelper)} && " +
                "chmod 755 ${shellQuote(rootHelper)}"
        )
        if (install.timedOut || install.exitCode != 0) {
            throw IllegalStateException(install.output.ifBlank { "Unable to install native helper through root." })
        }
    }

    private fun executeInteractive(commands: List<String>): CommandResult {
        val lines = mutableListOf("sim $selectedSim")
        lines += commands
        lines += "exit"
        val input = lines.joinToString("\n", postfix = "\n")
        val shell = "printf %s ${shellQuote(input)} | ${shellQuote(rootHelper)}"
        return runSu(shell)
    }

    private fun runSu(command: String): CommandResult {
        val process = ProcessBuilder("su", "-c", command)
            .redirectErrorStream(true)
            .start()
        val collector = StringBuilder()
        val readerThread = Thread {
            process.inputStream.bufferedReader().useLines { lines ->
                lines.forEach { line ->
                    if (collector.length < 96_000) collector.appendLine(line)
                }
            }
        }
        readerThread.start()
        val finished = process.waitFor(15, TimeUnit.SECONDS)
        if (!finished) {
            process.destroyForcibly()
            readerThread.join(500)
            return CommandResult(-1, collector.toString(), timedOut = true)
        }
        readerThread.join(500)
        return CommandResult(process.exitValue(), collector.toString())
    }

    private fun parseHardware(output: String): HardwareBands? {
        val clean = stripAnsi(output)
        val anchor = clean.lastIndexOf("HARDWARE SUPPORTED BANDS:")
        if (anchor < 0) return null
        val block = clean.substring(anchor)
        fun line(prefix: String): String? = block.lineSequence()
            .firstOrNull { it.trim().startsWith(prefix) }
            ?.trim()?.substringAfter(prefix)?.trim()

        val gsm = parseBandOutput(line("GSM:") ?: return null, "")
        val wcdma = parseBandOutput(line("WCDMA:") ?: return null, "B")
        val lte = parseBandOutput(line("LTE:") ?: return null, "B")
        val nr = parseBandOutput(line("NR-SA:") ?: line("NR-NSA:") ?: return null, "n")
        return HardwareBands(gsm, wcdma, lte, nr)
    }

    private fun parseBandOutput(raw: String, prefix: String): Set<Int> {
        if (raw.equals("none", true) || raw.equals("unavailable", true)) return emptySet()
        return raw.split(',').mapNotNull { token ->
            token.trim().removePrefix(prefix).toIntOrNull()
        }.toSet()
    }

    private fun stripAnsi(value: String): String =
        value.replace(Regex("\\u001B\\[[;?0-9]*[ -/]*[@-~]"), "")

    private fun shellQuote(value: String): String = "'" + value.replace("'", "'\\''") + "'"

    private fun appendConsole(message: String) {
        android.util.Log.d("QualcommForcings", message)
        if (message.startsWith("Validation:")) {
            Toast.makeText(this, message.removePrefix("Validation:").trim(), Toast.LENGTH_LONG).show()
        }
    }

    private fun setBusy(busy: Boolean) {
        progress.visibility = if (busy) View.VISIBLE else View.GONE
        actionButtons.forEach { it.isEnabled = !busy }
        simButton.isEnabled = !busy
    }

    private data class HardwareBands(
        val gsm: Set<Int>,
        val wcdma: Set<Int>,
        val lte: Set<Int>,
        val nr: Set<Int>
    )

    private data class CommandResult(
        val exitCode: Int,
        val output: String,
        val timedOut: Boolean = false
    )

    companion object {
        private const val ID_NR_BOTH = 201
        private const val ID_NR_NSA = 202
        private const val ID_NR_SA = 203
    }
}
