# PinaBio v1.0 (Paca) — especificación de hardware y firmware

**Documento para revisión externa (p. ej. Claude).** Describe la placa **tal como se pidió a JLCPCB** (pedido `SMT026091261919`), el firmware del repo, y una crítica de lo que está mal o a medias. No es un manual de usuario.

| Campo | Valor |
|--------|--------|
| Producto | PinaBio v1.0 (apodo **Paca**) |
| Nombre BLE (no cambiar) | `PinaBiosensor` |
| Ficheros KiCad | `hardware/PinaBiosensor_Mini.*` (el nombre Mini es histórico) |
| Congelado de pedido | `hardware/archive/PinaBio_v1.0/` y `exports/PinaBio_v1_*` |
| Firmware | `firmware/PinaBiosensor_Firmware_v1_1/PinaBiosensor_Firmware_v1_1.ino`. Protocolo: `docs/13_protocolo_firmware_v1_1.md` |
| Pedido fábrica | 5 PCB + 2 PCBA, verde, 1,6 mm, 2 capas, HASL, vias tented |
| Clasificación | Prototipo de **biofeedback / laboratorio personal**. **No es dispositivo médico.** No diagnostica. No cumple IEC 60601. |

**Supuestos de este texto (no se preguntó al usuario):** idioma español; base = **v1.0 soldada**, no el layout RF v1.1; firmware = V1.1 del repo; BOM “pedido real” = lo que JLCPCB emparejó en DFM (puede diferir 0,1 % vs 1 % en R3/R4 respecto al CSV del repo).

---

## 1. Qué hace y qué no hace

La placa lee:

1. Conductancia de piel (GSR) en un borne de 2 pines.
2. Pulso óptico (PPG) en un MAX30102 externo (pads I2C).
3. Temperatura cutánea aproximada en un MAX30205 externo.
4. ECG de juguete vía módulo AD8232 externo (pads de 6 hilos).
5. Dos bandas elásticas resistivas (tórax y abdomen).
6. Tensión de una LiPo 1S (el % lo calcula el teléfono).

El firmware **no** calcula HRV (RMSSD, SDNN, LF/HF). Detecta intervalos RR preliminares y envía PPG RAW para análisis posterior por un cliente externo.

**Sesión con piel:** solo Bluetooth. **Nunca** USB a un PC (masa de red / cargador) con electrodos, bandas o ECG puestos. Programar y cargar con la piel fuera.

---

## 2. Mecánica y stackup

| | |
|--|--|
| Contorno acabado | **110 × 70 mm** (no 80 mm) |
| Espesor | 1,6 mm, FR-4, 2 capas, 1 oz |
| Color / seda | Verde / blanca |
| Taladros de caja | H1–H4, M3, Ø 3,2 mm, centros (3,5; 3,5), (106,5; 3,5), (3,5; 66,5), (106,5; 66,5) mm |
| Panel JLCPCB | 110 × 80 mm = riel 5 mm + placa 70 mm + riel 5 mm; V-CUT a 5 mm y 75 mm. El ZIP `ok/ko` del ingeniero lo confirma. |

**Layout RF de esta v1.0 (defecto conocido):** zócalo U1 centrado en **(20, 20) mm**, USB hacia Y=0 pero **no** en el borde. Hay plano `GNDD` en F.Cu y B.Cu bajo el módulo. El ESP32-S3 de Seeed usa conector **U.FL** y cable-antena (no cerámica en el USB). Mitigación de uso: enchufar el cable de la bolsa y sacarlo de una caja de **plástico**, sin metal ni LiPo bajo el XIAO.

Existe un giro de cobre **v1.1** (U1 en Y=9,22 mm, USB al borde, keepout bajo el USB). **No** es este pedido. Ese keepout **tampoco** es la antena U.FL (queda hacia `BAT+`). Ver §16.

---

## 3. Árbol de alimentación

```
LiPo 1S + protección ──J1── V_BATT ──hilo── pad BAT+ del XIAO (U1 pad 15)
                         │
                         ├── J10 (medidor, paralelo a J1)
                         │
                         └── J9 pin2 ──[DPDT polo A, ON]── J9 pin1 LDO_VIN
                                                          │
USB-C del XIAO ── cargador interno del módulo ── (no cortar con el DPDT)
                                                          │
                                                    U2 ADP150AUJZ-3.3
                                                    VIN=EN=LDO_VIN
                                                    VOUT=LDO_OUT ──FB1── V_ANALOG
                                                          │
XIAO 3V3 ──────────────────────────────────────────── +3V3  (digital + I2C + ADS + MAX)
XIAO 5V  ──────────────────────────────────────────── +5V   (J2, opcional)
XIAO GND ──────────────────────────────────────────── GNDD ──NT1── GNDA
```

| Red | Origen | Qué alimenta |
|-----|--------|----------------|
| `V_BATT` | J1 pin1 | Carga del XIAO (hilo BAT+), J10, polo A del DPDT |
| `LDO_VIN` | J9 pin1 | VIN y EN del ADP150 |
| `V_ANALOG` | LDO + FB1 | MCP6004 V+, GSR/bandas 0,5 V, AD8232 VCC y SDN |
| `+3V3` | regulador del XIAO | ADS1115, MAX30102, MAX30205, pull-up I2C |
| `+5V` | pin 5V del XIAO | J2 (carga auxiliar; **no** es la vía LiPo) |
| `GNDD` / `GNDA` | un net-tie NT1 | digital vs analógico; un solo punto |

**Por qué ADS/MAX van a `+3V3` y no a `V_ANALOG`:** si se apaga el LDO con el XIAO vivo (USB), los pull-up I2C a 3,3 V **inyectarían** corriente en chips apagados por los diodos ESD de SDA/SCL (back-power). Decisión correcta. Efecto secundario: PPG y temperatura **siguen vivos** con “analógico off” si el XIAO tiene USB o batería.

**ADP150:** TSOT-23-5, pata 4 **NC** (no BYP, no 1 nF). C1~1 µF en VIN, C2~1 µF en VOUT, luego FB1 (600 Ω @ 100 MHz) y C4 10 µF hacia `V_ANALOG`. EN atado a VIN: el LDO vive solo si hay `LDO_VIN`.

**DPDT de caja (J9)** — 4 hilos, **no** debe cortar la carga:

| J9 | Seda | Net | Polo |
|----|------|-----|------|
| 1 | ANALOG | `LDO_VIN` | A — a BAT en ON |
| 2 | BAT | `V_BATT` | A |
| 3 | SLEEP | `SLEEP_N` | B — a GND en OFF |
| 4 | GND | `GNDD` | B |

OFF: polo B cierra `SLEEP_N` a masa → el firmware entra en deep sleep. ON: `SLEEP_N` flota (pull-up interno GPIO4). **Riesgo de uso:** un DPDT mal cableado puede dejar el LDO siempre on, o cortar BAT+ del XIAO (entonces no carga). No hay fusible ni MOSFET de protección de polaridad en `V_BATT`.

---

## 4. Microcontrolador (U1)

Seeed **XIAO ESP32-S3** en **zócalo**, no soldado por JLCPCB.

- Zócalo: **2 × tira hembra 1×7**, paso **2,54 mm**, distancia entre filas **17,78 mm**. Partir tiras de 40 pines. Macho en el módulo si viene desnudo.
- Pad 15 `BAT+` en la PCB: hilo corto al pad BAT+ de **debajo** del XIAO. Sin ese hilo no hay carga ni medida de batería por el camino previsto.
- Antena: **U.FL + cable** de la bolsa Seeed. Sin cable el BLE es muy flojo.

### Mapa zócalo ↔ Seeed ↔ firmware

USB hacia la seda `USB` (Y menor). Izquierda = pines 1–7; derecha = 14…8.

| Pad PCB | Seeed | GPIO (S3) | Net | Firmware |
|---------|-------|-----------|-----|----------|
| 1 | D0 / A0 | GPIO1 | `VBAT_DIV` | `A0`, `analogReadMilliVolts * 2` |
| 2 | D1 | GPIO2 | — | libre |
| 3 | D2 | GPIO3 **strapping** | — | **prohibido para sleep** |
| 4 | D3 | GPIO4 | `SLEEP_N` | `D3`, ISR FALLING, deep-sleep EXT0 nivel 1 |
| 5 | D4 SDA | GPIO5 | `SDA` | Wire |
| 6 | D5 SCL | GPIO6 | `SCL` | Wire 400 kHz |
| 7 | D6 TX | | — | libre |
| 8 | D7 RX | | — | libre |
| 9 | D8 | | `ECG_LOP` | `D8` leads-off + |
| 10 | D9 | | `ECG_LON` | `D9` leads-off − |
| 11 | D10 | | — | libre |
| 12 | 3V3 | | `+3V3` | |
| 13 | GND | | `GNDD` | |
| 14 | 5V | | `+5V` | |
| 15 | BAT+ (no es pin de tira) | | `V_BATT` | |

Divisor de batería **en PCB:** R1 47 kΩ (`V_BATT` → `VBAT_DIV`) y R2 47 kΩ (`VBAT_DIV` → `GNDD`). Relación 1/2. El comentario del firmware (“divisor 1/2 **en el XIAO**”) es **engañoso**: en esta placa el divisor es R1/R2. El S3 no replica el divisor interno del XIAO ESP32-C3. `analogReadMilliVolts(A0)*2` es coherente **solo** con ese 1/2 de placa. Calibrar con un polímetro: offset del ADC del S3 y tolerancia 1 % de R1/R2.

Pull-up I2C: **R7, R8 = 2,2 kΩ** a `+3V3`.

---

## 5. Referencia 0,50 V y MCP6004 (U3)

Desde `V_ANALOG` ≈ 3,3 V:

- R3 **56 kΩ** y R4 **10 kΩ** → `VREF_DIV` ≈ 3,3 × 10/(56+10) = **0,500 V**.
- **Intención de diseño:** R3 y R4 a **0,1 %** (CSV del repo: C4257433 / C2507497).
- **Pedido DFM real:** R3 **C18029** (56 k, serie UNI-ROYAL) y R4 **C17902** (10 k **1 %** Basic). La referencia de GSR/bandas en las placas soldadas es peor que el papel. Documentar como **desviación as-built**.

Op-amp **MCP6004** SOIC-14, V+ = `V_ANALOG`, V− = `GNDA`.

| Unidad | Patas | Función |
|--------|-------|---------|
| A | 1–2 unidos, 3 = `VREF_DIV` | Seguidor de la referencia 0,50 V → `N$1` / V_REF buffered. R5 **47 Ω** entre salida y C5 **4,7 µF**; la realimentación **antes** de R5 (si no, el MCP6004 oscila con 4,7 µF). |
| B | 5 = `TH_MID`, 6–7 unidos | Seguidor banda tórax |
| C | 10 = `AB_MID`, 8–9 unidos | Seguidor banda abdomen |
| D | 12 a `GNDA`, 13–14 unidos | Op-amp libre aparcado (no flota) |

---

## 6. Cadena GSR (J3)

```
V_REF 0,50 V ── R9 100 kΩ ── GSR_MID ── R11 1 kΩ ── GSR_ELEC ── J3 SIG
                     │                      │                    │
                     │                      └── D1 TVS 5 V ── GNDA
                     │
                     └── R10 470 Ω ── GSR_AIN ── ADS AIN1 (×8)
J3 GNDA ──────────────────────────────────────────────────── GNDA
```

Protección en electrodo: **R11 1 kΩ + D1 PESD5V0S1BA** (SOD-323, bidireccional). **R10 470 Ω** está entre el divisor y el ADC (no es el TVS).

**Energía en piel (orden de magnitud):** si J3 se cortocircuita, I ≈ 0,5 V / 100 kΩ = **5 µA**. No es aislamiento de paciente; solo limita corriente continua.

Firmware:

```
vMid = ADS1115 AIN1, ganancia ×8 (±0,512 V)
si vMid < 2 mV o vMid > 0,98×V_REF → gsr_uS = 0
rSkin = 100e3 * vMid / (0,50 - vMid)
gsr_uS = 1e6 / rSkin   (si rSkin ≥ 100 Ω)
```

Ganancia ×8: un GSR típico de pocos µS mueve milivoltios; ×1 se perdería en ruido. El ADS no es simultáneo: cada `readADC_SingleEnded` multiplexa.

---

## 7. Bandas de respiración (J6, J7)

Misma idea que GSR, serie **47 kΩ** (R12 tórax, R15 abdomen), electrodo `TH_ELEC` / `AB_ELEC`, TVS **D2 / D3**, 1 kΩ serie R14/R17, seguidores B/C, ADS **AIN2 / AIN3 ×1**.

El teléfono calibra min/max en ~20 s. Las tensiones van en JSON como `rt_v` y `ra_v` (voltios en el ADC), no un volumen pulmonar.

Sudor, gel y elástico mojado: la resistencia cae; no hay discriminación “banda rota vs piel húmeda”.

---

## 8. ECG (J8 + AD8232 de terceros)

Seis pads 2,54 mm + agujero de alivio. **Seda “3V3” = `V_ANALOG`**, no el pin 3V3 del XIAO. Así el módulo se apaga con el LDO.

| Pad | Net | Uso en el módulo típico |
|-----|-----|-------------------------|
| 1 | `V_ANALOG` | VCC |
| 2 | `GNDA` | GND |
| 3 | `ECG_OUT` | OUTPUT → R18 1 kΩ → `ECG_AIN` → ADS AIN0 ×1 |
| 4 | `ECG_LOP` | LO+ → D8 |
| 5 | `ECG_LON` | LO− → D9 |
| 6 | `V_ANALOG` | SDN (activo en alto; off con analógico off) |

Electrodos **solo** en el AD8232, no en la Paca. El JSON `ecg_mv` es **mV en el pin del ADS** (`volts * 1000`), no milivoltios fisiológicos de superficie. Cadencia: el `loop` llama `sampleEcg()` lo más rápido que multiplexa el ADS, pero el JSON sale cada **200 ms** con como máximo **8** muestras (`ECG_N`). Eso **no** es un ECG clínico (aliasing, huecos, sin filtro digital de red 50 Hz en firmware). `lo=1` si LO+ o LO− están en alto (electrodos mal / despegados), según el módulo.

**ESD en J8:** no hay TVS en la Paca para OUTPUT/LO. Se confía en el módulo barato. Insuficiente si se toca OUTPUT con el dedo a la placa.

---

## 9. ADS1115 (U4) — I2C 0x49

VSSOP-10 (IDGSR), VDD = `+3V3`, GND = `GNDA` (mezcla deliberada: ADC analógico con I2C digital).

| Pata | Función | Net |
|------|---------|-----|
| 1 ADDR | a VDD → **0x49** | `+3V3` (MAX30205 es 0x48; no chocan) |
| 2 ALERT | sin usar en firmware | `ADS_ALERT` |
| 3 GND | | `GNDA` |
| 4 AIN0 | ECG ×1 | `ECG_AIN` |
| 5 AIN1 | GSR ×8 | `GSR_AIN` |
| 6 AIN2 | tórax ×1 | `TH_AIN` |
| 7 AIN3 | abdomen ×1 | `AB_AIN` |
| 8 VDD | | `+3V3` |
| 9 SDA / 10 SCL | | bus |

Datasheet TI: ADDR a VDD = 1001001b = 0x49. Firmware `ADDR_ADS = 0x49`, 860 SPS, gana ×8 solo el instante de GSR y vuelve a ×1.

`ADS_ALERT` no va a un GPIO: oportunidad perdida para DRDY (muestreo más limpio).

---

## 10. PPG y temperatura (J4, J5)

Pads 4P: `+3V3`, `GNDD`, `SDA`, `SCL`. Cables soldados, no bornes. C10/C11 100 nF locales.

| Módulo | Dirección | Notas |
|--------|-----------|--------|
| MAX30102 | 0x57 | LED IR/rojo; HRV **no** en placa. Umbral dedo: IR ≥ 20 000 cuentas. |
| MAX30205 | 0x48 | Registro 0x00, °C = raw × 0,00390625 |

Cables I2C largos + 400 kHz + 2,2 kΩ: margen pobre (capacidad, diafonía con LED del MAX30102). No hay series 22–100 Ω ni TVS en SDA/SCL de J4/J5.

---

## 11. Conectores restantes y polaridad

| Ref | Pieza as-built | Pines |
|-----|----------------|-------|
| J1 | JST-PH 2P C173752 | 1 `V_BATT`, 2 `GNDD` |
| J2 | igual | 1 `+5V`, 2 `GNDD` |
| J10 | igual | paralelo J1 |
| J3/J6/J7 | KF128R-5.08-2P C475082 (DFM) | SIG / GNDA |
| J9 | JST-PH 4P C157926 | §3 |

U1 **no** está en el DFM (45 piezas, todas top). THT = `manualWeld` (ola/mano). SMD = `smtWeld`.

**Polaridad chips (ya revisada en visor 2D JLCPCB):** triángulo de seda = pata 1 de U2/U3/U4 alineada con el marcador morado de fábrica. CPL cliente U2/U3=180°, U4=270°; el ingeniero guarda otros ángulos internos de máquina. **La verdad de pata 1 es seda + 2D**, no el 3D morado.

D1–D3 son TVS **bidireccionales**: el sentido del cuerpo importa poco.

---

## 12. ESD, transitorios y lo que *no* hay

**Hay (v1.0):**

- TVS 5 V bidir en GSR, tórax, abdomen (D1–D3) + 1 kΩ serie.
- 1 kΩ entre ECG_OUT y ADS (R18): no es ESD de electrodo.
- Planos GND; vias tented.

**No hay:**

- Aislamiento galvánico USB ↔ piel (el fallo de seguridad **número 1**).
- TVS / resistencias serie en J4/J5 (I2C), J8 (salvo R18), J9 `SLEEP_N`, J1 `V_BATT`.
- CMC ni filtro de red en USB (el USB ni siquiera sale a la Paca; está en el XIAO).
- Fusible PTC en batería (se delega en la PCB de la LiPo).
- Protección de polaridad J1.
- Descarga de cinta / carcasa a tierra de chasis (caja de plástico prevista).
- IEC 61000-4-2 ensayado. Los TVS SOD-323 PESD5V0S1BA ayudan a contactos en bornes; **no** convierten esto en equipo de paciente.

GNDA y GNDD unidos en NT1: un ESD en J3 recorre el net-tie hacia el XIAO. Mejor que un único plano sucio, peor que un isolator.

---

## 13. Firmware vigente — V1.1 para la PCB fabricada

El sketch vigente es `firmware/PinaBiosensor_Firmware_v1_1/PinaBiosensor_Firmware_v1_1.ino`. El formato de frames, comandos y UUID se define en `docs/13_protocolo_firmware_v1_1.md`; las instrucciones y límites de verificación están en `firmware/README.md`. La adquisición utiliza un único task para ADS1115, otro para PPG, otro para telemetría lenta y uno para comunicaciones. No se usa hardware V2.

El firmware conserva `PinaBiosensor`, ECG y PPG RAW, telemetría, eventos y HRS. JSON v4 es solo depuración.

---

## 14. Seguridad de uso (normativa: ninguna)

Ya dicho: no sanitario. Añadir para el revisor:

- Corriente GSR limitada por 100 kΩ, **pero** un fallo del ADP150 a 3,3 V en el borne (R9 abierta, soldadura puente) podría aplicar ~3,3 V a piel a través de 1 kΩ → miliamperios. No hay detección de fallo de V_REF.
- AD8232 de AliExpress: calidad de aislamiento **desconocida**.
- LiPo: solo “con PCB de protección” en la celda. JST pin1 = +.
- Olor a quemado: USB y batería fuera; no basura doméstica si hay celda.

---

## 15. As-built vs intención (fábrica)

| Ítem | Papel / CSV repo | DFM `SMT026091261919` |
|------|------------------|------------------------|
| U1 zócalo | 2×1×7 | **No colocado** |
| U2/U3/U4 | C29149 / C1346056 / C37593 | Igual |
| R3 56 k 0,1 % | C4257433 | **C18029** (56 k, no 0,1 % Vishay) |
| R4 10 k 0,1 % | C2507497 | **C17902** (10 k **1 %**) |
| J3/J6/J7 | emparejar | **C475082** KF128 5,08 mm |
| Piezas | | 45, solo top |

Producción PCB (`ok/ko`): contorno útil **110 × 70 mm**. Confirmar pata 1 U2/U3/U4 en 2D (hecho: morado = triángulo).

---

## 16. Crítica y mejoras (prioridad)

Esto es lo que un revisor debe atacar. Varios fallos son **nuestros**, no del usuario.

### P0 — seguridad y “no mientas con la señal”

1. **USB y piel comparten GND.** Sin isolator USB, sin BLE-only lockout por hardware. Mejora: opto/isolator USB **o** (más barato y honesto) un **interlock**: MOSFET que corte `V_ANALOG` y las líneas de electrodo si VBUS del XIAO está presente; o documentar en seda enorme “NO USB + PIEL”. El firmware no detecta VBUS.

2. **ESD incompleta.** TVS solo en tres bornes de goma/piel. Faltan: I2C J4/J5 (TVS 3,3 V + 22–47 Ω serie), `SLEEP_N`, `ECG_OUT`/`LO±` en J8 o en el propio AD8232, TVS de 5,5–6 V en `V_BATT`. Clasificación 61000-4-2 contacto 8 kV como **objetivo de prototipo**, no de certificado.

3. **ECG y GSR en el mismo ADS multiplexado** + JSON 5 Hz + 8 muestras. O un segundo ADC / AFE para ECG, o quitar el ECG de esta placa y dejar el AD8232 en un canal dedicado (SPI AFE). Mientras tanto, describir el ECG como “forma de onda de laboratorio”.

4. **Antena v1.0.** Cobre bajo el XIAO. Uso: cable U.FL fuera. Próximo PCB: USB en un borde **y** U.FL/antena en un recorte **sin cobre** en todas las capas (el v1.1 actual solo vacía bajo el USB: **insuficiente**).

### P1 — analogía y energía

5. **R3/R4 0,1 %** como en el CSV, o medir y guardar ganancia GSR en NVS. As-built 1 % en R4 tuerce el 0,50 V.

6. **Inyección de carga del mux ADS** al cambiar ×8/×1. Leer GSR dos veces y tirar la primera; o dos ADS; o PGA externo fijo.

7. **I2C:** 100 kHz por defecto con cables >10 cm; series 33 Ω; pull-up 4,7 k si la capacitancia es baja, o buffer. Conector JST-SH 4P en vez de pads desnudos (strain relief ya hay agujero).

8. **Fusible / p-FET** en `V_BATT` (polaridad y cortocircuito del arnés J9).

9. **SLEEP y LDO en el mismo DPDT:** un error de cable deja analógico on y MCU dormido, o al revés. Mejor: un solo polo “sistema” + load switch con slew, y `SLEEP_N` generado en placa.

10. **MAX30102 y MAX30205 siguen on** con USB. Gate con MOSFET desde `V_ANALOG` o GPIO si se quiere “off de verdad”.

11. **`ADS_ALERT` a un GPIO** (no strapping) para muestreo por DRDY.

12. **Filtro RC / notch 50 Hz** en firmware para bandas y GSR (media móvil mínima). Watchdog I2C.

13. **Medida de batería:** calibrar; no fiarse del comentario “divisor en el XIAO”. Opcional ADC del XIAO BAT vs solo R1/R2: no duplicar.

### P2 — layout, DFMA, firmware de producto

14. **Seda J8 “3V3”** → `VANA` / `ANALOG 3V3` para no cablear el 3V3 digital al AD8232.

15. **Keepout U.FL** y silkscreen “ANTENA CABLE AQUÍ / NO METAL”.

16. **Pines GPIO libres** (D1, D2, D6, D7, D10): pull-down en HW o `INPUT_PULLDOWN` en FW (ahora flotan).

17. **Bonding BLE / MAC fijo** si se usa fuera de la mesa. JSON con `seq` y checksum.

18. **CPL vs visor 3D:** el 3D de JLCPCB **no** certifica pata 1. Procedimiento: 2D + triángulo. No repetir la caza de 180°.

19. **v1.1 RF ya generado** no debe usarse como “antena fuera” sin mover el extremo BAT+/U.FL al borde o recortar cobre ahí.

20. **Documentación cruzada:** `docs/01_revision.md` llama “v1.1” a cambios **eléctricos** que ya están en la v1.0 pedida. El layout v1.1 es otra cosa. Renombrar en una pasada de docs para no envenenar al siguiente LLM.

### Lo que sí está bien (no tirar)

- Sleep en **GPIO4**, no GPIO3.
- I2C sensors en `+3V3` (anti back-power).
- Snubber R5+C5 en V_REF.
- ADDR ADS a VDD (0x49) vs temp 0x48.
- TVS en los tres bornes de piel/goma (aunque no baste).
- DPDT que **no** corta carga si se cablea como §3.
- Protocolo binario V1.1 + 0x180D.
- Contorno 110×70 real en Gerber de producción.

---

## 17. Fuera de este documento

No incluye:

- Pasos KiCad para Grok (mover XIAO, keepout U.FL, re-anclar conectores). Eso es el **siguiente** encargo, con esta spec como contrato.
- Esquemático dibujado (está en `hardware/archive/PinaBio_v1.0/PinaBiosensor_Mini.kicad_sch`).
- Los clientes BLE externos deberán interpretar el protocolo V1.1.

---

## 18. Trazabilidad para el revisor

| Afirmación | Dónde comprobarla |
|------------|-------------------|
| Pines U2/U3/U4 / XIAO | PCB archive pads vs datasheet Analog / Microchip / TI / Seeed wiki |
| Contorno 110×70 | `exports/PinaBio_v1_gerbers.zip` Edge_Cuts; ZIP producción `ok/ko` |
| Firmware | `.ino` citado; no hay otra rama de FW en este texto |
| DFM 45 piezas | pedido SMT026091261919 / visor share JLCPCB |
| Seguridad USB | `docs/04_seguridad.md` |

**Autor de esta spec:** el mismo agente que participó en el layout v1.0, el CPL, el pedido y el firmware. Hay sesgo. Los P0–P2 existen para que un segundo modelo **no** acepte el diseño por inercia.
