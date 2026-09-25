# PinaBiosensor — Diff Técnico: Nueva "Versión Completa" (sobre base Mini)

> Documento de referencia para uso como contexto en Cursor.
> **Punto de partida**: PinaBiosensor Mini (documentado en `PinaBiosensor_Mini_Arquitectura_Hardware.md` y `PinaBiosensor_Mini_Firmware.ino`).
> **Esta nueva "Completa" sustituye conceptualmente a la v1.0 original** (la que usaba SHT41 y conectores AudioJack4). A partir de ahora, "Completa" = Mini + ECG + bandas de respiración, con MAX30205 (no SHT41) y sin jacks TRRS.
> Todo lo no mencionado aquí es **idéntico a la Mini** — no repetir su documento, solo tomarlo como base y aplicar estos añadidos.

---

## 0. Regla de decisión aplicada para conectores nuevos

Para ECG y bandas de respiración, se sigue el mismo criterio ya usado en la Mini para diferenciar GSR de los sensores I2C: **el tipo de conector depende de si el elemento se pone/quita del cuerpo en cada sesión, no del tipo de señal.**

| Elemento | ¿Se coloca/retira cada sesión? | Conector |
|---|---|---|
| GSR (electrodos) | Sí | Terminal de tornillo |
| Bandas de respiración | Sí | Terminal de tornillo |
| MAX30102, MAX30205 (módulos I2C) | No — se monta una vez | Pads soldados + anclaje |
| AD8232 (módulo ECG) | No — se monta una vez (los electrodos de piel se conectan al propio módulo AD8232, no a nuestra placa) | Pads soldados + anclaje |

---

## 1. Sensores — qué se añade sobre la Mini

| Sensor | Mini | Nueva Completa |
|---|---|---|
| ECG (AD8232) | No | **AÑADIDO** — salida analógica a `AIN0` del ADS1115, ganancia ×1 (±4.096V) |
| Banda respiración torácica | No | **AÑADIDO** — vía buffer MCP6004 canal B → `AIN2`, ganancia ×1 |
| Banda respiración abdominal | No | **AÑADIDO** — vía buffer MCP6004 canal C → `AIN3`, ganancia ×1 |
| GSR, MAX30102, MAX30205 | Presentes | **Sin cambios** respecto a la Mini |

**Confirmado explícitamente**: la Completa usa **MAX30205** para temperatura (no SHT41). Se pierde el canal de humedad ambiente en toda la familia de productos, no solo en la Mini — esto es una decisión de diseño consciente para unificar BOM y firmware entre ambas versiones.

---

## 2. ADS1115 — vuelve a usar los 4 canales

| Canal | Mini | Nueva Completa |
|---|---|---|
| `AIN0` | Sin conectar | ECG, ganancia ×1 |
| `AIN1` | GSR, ganancia ×8 | GSR, ganancia ×8 — sin cambios |
| `AIN2` | Sin conectar | Banda torácica, ganancia ×1 |
| `AIN3` | Sin conectar | Banda abdominal, ganancia ×1 |

**Impacto en firmware — importante**: en la Mini, la ganancia del ADS1115 se fija una única vez en `setup()` (`GAIN_EIGHT`, fijo) porque solo hay un canal activo. En la Completa **hay que recuperar el cambio de ganancia dinámico** entre lecturas, como en la v1.0 original:
- `ads.setGain(GAIN_ONE)` antes de leer `AIN0` (ECG), `AIN2` y `AIN3` (bandas).
- `ads.setGain(GAIN_EIGHT)` antes de la máquina de estados de GSR (`AIN1`).
- Mantener la protección ya existente en el firmware original: **no leer los canales rápidos (ECG/bandas) mientras la máquina de estados de GSR esté ocupando el multiplexor** (`if (okADS && gsrState == GSR_IDLE)`), para evitar colisiones de MUX.

---

## 3. MCP6004 — reactivar canales B y C

| Canal | Mini | Nueva Completa |
|---|---|---|
| A | Buffer V_REF | Buffer V_REF — sin cambios |
| B | No usado (GNDA + feedback) | **Reactivado** — buffer banda torácica |
| C | No usado (GNDA + feedback) | **Reactivado** — buffer banda abdominal |
| D | No usado (GNDA + feedback) | **Sigue sin usar** — mantiene la terminación correcta introducida en la Mini (mejora que se conserva respecto a la v1.0 original) |

---

## 4. Conectores — resumen completo de la nueva versión

| Sensor | Conector |
|---|---|
| GSR | Terminal de tornillo, 2 pines |
| Banda respiración torácica | Terminal de tornillo, 2 pines |
| Banda respiración abdominal | Terminal de tornillo, 2 pines |
| MAX30102 | 4 pads soldados (VCC/GND/SDA/SCL) + anclaje |
| MAX30205 | 4 pads soldados (VCC/GND/SDA/SCL) + anclaje |
| AD8232 (ECG) | 6 pads soldados (3.3V/GND/OUTPUT/LO+/LO-/SDN) + anclaje |

**Nota**: la conexión entre el módulo AD8232 y los electrodos de piel (habitualmente un jack de 3.5mm) es parte del propio módulo AD8232 tal como se vende comercialmente, y queda fuera del alcance de nuestro diseño de PCB — no hay que replicar ese jack en nuestra placa.

---

## 5. Firmware — cambios sobre el `.ino` de la Mini

- **Restaurar variables de ECG**: `e_v`, `he`, `ie`, `re` (igual que v2.5.0 original).
- **Restaurar variables de bandas**: `rt_v`, `ra_v`, `rrt`, `rra`.
- **Restaurar flags**: `enECG`, y equivalentes para bandas si se quiere control individual por Serial (`S_ECG:`, etc., como en `procesarComandos()` de la v1.0 original).
- **Restaurar arquitectura de 5 niveles** (en vez de los 2 niveles de la Mini): 50Hz (ECG/PPG/bandas), 2Hz (GSR/temperatura), y evaluar si se reintroducen los niveles de 0.1Hz (RSA respiratoria) y 30s (telemetría batería/reintentos) documentados en la memoria del proyecto para la v3.2.1.
- **Mantener de la Mini sin cambios**: `leerMAX30205()`, la máquina de estados de GSR, y `apagarSistema()` con su restricción de no usar `delay()` ni `BLEDevice::deinit()`.
- **JSON de salida**: ampliar de nuevo con los campos de ECG y bandas (`e, he, ie, re, rt, rrt, ra, rra`), manteniendo los ya existentes en la Mini (`s, hs, is, rs, g, t, b`). Vigilar el límite de MTU BLE (247 bytes) — con todos los campos activos puede que haga falta el fallback de recorte ya implementado.
- **Nombre BLE**: definir si la Completa usa un nombre distinto (p. ej. `PinaBiosensor_Completa`) para poder diferenciarla de la Mini al emparejar por Bluetooth.

---

## 6. Alimentación, batería y carga

**Sin cambios respecto a la Mini** (ya verificado en el diff anterior): monitor de batería presente sin modificar, carga gestionada íntegramente por el USB-C propio del XIAO, sin circuito de carga adicional, interruptor SW1 sin afectar a la ruta de carga.

---

## 7. BOM — solo lo que se añade sobre la Mini

- AD8232 (módulo ECG)
- 2× sensor de banda respiratoria resistiva (torácica + abdominal)
- 2× terminal de tornillo 2 pines adicionales (bandas) — mismo modelo ya usado para GSR
- 6× pads pasantes + 1 agujero de anclaje adicional (interfaz AD8232 → placa)
- Resistencias/condensadores de protección de entrada si se desea replicar el filtrado de la v1.0 original en las nuevas bandas (revisar si sigue siendo necesario sin el jack TRRS, dado que ahora es terminal de tornillo — recomendable mantenerlo por el mismo motivo que en GSR: un cable sin blindaje capta más ruido)

---

## 8. Resumen ejecutivo

- La nueva "Completa" = Mini + ECG + 2 bandas de respiración, manteniendo el resto de decisiones de la Mini (MAX30205 en vez de SHT41, sin jacks TRRS).
- Los conectores nuevos siguen el mismo criterio ya validado: **terminal de tornillo para lo que se pone/quita del cuerpo cada sesión** (bandas, igual que GSR), **pads soldados para módulos que se montan una vez** (AD8232, igual que MAX30102/MAX30205).
- El firmware debe recuperar el cambio de ganancia dinámico del ADS1115 y la protección anti-colisión de MUX con la máquina de estados de GSR, ambos presentes en la v1.0 original pero innecesarios en la Mini.
- Alimentación, batería y carga: sin ningún cambio en ningún escenario.
