# Firmware 1.3 code review guide (English)

Source: `firmware/PinaBiosensor_Firmware_v1_3/PinaBiosensor_Firmware_v1_3.ino`. Same hardware map, BLE name, UUIDs and binary layout as 1.2 (`PROTOCOL_VERSION=0x12`). Read [`firmware_v1_2_code_guide.en.md`](firmware_v1_2_code_guide.en.md) for the module table; this file only lists 1.3 changes.

## What 1.3 changes

1. `adsStartAndRead`: `delayMicroseconds(ADS_860_SETTLE_US)` then 50 µs OS polls. No `vTaskDelay(1)` inside the conversion wait.
2. `dropStateGuard` + START / `SET PPG_RATE` / `DEFAULTS` release `stateMutex` during long I²C, then take a new `StateGuard`.
3. `configurePpgHardware(rate, previous, stopped)` + `applyPpgHardwareRateLocked` read back MAX30102 `SPO2_CONFIG` sample-rate bits and `MODE_CONFIG.SHDN`. A failed apply triggers a verified attempt to restore `previous`. `PPG_APPLIED` alone updates `ppgRateCfg`. `PPG_UNVERIFIED` marks PPG unavailable and prevents START/deep sleep until reboot with successful setup; `PPG_UNCHANGED` retains the previous rate.
4. `clearBeatPreview()` used by START, STOP and PPG rate change. STOP no longer leaves `ppgHr`/`ppgRrMs` sticky.
5. `ppgTask` snapshots `ppgRateCfg` under `stateMutex` **before** `i2cMutex` (lock order: never I²C then state). Commands snapshot `previous`, run I²C without `stateMutex`, then publish `ppgRateCfg`, `ppgParked`, `ppgPowerUnverified` and `ppgOk` under a new `StateGuard`. The FIFO clear helper never writes `ppgParked` outside this lock.
6. Autosave calls `saveConfig()` under `StateGuard`.
7. `STATUS` includes `fw=1.3 proto=0x12`. `FIRMWARE_VERSION[]` is `"1.3"`.

## What 1.3 does not change

No VBUS/USB+skin detector. Pin map and I²C addresses unchanged. Sleep/SHDN/CCCD/peek-discard/telemetry seq behaviour from 1.2 remains.

Measured SPS on the PCBA is still the authority for ECG 250 / AUTO_ECG 500.
