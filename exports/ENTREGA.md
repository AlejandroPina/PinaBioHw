# Entrega — PinaBio v1.0 (adaptación caja)

> **Paquete del pedido restaurado en `main` (28-09-2026).** `exports/PinaBio_v1_gerbers.zip`, BOM y CPL coinciden con los SHA-256 de `exports/archive_v1.0/SHA256SUMS`. Comprueba los hashes antes de repetir el pedido. No usar `exports/gerbers/` como sustituto.

Producto: **PinaBio v1.0**. Gerbers para pedir: `exports/PinaBio_v1_gerbers.zip` (copia byte a byte de `PinaBiosensor_Completa_gerbers.zip`; no se regeneró cobre).

Fecha: 2026-09-12  
Para: Alejandro

## Qué cambió (caja)

1. **Eliminado SW1** (slide `SW_CK_JS202011CQN_DPDT_Straight`) del esquemático y de la PCB.
2. **J9 INT CAJA** — JST-PH 4P (`JST_PH_S4B-PH-K_1x04_P2.00mm_Horizontal`) para DPDT de panel:
   - 1 ANALOG → `LDO_VIN`
   - 2 BAT → `V_BATT`
   - 3 SLEEP → `SLEEP_N`
   - 4 GND → `GNDD`
3. **J10 BATT METER** — JST-PH 2P (misma familia que J1), paralelo a J1 (`V_BATT` / `GNDD`), zona libre cerca del borde inferior izquierdo.
4. Esquemático con símbolos reales Conn_01x04 / Conn_01x02; nota de título: panel DPDT vía J9; fábrica no monta slide.
5. BOM, docs `08_pedido_jlcpcb.md` y `09_caja_interruptor_medidor.md` actualizados.

## Congelado respetado

- Sleep en **D3/GPIO4**
- I2C a **+3V3**
- ADP150 pin4 = **NC** (sin C3 BYP)
- Un solo NT1; GSR / bandas / ECG sin cambios
- Tamaño placa **110 × 70 mm**
- Camino de carga BAT del XIAO **no cortado**

## DRC

**0 errores / 0 unconnected** (`exports/drc_final.json`, `exports/drc_report.txt`).

## Exports

- Gerbers: `exports/gerbers/` + ZIP `exports/PinaBio_v1_gerbers.zip` (alias: `PinaBiosensor_Completa_gerbers.zip`)
- CPL JLCPCB: `exports/PinaBio_v1_cpl.csv` (alias: `PinaBiosensor_Completa_cpl.csv`)
- BOM JLCPCB: `exports/PinaBio_v1_bom_jlc.csv` (alias: `PinaBiosensor_Completa_bom_jlc.csv`)
- BOM diseño + LCSC: `bom/BOM.csv`
- 3D: `exports/pcb_3d_caja.png` (también `pcb_3d_caja_iso.png`)

## Ensamblaje

Soldar todo SMD (incl. ADS1115), zócalo XIAO, J1, J2, **J10**, bornes J3/J6/J7, conector **J9**.  
**No montar** slide SW1 — el DPDT va en el panel de la caja (4 hilos a J9).

Ver `docs/09_caja_interruptor_medidor.md` y `docs/08_pedido_jlcpcb.md`.

**No es un dispositivo médico.** Sesión con piel: solo Bluetooth, USB del PC desconectado.
