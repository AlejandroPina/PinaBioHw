package com.pinabiosensor.mini

import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.min
import kotlin.math.sin
import kotlin.math.sqrt

data class HrvReport(
    val beats: Int,
    val seconds: Double,
    val meanRrMs: Double?,
    val sdnnMs: Double?,
    val rmssdMs: Double?,
    val pnn50Pct: Double?,
    val lf: Double?,
    val hf: Double?,
    val lfHf: Double?,
    val respRpm: Double?,
    val note: String,
)

class HrvEngine {
    private val rr = ArrayDeque<Int>()
    private var spanMs = 0L

    fun reset() {
        rr.clear()
        spanMs = 0L
    }

    fun addIntervals(intervals: List<Int>) {
        for (ms in intervals) {
            if (ms < 300 || ms > 1500) continue
            rr.addLast(ms)
            spanMs += ms
            while (spanMs > WINDOW_MS && rr.size > 2) {
                spanMs -= rr.removeFirst()
            }
        }
    }

    fun report(): HrvReport {
        val n = rr.size
        val sec = spanMs / 1000.0
        if (n < 8) {
            return HrvReport(n, sec, null, null, null, null, null, null, null, null,
                "Aún no hay latidos suficientes. Dedo quieto ~1 minuto.")
        }
        val mean = rr.average()
        val sdnn = stdev(rr, mean)
        val rmssd = rmssdOf(rr)
        val pnn50 = pnn50Of(rr)
        if (n < 40 || sec < 60.0) {
            return HrvReport(n, sec, mean, sdnn, rmssd, pnn50, null, null, null, null,
                "Tiempo (RMSSD, SDNN) listo. Sigue ~1–2 min quieto para el espectro y la respiración.")
        }
        val spec = spectrum(rr.toList())
        val note = if (spec.respRpm == null) {
            "Espectro listo. Respiración incierta (muévete menos o espera más)."
        } else {
            "Todo calculado en el teléfono, no en la placa."
        }
        return HrvReport(n, sec, mean, sdnn, rmssd, pnn50, spec.lf, spec.hf, spec.lfHf, spec.respRpm, note)
    }

    private data class Spec(val lf: Double, val hf: Double, val lfHf: Double, val respRpm: Double?)

    private fun spectrum(values: List<Int>): Spec {
        val fs = 4.0
        val y = resample(values, fs) ?: return Spec(0.0, 0.0, 0.0, null)
        val n = y.size
        val mean = y.average()
        for (i in y.indices) {
            val w = 0.5 - 0.5 * cos(2.0 * PI * i / (n - 1).coerceAtLeast(1))
            y[i] = (y[i] - mean) * w
        }
        val df = fs / n
        var lf = 0.0
        var hf = 0.0
        var bestP = 0.0
        var bestF = 0.0
        var bandP = 0.0
        var bandN = 0
        val nFreq = n / 2
        for (k in 1 until nFreq) {
            var re = 0.0
            var im = 0.0
            val ang = 2.0 * PI * k / n
            for (i in y.indices) {
                re += y[i] * cos(ang * i)
                im -= y[i] * sin(ang * i)
            }
            val f = k * df
            val p = (re * re + im * im) / (fs * n)
            if (f >= 0.04 && f < 0.15) lf += p * df
            if (f >= 0.15 && f < 0.40) hf += p * df
            if (f >= 0.12 && f <= 0.50) {
                bandP += p
                bandN++
                if (p > bestP) {
                    bestP = p
                    bestF = f
                }
            }
        }
        val lfHf = if (hf > 1e-12) lf / hf else 0.0
        val meanBand = if (bandN > 0) bandP / bandN else 0.0
        val resp = if (bestF > 0 && bestP > meanBand * 1.4) bestF * 60.0 else null
        return Spec(lf, hf, lfHf, resp)
    }

    private fun resample(values: List<Int>, fs: Double): DoubleArray? {
        val t = DoubleArray(values.size)
        var acc = 0.0
        for (i in values.indices) {
            acc += values[i] / 1000.0
            t[i] = acc
        }
        val tEnd = t.last()
        val tStart = (tEnd - 120.0).coerceAtLeast(t.first())
        val duration = tEnd - tStart
        val n = min(512, (duration * fs).toInt())
        if (n < 64) return null
        val y = DoubleArray(n)
        var j = 0
        for (i in 0 until n) {
            val ti = tStart + i / fs
            while (j + 1 < t.size && t[j + 1] < ti) j++
            val rr = values[j.coerceAtMost(values.lastIndex)].toDouble()
            y[i] = rr
        }
        return y
    }

    private fun stdev(xs: Collection<Int>, mean: Double): Double {
        if (xs.size < 2) return 0.0
        var acc = 0.0
        for (v in xs) {
            val d = v - mean
            acc += d * d
        }
        return sqrt(acc / (xs.size - 1))
    }

    private fun rmssdOf(xs: List<Int>): Double {
        if (xs.size < 2) return 0.0
        var acc = 0.0
        var c = 0
        for (i in 1 until xs.size) {
            val d = (xs[i] - xs[i - 1]).toDouble()
            acc += d * d
            c++
        }
        return sqrt(acc / c)
    }

    private fun pnn50Of(xs: List<Int>): Double {
        if (xs.size < 2) return 0.0
        var hits = 0
        val den = xs.size - 1
        for (i in 1 until xs.size) {
            if (kotlin.math.abs(xs[i] - xs[i - 1]) > 50) hits++
        }
        return 100.0 * hits / den
    }

    companion object {
        private const val WINDOW_MS = 300_000L

        fun syntheticRestingRr(seconds: Int = 120, respRpm: Double = 12.0): List<Int> {
            val out = ArrayList<Int>()
            var t = 0.0
            val f = respRpm / 60.0
            while (t < seconds) {
                val rr = 0.800 + 0.050 * sin(2.0 * PI * f * t)
                val ms = (rr * 1000.0).toInt().coerceIn(300, 1500)
                out.add(ms)
                t += ms / 1000.0
            }
            return out
        }
    }
}
