# PinaBiosensor V1 - ADS1115 Scheduler Design v0.2

## Objective
Use the fabricated V1 unchanged. The ADS1115 has one multiplexed ADC serving:
- AIN0 ECG
- AIN1 GSR
- AIN2 thorax
- AIN3 abdomen

The scheduler is dynamic: only enabled channels consume ADC conversions.

## Important ADS1115 fact
The ADS1115 is a 16-bit, 4-channel multiplexed delta-sigma ADC with programmable data rates up to 860 SPS and single-cycle settling. Therefore 860 SPS is the conversion-rate ceiling, not 860 SPS per input channel.

## Initial targets
- ECG: 250 SPS base, automatic boost up to 500 SPS
- GSR: 10 SPS
- Thorax: 20 SPS
- Abdomen: 20 SPS

PPG is NOT part of this scheduler. The MAX30102 has its own FIFO/stream and is handled independently.

## Conversion budget
At 860 SPS the nominal conversion period is about 1163 us.

| Active set | ECG target | GSR | Thorax | Abdomen | Total target SPS | Nominal ADC conversion occupancy |
|---|---:|---:|---:|---:|---:|---:|
| ECG only | 500 | 0 | 0 | 0 | 500 | 58.1% |
| ECG + GSR | 500 | 10 | 0 | 0 | 510 | 59.3% |
| ECG + thorax | 500 | 0 | 20 | 0 | 520 | 60.5% |
| ECG + thorax + abdomen | 500 | 0 | 20 | 20 | 540 | 62.8% |
| All four | 500 | 10 | 20 | 20 | 550 | 64.0% |
| All four, ECG base | 250 | 10 | 20 | 20 | 300 | 34.9% |

Theoretical conversion-time headroom at the all-active boosted case is about 36% before software/I2C overhead. With a modeled extra 200 us per conversion, total service occupancy is about 75%, leaving useful margin. This is a design target, not a measured guarantee.

## Scheduler strategy
Use a single FreeRTOS task as the sole owner of the ADS1115. Other tasks never call ADS functions directly.

The ADC task maintains one due time per enabled channel. The next channel is the enabled channel with the earliest due time. After a successful conversion, its next due time advances by 1/rate. This is a deadline/rate scheduler and automatically adapts when channels are enabled/disabled or rates change.

Do NOT create one fixed sequence such as ECG-GSR-TH-AB forever. A fixed round-robin wastes ECG bandwidth when slow channels are disabled.

## Why no blanket 20 ms GSR settle
The ADS1115 itself has single-cycle settling. A blanket 20 ms delay after every GSR mux switch would unnecessarily steal ADC time. The fabricated V1 documentation does not establish a 100 nF GSR shunt capacitor, so that delay should not be assumed. The actual GSR analog settling must be measured on the PCBA.

The firmware should therefore:
1. switch MUX/gain;
2. take the conversion;
3. record the actual value and timestamp;
4. if lab data shows GSR settling error after the switch, add a configurable discard/settle policy specifically for GSR.

## Timing data
For ECG/PPG raw streams use session-relative microsecond timestamps. Millisecond-only sample timestamps are not sufficient when running 500 SPS because consecutive samples are only 2 ms apart.

A raw frame should contain:
- stream type
- protocol version
- sequence number
- first-sample timestamp
- nominal sample period
- sample count
- raw samples
- CRC16

For ECG, the application should also be able to detect packet loss from the sequence number. PPG is independently buffered.

## Buffering
ECG and PPG use separate ring buffers. Acquisition never waits for BLE. BLE drains buffers in blocks.

If BLE is temporarily slow:
- acquisition continues;
- queue depth grows;
- diagnostics report queue depth and drops;
- once the buffer is exhausted, the firmware reports a real drop instead of silently inventing data.

## Dynamic channel examples
### Only ECG
ECG can use the full configured high-rate allocation. No mux switching occurs.

### ECG + GSR
ECG remains dominant. GSR gets a small periodic allocation.

### All channels
Target 500 + 10 + 20 + 20 = 550 conversions/s. This is comfortably below 860 SPS at the ADC-conversion level.

### ECG disabled
The scheduler immediately stops reserving ADC conversions for AIN0. The ADC bandwidth is redistributed to the remaining enabled channels or left idle.

## Firmware corrections required before PCBA test
1. Store ECG/PPG timestamps in microseconds, not milliseconds.
2. Do not consume ring-buffer samples until a BLE notification succeeds, or implement explicit requeue/rollback.
3. Keep BLE packets independent for ECG and PPG.
4. Use MAX30102 FIFO as a true raw stream source.
5. Fix the PPG beat counter so it cannot wrap and divide by zero after 255 beats.
6. Keep MAX30205 at 0x48; do not use SHT4x/0x44.
7. Keep USB and BLE commands routed through the same command processor.
8. Keep Wi-Fi disabled.
9. Keep D3/GPIO4 sleep and D8/D9 leads-off support.
10. Report measured ECG/PPG SPS and drops in diagnostics.

## Acceptance test on first PCBA
Run these modes:
1. ECG only
2. ECG + GSR
3. ECG + thorax + abdomen
4. all four ADS channels
5. PPG only
6. ECG + PPG
7. all streams + BLE

For each mode record:
- requested SPS
- measured SPS per stream
- jitter
- mux switch count
- buffer high-water mark
- dropped samples
- BLE notification failures
- I2C errors

The first hardware test decides the final default SPS. The architecture does not need to change if the measured numbers differ from the targets.
