# Documento de Arquitectura de Hardware: PinaBiosensor Mini

**Versión:** 1.0 (basada en PinaBiosensor 1.0 v2.0, reducida)
**Objetivo:** Especificaciones para el diseño de la PCB en KiCad de una versión simplificada del biofeedback, centrada en tres señales: **conductancia de la piel (GSR)**, **temperatura de la piel** y **pletismografía/HRV (PPG)**.

Este documento asume que llevas tiempo sin tocar electrónica, así que cada bloque incluye un breve repaso de "por qué" antes del "cómo".

---

## 0. Qué cambia respecto a la versión completa

| Bloque | Versión completa | Versión Mini |
|---|---|---|
| ECG (AD8232) | Sí | **Eliminado** |
| Respiración (bandas resistivas x2) | Sí | **Eliminado** |
| GSR | Sí (jack TRRS) | Sí, pero con **terminal de tornillo** |
| SpO2/PPG (MAX30102) | Sí (jack TRRS) | Sí, **cable soldado directo** |
| Temperatura | Analógica, vía ADS1115 (no existía en la versión completa; aquí se introduce) | **MAX30205, digital I2C**, cable soldado directo |
| MCP6004 (op-amp) | 3 canales en uso (A, B, C) | **1 canal en uso (A)**, resto sin usar (ver §3.4) |
| ADS1115 | 4 canales multiplexados | **1 canal en uso (AIN1)** |
| Conectores de sensores | AudioJack4 (TRRS 3.5mm) | Terminal de tornillo (GSR) + pads soldados con anclaje (I2C) |
| Alimentación, carga, switch ON/OFF | — | **Sin cambios** |

---

## 1. Microcontrolador

**Seeed Studio XIAO ESP32-S3**, montado igual que en la versión completa: no va soldado directamente a la placa base, sino insertado en dos tiras de conectores hembra `Conn_01x07`, para poder sacarlo y reprogramarlo o reutilizarlo en otro proyecto.

---

## 2. Sistema de Alimentación (sin cambios)

Repaso rápido de por qué esto es así, ya que es la parte más delicada del diseño:

- **V_BATT**: la batería LiPo se conecta directamente al pin `BAT` del XIAO. El propio módulo XIAO ya incluye el circuito de carga (chip cargador de Li-Po), así que no hay que diseñar nada de carga desde cero.
- **Monitor de batería**: un divisor resistivo de dos resistencias de 47kΩ (1%) reduce la tensión de la batería a la mitad, y ese punto medio va al pin `A0` (ADC interno del ESP32-S3) para poder leer el nivel de batería por software.
- **V_ANALOG (3.3V "limpios")**: la circuitería analógica (sensores) es sensible al ruido que meten los circuitos digitales (el WiFi/BLE del XIAO, por ejemplo). Por eso no se alimenta directamente de V_BATT, sino de un regulador LDO dedicado (**ADP150AUJZ-3.3**), seguido de un filtro LC (ferrita de 600Ω @ 100MHz + condensador de 10µF) que actúa como un "colador" de ruido de alta frecuencia.
- **V_REF (0.5V exactos)**: tensión de referencia necesaria para el circuito de GSR (ver §3.1). Se genera con un divisor de precisión (56kΩ y 10kΩ al 0.1%) desde V_ANALOG, y se "bufferiza" con un amplificador operacional en configuración seguidor de tensión — es decir, el op-amp copia esa tensión pero con muy baja impedancia de salida, para que no varíe cuando el circuito de GSR "tire" de corriente de ella.

**Interruptor ON/OFF (SW1)** y **carga (J_CHARGE, J_BATT)**: idénticos a la versión completa. El interruptor solo corta la alimentación del dominio analógico (V_ANALOG) y fuerza el microcontrolador a Deep Sleep (contacto a GPIO3), pero **nunca interrumpe la ruta de carga**, que va directa a los pads BAT del XIAO. Esto significa que puedes cargar la batería con el sistema "apagado" sin ningún problema — no hay que rediseñar nada aquí.

---

## 3. Sensores

### 3.1 Conductancia de la Piel (GSR) — el sensor más delicado

**Repaso del concepto:** la piel actúa como una resistencia variable que cambia con la sudoración (respuesta de conductancia, muy usada en biofeedback de estrés/relajación). Para medirla, hacemos un divisor de tensión donde una de las "resistencias" es la propia piel, y medimos el punto medio.

**Topología** (idéntica a la versión completa):

```
V_REF (0.5V) → R_FIXED (100kΩ) → Nodo_MID → Electrodos (piel) → GNDA
```

- El **Nodo_MID** se conecta a `AIN1` del ADS1115.
- **Electrodos**: par de electrodos Velcro tipo GSR/SC (los que te han recomendado), conectados a la placa mediante un **terminal de tornillo de 2 pines** (ver §4). No es necesario respetar polaridad — la piel actúa como resistencia pura, no como diodo.

**Filtros de protección** (mantener tal cual, incluso más importantes ahora sin jack blindado):

1. **C_shunt (100nF)**: en paralelo con los electrodos, entre el terminal de tornillo y GNDA. Estabiliza la lectura frente a pequeños movimientos/contactos intermitentes.
2. **R_limit (470Ω)**: en serie entre el Nodo_MID y el pin AIN1, protege la entrada del ADC.
3. **R_series (47Ω)**: en serie justo a la entrada del terminal de tornillo, protección ESD (descargas electrostáticas) — recuerda que aquí hay piel humana en contacto directo con el circuito.

**ADC**: el ADS1115 se alimenta desde V_ANALOG, con `ADDR` conectado también a V_ANALOG (fija la dirección I2C en `0x49`). Ya que solo usamos un canal, el firmware configura ganancia ×8 (rango ±0.512V) para aprovechar toda la resolución del ADC en el rango de tensión donde se mueve la señal.

### 3.2 Pletismografía / HRV — MAX30102

Sensor óptico todo-en-uno (LED + fotodiodo + ADC interno), dirección I2C `0x57`. No necesita ningún acondicionamiento analógico externo — toda la "electrónica difícil" ya está dentro del chip.

- **VCC** → V_ANALOG (alimentación limpia; mejora la relación señal/ruido de la medida óptica).
- **GND** → GNDD (es un chip digital con picos de corriente al encender los LEDs; conviene que esos picos vayan al plano digital y no contaminen GNDA).
- **SDA/SCL** → bus I2C compartido (pull-ups de 2.2kΩ a 3.3V digital, ya presentes en el bus).
- **Conexión física**: cable de 4 hilos soldado directo a 4 pads en la placa (ver §4).

### 3.3 Temperatura de la Piel — MAX30205 (recomendado sobre NTC)

Sensor digital I2C específico para temperatura corporal, dirección `0x48`.

**Por qué este y no un NTC** (repaso, ya que llevas tiempo sin ver esto): un NTC es una resistencia que cambia con la temperatura, pero para leerla necesitas (a) un divisor de tensión con otra resistencia de precisión, (b) un canal ADC libre, y (c) calcular la temperatura con una fórmula de calibración (ecuación de Steinhart-Hart) en el firmware. El MAX30205 se salta los tres pasos: es un termómetro que habla I2C y te devuelve grados directamente, con una resolución de 0.0039°C — más que suficiente para detectar las tendencias lentas típicas del biofeedback de temperatura periférica (técnica de "manos calientes").

- **VCC** → V_ANALOG
- **GND** → GNDD
- **SDA/SCL** → mismo bus I2C
- **Conexión física**: cable de 4 hilos soldado directo (comparte el mismo patrón de pads que el MAX30102, ver §4).

### 3.4 Amplificador Operacional MCP6004 — uso reducido

En la versión completa se usaban 3 de los 4 canales del MCP6004 (buffer de V_REF, y dos buffers de respiración). En la versión Mini **solo se usa el canal A** (buffer de V_REF para el GSR).

**Importante (repaso de un detalle que se olvida fácilmente):** los canales B, C y D del MCP6004 **no pueden quedar con la entrada al aire**. Un op-amp con la entrada flotante puede oscilar y meter ruido de alta frecuencia en toda la placa, incluyendo en el canal A que sí usas (todos comparten el mismo chip y la misma alimentación interna). La solución estándar y sin coste: conecta la entrada no inversora de cada canal no usado a GNDA, y realimenta la salida a la entrada inversora (configuración de seguidor de tensión con entrada a masa). Esto los deja "en reposo" de forma segura.

---

## 4. Conectores de Sensores (nuevo respecto a la versión completa)

Se elimina el conector `AudioJack4` (TRRS) para todos los sensores. En su lugar:

| Sensor | Conector | Motivo |
|---|---|---|
| GSR (electrodos Velcro) | **Terminal de tornillo, 2 pines**, paso 5.08mm | Se pone y se quita en cada uso; no requiere soldar ni crimpar, solo pelar el hilo e insertarlo |
| MAX30102 | **4 pads pasantes** (VCC, GND, SDA, SCL) + 1 **agujero de anclaje** junto a ellos | Cable soldado directo, se instala una vez; el agujero permite pasar una brida o poner pegamento caliente para evitar que el tirón del cable arranque los pads |
| MAX30205 | **4 pads pasantes** (VCC, GND, SDA, SCL) + 1 **agujero de anclaje** | Igual que el anterior |

**Recomendación práctica de montaje:** separa los pads lo suficiente (al menos 2.54mm entre centros) para poder soldar cómodamente con un soldador normal, y coloca el agujero de anclaje a unos 5mm del grupo de pads, no pegado a ellos, para que la brida no interfiera con la soldadura.

---

## 5. Planos de Masa (sin cambios de criterio, menos superficie a repartir)

- **GNDA** (masa analógica): bajo el ADS1115, el MCP6004, y el circuito de GSR (R_FIXED, C_shunt, terminal de tornillo).
- **GNDD** (masa digital): bajo el XIAO, el bus I2C, y los pads de MAX30102/MAX30205.
- **Net-Tie**: un único punto de unión entre GNDA y GNDD, situado cerca del LDO ADP150 y del XIAO, igual que en la versión completa.

Al haber menos componentes analógicos, el plano GNDA es más pequeño, pero el criterio de separación es exactamente el mismo: evitar que el ruido digital (conmutación del XIAO, BLE) se cuele en la medida de GSR, que es la más sensible de las tres señales.

---

## 6. Clases de Red (Net Classes)

Sin cambios respecto a la versión completa:

- **Alimentación** (V_BATT, 3.3V, V_ANALOG): 0.4–0.5 mm de ancho de pista.
- **Señales analógicas** (Nodo_MID del GSR, V_REF): 0.25 mm, alejadas de las pistas I2C.
- **Bus I2C** (SDA, SCL): 0.25 mm, cortas y paralelas, sobre el plano GNDD.

---

## 7. Lista de Componentes (BOM) — resumen

| Componente | Cantidad | Notas |
|---|---|---|
| Seeed XIAO ESP32-S3 | 1 | Módulo plug-in, no soldado |
| Conn_01x07 hembra | 2 | Zócalo para el XIAO |
| LDO ADP150AUJZ-3.3 | 1 | V_ANALOG |
| Ferrita 600Ω @ 100MHz | 1 | Filtro LC de V_ANALOG |
| Condensador 10µF | 1 | Filtro LC de V_ANALOG |
| Resistencias 47kΩ 1% | 2 | Divisor monitor de batería |
| Resistencias 56kΩ / 10kΩ 0.1% | 1 + 1 | Divisor V_REF |
| MCP6004 | 1 | Solo canal A en uso |
| Condensador 4.7µF | 1 | Estabilidad salida buffer V_REF |
| ADS1115 | 1 | Solo canal AIN1 en uso |
| Resistencias pull-up I2C 2.2kΩ | 2 | SDA, SCL |
| Resistencia R_FIXED 100kΩ | 1 | Divisor GSR |
| Condensador C_shunt 100nF | 1 | GSR |
| Resistencia R_limit 470Ω | 1 | GSR |
| Resistencia R_series 47Ω | 1 | GSR |
| Terminal de tornillo 2 pines, 5.08mm | 1 | Electrodos GSR |
| MAX30102 (módulo/breakout) | 1 | I2C 0x57, pads soldados |
| MAX30205 | 1 | I2C 0x48, pads soldados |
| SW1 (interruptor doble contacto) | 1 | Sin cambios vs. versión completa |
| J_CHARGE, J_BATT | 1 + 1 | Sin cambios |

---

## Siguiente documento

Como no tienes experiencia previa con KiCad, el siguiente documento (`PinaBiosensor_Mini_Guia_KiCad_Principiantes.md`) te lleva paso a paso desde instalar el programa hasta generar los ficheros para fabricar esta placa concreta.
