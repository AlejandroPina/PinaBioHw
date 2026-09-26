# PinaBiosensor firmware 1.2 — wire protocol (English)

This specification applies only to the fabricated PinaBio Paca v1.0 board and `PinaBiosensor_Firmware_v1_2.ino`. It is an engineering biofeedback protocol, not a medical one. Version **0x12** changes the version byte from 1.1 (0x11); the binary field layout and proprietary UUIDs remain the same. A receiver must reject unknown versions before decoding payloads.

## Transport and GATT

- Advertised name: exactly `PinaBiosensor`.
- Proprietary service: `6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90`.
- JSON debug `...0002`, COMMAND `...0003`, ECG `...0004`, PPG `...0005`, TELEMETRY `...0006`, EVENT `...0007`; all share suffix `-5e8a-4c2f-9b3a-2c7f0e1a4d90`.
- Standard HRS service `0x180D`, Heart Rate Measurement `0x2A37`, Body Sensor Location `0x2A38`. Heart rate is a preview from IR pulse detection, not a clinical result or HRV.
- COMMAND accepts ASCII/UTF-8 text writes and emits text notifications. Long replies are split into consecutive characteristic notifications; they have no additional chunk header. USB CDC accepts CR/LF-delimited commands at 115200 baud.

BLE notifications have **no application acknowledgement**. `ble_notify_calls` counts invocations of the local API, not receipt on the phone. A USB `Serial.write` can also be short; `usb_short` counts these faults. The host should resynchronize on magic, length and CRC, and inspect sequence gaps. The one shared acquisition ring is consumed after a local submission succeeds on at least one requested transport; simultaneous BLE and USB are not independently replayed.
The firmware checks the characteristic's CCCD notification subscription before considering BLE usable. A disconnected or unsubscribed client cannot consume the ring through a no-op notify call.

## Binary frame

All multi-byte values are little-endian. The fixed header is exactly **13 bytes**, followed by the payload and a two-byte CRC.

| Offset | Bytes | Meaning |
|---:|---:|---|
| 0 | 1 | Magic `0xA5` |
| 1 | 1 | Type: 1 ECG, 2 PPG, 3 telemetry, 4 event |
| 2 | 1 | Protocol version `0x12` |
| 3 | 1 | Flags, currently zero |
| 4 | 2 | Unsigned sequence, per type, wraps at 65536 |
| 6 | 4 | Microseconds since START modulo 2³² |
| 10 | 2 | Estimated sample period in µs; zero for telemetry/event |
| 12 | 1 | ECG/PPG sample count, event text length, or zero for telemetry |

CRC is **CRC-16/CCITT-FALSE**: initial `0xFFFF`, polynomial `0x1021`, MSB-first, no reflection, no final XOR. It covers offset 0 through the final payload byte; the two CRC bytes themselves are excluded and sent little-endian. Test vectors: byte `00` gives `0xE1F0` (wire `F0 E1`); ASCII `123456789` gives `0x29B1`. Total length is `13 + payload_length + 2`.

### Frame payloads

- ECG: `count` signed 16-bit ADS1115 raw AIN0 values, PGA ±4.096 V. Length `15 + 2×count`; count 1–80.
- PPG: `count` pairs of unsigned 24-bit little-endian IR and RED. Only 18 source bits are meaningful. Length `15 + 6×count`; count 1–30. FIFO sample times are estimated from FIFO read time and configured rate, not hardware per-sample timestamps.
- Telemetry: fixed 24-byte payload, 39-byte frame, count byte zero. Relative offsets: GSR µS×1000 `uint32` at 0; temperature °C×100 `int16` at 4; battery mV `uint16` at 6; thorax mV at 8; abdomen mV at 10; HR bpm at 12; RR ms at 14; flags at 16; ECG ring depth at 18; PPG ring depth at 20; telemetry ring depth at 22. Flag bits: 0 ADS present, 1 PPG present, 2 last temperature reading valid, 3 ECG enabled, 4 GSR enabled, 5 thorax enabled, 6 abdomen enabled, 7 leads-off. The first three flags are health indicators at acquisition, not continuous I²C self-tests.
- Event: `count` text bytes, truncated to 50; length `15 + count`. `BOOT` may be emitted before a BLE connection. `SHUTDOWN` may not fit MTU 23 and is best effort.

`START` resets ECG/PPG/event sequences and starts a new 64-bit internal time origin. The wire timestamp wraps every about 71.6 minutes; subtract modulo 2³². Telemetry sequence is allocated at acquisition, not send, so a ring overwrite or unsent snapshot can produce a visible sequence gap. Frame sequences do not identify sample-level losses inside an ADC/PPG ring; inspect `drops`, `fifo_ovf`, and `sw_ovf` as well.

## MTU and transport selection

The firmware requests MTU 247, but uses the negotiated value. Maximum ATT notification value = `MTU − 3`. Maximum samples = `floor((value_max − 15) / bytes_per_sample)`, further capped at 80 ECG or 30 PPG. MTU 23 allows two ECG samples but no PPG sample; PPG requires MTU **24**, telemetry **42**, and `SHUTDOWN` event **26**. If BLE cannot hold one sample and USB binary streaming is enabled, the firmware sends a USB-sized frame and skips BLE for that frame. When both can carry it, the BLE limit determines their common frame size. A low BLE MTU therefore does not block USB PPG.

## Commands and USB modes

`START`, `STOP`, `STATUS`, `HELP`, `DEFAULTS`, `SAVE`, `SLEEP`; `ECG`, `GSR`, `THORAX`, `ABDOMEN`, `PPG`, `TEMP`, `BAT`, `ECG_STREAM`, `PPG_STREAM`, `TELEM_STREAM`, `JSON`, `USB_ECG`, `USB_PPG`, `USB`, `BLE` followed by `ON|OFF`; `SET ECG_RATE n`, `SET PPG_RATE 100|200|400`, `SET GSR_RATE n`, `SET THORAX_RATE n`, `SET ABDOMEN_RATE n`, `SET AUTO_ECG ON|OFF`, `SET VREF 0.45..0.55`; `USB_MODE TEXT|BINARY`.

USB starts in TEXT on every boot; this mode is never saved. `USB_MODE BINARY` sends a final text acknowledgement and then suppresses text/DIAG/JSON on USB while binary frames may be written. Commands continue to be accepted in BINARY; `USB_MODE TEXT` restores text output. `USB_ECG` and `USB_PPG` need BINARY. `SAVE` returns `ERR SAVE_NVS` if any NVS write fails. `DIAG` is an automatic periodic text line, **not a command**. JSON `v=4` is optional debug data and is not compatible with an older Android JSON client.

## Session and sleep

`START` closes the previous generation, clears software rings, verifies a physical MAX30102 FIFO clear, resets the time origin and resumes acquisition. On FIFO clear failure it leaves acquisition stopped and returns `ERR START_PPG_FIFO`. `STOP` invalidates in-flight work. Disabling a channel prevents an in-flight ADC/PPG batch from publishing after OFF. `SET PPG_RATE` and `DEFAULTS` require `STOP` first (`ERR STOP_REQUIRED`). Changing PPG rate reconfigures the physical sensor, clears its ring/FIFO and invalidates in-flight acquisition.

`SLEEP` or D3/GPIO4 LOW for 40 ms requests deep sleep. The coordinator stops acquisition, waits for ADC/SLOW/COMMS to park and the PPG worker to verify MAX30102 shutdown. If not complete within 2 s, it aborts and reports `ERR SLEEP_NOT_QUIESCENT`; no deep sleep is entered. MAX30205 remains supplied by the fixed PCB. GPIO4 wake level is the opposite of its level at sleep entry. Wake reboots and USB returns to TEXT.
