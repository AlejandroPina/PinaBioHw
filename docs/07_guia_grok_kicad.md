# Encargo para Grok — generar KiCad de PinaBio v1.0

Documento para pegar o adjuntar a **Grok** (Cursor u otro agente con acceso al repo y a KiCad).  
Autor del proyecto: Alejandro. **No es un dispositivo médico.**

---

## 0. ¿Es posible?

**Sí, con matices.**

| Encargo | Realista |
|---|---|
| Esquemático **de verdad** (símbolos + hilos + etiquetas globales) | Sí, si Grok usa KiCad 8 y las librerías de este repo |
| PCB colocada, planos GNDA/GNDD, net-tie | Sí |
| Pistas **sin errores DRC** (listo para JLCPCB) | **A veces no a la primera.** Debe **iterar**: DRC → corregir → DRC hasta 0 errores. No entregar Manhattan sucio y decir “listo” |
| Firmware / Android | **No los reinventes.** Ya existen. Solo KiCad + Gerbers + BOM si cambia |

Si no tienes KiCad instalado, **no inventes un `.kicad_pcb` a mano**. Di que falta KiCad. No sustituyas la placa por un dibujo ASCII.

---

## 1. Qué eres y qué no

Eres el **layoutista**. El circuito **ya está decidido**. No “mejoras” la arquitectura (no cambies pines de sleep, no pongas cargador extra, no alimentes I2C desde V_ANALOG, no uses GPIO3 para el interruptor).

**Olvida la Mini como producto.** El diseño se llama **PinaBio v1.0**. Los archivos KiCad siguen `PinaBiosensor_Mini.*` (**no los renombres**). Gerbers de pedido: `exports/PinaBio_v1_gerbers.zip` (hay copia `PinaBiosensor_Completa_gerbers.zip`). No dejes dos placas distintas.

---

## 2. Archivos que debes leer primero (en este orden)

1. Este archivo: `docs/07_guia_grok_kicad.md`
2. `docs/02_arquitectura.md`
3. `docs/04_seguridad.md`
4. `scripts/generate_kicad.py` — **fuente eléctrica** (nets y pines)
5. `hardware/sym-lib/Pina.kicad_sym`
6. `hardware/footprints.pretty/` (XIAO zócalo, cables 4P y 6P)
7. `firmware/PinaBiosensor_V1_Firmware_Final/PinaBiosensor_V1_Firmware_Final.ino` — pines de la PCB v1.0 (no contradecir).

No uses `uploads/PinaBiosensor_Mini_Guia_KiCad_Principiantes_*.md` (está mal: GPIO3, etiquetas jerárquicas, sensores a V_ANALOG).

---

## 3. Circuito congelado (no negociar)

### MCU
Seeed **XIAO ESP32-S3** en zócalo 2×7, paso 2,54 mm, filas 17,78 mm. USB hacia el **borde** de la placa.  
Pad **BAT+** no está en los 14 pines: huella extra + **hilo** al pad BAT del módulo.

| Función | Pin XIAO | GPIO |
|---|---|---|
| Divisor batería | D0 / A0 | 1 |
| Sleep SW1 | **D3** | **4** (nunca D2/GPIO3) |
| SDA | D4 | 5 |
| SCL | D5 | 6 |
| ECG LO+ | D8 | 7 |
| ECG LO− | D9 | 8 |

Carga LiPo: **solo el USB-C del XIAO**. SW1 **no** corta BAT.  
SW1 DPDT: polo A = V_BATT → VIN del LDO analógico. Polo B OFF = D3 a GND.

### Alimentación analógica
ADP150-3.3. Datasheet: ≥1 µF en VIN y VOUT. Pin 4 del ADP150 es **NC**, no BYP: **no pongas 1 nF a un pin BYP que no existe** si el símbolo está mal; corrige el símbolo a NC si hace falta. Ferrita + 10 µF **después** del LDO = `V_ANALOG`.

### Masas
`GNDA` (GSR, bandas, MCP6004, ADS pin GND, AD8232 GND) y `GNDD` (XIAO, LDO GND, I2C). **Un** net-tie `NT1` junto al LDO.

### I2C
Pull-ups 2,2 kΩ a **`+3V3` del XIAO**. ADS1115, MAX30102, MAX30205 alimentados de **`+3V3`**, no de V_ANALOG.  
ADS ADDR a +3V3 → **0x49**.

### V_REF y GSR
56k/10k 0,1 % desde V_ANALOG → ~0,50 V. MCP6004 canal **A** seguidor. **Riso 47 Ω** entre salida y 4,7 µF (realimentación **antes** de Riso).  
GSR: V_REF → 100 kΩ → MID → piel → GNDA. Serie 1 kΩ + TVS 5 V + 100 nF en electrodo. 470 Ω a AIN1. Ganancia firmware ×8.

### Bandas (goma resistiva)
Igual idea, 47 kΩ desde V_REF. Canal **B** pecho (J6), **C** abdomen (J7). Canal **D** seguidor a GNDA. TVS + 100 nF + 1 kΩ como GSR.

### ECG
Módulo **AD8232** fuera de la PCB. J8 6 pads: 3V3(V_ANALOG) / GNDA / OUT / LO+ / LO− / SDN.  
SDN a **V_ANALOG** (activo en alto mientras el analógico está ON). OUT → 1 kΩ → AIN0, 100 nF a GNDA. Ganancia ×1.

### Conectores
| Ref | Qué |
|---|---|
| J1 | LiPo JST-PH |
| J2 | 5 V opcional (mismo 5V XIAO) |
| J3 | GSR tornillo 2P |
| J4 | MAX30102 4 pads + agujero brida |
| J5 | MAX30205 4 pads + agujero |
| J6 | Banda pecho tornillo 2P |
| J7 | Banda abdomen tornillo 2P |
| J8 | AD8232 6 pads + agujero |

---

## 4. Entregables (obligatorios)

Al terminar, deben existir y **abrir en KiCad 8 sin errores de librería**:

- `hardware/PinaBiosensor_Mini.kicad_pro` (o `PinaBiosensor.kicad_pro` si renombras todo)
- `.kicad_sch` — **símbolos reales**, no un muro de texto
- `.kicad_pcb` — contorno, piezas, pistas, vias, zonas GNDA/GNDD rellenas
- `bom/BOM.csv` actualizado si cambian valores
- `exports/gerbers/` regenerados **después** de DRC = 0 errores
- Captura o log: **DRC 0 errors** (avisos courtyard se discuten, no se ignoran a ciegas)

No hace falta firmware nuevo.

---

## 5. Paso a paso (sigue este orden)

### Paso A — Entorno
1. KiCad **8** (o 7) **con librerías oficiales**.
2. Abre el proyecto desde el `.kicad_pro`, no el PCB suelto.
3. Comprueba `fp-lib-table` / `sym-lib-table`: librería **Pina** → `footprints.pretty` y `sym-lib/Pina.kicad_sym`.

### Paso B — Esquemático de verdad
1. Puedes partir de `scripts/generate_kicad.py` (SKiDL) para no perder nets, **pero** el `.kicad_sch` final debe ser dibujado o exportado con símbolos visibles.
2. Etiquetas **globales** (`V_ANALOG`, `+3V3`, `GNDA`, `GNDD`, `SDA`, `SCL`…). **Prohibido** “etiqueta jerárquica global”.
3. Power flags / símbolos de alimentación en cada red de power.
4. ERC: 0 errores. Warnings de “insufficient drive” en power a veces se limpian con PWR_FLAG; no dejes pines de chip al aire (AIN usados; MCP D terminado).
5. Anota en el cajetín: “PinaBio v1.0 — no sanitario — USB nunca con piel”.

### Paso C — Asociar huellas
Todos los ref del netlist tienen footprint. ADS1115 = VSSOP-10 / TSSOP-10 3×3 P0.5 **de máquina**. Resistencias/condensadores **1206**.

### Paso D — PCB mecánica
1. Contorno ~**110 × 70 mm** (2 capas, 1,6 mm). 4 taladros M3 en esquinas.
2. Colocación:
   - XIAO USB al borde superior.
   - Analógico (MCP, GSR, bandas, ADS, AD8232 pads) **a un lado**; digital/XIAO al otro; NT1 en la frontera.
   - Tres bornes en un borde, fáciles de cablear.
   - J8 lejos del borde USB si puedes (menos lío de cables).
3. Clases: alimentación ≥ 0,45 mm; resto ≥ 0,25 mm; holgura ≥ 0,15 mm (JLCPCB 2 capas estándar).

### Paso E — Planos y pistas
1. Zona **GNDD** (F+B) zona izquierda/centro; **GNDA** derecha; solape solo en NT1.
2. Rellena zonas (`B`).
3. Enruta (a mano o Freerouting). **Prohibido** dejar el router Manhattan cruzado como entrega final.
4. `Inspeccionar → DRC`. Corrige **todos** los errores: clearance, unconnected, short, edge clearance, hole.
5. Repite E4 hasta **0 errors**. Si en 3 ciclos no bajas de ~10 errores, cambia estrategia (menos autorruta, más a mano en I2C y GSR).

### Paso F — Revisión visual (obligatoria)
Visor 3D:

- USB fuera, no contra el centro.
- Silkscreen: J1 BAT+/GND, J3 GSR, J6 PECHO, J7 ABD, J8 ECG, pad BAT+.
- Nada de courtyard aplastado sin comentario.

### Paso G — Fábrica
Gerbers + drill. ZIP. No subir Gerbers **viejos** de `exports/` de la Mini.

Pedido sugerido (coméntalo, no lo pagues tú): 5 PCB, 2 ensambladas SMT (incluido ADS1115), 3 vírgenes. THT lo suelda Alejandro.

---

## 6. Prohibiciones explícitas

- USB isolator en PinaBio v1.0 (D+/D− no salen al zócalo).
- Cortar la carga con SW1.
- Sleep en GPIO3.
- MAX30102 a V_ANALOG.
- 4,7 µF colgado en la salida del MCP **sin** Riso.
- Jacks TRRS (ya descartados).
- SHT41 (usamos MAX30205).
- Inventar SpO2 o HRV en la placa.
- Crear un proyecto KiCad **vacío** ignorando este repo.

---

## 7. Cómo sabe Alejandro que has acabado

1. Abre el `.kicad_pro` → esquemático se **ve** (chips, no solo párrafos).
2. PCB visor 3D: 3 bornes + conector 6 pads ECG.
3. DRC: **0 errores**.
4. Mensaje corto en castellano sencillo: qué tocaste y qué Gerber usar.

Si el DRC no llega a cero, **no mientas**. Lista los errores que quedan y por qué.

---

## 8. Prompt corto para pegar a Grok

```
Lee docs/07_guia_grok_kicad.md y obedece. Eres layoutista de PinaBio v1.0.
Genera esquemático real + PCB DRC limpio + Gerbers nuevos.
No cambies la arquitectura ni el firmware. No uses la guía de principiantes de uploads/.
Itera DRC hasta 0 errores. Al final explica en castellano simple qué entregaste.
```
