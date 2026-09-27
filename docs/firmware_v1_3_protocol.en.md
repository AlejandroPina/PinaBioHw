# Firmware PinaBiosensor 1.3 — protocol (English)

This specification is for fabricated PinaBio Paca v1.0 and sketch `firmware/PinaBiosensor_Firmware_v1_3/PinaBiosensor_Firmware_v1_3.ino`.

**Wire format is identical to firmware 1.2.** The version byte remains **`0x12`**. Receivers that already accept 1.2 must keep accepting `0x12`. Do not require `0x13`.

Full frame layout, UUIDs, CRC, MTU rules, commands and sleep behaviour: [`firmware_v1_2_protocol.en.md`](firmware_v1_2_protocol.en.md). Firmware 1.2 sources stay in the tree.

## 1.3 differences (commands / STATUS only)

- Boot serial line: `PinaBiosensor firmware v1.3 ... READY`.
- `STATUS` / `DIAG` prefix fields: `fw=1.3 proto=0x12` then the 1.2 fields.
- `STOP` clears the MCU beat preview (HR/RR become 0). Binary telemetry after STOP is not produced because acquisition is stopped.
- `ERR START_LOCK`, `ERR DEFAULTS_LOCK`, `ERR PPG_RATE_LOCK` if `stateMutex` cannot be retaken after I²C. Rare.
- `SET PPG_RATE` / `DEFAULTS` still require `STOP`. A failed PPG reconfigure leaves the previous hardware rate.

There is **no** `CONFIRM_SKIN_SESSION` and **no** `ERR USB_SKIN_INTERLOCK`. This PCB cannot sense USB VBUS. Skin sessions: battery + BLE. Charge (including power bank) and flash with electrodes off.

Sketch 1.2 remains at `firmware/PinaBiosensor_Firmware_v1_2/` for rollback.
