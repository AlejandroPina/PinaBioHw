# Guía de revisión del firmware 1.3 (español)

Fuente: `firmware/PinaBiosensor_Firmware_v1_3/PinaBiosensor_Firmware_v1_3.ino`. Mismo mapa de pines, BLE, UUID y layout binario que 1.2 (`PROTOCOL_VERSION=0x12`). La tabla de módulos está en [`firmware_v1_2_code_guide.es.md`](firmware_v1_2_code_guide.es.md); aquí solo van los cambios 1.3.

## Qué cambia

1. `adsStartAndRead`: espera `ADS_860_SETTLE_US` y sondea OS a 50 µs. Sin `vTaskDelay(1)` en esa espera.
2. `dropStateGuard`: START / `SET PPG_RATE` / `DEFAULTS` sueltan `stateMutex` durante I²C largo.
3. `configurePpg(rate, stopped)` restaura la tasa anterior en el chip si falla la verificación; `ppgRateCfg` solo cambia si hay éxito.
4. `clearBeatPreview()` en START, STOP y cambio de tasa PPG.
5. `ppgTask` copia `ppgRateCfg` bajo mutex **antes** de I²C.
6. Autosave NVS con `StateGuard`.
7. `STATUS` incluye `fw=1.3 proto=0x12`.

## Qué no cambia

No hay detector USB+piel. Cobre, direcciones I²C y frames 0x12 iguales. Sleep, SHDN, CCCD y peek/discard siguen como en 1.2.

La tasa medida en la PCBA manda sobre el objetivo 250/500.
