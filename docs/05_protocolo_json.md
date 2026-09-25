# Protocolo JSON v3 — legado

El firmware actual de la PCBA v1.0 es **binario** (ECG/PPG RAW + telemetría + comandos). Documento canónico:

- `docs/13_protocolo_firmware_v1.md`
- Sketch: `firmware/PinaBiosensor_V1_Firmware_Final/PinaBiosensor_V1_Firmware_Final.ino`

Lo que sigue es el **JSON v3 de debug/legado** (`6b1d0002-…`), que el firmware nuevo puede seguir emitiendo si `JSON ON`. No es el flujo principal.

# Protocolo JSON v3 — PinaBio v1.0 (legado)

La placa manda sensores. El teléfono calcula HRV y la profundidad de respiración.

Nombre BLE (emparejamiento): `PinaBiosensor`. Producto: PinaBio v1.0.

```json
{"v":3,"ms":12345,"gsr_uS":8.42,"t_c":33.16,"hr":72,"rr_ms":[833],"ir":87421,"batt_v":3.87,"ok":15,"lo":0,"rt_v":0.182,"ra_v":0.165,"ecg_mv":[1650,1662,1640]}
```

| Clave | Qué |
|---|---|
| `rr_ms` | Tiempos entre latidos nuevos (HRV en el móvil) |
| `rt_v` / `ra_v` | Tensión de la banda pecho / abdomen |
| `ecg_mv` | Muestras de ECG en mV desde el último paquete |
| `lo` | 1 = electrodos de ECG mal puestos |
| `batt_v` | Voltios; el % lo saca el móvil |

Calibración de respiración: solo en la app (mínimo/máximo de una prueba de ~20 s).
