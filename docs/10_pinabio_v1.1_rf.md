# PinaBio v1.1 — giro de layout RF (no es el pedido actual)

**ES:** Esto es el **siguiente** giro de placa. El pedido JLCPCB que ya está en marcha (**SMT026091261919**) sigue siendo **v1.0**. No uses estos Gerbers v1.1 para ese pedido. No hace falta cancelar v1.0: v1.0 sirve para probar el circuito; v1.1 es para BLE/Wi‑Fi cuando quieras otra tirada.

**EN:** This is the **next** PCB spin. The paid JLCPCB order stays **v1.0**. Do not upload `PinaBio_v1.1_*` to that order.

Apodo **Paca** igual. Bluetooth del teléfono: **`PinaBiosensor`**. Circuito eléctrico **igual** (mismas redes, mismas piezas, mismos conectores). Solo cambia el **dibujo de cobre**: el zócalo XIAO (U1) se acerca al borde superior para enchufar el USB-C.

## Qué se movió

- Placa sigue **110 × 70 mm**, 2 capas.
- **U1** (zócalo Seeed XIAO ESP32-S3) centro en **(20.00 mm, 9.22 mm)**, rotación 0°. USB hacia el borde superior (Y = 0).
- El USB-C del módulo queda en el borde. Keepout KiCad `ANT_KEEPOUT` bajo ese extremo: **12.4–27.6 mm × 0.40–6.40 mm**, F.Cu y B.Cu (sin plano, sin pistas, sin vias). Eso evita cobre bajo el metal del USB, no es la antena de radio.
- El XIAO ESP32-S3 **no** lleva antena cerámica en el USB. El Bluetooth usa el conector **U.FL** (en el módulo, hacia `BAT+`) y el **cable-antena de la bolsa**. Hay que enchufarlo y sacar el cable de la caja de plástico. Sirve igual en v1.0 y en v1.1. No recortes la placa v1.0.
- Conectores J1, J2, J10, J9, J3, J6, J7, J8, J4, J5 **no se quitaron**. SW1 **no existe**.
- Seda: **PINABio v1.1** (sigue sin “Paca” en cobre).

## Archivos de fábrica v1.1 (próxima tirada)

- `exports/PinaBio_v1.1_gerbers.zip`
- `exports/PinaBio_v1.1_bom_jlc.csv`
- `exports/PinaBio_v1.1_cpl.csv`

## Archivos del pedido v1.0 (no tocar)

- `exports/PinaBio_v1_gerbers.zip`
- `exports/PinaBio_v1_bom_jlc.csv`
- `exports/PinaBio_v1_cpl.csv`
- Copia: `hardware/archive/PinaBio_v1.0/` y `exports/archive_v1.0/`

## DRC

v1.1: **0 errores / 0 unconnected** (`exports/drc_v1.1.json`).
