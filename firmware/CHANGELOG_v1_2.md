# Firmware 1.2 changelog / Registro de cambios 1.2

Target / Destino: fabricated PinaBio Paca v1.0 with XIAO ESP32-S3. The BLE name, proprietary UUIDs, pin map, sensors and Wi-Fi-off behavior are unchanged. Binary format stays the same but the protocol version byte is **0x12**, so receivers must explicitly accept 1.2.

## English

1. **PPG USB versus small BLE MTU:** `sendPpgFrame` (and ECG) now checks whether BLE can hold at least one sample. If not, USB binary still uses its own size limit. With both usable, BLE determines the common frame length. This fixes the V1.1 case where BLE MTU 23 blocked USB PPG.
2. **Honest transport accounting:** renamed `framesAccepted` to `framesSubmitted`; added `usbShortWrites` and a CCCD subscription check before BLE can consume a ring. BLE `notify()` gives no phone acknowledgement, so `ble_notify_calls` means a local invocation only. The shared ring still cannot guarantee delivery to two independent consumers.
3. **PPG power and sleep:** `ppgHardware` returns success only after reading MAX30102 MODE_CONFIG.SHDN. PPG retries failed shutdown and does not acknowledge parking. Sleep waits for ADC, PPG, SLOW and COMMS, aborts after 2 s on failure, restores the previous run state and reports an error. COMMS parks cooperatively, avoiding a resume-before-suspend race.
4. **Session isolation:** START now clears and verifies the *hardware* PPG FIFO before opening a new session. It stops with an explicit error if this fails. ADC/PPG also check that the channel is still enabled when committing work. PPG rate changes invalidate in-flight batches, clear its ring and reset beat history.
5. **Telemetry loss visibility:** telemetry is sequenced at acquisition; failed sends leave the snapshot queued. A ring overwrite increments `telDrops`; conditional discard avoids consuming the next snapshot if an overwrite happens during output. Sequence gaps expose missing snapshots.
6. **Persistence and resource failures:** SAVE checks Preferences open and every byte count; NVS failures return `ERR SAVE_NVS` and retain dirty state. Setup checks synchronization/task allocations. STATUS exposes sensor health, PPG power errors, sleep aborts, short USB writes and FreeRTOS stack high-water marks.
7. **Core compatibility:** the BLE write callback uses the return type of `getValue()` via `auto`, then copies command bytes into a queue. This handles the String/std::string return difference across Arduino-ESP32 generations; full build compatibility still needs a real compile.
8. **Review documentation:** English and Spanish protocol and code guides explain hardware, modules, variables, lock ownership, important statements, wire format and known limits. V1.1 source and protocol are removed from the current tree; history remains in Git.
9. **MUX diagnostic:** `adcMuxChanges` now compares each conversion against the actual previous ADS channel/gain. The old per-slot comparison could stay at zero even while switching channels.

## Español

1. **PPG USB con MTU BLE pequeño:** `sendPpgFrame` y ECG comprueban si BLE admite una muestra. Si no, USB binario conserva su límite de tamaño. Si ambos sirven, BLE limita el frame común. Se corrige el bloqueo PPG USB con MTU BLE 23 de V1.1.
2. **Contadores honestos:** `framesAccepted` pasa a `framesSubmitted`; se añade `usbShortWrites` y se comprueba suscripción CCCD antes de consumir por BLE. BLE `notify()` no confirma recepción móvil: `ble_notify_calls` es solo llamada local. Un ring común no garantiza entrega independiente a dos clientes.
3. **Energía PPG y sleep:** `ppgHardware` devuelve éxito solo tras leer MODE_CONFIG.SHDN del MAX30102. Si falla el apagado PPG reintenta y no confirma reposo. Sleep espera ADC, PPG, SLOW y COMMS; a los 2 s cancela, restaura estado y comunica error. COMMS se aparca cooperativamente para evitar carrera suspend/resume.
4. **Aislamiento de sesiones:** START limpia y verifica la FIFO *física* PPG antes de abrir sesión. En fallo deja adquisición parada. ADC/PPG comprueban habilitación al publicar. Cambiar tasa PPG invalida lotes en curso, vacía ring y reinicia historia de latidos.
5. **Pérdidas de telemetría visibles:** se asigna secuencia al adquirir; un fallo de envío mantiene snapshot en cola. Sobrewrite suma `telDrops`; descarte condicional evita quitar el siguiente snapshot si hubo sobrewrite durante envío. Los saltos de secuencia muestran snapshots faltantes.
6. **Fallos NVS y recursos:** SAVE comprueba apertura y cada escritura; devuelve `ERR SAVE_NVS` y deja configuración pendiente. Setup comprueba creación de mutex/cola/tareas. STATUS muestra salud, errores de apagado PPG, abortos sleep, escrituras USB cortas y margen mínimo de stack.
7. **Compatibilidad del core:** callback BLE deduce con `auto` el retorno de `getValue()` y copia el comando a cola. Resuelve la diferencia String/std::string; la compatibilidad completa exige compilación real.
8. **Documentación:** protocolo y guía de código en inglés/español explican hardware, módulos, variables, mutex, sentencias clave, formato y límites. Se elimina V1.1 del árbol vigente; Git conserva historial.
9. **Diagnóstico MUX:** `adcMuxChanges` compara con el canal/ganancia de la conversión ADS anterior real. La comparación por slot podía quedarse en cero pese a cambiar de canal.

**Verification / Verificación:** static review only in this workspace. No Arduino compilation, flash or physical PCBA measurements were performed here. / Solo revisión estática en este espacio. Aquí no se compiló, flasheó ni midió la PCBA.
