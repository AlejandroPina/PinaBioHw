# Prompt para ChatGPT — revisión PinaBio v1.0 (Paca)

Copia **desde la línea “INICIO DEL PROMPT”** hasta **“FIN DEL PROMPT”** y pégalo en un chat nuevo de ChatGPT. Si el modelo recorta por longitud, pega primero el bloque A (encargo + restricciones) y luego B, C, D en mensajes siguientes, diciendo “esto es continuación del mismo encargo”.

---

INICIO DEL PROMPT

## A. Quién eres y qué te pido

Eres un revisor senior de electrónica analógica de bajo ruido, KiCad, ESP32 y BLE, y de prototipos de biofeedback **no médicos**. No eres un comercial. No endulces.

Voy a darte el diseño **congelado** de la placa **PinaBio v1.0** (apodo **Paca**) **ya pedida a JLCPCB**, el firmware que la acompaña, y dos revisiones externas (Gemini y Claude) más el análisis posterior de otro ingeniero. Quiero que:

1. Valides o rebatan, **punto por punto**, si lo fabricado + el firmware **puede funcionar** como prototipo de laboratorio personal (GSR + PPG/HRV en el móvil + temp + dos bandas resistivas + ECG de juguete).
2. Separes **fallos de cobre** (solo se arreglan con otra PCB o un hilo) de **fallos de firmware/app** de **falsos positivos de los revisores**.
3. Digas qué merece **Rev 2 de PCB** y qué no. El pedido actual **no se cancela**.
4. Propongas, si procede, un **protocolo BLE binario** (además o en lugar del JSON v3) y un **apaño de ECG** en la placa ya encargada (AD8232 → ADC del XIAO).
5. Señales riesgos de **seguridad eléctrica** (USB + piel) con claridad, sin convertirlo en un IEC 60601.

Responde en **español claro**. No pidas gerbers. Trabaja solo con este texto. Si algo no se puede saber sin lab, dilo.

**Restricciones del producto (no las cambies):**

- No es dispositivo médico. No diagnostica. No IEC 60601.
- Nombre BLE **exacto**: `PinaBiosensor`. Paquete Android: `com.pinabiosensor.mini`.
- MCU: **Seeed XIAO ESP32-S3** en zócalo (fábrica no lo suelda).
- Pedido fábrica **SMT026091261919**: 5 PCB + 2 PCBA, verde, 1,6 mm, FR-4, 2 capas, HASL, vias tented, contorno útil **110 × 70 mm**.
- No sustituyas el ADP150AUJZ-3.3 por otro LDO en el análisis de *esta* revisión.
- Sleep de caja: **D3 / GPIO4**, nunca GPIO3 (strapping).
- I2C de MAX30102 / MAX30205 / ADS1115 alimentados desde **`+3V3` del XIAO**, no desde `V_ANALOG` (anti back-power por diodos ESD de SDA/SCL).
- HRV (RMSSD, etc.) lo calcula el **teléfono**. La placa solo detecta IBI toscos (PPG) y los manda.

---

## B. Diseño de hardware enviado a fabricar

### B.1 Qué mide

1. GSR (conductancia de piel) en borne 2 pines J3.
2. PPG / pulso: módulo **MAX30102** externo, pads I2C J4, dirección **0x57**.
3. Temperatura cutánea aproximada: **MAX30205** en J5, **0x48**.
4. ECG de laboratorio: módulo **AD8232** externo (SparkFun o clones), pads 6P J8.
5. Dos bandas elásticas resistivas: tórax J6, abdomen J7.
6. Tensión LiPo 1S (el % lo hace el teléfono).

Sesión con piel: **solo BLE**. USB-C del XIAO solo para programar/cargar **con electrodos fuera**.

### B.2 Mecánica y RF as-built

- Placa 110 × 70 mm (el panel JLCPCB es 110 × 80 = riel 5 + 70 + riel 5).
- 2 capas, 1 oz, taladros M3 H1–H4.
- Zócalo XIAO U1 centrado en **(20, 20) mm**, USB hacia Y=0 **pero no en el borde**. Hay cobre `GNDD` bajo el módulo.
- La antena del XIAO ESP32-S3 Seeed es **U.FL + cable de la bolsa**, no cerámica en el USB. Sin cable el BLE es flojo. Caja prevista de plástico, sin metal ni LiPo bajo el XIAO.
- Existe un layout RF “v1.1” (XIAO más al borde). **No es este pedido.**

### B.3 Árbol de alimentación

```
LiPo 1S con PCB de protección ──J1── V_BATT ──hilo── pad BAT+ del XIAO (U1 pad 15)
                                 │
                                 ├── J10 medidor (paralelo a J1)
                                 │
                                 └── J9 pin2 ──[DPDT polo A, ON]── J9 pin1 LDO_VIN
                                                                    │
USB-C XIAO ── cargador del módulo (el DPDT NO debe cortar esto)
                                                                    │
                                                              U2 ADP150AUJZ-3.3
                                                              VIN=EN=LDO_VIN
                                                              pin4 = NC (TSOT-23-5; NO es BYP)
                                                              VOUT=LDO_OUT ──FB1 600 Ω@100 MHz── V_ANALOG
                                                              C1 1 µF VIN, C2 1 µF VOUT, C4 10 µF en V_ANALOG

XIAO 3V3 ── +3V3  → ADS1115, MAX30102, MAX30205, pull-ups I2C R7/R8
XIAO 5V  ── +5V   → J2 opcional (NO es la vía LiPo)
XIAO GND ── GNDD ── net-tie NT1 ── GNDA
```

**DPDT de caja (J9, JST-PH 4P).** Fábrica monta el conector, NO un slide en PCB.

| J9 | Seda | Net | Polo |
|----|------|-----|------|
| 1 | ANALOG | LDO_VIN | A — a BAT en ON |
| 2 | BAT | V_BATT | A |
| 3 | SLEEP | SLEEP_N | B — a GND en OFF |
| 4 | GND | GNDD | B |

OFF: `SLEEP_N` a masa → firmware deep sleep. ON: `SLEEP_N` flota, pull-up GPIO4. Riesgo: DPDT mal cableado corta carga o deja LDO siempre on. No hay fusible ni p-FET de polaridad en `V_BATT`.

Efecto buscado: el DPDT apaga `V_ANALOG` (MCP6004, GSR, bandas, AD8232). **No apaga** ADS/MAX (`+3V3`).

### B.4 MCU y zócalo (U1)

Seeed XIAO ESP32-S3. 2 × tira hembra 1×7, paso 2,54 mm, filas a 17,78 mm. Pad 15 BAT+ requiere **hilo** al pad BAT+ de debajo del módulo. Sin ese hilo no hay carga ni el camino de batería previsto.

USB hacia seda `USB` (Y menor). Izquierda pads 1–7; derecha 14…8.

| Pad | Seeed | GPIO | Net en v1.0 | Firmware actual |
|-----|-------|------|-------------|-----------------|
| 1 | D0/A0 | GPIO1 | VBAT_DIV | analogReadMilliVolts(A0)*2 |
| 2 | D1 | GPIO2 | **libre** | — |
| 3 | D2 | GPIO3 | libre | **no usar para sleep** (strapping) |
| 4 | D3 | GPIO4 | SLEEP_N | ISR FALLING; EXT0 wake nivel 1 |
| 5 | D4 | GPIO5 | SDA | Wire |
| 6 | D5 | GPIO6 | SCL | 400 kHz |
| 7 | D6 | GPIO43 | libre | — |
| 8 | D7 | GPIO44 | libre | — |
| 9 | D8 | GPIO7 | ECG_LOP | leads-off + |
| 10 | D9 | GPIO8 | ECG_LON | leads-off − |
| 11 | D10 | GPIO9 | **libre** | — |
| 12 | 3V3 | | +3V3 | |
| 13 | GND | | GNDD | |
| 14 | 5V | | +5V | |
| 15 | BAT+ | | V_BATT | hilo |

Divisor batería **en PCB**: R1=R2=**47 kΩ** 1 % (V_BATT → VBAT_DIV → GNDD), relación 1/2. El comentario del firmware (“divisor 1/2 en el XIAO”) es **falso** para el S3. Calibrar con polímetro.

Pull-ups I2C **R7, R8 = 2,2 kΩ a +3V3** (están en placa; no se depende de los módulos).

### B.5 Referencia 0,50 V y MCP6004 (U3, SOIC-14, C1346056)

Desde V_ANALOG ≈ 3,3 V: R3 **56 kΩ** + R4 **10 kΩ** → VREF_DIV ≈ **0,500 V**.

- Intención: R3/R4 **0,1 %**.
- **As-built DFM:** R3 C18029 (56 k, no 0,1 % Vishay), R4 **C17902 10 k 1 % Basic**. La referencia en las placas soldadas es peor que el CSV del repo.

MCP6004: V+ = V_ANALOG, V− = GNDA.

| Unidad | Patas | Función |
|--------|-------|---------|
| A | 1–2 unidos, 3 = VREF_DIV | Seguidor V_REF. R5 **47 Ω** + C5 **4,7 µF**; realimentación **antes** de R5 (evita oscilar). |
| B | 5 = TH_MID, 6–7 unidos | Seguidor tórax |
| C | 10 = AB_MID, 8–9 unidos | Seguidor abdomen |
| D | 12 a GNDA, 13–14 unidos | Aparcado, no flota |

En el **PCB** estas redes existen por separado (N$1…N$4). Ver más abajo el problema del *fichero* esquemático.

### B.6 GSR (J3)

```
V_REF 0,50 V ── R9 100 kΩ ── GSR_MID ── R11 1 kΩ ── GSR_ELEC ── J3 SIG
                     │                      │
                     │                      └── D1 PESD5V0S1BA ── GNDA
                     └── R10 470 Ω ── GSR_AIN ── ADS AIN1, ganancia ×8 (±0,512 V)
J3 GNDA ──────────────────────────────── GNDA
```

Si J3 se cortocircuita: I ≈ 0,5 V / 100 kΩ = **5 µA**. No es aislamiento de paciente.

Fórmula firmware (V_REF fijo 0,50; **no resta R11**):

```
si vMid < 2 mV o vMid > 0,98*V_REF → gsr_uS = 0
rSkin = 100e3 * vMid / (0,50 - vMid)
gsr_uS = 1e6 / rSkin   (si rSkin ≥ 100 Ω)
```

### B.7 Bandas (J6, J7)

Igual idea, R serie **47 kΩ** (R12 tórax, R15 abdomen), TVS D2/D3, 1 kΩ serie, seguidores B/C, ADS **AIN2 / AIN3 a ganancia ×1**. El móvil calibra min/max ~20 s. JSON: `rt_v`, `ra_v` en voltios ADC, no litros.

### B.8 ECG (J8 + AD8232)

Seis pads + agujero de alivio. Seda “3V3” en J8 es **`V_ANALOG`**, no el 3V3 del XIAO (el AD8232 se apaga con el LDO).

| Pad J8 | Net | Uso típico módulo |
|--------|-----|-------------------|
| 1 | V_ANALOG | VCC |
| 2 | GNDA | GND |
| 3 | ECG_OUT | OUTPUT → R18 1 kΩ → ECG_AIN → **ADS AIN0 ×1** |
| 4 | ECG_LOP | LO+ → D8 |
| 5 | ECG_LON | LO− → D9 |
| 6 | V_ANALOG | SDN (activo en alto) |

Electrodos solo en el AD8232. `ecg_mv` = voltios ADS × 1000 (mV en el pin), no mV fisiológicos. **No hay TVS** en J8 (solo R18).

ADS1115 máximo **860 SPS**. El firmware **no** lo usa así para ECG: buffer `ECG_N=8` por JSON cada 200 ms → **≤40 Hz efectivos** + mux con GSR/bandas. El silicio no es el cuello; el protocolo y el mux sí.

Pines libres del zócalo **D1 (GPIO2, ADC1)** y **D10 (GPIO9, ADC1)** permiten un **hilo** ECG_AIN → D1 y leer con `analogRead` del S3, dejando el ADS para GSR/bandas.

### B.9 ADS1115 (U4) — I2C 0x49

Pieza **ADS1115IDGSR**, VSSOP-10, LCSC C37593. VDD=+3V3, GND=GNDA.

Datasheet TI **SBAS444E (dic 2024)**, paquetes RUG/DYN/**DGS unificados**:

| Pin | Función | Net en el PCB pedido |
|-----|---------|----------------------|
| 1 ADDR | a VDD → 0x49 | +3V3 |
| 2 ALERT | sin GPIO en FW | ADS_ALERT (red nombrada, no a XIAO) |
| 3 GND | | GNDA |
| 4 AIN0 | ECG ×1 | ECG_AIN |
| 5 AIN1 | GSR ×8 | GSR_AIN |
| 6 AIN2 | tórax ×1 | TH_AIN |
| 7 AIN3 | abdomen ×1 | AB_AIN |
| 8 VDD | | +3V3 |
| 9 SDA | | SDA |
| 10 SCL | | SCL |

Esto se comprobó en el `.kicad_pcb` (pads de U4), no solo en el sch. MAX30205 en 0x48 no choca.

### B.10 I2C módulos (J4, J5)

Pads 4P: +3V3, GNDD, SDA, SCL. C10/C11 100 nF. Cables soldados. Sin TVS ni 22–47 Ω serie. 400 kHz + cables largos = margen pobre.

### B.11 Conectores y ESD

| Ref | Pieza | Notas |
|-----|-------|-------|
| J1 J2 J10 | JST-PH 2P C173752 | J1 LiPo; J2 +5V; J10 medidor |
| J9 | JST-PH 4P C157926 | DPDT caja |
| J3 J6 J7 | KF128R-5.08 C475082 (DFM) | piel / goma |
| J4 J5 J8 | solo pads | DNP fábrica |

ESD **hay**: D1 D2 D3 **PESD5V0S1BA** SOD-323 bidir LCSC **C19224** (BOM de pedido). El *símbolo* KiCad a veces dice PESD5V0S1UL (otro encapsulado); **fábrica es BA**.

ESD **no hay**: I2C, SLEEP_N, V_BATT, J8 OUTPUT/LO, aislamiento USB↔piel. GNDA y GNDD unidos en NT1.

U1 no está en DFM (45 piezas top). Polaridad U2/U3/U4: triángulo de seda = pin 1; visor 3D JLCPCB no se usa como verdad.

### B.12 BOM chips clave

- U2 ADP150AUJZ-3.3 TSOT-23-5 C29149; pin 4 NC.
- U3 MCP6004-I/SL SOIC-14 C1346056.
- U4 ADS1115IDGSR C37593.
- FB1 600 Ω @ 100 MHz 0805.

### B.13 Advertencia sobre el fichero `.kicad_sch`

El esquemático del repo se generó **sin wires**, solo etiquetas globales. Claude reconstruyó conectividad por geometría y vio Y invertida (símbolo Y+ arriba vs hoja Y+ abajo), 23 etiquetas que no tocan pin, MCP6004 **una sola unidad** en el sch con etiquetas apiladas.

**Importante:** eso describe el **fichero de dibujo**. El **PCB pedido** tiene nets nombradas y pads de U2/U3/U4/XIAO coherentes con esta spec y con los datasheets actuales. Un ERC de KiCad sobre el `.kicad_sch` puede ser rojo y el cobre ser correcto. Criterio: netlist/PCB, no fiarse del sch hasta rehacerlo.

---

## C. Firmware enviado (el que corre con DEMO_BLE=0)

Arduino-ESP32, un solo `.ino`. Librerías: Wire, Adafruit ADS1X15, SparkFun MAX30105 + heartRate.h, BLE Espressif, rtc_io.

### C.1 Constantes

- JSON cada **200 ms**. BLE name `PinaBiosensor`. MTU pedido 247.
- Servicio `6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90`, característica JSON `6b1d0002-…`.
- Heart Rate 0x180D / 0x2A37 flags 0x10 (uint8 HR + RR en 1/1024 s). 0x2A38 = 3 (finger).
- V_REF=0.50, R_SERIES=100e3, IR_FINGER_MIN=20000, IBI 300–1500 ms, ECG_N=8, RR_Q=8.
- ok bits: 1 ADS, 2 PPG, 4 TEMP, 8 FINGER, 16 SHUTDOWN.

### C.2 Loop (resumen fiel al código)

```
setup: D3 pullup, D8/D9 input, ADC 12 bit, I2C 400 kHz
  ads.begin(0x49), GAIN_EIGHT, 860 SPS
  ppg 0x57: 100 sps, IR 0x1F, red 0x0A
  ping 0x48
  ISR D3 FALLING
  BLE JSON + 0x180D

loop:
  processIr(ppg.getIR())  // checkForBeat; notify Polar si IBI válido
  sampleEcg(): ads GAIN_ONE, readADC_SingleEnded(0), buffer máx 8
  si SLEEP_N low > 40 ms: JSON con SHUTDOWN, BLE off, EXT0 GPIO4=1, deep sleep
     **no** hace shutdown del MAX30102 ni MAX30205
  cada 200 ms:
    Vbat = analogReadMilliVolts(A0)/1000*2
    GAIN_EIGHT, AIN1 → gsr
    GAIN_ONE, AIN2, AIN3 → rt_v, ra_v
    MAX30205
    notify JSON + Serial.println
```

`readADC_SingleEnded` es bloqueante. Mezclar ×8/×1 y canales inyecta carga en el mux (no medido en lab).

### C.3 JSON v3 (ejemplo)

```json
{"v":3,"ms":12345,"gsr_uS":8.42,"t_c":33.16,"hr":72,"rr_ms":[833],"ir":87421,"batt_v":3.87,"ok":15,"lo":0,"rt_v":0.182,"ra_v":0.165,"ecg_mv":[1650,1662,1640]}
```

Sin CRC, sin seq, sin bonding. Si el móvil no negocia MTU, el JSON ~300 B puede cortarse (la app tiene fallback). Al disconnect: `startAdvertising()`.

`DEMO_BLE=1`: mismo BLE/JSON con senos falsos, sin I2C (para Android sin PCB). En placa real: 0.

No hay watchdog. Un cuelgue I2C deja de notificar.

---

## D. Revisión Gemini (4 puntos) y contra-análisis

Gemini pidió **cambios de esquemático** como si fuera una Rev 2 genérica. No había abierto esta BOM con rigor.

**G1. ADP150 pin 4 BYP + cap 10–100 nF.**  
Pieza real: **ADP150AUJZ-3.3**. Pin 4 es **NC**, no BYP (BYP es otras cápsulas). El sch ya dice NC sin C3. **No añadir C3.**

**G2. MOSFET que corte el divisor VBAT_DIV en sleep.**  
47k+47k ≈ **35–45 µA**. Idea válida, impacto pequeño frente a MAX vivos en +3V3. P2, no P0. GPIO debe quedar en OFF durante deep sleep.

**G3. TVS USBLC6-2SC6 en J9, J10 e I2C.**  
Hueco ESD real en I2C/SLEEP_N/V_BATT. **Pieza incorrecta** (array USB). D1–D3 ya cubren piel. J9/J10 van dentro de caja. En Rev 2: TVS 3V3 baja C + series en I2C, no USBLC6 en batería.

**G4. “Faltan pull-ups I2C 2.2–4.7 k.”**  
**Falso.** R7/R8 = 2,2 kΩ en placa. Añadir más empeora consumo. Sleep: el problema es +3V3 vivo, no el sizing.

---

## E. Revisión Claude y contra-análisis

Claude coincidió con la autocrítica de la spec (USB+piel, ECG de juguete, MAX on en sleep, mux ADS). Añadió:

**E1. Bloqueante: esquemático KiCad “roto”** (sin wires, Y invertida, MCP6004 una unidad, etiquetas que caerían en AIN equivocado: tórax en AIN0, abdomen huérfano, VREF/TH/AB/GNDA fusionados).  
**Parcialmente cierto:** el `.kicad_sch` es un mal dibujo. **Falso si se aplica al PCB pedido:** pads U4 AIN0=ECG_AIN, AIN1=GSR_AIN, AIN2=TH_AIN, AIN3=AB_AIN; MCP6004 seguidores en pads 1–14 coherentes; LDO VIN/GND/EN/NC/VOUT coherentes; XIAO D0=VBAT_DIV, D3=SLEEP, D4/D5=I2C, D8/D9=LO, 3V3/GND/5V/BAT+. Pedir ERC y 5 redes con polímetro al llegar (ADS pin 8 = 3,3 V, pin 3 = GND, pin 10 = SCL).

**E2. Shutdown MAX30102/MAX30205 antes de `esp_deep_sleep_start`.** Cierto. Firmware. Medir µA en serie con la pila. Suelo teórico citado por Claude: ~14 µA MCU + ~45 µA divisor + pull-up GPIO4; **si los MAX no se duermen, ~mA**.

**E3. Sacar ECG del ADS; ≥250 Hz; BLE binario; notch 50 Hz en el móvil.** Cierto que 8 muestras/200 ms no resuelven QRS. El ADS **sí llega a 860 SPS** si se dedica tiempo. JSON texto no aguanta el caudal. Soluciones en **esta** placa: (a) firmware: muestrear AIN0 en bucle y otra característica BLE; (b) hilo ECG_AIN→D1. Segundo ADS 0x4A: solo Rev 2.

**E4. GSR y bandas todo ×8; restar R11 1 kΩ; promedio 4–8 lecturas; tirar la primera tras cambio de canal.** Cierto, casi todo firmware. ×8 en bandas: fondo ±0,512 V cubre los 0,5 V de V_REF.

**E5. 100 nF en VBAT_DIV; opcional 220k/220k.** El cap ayuda al ADC S3 (ahora ve ~23,5 kΩ). 220k ahorra poco. Calibrar igual.

**E6. Detectar VBUS** con divisor 100k/100k a D6/D7 desde el pin 5V del XIAO (ya está en placa como +5V). Firmware: USB presente → no medir piel / bit en `ok`. Aislador USB y MOSFET en EN del LDO: Claude dice que no compensan ahora porque GNDA=GNDD; aislamiento de verdad solo si hay terceros. Coherente.

**E7. R3/R4 0,1 % no merecen respin.** Medir V_REF y guardar en NVS. De acuerdo.

**E8. Rev 2: buffer GSR con opamp D; ADS_ALERT a GPIO; keepout antena.** Razonable.

**E9. TVS UL vs BA; nombres Mini/Completa/Paca.** BOM de fábrica = BA. Cosmético.

---

## F. Decisiones del dueño del proyecto (contexto, no las ignores)

- Pedido **en fabricación**; no cambiar Gerbers de este SMT.
- Interés en **trama BLE no texto** interpretada por el móvil, al menos para ECG.
- Interés en **conectar el AD8232 al ADC del XIAO** (D1) y no usar el ADS para ECG.
- Quiere saber si v1.0 **funcionará de verdad** al llegar, y qué meter en una Rev 2 si la hay.

---

## G. Preguntas concretas que debes responder

1. ¿El cobre de v1.0 (PCB, no el sch) es eléctricamente coherente para GSR, bandas, LDO, I2C, DPDT, ADS pinout DGS 2024?
2. ¿El firmware actual hará que GSR/PPG/temp/bandas “se vean” en Android con JSON v3, con qué matices (calibración, ×1 en bandas, R11, mux)?
3. ECG: ¿por qué no es “de verdad”? ¿ADS lento o firmware/protocolo? ¿Hilo a D1 vs muestrear AIN0 más rápido vs JSON→binario? Ganancias y pérdidas de cada uno.
4. Protocolo: ¿matar JSON, híbrido (JSON lento + binario ECG), o binario único? Layout de bytes sugerido, MTU, seq/CRC, impacto en app y en 0x180D.
5. Tabla: cada punto Gemini G1–G4 y Claude E1–E9 → de acuerdo / en desacuerdo / a medias, y **firmware / hilo / Rev 2 / no hacer**.
6. Lista Rev 2 mínima que **sí** pagaría otra tanda, y lista de cosas que **no**.
7. Checklist de puesta en marcha cuando llegue el PCBA (polímetro, 5 redes, U.FL, hilo BAT+, DPDT, DEMO_BLE=0).
8. Riesgos que podrían hacer **no funcionar** (I2C 400 kHz, back-power, DPDT, pin1 ADS, RF, USB+piel).

No rediseñes un producto médico. No propongas cambiar el BLE name. No inventes un pinout DGS viejo (pre-unificación) que contradiga SBAS444E.

FIN DEL PROMPT
