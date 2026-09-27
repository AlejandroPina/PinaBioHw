# Firmware PinaBiosensor 1.3 — fabricated Paca v1.0

[Español](README.es.md) · [Protocol 1.3](../docs/firmware_v1_3_protocol.en.md) · [Code guide 1.3](../docs/firmware_v1_3_code_guide.en.md) · [Changelog 1.3](CHANGELOG_v1_3.md)

**Current sketch:** `PinaBiosensor_Firmware_v1_3/PinaBiosensor_Firmware_v1_3.ino`

**Kept for rollback:** `PinaBiosensor_Firmware_v1_2/` with [protocol 1.2](../docs/firmware_v1_2_protocol.en.md) and [changelog 1.2](CHANGELOG_v1_2.md).

Same Paca v1.0 hardware: ADS1115 `0x49`, MAX30102 `0x57`, MAX30205 `0x48`; D4/D5 I²C, D3/GPIO4 sleep, D8/D9 leads-off, D0/A0 battery. BLE name `PinaBiosensor`. Wi-Fi off. Not a medical device. Binary frames still use version byte **`0x12`**.

## Build and upload

Arduino IDE or Arduino CLI, board **Seeed XIAO ESP32-S3**, Arduino-ESP32 core, SparkFun MAX3010x Pulse and Proximity Sensor Library. Folder name must match the `.ino`. Disconnect electrodes before USB flash.

## Operation

Same commands as 1.2 (`START`, `STOP`, `STATUS`, `USB_MODE`, rates, `SLEEP`, …). `STATUS` starts with `fw=1.3 proto=0x12`. `STOP` clears the on-MCU pulse preview. `START` still returns `ERR START_PPG_FIFO` if the MAX30102 FIFO cannot be verified.

There is no software USB-presence interlock (no VBUS GPIO). Charge with a power bank if you want; still keep skin contacts off while the cable is in.

## Verification status

1.3 is a behavioural fix on 1.2. Compile and bring-up on the XIAO are required before calling it session-ready. Measure ECG SPS after the ADS poll change.
