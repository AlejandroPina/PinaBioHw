package com.pinabiosensor.mini

import kotlin.math.PI
import kotlin.math.sin

/** JSON v3 de mentira a 5 Hz: misma forma que el firmware, para probar la app sin placa. */
object DemoStream {
    fun packet(tMs: Long): Telemetry {
        val t = tMs / 1000.0
        val breath = 0.5 + 0.5 * sin(2 * PI * t / 5.0)
        val gsr = 6.0 + 1.5 * sin(2 * PI * t / 11.0)
        val ibi = (820 + 40 * sin(2 * PI * t / 5.0)).toInt()
        val fireRr = tMs % 800L < 200L
        val ecg = List(8) { i ->
            val ph = (tMs / 8.0 + i) % 40.0
            when {
                ph < 3 -> 200.0
                ph < 6 -> 1600.0
                else -> 400.0 + 40 * sin(ph)
            }
        }
        var ok = 1 or 2 or 4 or 8
        return Telemetry(
            v = 3,
            ms = tMs,
            gsrUs = gsr,
            tempC = 33.2 + 0.15 * sin(t / 20),
            hr = (60000.0 / ibi).toInt(),
            rrMs = if (fireRr) listOf(ibi) else emptyList(),
            ir = 90_000,
            battV = 3.92,
            ok = ok,
            leadsOff = false,
            thoraxV = 0.12 + 0.08 * breath,
            abdomenV = 0.11 + 0.07 * breath,
            ecgMv = ecg,
        )
    }
}
