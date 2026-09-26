# Firmware 1.2 code review guide (English)

Source: `firmware/PinaBiosensor_Firmware_v1_2/PinaBiosensor_Firmware_v1_2.ino`. This guide describes the *implemented code*, its ownership rules, failure paths and limitations so a reviewer can check it without relying on prior chat history. The source is one Arduino sketch because the Arduino build expects a matching folder/sketch name. It targets the fabricated Paca v1.0, never a future PCB.

## 1. Fixed hardware and dependencies

Seeed XIAO ESP32-S3; ADS1115 `0x49` AIN0 ECG, AIN1 GSR, AIN2 thorax, AIN3 abdomen; MAX30102 `0x57`; MAX30205 `0x48`. D0/A0/GPIO1 battery divider 47k/47k; D3/GPIO4 active-low sleep; D8/GPIO7 LO+, D9/GPIO8 LO−; D4/GPIO5 I²C. `PIN_*`, `*_ADDR`, BLE name `PinaBiosensor`, and UUID constants should not be edited to match a different board. Wi-Fi is disabled in `setup()`. ADS ALERT is not wired, so the ADS driver polls OS. SparkFun MAX3010x supplies FIFO and beat helper routines; Arduino-ESP32 supplies BLE, FreeRTOS, Preferences and deep sleep.

## 2. Modules, owners and significant variables

| Code section | Key functions | State and meaning | Owner / lock |
|---|---|---|---|
| Hardware/protocol constants | `adsMuxBits`, `adsGainBits`, frame builders | `PROTOCOL_VERSION=0x12`, addresses, pins, acquisition targets, `PACKET_MAGIC` | Compile-time constants |
| Session/configuration | `handleCommand`, `loadConfig`, `saveConfig`, `defaultsConfig` | `runState`, `sessionGeneration`, `sessionStartUs`, `en*`, `stream*`, `tx*`, `*RateCfg`, `gsrVref`, `configDirty` | `stateMutex`; COMMS writes commands, setup initializes |
| ADC scheduling | `pickNextAdcChannel`, `rebuildAdcSchedule`, `adcTask` | `adcSlots[].nextDueUs` soft due time; `lastAdsChan/lastAdsGain` actual prior conversion; `adcScheduleDirty`; `adcSamples[]`, `adcErrors[]` | ADC task alone changes slots; config and commit use `stateMutex` |
| ADS I²C | `adsWriteConfig`, `adsReadReg16`, `adsStartAndRead` | MUX AIN0–3 (`0x4000`+channel shift); ECG/bands PGA ±4.096 V (`0x0200`); GSR ±0.512 V (`0x0800`); DR 860 | Every Wire transaction uses `i2cMutex`, released on every exit |
| PPG acquisition/power | `configurePpg`, `ppgHardware`, `clearPpgFifoForSession`, `ppgTask`, `processPpgBeat` | `ppgRateCfg`, `ppgFifoOverflow`, `ppgSoftwareOverflow`, `ppgPowerErrors`, `ppgHr`, `ppgRrMs`, `lastBeatTsMs` | PPG task owns regular reads/power; setup/commands reconfigure under `i2cMutex`; result commit under `stateMutex` |
| Slow acquisition | `max30205Read`, `slowTask` | `lastTempC`, `lastBattV`, `lastLo`, `tempOk`, `tempI2cErrors`, `telSeq` | SLOW task; I²C under `i2cMutex`, snapshot under `stateMutex` |
| Rings | `ecgPush/Peek/Discard`, `ppgPush/Peek/Discard`, `telPush/Peek/DiscardIfSeq` | `ecgRing`, `ppgRing`, `telRing`, heads/tails, `ecgDrops`, `ppgDrops`, `telDrops` | Each ring's dedicated `portMUX` protects indices/data; no I/O while held |
| BLE/USB | `bleNotify`, `bleText`, `usbWriteFrame`, `send*Frame`, `process*Commands` | `bleConnected`, `usbBinaryMode`, `bleNotifyAttempts`, `bleNotifyRejected`, `framesGenerated`, `framesSubmitted`, `usbFrames`, `usbShortWrites` | COMMS normally sends; `sleepTask` sends final event after COMMS parks; `bleMutex` serializes setValue/notify |
| Sleep | `sleepTask`, `performDeepSleep` | `sleepRequested`, `deepSleeping`, `adcParked`, `ppgParked`, `slowParked`, `commsParked`, `sleepAborts`, `sleepFailureLatch` | `sleepTask` coordinates; all park flags use `stateMutex` |
| Diagnostics | `statusText`, `printDiagnostics` | `measuredAdcSps[]`, `measuredPpgSps`, health flags, ring depths, stack high-water marks | COMMS normally prints; status values are observations, not rate guarantees |

`ESample` and `PSample` contain raw readings and absolute `uint64_t` microseconds. `SlowSample` contains a telemetry snapshot and a sequence assigned **at acquisition**; this makes dropped snapshots visible. `CmdMsg` copies up to 127 command bytes out of the BLE callback so the callback buffer cannot be used after it changes. `StateGuard` is an RAII wrapper: each successful take has a destructor Give; `pickNextAdcChannel` explicitly gives early and clears `held` to avoid a double Give.

### Global variable dictionary

- `ppg` is the SparkFun sensor driver; `prefs` is the NVS handle. `bleServer` and `chJson/chCmd/chEcg/chPpg/chTel/chEvt/chHr` are created once in `setupBle()` and remain valid until BLE deinit.
- `i2cMutex`, `bleMutex`, `stateMutex` define the three serialized resources. `cmdQueue` transfers copied BLE command text to COMMS. `adcTaskHandle/ppgTaskHandle/slowTaskHandle/commsTaskHandle/sleepTaskHandle` support allocation checks and stack diagnostics. `ecgMux/ppgMux/telMux` protect short ring critical sections.
- `bleConnected` is set by BLE callbacks; `sleepRequested`, `deepSleeping`, `sleepAbortPending`, `sleepFailureLatch` are one-way coordination flags. `adcParked/ppgParked/slowParked/commsParked` are acknowledgements protected by `stateMutex`. `ppgPowerUnverified` blocks a false PPG park acknowledgement after failed setup. `sessionGeneration` invalidates old work; `usbBinaryMode` resets false on boot.
- `runState` controls acquisition. `enEcg/enGsr/enThorax/enAbdomen/enPpg/enTemp/enBattery` enable acquisition; `streamEcg/streamPpg/streamTelemetry/streamJson` control output; `txUsb/txBle` gate transports; `usbEcgStream/usbPpgStream` gate raw USB frames; `autoEcgBoost` changes the ECG target calculation.
- `ecgRateCfg/ppgRateCfg/gsrRateCfg/thoraxRateCfg/abdomenRateCfg` are requested SPS, not measured values. `gsrVref` is the calibration voltage used in GSR conversion. `adcSlots[4]` carry due times; `lastAdsChan/lastAdsGain` record the actual prior conversion for mux accounting; `adcScheduleDirty` requests a rebuild.
- `adsOk/ppgOk` record boot detection/configuration, not continuous health. `tempOk` reflects the most recent MAX30205 read. `lastEcgV/lastGsrUs/lastThoraxV/lastAbdomenV/lastTempC/lastBattV/lastLo` are the latest derived readings used for JSON/telemetry.
- `ppgHr/ppgRrMs`, `lastBeatTsMs`, `hrHist[4]`, `hrHistCount` belong to the approximate beat preview. They reset on START and PPG rate change.
- `adcSamples[4]/adcErrors[4]`, `adcConversions`, `adcMuxChanges`, `ppgSamples` count acquisition and failures. `ppgFifoOverflow` is a lower bound from hardware's 5-bit counter; `ppgOverflowPrev` is its prior read; `ppgSoftwareOverflow` estimates SparkFun ring loss; `ppgI2cErrors` covers explicit PPG register failures; `ppgPowerErrors` covers failed SHDN/wake verification; `tempI2cErrors` counts temperature read failures.
- `ecgDrops/ppgDrops` count rejected new ring samples; `telDrops` counts overwritten oldest telemetry; `cmdDrops` counts BLE commands rejected by the full command queue. `ecgPackets/ppgPackets/telPackets/eventPackets` count successfully submitted frame attempts by type.
- `framesGenerated` counts constructed frames; `framesSubmitted` counts frames with at least one local BLE submission or complete USB write; `bleNotifyAttempts` and `bleNotifyRejected` are local API outcomes; `usbFrames` and `usbShortWrites` count complete/short USB frame writes; `sleepAborts` counts failed sleep barriers. None proves phone reception.
- `ecgSeq/ppgSeq/evtSeq` increment on submitted frames; `telSeq` increments on each telemetry acquisition. `sessionStartUs` is a 64-bit clock origin; `lastTelemetryMs/lastDiagMs` pace output; `configDirtySinceMs/configDirty` pace NVS retries; `bootEventPending` lets COMMS emit BOOT as the sole routine sender after task creation.
- `ecgRing/ppgRing/telRing` hold samples; `ecgHead/ecgTail`, `ppgHead/ppgTail`, `telHead/telTail` are circular indices protected by their matching portMUX. Each ring keeps one slot unused to distinguish full from empty.

Important local names: `generation` is a worker's captured session; `active`/`want` are its start-of-work decision; `hwOn` is PPG's verified power state; `maxLen` is the frame limit; `canBle` requires subscription and sufficient MTU; `submitted` means local API/write acceptance; `periodUs` is estimated sample spacing; `parked` combines sleep acknowledgements. None is a hardware delivery guarantee.

## 3. The important statements and invariants

1. `adsRawToVolts()` uses the **same full-scale range** as `adsGainBits()`. For GSR both select ±0.512 V. `calcGsrUs()` rejects invalid/out-of-range voltages rather than reporting a fabricated conductance.
2. `adsStartAndRead()` writes single-shot configuration, polls the OS bit and reads the conversion. The configured 860 SPS is the ADC's maximum data rate, **not** the number of ECG samples this shared scheduler can deliver. `STATUS actual` and periodic `DIAG` must be measured on the real board.
3. `pickNextAdcChannel()` selects the earliest due enabled slot and releases `stateMutex` before waiting or I²C. This is a soft-deadline scheduler. On a late conversion it advances the slot instead of looping indefinitely to catch up.
4. ADC/PPG/SLOW capture `sessionGeneration` before work and compare it under `stateMutex` before publishing. START/STOP and PPG rate changes invalidate prior work. ADC also checks that its channel remains enabled; PPG checks `enPpg` at commit.
5. `START` freezes the MAX30102, clears and verifies its **physical** FIFO pointers, and drains SparkFun's small RAM ring in addition to the firmware rings. On failure it leaves the session stopped. This avoids old samples entering a new time origin.
6. `ppgHardware(false)` uses SparkFun shutdown, then reads MAX30102 MODE_CONFIG register `0x09` and requires SHDN bit 7. On a mutex timeout or failed read it returns false, increments `ppgPowerErrors`, and PPG does not acknowledge parking. Failed initial configuration sets `ppgPowerUnverified`, also preventing sleep from assuming shutdown. Sleep then aborts after 2 s. A register read proves the bit was set, not that supply current is zero.
7. ECG/PPG builders calculate length from **13-byte header + payload + 2-byte CRC**. They peek first; they discard only after a BLE API invocation with CCCD notifications enabled or a full USB write. A BLE notification can still be lost after the call: this code does not claim end-to-end delivery.
8. If BLE MTU cannot hold one sample, `canBle=false` but USB may still send. When both can send, the BLE size caps the shared frame. The firmware does not keep separate per-transport cursors or replay a frame independently to a lagging client.
9. Telemetry is sequenced in `slowTask`, before queuing. `telPeek()` leaves it queued on send failure; `telDiscardIfSeq()` avoids discarding the next snapshot if the producer overwrote the oldest during a send. `telDrops` exposes ring overwrites.
10. `usbBinaryMode` is false at boot and deliberately absent from `saveConfig()`. In BINARY mode text output is suppressed after the final mode acknowledgement; USB commands remain readable. `usbWriteFrame()` counts short writes, and the host must resynchronize on CRC.
11. `saveConfig()` checks `Preferences.begin` and each byte count returned by `put*`; an error returns `ERR SAVE_NVS` and leaves `configDirty` true for retry. NVS stores multiple independent keys, so this is **not an atomic transaction** across power loss.
12. `performDeepSleep()` first stops acquisition and advances the generation, then waits for four parked acknowledgements. PPG's acknowledgement includes a verified shutdown. If any task fails to park within 2 s, sleep is cancelled, previous run state restored, and `ERR SLEEP_NOT_QUIESCENT` reported. The MAX30205 is still physically powered; measure PCBA sleep current.
13. `STATUS frames_submitted` and `ble_notify_calls` represent *local submissions*. Neither is an ACK from the mobile application. Ring drops, FIFO overflow and sequence gaps must also be checked. `stack_hwm` gives the FreeRTOS minimum remaining stack (ESP-IDF units); watch it during bring-up.

## 4. Review boundaries and limitations

There is no hardware-level validation in this source review. ADS 250 SPS ECG/500 boost, PPG 200 SPS, I²C contention, optical behavior, FIFO overflow and sleep current must be measured. SparkFun's `check()` does not return a complete I²C error report, so `ppg_i2c` mainly counts explicit register operations; it is not a complete PPG bus health metric. The 5-bit MAX30102 overflow counter can saturate; `fifo_ovf` is a lower bound. PPG timestamps are estimates. BLE notify has no phone ACK. JSON v4 remains debug-only. No Android v1.2 client is in this repository.

For a first bring-up, disconnect all electrodes and skin contacts before USB. Compile with a pinned Arduino-ESP32 core and SparkFun library, upload, inspect boot probes and `STATUS`, then measure DIAG after two intervals. Test START/STOP, CRC, MTU 23/24/42/247, USB BINARY with BLE MTU 23, NVS error handling where possible, PPG shutdown register and sleep current. Skin-connected operation uses battery and BLE only. This is not a medical device.

## Primary references

- [TI ADS1115 datasheet](https://www.ti.com/lit/ds/symlink/ads1115.pdf): MUX, PGA and conversion mode/rate.
- [Analog Devices MAX30102 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max30102.pdf): FIFO registers 0x04–0x06 and MODE_CONFIG.SHDN at 0x09 bit 7.
- [SparkFun MAX3010x MAX30105.cpp](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/blob/master/src/MAX30105.cpp): `clearFIFO`, `shutDown`, `wakeUp`, `check` behavior.
- [Espressif Arduino Preferences reference](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html): `put*` byte counts and failure returns.
