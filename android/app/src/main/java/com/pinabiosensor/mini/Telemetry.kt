package com.pinabiosensor.mini

import org.json.JSONObject

data class Telemetry(
    val v: Int,
    val ms: Long,
    val gsrUs: Double,
    val tempC: Double,
    val hr: Int,
    val rrMs: List<Int>,
    val ir: Long,
    val battV: Double,
    val ok: Int,
    val leadsOff: Boolean = false,
    val thoraxV: Double = 0.0,
    val abdomenV: Double = 0.0,
    val ecgMv: List<Double> = emptyList(),
) {
    val adsOk get() = ok and 1 != 0
    val ppgOk get() = ok and 2 != 0
    val tempOk get() = ok and 4 != 0
    val fingerOn get() = ok and 8 != 0
    val shutdown get() = ok and 16 != 0
    val lastRr get() = rrMs.lastOrNull() ?: 0

    /** Tanto por ciento grosero a partir de voltios (LiPo 1S). No es un chip "fuel gauge". */
    val battPct get() = lipoPercent(battV)

    companion object {
        fun parse(json: String): Telemetry {
            val o = JSONObject(json.trim())
            val v = o.getInt("v")
            if (v != 1 && v != 2 && v != 3) {
                throw IllegalArgumentException("JSON v=$v no soportado")
            }
            val rr = mutableListOf<Int>()
            val arr = o.optJSONArray("rr_ms")
            if (arr != null) {
                for (i in 0 until arr.length()) rr.add(arr.getInt(i))
            } else if (o.has("ibi_ms")) {
                val ibi = o.getInt("ibi_ms")
                if (ibi > 0) rr.add(ibi)
            }
            val ecg = mutableListOf<Double>()
            val earr = o.optJSONArray("ecg_mv")
            if (earr != null) {
                for (i in 0 until earr.length()) ecg.add(earr.getDouble(i))
            }
            return Telemetry(
                v = v,
                ms = o.getLong("ms"),
                gsrUs = o.getDouble("gsr_uS"),
                tempC = o.getDouble("t_c"),
                hr = o.getInt("hr"),
                rrMs = rr,
                ir = o.getLong("ir"),
                battV = o.getDouble("batt_v"),
                ok = o.getInt("ok"),
                leadsOff = o.optInt("lo") != 0,
                thoraxV = o.optDouble("rt_v", 0.0),
                abdomenV = o.optDouble("ra_v", 0.0),
                ecgMv = ecg,
            )
        }
    }
}

/** Curva típica 1S: 3.30 V vacío, 4.20 V lleno. Mentira útil, no laboratorio. */
fun lipoPercent(volts: Double): Int {
    val table = listOf(
        3.30 to 0.0,
        3.50 to 10.0,
        3.60 to 20.0,
        3.70 to 40.0,
        3.75 to 50.0,
        3.85 to 70.0,
        3.95 to 85.0,
        4.10 to 95.0,
        4.20 to 100.0,
    )
    if (volts <= table.first().first) return 0
    if (volts >= table.last().first) return 100
    for (i in 1 until table.size) {
        val (v0, p0) = table[i - 1]
        val (v1, p1) = table[i]
        if (volts <= v1) {
            val t = (volts - v0) / (v1 - v0)
            return (p0 + t * (p1 - p0)).toInt().coerceIn(0, 100)
        }
    }
    return 100
}

object BleIds {
    const val DEVICE_NAME = "PinaBiosensor"
    const val SERVICE = "6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90"
    const val JSON_CHAR = "6b1d0002-5e8a-4c2f-9b3a-2c7f0e1a4d90"
    const val HR_SERVICE = "0000180d-0000-1000-8000-00805f9b34fb"
}
