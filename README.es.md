[English](README.md) | [Español](#)

# PinaBio v1.0 (Paca)

El **pedido JLCPCB que ya está pagado** es **v1.0** (`exports/PinaBio_v1_*`). Hay un **layout v1.1** (USB del XIAO al borde) para **otra tirada después**: `exports/PinaBio_v1.1_*` y `docs/10_pinabio_v1.1_rf.md`. Mismo circuito. No subas v1.1 al pedido actual.

**PinaBio** es una placa de **biofeedback** (piel, pulso, temperatura, ECG, respiración). **No es un dispositivo médico** y no diagnostica nada. El apodo **Paca** es un homenaje a la abuela de Alejandro.

En el teléfono, el Bluetooth se anuncia como **`PinaBiosensor`**. Conserva este nombre BLE para los clientes del firmware.

## Qué es la placa

Una PCB de unos **110 × 70 mm**, micro **Seeed XIAO ESP32-S3** en zócalo:

- GSR (piel)
- Pulso / HRV (MAX30102) — las cuentas de ritmo van en el **teléfono**
- Temperatura (MAX30205)
- ECG (módulo AD8232 — juguete de laboratorio, no un electrocardiógrafo de hospital)
- Respiración pecho + abdomen (gomas que cambian de resistencia)

En textos viejos esta misma placa se llama «Completa». Los ficheros KiCad siguen `PinaBiosensor_Mini.*` a propósito (renombrar rompe el proyecto). En la seda pone **PINA** / **PINABio v1.0** (no cabe «Paca» en la placa).

Carga el XIAO por su USB-C. **Con electrodos o bandas en la piel: solo Bluetooth. Nunca USB del PC y piel a la vez.**

Especificación de hardware + firmware (para revisión): `docs/11_especificacion_hw_firmware.md`.
Firmware vigente de la placa fabricada: `firmware/PinaBiosensor_Firmware_v1_2/`.
PDF de hardware para adjuntar: `docs/PinaBio_v1.0_para_ChatGPT.pdf`.

Los archivos de fabricación están en la rama **`kicad-completa`** (`exports/PinaBio_v1_*.zip` / `.csv`). Una copia en OneDrive puede estar desactualizada.

## Seguridad (léelo)

El diseño **no** está aislado de USB. Texto completo: `docs/04_seguridad.md`.

1. **Nunca** USB a un ordenador con GSR, bandas o ECG en el cuerpo.
2. Carga y programa el firmware **sin piel**.
3. Sesión con piel = **solo BLE**.
4. Batería LiPo **con placa de protección**. JST J1: pin 1 +, pin 2 masa.
5. No diagnostica, no sobre heridas, no es producto sanitario.

## Pedir las placas (JLCPCB)

**5** placas, **2** ensambladas (PCBA). Guía: [pedido en español](docs/08_pedido_jlcpcb.md) · [English](docs/08_jlcpcb_order.md).

Sube estos tres archivos de **`kicad-completa`**:

- `exports/PinaBio_v1_gerbers.zip`
- `exports/PinaBio_v1_bom_jlc.csv`
- `exports/PinaBio_v1_cpl.csv`

**No** uses los gerbers Mini de `exports/gerbers/PinaBiosensor_Mini-...`.

## Firmware

Sketch de la PCBA pedida (Arduino, XIAO ESP32-S3):

`firmware/PinaBiosensor_Firmware_v1_2/PinaBiosensor_Firmware_v1_2.ino`

BLE: `PinaBiosensor`. Protocolo vigente: `docs/firmware_v1_2_protocol.es.md`. Instrucciones: `firmware/README.md`. Cambios: `firmware/CHANGELOG_v1_2.md`.

El JSON v4 es solo para depuración; el formato principal son los frames binarios del protocolo.

## Abrir el PCB en KiCad

Ver `docs/03_guia_kicad.md`. En corto:

1. Instala [KiCad 8](https://www.kicad.org/) **con librerías**. No crees un proyecto nuevo.
2. Abre `hardware/PinaBiosensor_Mini.kicad_pro`.
3. El esquemático es un mapa en texto. La placa fabricable es el PCB.

## Fabricación (resumen)

- JLCPCB / PCBWay, 2 capas, FR4 1,6 mm, máscara verde, HASL o ENIG.
- Fábrica SMT: 1206, SOT-23-5, SOIC-14, ADS1115 (VSSOP-10, muy pequeño).
- Tú pones: XIAO, módulos MAX/AD8232, batería, interruptor de caja en **J9**, medidor opcional en **J10**.

Lista de diseño: `bom/BOM.csv`.

## Más documentos

- `docs/02_arquitectura.md` — arquitectura
- `docs/04_seguridad.md` — seguridad (EN + ES)
- `docs/firmware_v1_2_protocol.es.md` — BLE, UUIDs y frames V1.2
- `docs/08_jlcpcb_order.md` — order (English)
- `docs/08_pedido_jlcpcb.md` — pedido (español)
- `docs/09_caja_interruptor_medidor.md` — DPDT de caja (4 hilos) y J10
