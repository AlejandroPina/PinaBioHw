# PinaBiosensor firmware 1.2 — fabricated Paca v1.0

[Español](README.es.md) · [Protocol](../docs/firmware_v1_2_protocol.en.md) · [Code review guide](../docs/firmware_v1_2_code_guide.en.md) · [Verification](../docs/firmware_v1_2_verification.md) · [Changelog](CHANGELOG_v1_2.md)

The current and only firmware sketch is `PinaBiosensor_Firmware_v1_2/PinaBiosensor_Firmware_v1_2.ino`. It targets Seeed XIAO ESP32-S3 on the fabricated Paca v1.0: ADS1115 `0x49`, MAX30102 `0x57`, MAX30205 `0x48`; D4/D5 I²C, D3/GPIO4 sleep, D8/D9 leads-off, D0/A0 battery. No future hardware is included. BLE name `PinaBiosensor` and UUIDs are unchanged. Wi-Fi is disabled. This is not a medical device.

## Build and upload

Use Arduino IDE or Arduino CLI with the **Seeed XIAO ESP32-S3** board, a pinned Arduino-ESP32 core, and SparkFun MAX3010x Pulse and Proximity Sensor Library. Select variant and USB CDC settings appropriate to the physical XIAO. Sketch folder and `.ino` names must match. This repo has no board package or lockfile. A real build and upload are required before claiming compatibility. The BLE command callback uses `auto` for `getValue()` so the String/std::string difference between core versions does not itself force one version.

Disconnect all electrodes and bands from the body before USB flashing or diagnosis. For skin-connected sessions use battery and BLE only. USB is not isolated.

## Operation

- Serial 115200: inspect boot probes and send `STATUS`. `DIAG` is automatic every 2 s; first pass sets a baseline. It is not a command.
- `STOP`, `START`, `STATUS` exercise a clean session. `START` verifies MAX30102 FIFO reset; failure returns `ERR START_PPG_FIFO` and leaves acquisition stopped.
- Initial targets: ECG 250 SPS, GSR 10, thorax 20, abdomen 20, PPG 200. `SET AUTO_ECG ON` allows a logical ECG target up to 500. Measured SPS is authoritative.
- `USB_MODE BINARY` acknowledges in text, then emits only binary frames on USB. `USB_ECG ON` and `USB_PPG ON` enable those streams. Commands still enter in BINARY; `USB_MODE TEXT` restores text. USB starts in TEXT after every boot.
- `SAVE` checks NVS writes and returns `ERR SAVE_NVS` on failure. It stores sensors, rates and streams, not USB mode.
- `SLEEP` or D3/GPIO4 LOW requests deep sleep. The code waits for acquisition and verified PPG shutdown; it cancels with `ERR SLEEP_NOT_QUIESCENT` if a worker cannot park within 2 s.
- `STATUS` exposes targets, measured SPS, health, drops, FIFO loss lower bound, transport requests/short writes, sleep aborts and stack high-water marks. A BLE notify call is not evidence of phone receipt.

The linked guides detail every module and state group, wire offsets, lock rules, important statements and limitations. Host software must check version `0x12`, CRC and sequences. There is no Android v1.2 app in this repository.

## Verification status

The changes received a second static review of buffer limits, mutex exits and sleep/transport races. A matching Arduino toolchain and physical PCBA were unavailable here; **the sketch has not been compiled or flashed in this workspace**. Before declaring it validated, build and bring it up on the board; measure SPS, jitter, ring/FIFO loss, negotiated MTU, PPG SHDN bit, sleep current and stack margin.
