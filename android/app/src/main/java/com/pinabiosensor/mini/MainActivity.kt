package com.pinabiosensor.mini

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.runtime.snapshots.SnapshotStateList
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import android.os.Handler
import android.os.Looper
import java.util.Locale

class MainActivity : ComponentActivity() {

    private var status by mutableStateOf("Pulsa Escanear con la placa encendida y cerca.")
    private var packet by mutableStateOf<Telemetry?>(null)
    private var hrv by mutableStateOf(HrvEngine().report())
    private var rawLog by mutableStateOf("")
    private var ble: BiosensorBle? = null
    private val engine = HrvEngine()
    private lateinit var respCal: RespCalibrator
    private var calMsg by mutableStateOf("Calibra la respiración una vez: bandas puestas, 20 s respirando normal y una inspiración honda.")
    private var capturingUi by mutableStateOf(false)
    private var demoOn by mutableStateOf(false)
    private val gsrHist = mutableStateListOf<Float>()
    private val thoraxHist = mutableStateListOf<Float>()
    private val abdomenHist = mutableStateListOf<Float>()
    private val ecgHist = mutableStateListOf<Float>()
    private val demoHandler = Handler(Looper.getMainLooper())
    private var demoT0 = 0L
    private val demoTick = object : Runnable {
        override fun run() {
            if (!demoOn) return
            ingest(DemoStream.packet(System.currentTimeMillis() - demoT0), fromDemo = true)
            demoHandler.postDelayed(this, 200)
        }
    }

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions(),
    ) { granted ->
        if (granted.values.all { it }) {
            ble?.startScan()
        } else {
            status = "Sin permiso Bluetooth no puedo escanear."
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        respCal = RespCalibrator(this)
        ble = BiosensorBle(
            this,
            onStatus = { runOnUiThread { status = it } },
            onPacket = { t ->
                runOnUiThread { ingest(t) }
            },
        )
        setContent {
            MaterialTheme(colorScheme = darkColorScheme(primary = Color(0xFF7DDBD0))) {
                Surface(Modifier.fillMaxSize()) {
                    AppScreen(
                        status = status,
                        packet = packet,
                        hrv = hrv,
                        calMsg = calMsg,
                        thoraxPct = packet?.let { respCal.depthPct(it.thoraxV.toFloat(), true) },
                        abdomenPct = packet?.let { respCal.depthPct(it.abdomenV.toFloat(), false) },
                        capturing = capturingUi,
                        demoOn = demoOn,
                        gsrHist = gsrHist,
                        thoraxHist = thoraxHist,
                        abdomenHist = abdomenHist,
                        ecgHist = ecgHist,
                        rawLog = rawLog,
                        onScan = {
                            stopDemo()
                            engine.reset()
                            hrv = engine.report()
                            clearHist()
                            ensurePermsAndScan()
                        },
                        onDisconnect = {
                            ble?.disconnect()
                        },
                        onSimulate = { loadExample() },
                        onLiveDemo = { startDemo() },
                        onCalStart = {
                            respCal.startCapture()
                            capturingUi = true
                            calMsg = "Grabando… respira normal y, una vez, bien hondo. ~20 s."
                        },
                        onCalStop = {
                            capturingUi = false
                            calMsg = if (respCal.stopAndSave()) {
                                "Calibración guardada. 0 % = más vacío, 100 % = lo más hondo de esa prueba."
                            } else {
                                "No se notó movimiento. Aprieta las bandas y repite."
                            }
                        },
                    )
                }
            }
        }
    }

    private fun ingest(t: Telemetry, fromDemo: Boolean = false) {
        packet = t
        rawLog = t.toRaw()
        if (t.fingerOn) engine.addIntervals(t.rrMs)
        respCal.feed(t.thoraxV.toFloat(), t.abdomenV.toFloat())
        hrv = engine.report()
        pushHist(gsrHist, t.gsrUs.toFloat())
        pushHist(thoraxHist, t.thoraxV.toFloat())
        pushHist(abdomenHist, t.abdomenV.toFloat())
        t.ecgMv.forEach { pushHist(ecgHist, it.toFloat(), 160) }
        if (t.shutdown) status = "La placa pidió apagado (interruptor)."
        else if (!fromDemo) status = "En directo · ${t.hr} lpm · GSR ${"%.2f".format(Locale.US, t.gsrUs)} µS"
    }

    private fun pushHist(list: SnapshotStateList<Float>, v: Float, cap: Int = 80) {
        list.add(v)
        while (list.size > cap) list.removeAt(0)
    }

    private fun clearHist() {
        gsrHist.clear()
        thoraxHist.clear()
        abdomenHist.clear()
        ecgHist.clear()
    }

    private fun startDemo() {
        ble?.disconnect()
        stopDemo()
        engine.reset()
        clearHist()
        demoT0 = System.currentTimeMillis()
        demoOn = true
        status = "Demo en vivo (sin Bluetooth). Misma cadencia que la placa (~5 Hz)."
        demoHandler.post(demoTick)
    }

    private fun stopDemo() {
        demoOn = false
        demoHandler.removeCallbacks(demoTick)
    }

    override fun onDestroy() {
        stopDemo()
        ble?.disconnect()
        super.onDestroy()
    }

    private fun neededPermissions(): Array<String> {
        return if (Build.VERSION.SDK_INT >= 31) {
            arrayOf(
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
            )
        } else {
            arrayOf(
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.ACCESS_FINE_LOCATION,
            )
        }
    }

    private fun ensurePermsAndScan() {
        val missing = neededPermissions().filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        if (missing.isEmpty()) ble?.startScan()
        else permissionLauncher.launch(missing.toTypedArray())
    }

    private fun loadExample() {
        stopDemo()
        engine.reset()
        clearHist()
        val sample = assets.open("ejemplo.json").bufferedReader().use { it.readText() }
        val last = Telemetry.parse(sample)
        engine.addIntervals(HrvEngine.syntheticRestingRr())
        packet = last
        rawLog = sample.trim()
        hrv = engine.report()
        status = "Ejemplo: 2 min de latidos de mentira (respiración ~12/min). Sin Bluetooth."
    }
}

private fun Telemetry.toRaw(): String {
    val rr = rrMs.joinToString(prefix = "[", postfix = "]")
    return "{\"v\":$v,\"ms\":$ms,\"gsr_uS\":$gsrUs,\"t_c\":$tempC,\"hr\":$hr," +
        "\"rr_ms\":$rr,\"ir\":$ir,\"batt_v\":$battV,\"ok\":$ok," +
        "\"lo\":${if (leadsOff) 1 else 0},\"rt_v\":$thoraxV,\"ra_v\":$abdomenV," +
        "\"ecg_mv\":[${ecgMv.joinToString(",")}]}"
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun AppScreen(
    status: String,
    packet: Telemetry?,
    hrv: HrvReport,
    calMsg: String,
    thoraxPct: Int?,
    abdomenPct: Int?,
    capturing: Boolean,
    demoOn: Boolean,
    gsrHist: List<Float>,
    thoraxHist: List<Float>,
    abdomenHist: List<Float>,
    ecgHist: List<Float>,
    rawLog: String,
    onScan: () -> Unit,
    onDisconnect: () -> Unit,
    onSimulate: () -> Unit,
    onLiveDemo: () -> Unit,
    onCalStart: () -> Unit,
    onCalStop: () -> Unit,
) {
    Scaffold(
        topBar = {
            TopAppBar(title = { Text("PinaBio v1.0") })
        },
    ) { padding ->
        Column(
            Modifier
                .padding(padding)
                .padding(16.dp)
                .fillMaxSize()
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            Text(status, style = MaterialTheme.typography.bodyLarge)
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(onClick = onScan) { Text("Escanear BLE") }
                OutlinedButton(onClick = onDisconnect) { Text("Cortar") }
            }
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedButton(onClick = onLiveDemo) {
                    Text(if (demoOn) "Demo en marcha" else "Demo en vivo")
                }
                OutlinedButton(onClick = onSimulate) { Text("JSON fijo") }
            }
            if (capturing) {
                Button(onClick = onCalStop) { Text("Terminar calibración") }
            } else {
                OutlinedButton(onClick = onCalStart) { Text("Calibrar respiración") }
            }
            Text(calMsg, style = MaterialTheme.typography.bodyMedium)
            if (packet == null) {
                Text(
                    "Pulsa Escanear BLE con la XIAO encendida (nombre PinaBiosensor). Sin placa: Demo en vivo. USB del PC nunca con electrodos.",
                    style = MaterialTheme.typography.bodyMedium,
                )
            } else {
                LiveGrid(packet, thoraxPct, abdomenPct)
                ChipRow(packet)
                SparklineCard("GSR (piel)", gsrHist)
                SparklineCard("Respiración pecho", thoraxHist)
                SparklineCard("Respiración abdomen", abdomenHist)
                SparklineCard("ECG (mV)", ecgHist)
                Text("HRV (calculado en el teléfono)", style = MaterialTheme.typography.titleMedium)
                HrvGrid(hrv)
                Text(hrv.note, style = MaterialTheme.typography.bodyMedium)
                Text("Último JSON", style = MaterialTheme.typography.titleMedium)
                Text(rawLog, fontFamily = FontFamily.Monospace, style = MaterialTheme.typography.bodySmall)
            }
        }
    }
}

@Composable
private fun LiveGrid(t: Telemetry, thoraxPct: Int?, abdomenPct: Int?) {
    val loc = Locale.US
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            MetricCard("GSR (piel)", String.format(loc, "%.2f µS", t.gsrUs), Modifier.weight(1f))
            MetricCard("Temperatura", String.format(loc, "%.2f °C", t.tempC), Modifier.weight(1f))
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            MetricCard("Pulso", if (t.hr == 0) "—" else "${t.hr} lpm", Modifier.weight(1f))
            MetricCard(
                "Último latido",
                if (t.lastRr == 0) "—" else "${t.lastRr} ms",
                Modifier.weight(1f),
            )
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            MetricCard(
                "Batería",
                String.format(loc, "%.2f V · %d %%", t.battV, t.battPct),
                Modifier.weight(1f),
            )
            MetricCard("IR (dedo)", t.ir.toString(), Modifier.weight(1f))
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            MetricCard(
                "Pecho",
                if (thoraxPct == null) String.format(loc, "%.3f V", t.thoraxV) else "$thoraxPct %",
                Modifier.weight(1f),
            )
            MetricCard(
                "Abdomen",
                if (abdomenPct == null) String.format(loc, "%.3f V", t.abdomenV) else "$abdomenPct %",
                Modifier.weight(1f),
            )
        }
        MetricCard(
            "ECG",
            if (t.leadsOff) "Electrodos mal puestos"
            else if (t.ecgMv.isEmpty()) String.format(loc, "%.0f mV", t.ecgMv.lastOrNull() ?: 0.0)
            else String.format(loc, "%.0f mV", t.ecgMv.last()),
            Modifier.fillMaxWidth(),
        )
    }
}

@Composable
private fun HrvGrid(h: HrvReport) {
    val loc = Locale.US
    fun n(v: Double?, fmt: String, empty: String = "—") =
        if (v == null) empty else String.format(loc, fmt, v)
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(
            "${h.beats} latidos · ${String.format(loc, "%.0f", h.seconds)} s de ventana",
            style = MaterialTheme.typography.bodySmall,
        )
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            MetricCard("RMSSD", n(h.rmssdMs, "%.1f ms"), Modifier.weight(1f))
            MetricCard("SDNN", n(h.sdnnMs, "%.1f ms"), Modifier.weight(1f))
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            MetricCard("pNN50", n(h.pnn50Pct, "%.1f %%"), Modifier.weight(1f))
            MetricCard("LF/HF", n(h.lfHf, "%.2f"), Modifier.weight(1f))
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            MetricCard("LF", n(h.lf, "%.1f"), Modifier.weight(1f))
            MetricCard("HF", n(h.hf, "%.1f"), Modifier.weight(1f))
        }
        MetricCard(
            "Respiración (estimada)",
            if (h.respRpm == null) "—" else String.format(loc, "%.0f / min", h.respRpm),
            Modifier.fillMaxWidth(),
        )
    }
}

@Composable
private fun MetricCard(label: String, value: String, modifier: Modifier) {
    Card(modifier) {
        Column(Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Text(label, style = MaterialTheme.typography.labelMedium)
            Text(value, style = MaterialTheme.typography.headlineSmall)
        }
    }
}

@Composable
private fun ChipRow(t: Telemetry) {
    val parts = buildList {
        add(if (t.adsOk) "ADS sí" else "ADS no")
        add(if (t.ppgOk) "PPG sí" else "PPG no")
        add(if (t.tempOk) "Temp sí" else "Temp no")
        add(if (t.fingerOn) "Dedo" else "Sin dedo")
        add(if (t.leadsOff) "ECG suelto" else "ECG ok")
        if (t.shutdown) add("Apagando")
    }
    Text(parts.joinToString(" · "), style = MaterialTheme.typography.bodyMedium)
}
