# Firmware PinaBiosensor 1.3 — protocolo (español)

Especificación para la Paca v1.0 fabricada y el sketch `firmware/PinaBiosensor_Firmware_v1_3/PinaBiosensor_Firmware_v1_3.ino`.

**El formato en el cable es el de firmware 1.2.** El byte de versión sigue siendo **`0x12`**. Un receptor que ya acepta 1.2 debe seguir aceptando `0x12`. No exige `0x13`.

Layout de frames, UUID, CRC, MTU, comandos y sleep: [`firmware_v1_2_protocol.es.md`](firmware_v1_2_protocol.es.md). El código 1.2 se conserva en el árbol.

## Diferencias 1.3 (solo comandos / STATUS)

- Arranque serie: `PinaBiosensor firmware v1.3 ... READY`.
- `STATUS` / `DIAG` empiezan con `fw=1.3 proto=0x12` y siguen los campos de 1.2.
- `STOP` pone a cero la vista previa de pulso del MCU.
- `ERR START_LOCK`, `ERR DEFAULTS_LOCK`, `ERR PPG_RATE_LOCK` si no se puede recuperar `stateMutex` tras I²C. Raro.
- `SET PPG_RATE` / `DEFAULTS` siguen exigiendo `STOP`. Si falla el PPG, el chip queda en la tasa anterior.

**No** hay `CONFIRM_SKIN_SESSION` ni `ERR USB_SKIN_INTERLOCK`. Esta PCB no siente VBUS. Sesión con piel: batería + BLE. Cargar (también con power bank) y flashear con electrodos fuera.

El sketch 1.2 sigue en `firmware/PinaBiosensor_Firmware_v1_2/` por si hay que volver atrás.
