# Caja: interruptor DPDT de panel + medidor de batería

PinaBio v1.0 adaptada para **caja**: el slide SW1 ya no se monta en la PCB.
El encendido/apagado va en un **DPDT de panel** cableado a **J9**. El medidor de batería usa **J10**.

## J9 — INT CAJA (JST-PH 4 pines)

Conector en la PCB: `JST_PH_S4B-PH-K` horizontal. Sedas: **J9: INT CAJA**, pines 1–4.

| Pin J9 | Seda     | Net PCB   | Cable al DPDT de panel                         |
|--------|----------|-----------|------------------------------------------------|
| 1      | ANALOG   | LDO_VIN   | Polo A — lado “alimentación analógica / LDO”   |
| 2      | BAT      | V_BATT    | Polo A — lado batería                          |
| 3      | SLEEP    | SLEEP_N   | Polo B — señal sleep (activo al GND)           |
| 4      | GND      | GNDD      | Polo B — masa                                  |

### Cableado del DPDT de panel (4 hilos)

Misma lógica que el antiguo slide:

1. **Polo A (ON = enciende analógico):** une **BAT (pin 2)** ↔ **ANALOG (pin 1)**  
   → En ON: `V_BATT` alimenta `LDO_VIN`.
2. **Polo B (OFF = sleep):** une **SLEEP (pin 3)** ↔ **GND (pin 4)**  
   → En OFF: `SLEEP_N` a masa (duerme). En ON: ese polo queda abierto.
3. Los contactos “extras” del DPDT (equivalentes a pines 3 y 6 del slide viejo) **no se usan** (N/C en el arnés).

**No cortes** el camino de carga BAT del XIAO (sigue por su propio hilo/pads).

Pistas de potencia en PCB ≥ 0,45 mm.

## J10 — BATT METER (JST-PH 2 pines)

Paralelo a **J1** (LiPo):

| Pin J10 | Net    | Uso                          |
|---------|--------|------------------------------|
| 1       | V_BATT | Positivo batería / medidor   |
| 2       | GNDD   | Masa                         |

Seda: **J10: BATT METER**. Sirve para enchufar un voltímetro / monitor de batería sin desconectar J1.

## Fábrica / ensamblaje

- Montar **J9** y **J10** (y el resto SMD + zócalo XIAO + J1/J2 + bornes).
- **No montar** el slide SW1.
- El DPDT y el cable de 4 hilos van en la **caja** (panel).

## Seguridad

No es un dispositivo médico. Con electrodos en piel: solo Bluetooth; USB del PC desconectado.
