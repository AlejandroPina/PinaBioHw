package com.pinabiosensor.mini

import android.content.Context
import android.content.SharedPreferences
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

/** Calibración de bandas: no son litros de aire, solo “qué tan hondo respecto a tu prueba”. */
class RespCalibrator(ctx: Context) {
    private val p: SharedPreferences =
        ctx.applicationContext.getSharedPreferences("resp_cal", Context.MODE_PRIVATE)

    var capturing = false
        private set
    private var tMin = Float.POSITIVE_INFINITY
    private var tMax = Float.NEGATIVE_INFINITY
    private var aMin = Float.POSITIVE_INFINITY
    private var aMax = Float.NEGATIVE_INFINITY
    private var samples = 0

    fun hasCal(): Boolean = p.contains("t_min") && spanOk()

    private fun spanOk(): Boolean {
        val ts = p.getFloat("t_max", 0f) - p.getFloat("t_min", 0f)
        val as_ = p.getFloat("a_max", 0f) - p.getFloat("a_min", 0f)
        return ts > 0.01f || as_ > 0.01f
    }

    fun startCapture() {
        capturing = true
        samples = 0
        tMin = Float.POSITIVE_INFINITY
        tMax = Float.NEGATIVE_INFINITY
        aMin = Float.POSITIVE_INFINITY
        aMax = Float.NEGATIVE_INFINITY
    }

    fun feed(thoraxV: Float, abdomenV: Float) {
        if (!capturing) return
        tMin = min(tMin, thoraxV)
        tMax = max(tMax, thoraxV)
        aMin = min(aMin, abdomenV)
        aMax = max(aMax, abdomenV)
        samples++
    }

    fun stopAndSave(): Boolean {
        capturing = false
        if (samples < 20) return false
        if (tMax - tMin < 0.008f && aMax - aMin < 0.008f) return false
        p.edit()
            .putFloat("t_min", tMin)
            .putFloat("t_max", tMax)
            .putFloat("a_min", aMin)
            .putFloat("a_max", aMax)
            .apply()
        return true
    }

    fun depthPct(value: Float, thorax: Boolean): Int? {
        if (!hasCal()) return null
        val lo = p.getFloat(if (thorax) "t_min" else "a_min", 0f)
        val hi = p.getFloat(if (thorax) "t_max" else "a_max", 0f)
        val span = hi - lo
        if (abs(span) < 0.008f) return null
        return (((value - lo) / span) * 100f).toInt().coerceIn(0, 100)
    }
}
