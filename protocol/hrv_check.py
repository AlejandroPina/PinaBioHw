#!/usr/bin/env python3
"""Comprueba que, con latidos de mentira a 12 respiraciones/min, el pico cae cerca."""
import math

def synthetic(seconds=120, resp_rpm=12.0):
    out = []
    t = 0.0
    f = resp_rpm / 60.0
    while t < seconds:
        rr = 0.800 + 0.050 * math.sin(2 * math.pi * f * t)
        ms = int(rr * 1000)
        ms = max(300, min(1500, ms))
        out.append(ms)
        t += ms / 1000.0
    return out


def resample(values, fs=4.0):
    t = []
    acc = 0.0
    for v in values:
        acc += v / 1000.0
        t.append(acc)
    t_end = t[-1]
    t_start = max(t[0], t_end - 120.0)
    duration = t_end - t_start
    n = min(512, int(duration * fs))
    if n < 64:
        return None
    y = []
    j = 0
    for i in range(n):
        ti = t_start + i / fs
        while j + 1 < len(t) and t[j + 1] < ti:
            j += 1
        y.append(float(values[min(j, len(values) - 1)]))
    return y, fs


def resp_rpm(values):
    packed = resample(values)
    if packed is None:
        raise SystemExit("pocos latidos")
    y, fs = packed
    n = len(y)
    mean = sum(y) / n
    for i in range(n):
        w = 0.5 - 0.5 * math.cos(2 * math.pi * i / max(n - 1, 1))
        y[i] = (y[i] - mean) * w
    df = fs / n
    best_p = 0.0
    best_f = 0.0
    band = []
    n_freq = n // 2
    for k in range(1, n_freq):
        re = im = 0.0
        ang = 2 * math.pi * k / n
        for i, xi in enumerate(y):
            re += xi * math.cos(ang * i)
            im -= xi * math.sin(ang * i)
        f = k * df
        p = (re * re + im * im) / (fs * n)
        if 0.12 <= f <= 0.50:
            band.append(p)
            if p > best_p:
                best_p = p
                best_f = f
    mean_band = sum(band) / len(band)
    if best_f <= 0 or best_p <= mean_band * 1.4:
        raise SystemExit("no hay pico de respiración")
    return best_f * 60.0


def rmssd(xs):
    acc = 0.0
    for a, b in zip(xs, xs[1:]):
        d = b - a
        acc += d * d
    return math.sqrt(acc / (len(xs) - 1))


rr = synthetic()
rpm = resp_rpm(rr)
if not (10.0 <= rpm <= 14.0):
    raise SystemExit(f"respiración estimada {rpm:.1f} rpm, se esperaba ~12")
if rmssd(rr) < 5:
    raise SystemExit("RMSSD demasiado bajo para una señal con RSA")
print(f"hrv_check OK: {len(rr)} latidos, respiración ~{rpm:.1f} / min")
