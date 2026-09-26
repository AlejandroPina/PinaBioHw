# Firmware 1.2 verification record / Registro de verificación 1.2

## English

**Scope:** static review of the new sketch and documentation for fabricated Paca v1.0. This record does not claim a successful Arduino compile, flash, BLE reception test or PCBA measurement. No compatible Arduino IDE/CLI, ESP32 board package or physical Paca was present in this workspace.

**Checks completed:**

1. Source delimiters checked with a lightweight lexer that ignores C++ comments and strings; no unmatched parentheses/braces/brackets. This is weaker than compilation.
2. CRC-16/CCITT-FALSE independent vectors: `00` → `0xE1F0`; `123456789` → `0x29B1`. The source initializes `0xFFFF`, uses polynomial `0x1021`, and writes the result little-endian after computing through the payload.
3. ATT/MTU arithmetic checked for MTU 23/24/42/247: maximum uncapped ECG samples 2/3/12/114; PPG 0/1/4/38; telemetry 39-byte frame fits at 42 and 247. The source caps ECG at 80, PPG at 30. With BLE MTU 23 and USB BINARY active, PPG now uses USB capacity. Bluedroid CCCD subscription is checked before a BLE submission is considered usable.
4. Every explicit `xSemaphoreTake` path was reviewed for a matching `xSemaphoreGive`; unsuccessful takes return without giving. `StateGuard` releases in its destructor, except for the deliberate early release in `pickNextAdcChannel` where `held=false` prevents a double Give. New PPG FIFO/power helpers each release I²C on the acquired path.
5. Sleep state transitions were reviewed: workers advertise parked only after leaving acquisition; PPG verifies SHDN; COMMS uses cooperative parking; timeout restores prior run state and avoids deep sleep. The remaining effect of an unsuccessful shutdown is a reported sleep abort.
6. Source, root READMEs and hardware cross-references point to firmware 1.2. The old sketch/protocol/changelog are absent from the current tree; historical versions remain recoverable through Git. `git diff --check` passed.
7. MAX30102 register use was cross-checked against the Analog Devices datasheet and SparkFun source: FIFO pointer/counter registers `0x04..0x06`, MODE_CONFIG `0x09`, SHDN bit 7, `clearFIFO()` writes three zeroes, `shutDown()`/`wakeUp()` change SHDN. Preferences byte counts were checked against Espressif documentation.

**Not demonstrated yet:** compile against a pinned Arduino-ESP32 and SparkFun release; actual stack margins, task deadlines, ADS 250/500 SPS, PPG FIFO behavior, BLE delivery at negotiated MTUs, USB short-write recovery and power consumption. `notify()` has no application ACK; all counters named submissions/calls must be treated accordingly. A real build is the next hard gate before a laboratory flash.

**PCBA bring-up checklist (without skin contacts on USB):** boot probe messages for ADS `0x49`, MAX30102 `0x57`, MAX30205 `0x48`; `STATUS` health `1/1/1` and nonzero `stack_hwm`; `STOP` → `START` → `STATUS`; two DIAG intervals and sustained SPS/drops; BLE MTU 23/24/42/247 with CRC/version/sequence checking; USB BINARY PPG while BLE remains at MTU 23; repeated START/STOP and FIFO counter; SLEEP command and GPIO4 entry/wake; verify MAX30102 MODE_CONFIG.SHDN and PCBA sleep current. If any of these fail, do not treat V1.2 as session-ready.

## Español

**Alcance:** segunda revisión estática del sketch y documentación para Paca v1.0 fabricada. No demuestra compilación Arduino, flasheo, recepción BLE ni medidas de la PCBA. Este espacio no tenía Arduino IDE/CLI compatible, paquete ESP32 ni Paca física.

**Comprobaciones realizadas:**

1. Delimitadores del C++ comprobados con analizador ligero que ignora comentarios y cadenas: paréntesis/llaves/corchetes equilibrados. Es una prueba más débil que compilar.
2. Vectores CRC-16/CCITT-FALSE independientes: `00` → `0xE1F0`; `123456789` → `0x29B1`. El código inicia `0xFFFF`, usa `0x1021` y transmite little-endian tras cubrir el payload.
3. Aritmética ATT/MTU para 23/24/42/247: ECG sin límite interno 2/3/12/114; PPG 0/1/4/38; telemetría 39 bytes cabe desde 42. El sketch limita ECG a 80 y PPG a 30. Con MTU BLE 23 y USB BINARY, PPG usa capacidad USB. Se comprueba suscripción CCCD Bluedroid antes de considerar BLE utilizable.
4. Revisadas las rutas explícitas `xSemaphoreTake`/`xSemaphoreGive`; si falla Take se vuelve sin Give. `StateGuard` libera en destructor, salvo liberación anticipada deliberada en `pickNextAdcChannel` con `held=false`. Los nuevos helpers PPG liberan I²C tras tomarlo.
5. Revisados estados sleep: tareas confirman reposo fuera de adquisición; PPG comprueba SHDN; COMMS se aparca sin carrera suspend/resume; timeout restaura estado anterior y evita deep sleep. Si falla apagado PPG se informa abortando sleep.
6. Sketch, README y referencias de hardware apuntan a V1.2. Sketch/protocolo/changelog V1.1 ya no están en árbol actual; Git conserva historial. `git diff --check` pasó.
7. Registros MAX30102 contrastados con datasheet Analog Devices y código SparkFun: FIFO `0x04..0x06`, MODE_CONFIG `0x09`, SHDN bit 7; `clearFIFO()` escribe tres ceros, `shutDown()`/`wakeUp()` modifican SHDN. Bytes devueltos por Preferences contrastados con Espressif.

**Aún sin demostrar:** compilación con versiones fijadas Arduino-ESP32/SparkFun; margen real de stack, plazos, ADS 250/500 SPS, FIFO PPG, entrega BLE con MTU negociado, recuperación ante escritura USB corta y consumo. `notify()` no tiene ACK de aplicación. La compilación real es la siguiente puerta obligatoria antes de flashear en laboratorio.

**Bring-up PCBA (sin contacto corporal durante USB):** mensajes de ADS `0x49`, MAX30102 `0x57`, MAX30205 `0x48`; `STATUS` health `1/1/1` y `stack_hwm` distinto de cero; `STOP` → `START` → `STATUS`; dos intervalos DIAG y SPS/drops sostenidos; BLE MTU 23/24/42/247 con CRC/versión/secuencia; PPG USB BINARY con BLE en MTU 23; START/STOP repetidos y contador FIFO; SLEEP por comando/GPIO4, entrada/wake; comprobar SHDN del MAX30102 y corriente de sleep. Si algo falla, no considerar V1.2 lista para sesiones.
