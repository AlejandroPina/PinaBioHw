# Firmware 1.3 changelog / Registro de cambios 1.3

Target / Destino: fabricated PinaBio Paca v1.0. Firmware 1.2 remains in `firmware/PinaBiosensor_Firmware_v1_2/`. Binary layout is unchanged: **PROTOCOL_VERSION stays 0x12**. Hosts that already accept 1.2 frames do not need a new parser.

## English

1. **ADS poll:** `adsStartAndRead` waits ~1.16 ms (860 SPS conversion) then polls OS with 50 µs delays. It no longer uses `vTaskDelay(1)` on every poll, which could keep ECG well below the 250 SPS target.
2. **STOP heart-rate leftover:** STOP (and a shared `clearBeatPreview`) zeros IR beat preview so HRS/telemetry do not keep the last pulse after the session ends.
3. **PPG rate rollback:** `configurePpgHardware` reads back SHDN and the MAX30102 sample-rate bits after both an apply and any attempted restoration. Only a verified apply changes `ppgRateCfg`. If both operations fail, PPG is unavailable and START/deep sleep are blocked until reboot with successful setup.
4. **START / PPG I²C without `stateMutex`:** START, `SET PPG_RATE` and `DEFAULTS` drop `stateMutex` around long I²C. Shared configuration and park flags are committed only after retaking the mutex. `ppgTask` copies `ppgRateCfg` under the mutex *before* taking I²C (never hold both).
5. **NVS autosave** takes `stateMutex` so it does not race command writes.
6. **STATUS** prints `fw=1.3` and `proto=0x12`.
7. **Not included:** no software USB+skin interlock. This PCB has no VBUS GPIO; CDC “connected” is not 5 V. Charge/flash with electrodes off. Power-bank charging is a user procedure, not a detector.

**Verification:** try a real Arduino-ESP32 compile in this workspace when the core is available. No flash or PCBA measurement is claimed here.

## Español

1. **Sondeo ADS:** espera ~1,16 ms y consulta OS con 50 µs. Ya no usa `vTaskDelay(1)` en cada poll.
2. **STOP:** pone a cero la vista previa de pulso IR.
3. **Tasa PPG:** se leen de vuelta SHDN y los bits de tasa del MAX30102. También se verifica cualquier intento de restaurar la tasa anterior. Si fallan ambas operaciones, PPG queda no disponible y START/deep sleep se bloquean hasta reiniciar con configuración correcta.
4. **START / I²C PPG** sueltan `stateMutex` durante I²C largo, pero publican configuración y reposo solo al recuperarlo. PPG copia la tasa bajo mutex *antes* de tomar I²C.
5. **Autosave NVS** bajo `stateMutex`.
6. **STATUS** incluye `fw=1.3 proto=0x12`.
7. **No hay** interlock USB+piel por software: no hay GPIO de VBUS.

## Repository correction / Corrección del repositorio (2026-09-28)

The old `kicad-completa` branch is absent, but the exact v1.0 gerber ZIP was recovered from a local archive and restored to `main`. It and the committed BOM/CPL bytes match `exports/archive_v1.0/SHA256SUMS`; `.gitattributes` prevents line-ending conversion of those three files. Root READMEs and JLCPCB guides now point to `main`. The V1.2 hardware review is labelled historical; V1.3 is current. / La rama antigua no existe, pero el ZIP exacto v1.0 se recuperó de un archivo local y se restauró en `main`. Él y los bytes BOM/CPL guardados en Git coinciden con `exports/archive_v1.0/SHA256SUMS`; `.gitattributes` evita cambiar finales de línea. README y guías JLCPCB apuntan a `main`. V1.2 queda como revisión histórica; V1.3 es vigente.
