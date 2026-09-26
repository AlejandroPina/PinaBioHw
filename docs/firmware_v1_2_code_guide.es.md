# Guía de revisión del firmware 1.2 (español)

Fuente: `firmware/PinaBiosensor_Firmware_v1_2/PinaBiosensor_Firmware_v1_2.ino`. Esta guía describe el *código implementado*, quién modifica cada dato, los fallos posibles y los límites conocidos. Permite revisarlo sin leer conversaciones anteriores. Es un solo sketch Arduino con nombre igual a su carpeta. Su hardware es la Paca v1.0 ya fabricada; no incluye ninguna placa futura.

## 1. Hardware fijo y dependencias

Seeed XIAO ESP32-S3; ADS1115 `0x49`: AIN0 ECG, AIN1 GSR, AIN2 tórax, AIN3 abdomen; MAX30102 `0x57`; MAX30205 `0x48`. D0/A0/GPIO1 para batería con divisor 47k/47k; D3/GPIO4 sleep activo bajo; D8/GPIO7 LO+, D9/GPIO8 LO−; D4/GPIO5 I²C. Los `PIN_*`, `*_ADDR`, nombre BLE `PinaBiosensor` y UUID corresponden a este cobre. `setup()` apaga Wi-Fi. ALERT de ADS no llega al XIAO: el driver consulta OS. SparkFun MAX3010x aporta FIFO/detector de pulso; Arduino-ESP32 aporta BLE, FreeRTOS, Preferences y deep sleep.

## 2. Módulos, variables y propietarios

| Módulo | Funciones clave | Variables y significado | Propietario / protección |
|---|---|---|---|
| Constantes | `adsMuxBits`, `adsGainBits`, builders | `PROTOCOL_VERSION=0x12`, direcciones, pines, objetivos SPS, `PACKET_MAGIC` | Constantes de compilación |
| Sesión/configuración | `handleCommand`, `loadConfig`, `saveConfig`, `defaultsConfig` | `runState`, `sessionGeneration`, `sessionStartUs`, `en*`, `stream*`, `tx*`, `*RateCfg`, `gsrVref`, `configDirty` | `stateMutex`; COMMS cambia órdenes, setup inicializa |
| Planificación ADC | `pickNextAdcChannel`, `rebuildAdcSchedule`, `adcTask` | `adcSlots[].nextDueUs`: plazo flexible; `lastAdsChan/lastAdsGain`: conversión anterior real; `adcScheduleDirty`; `adcSamples[]`, `adcErrors[]` | Solo ADC cambia slots; configuración/publicación bajo `stateMutex` |
| ADS por I²C | `adsWriteConfig`, `adsReadReg16`, `adsStartAndRead` | MUX AIN0–3 (`0x4000`+desplazamiento); PGA ECG/bandas ±4,096 V (`0x0200`), GSR ±0,512 V (`0x0800`); DR 860 | Cada transacción Wire con `i2cMutex`, liberado en todas las salidas |
| PPG/energía | `configurePpg`, `ppgHardware`, `clearPpgFifoForSession`, `ppgTask`, `processPpgBeat` | `ppgRateCfg`, `ppgFifoOverflow`, `ppgSoftwareOverflow`, `ppgPowerErrors`, `ppgHr`, `ppgRrMs`, `lastBeatTsMs` | PPG hace lecturas/apagado normales; configuración bajo `i2cMutex`; publicación bajo `stateMutex` |
| Sensores lentos | `max30205Read`, `slowTask` | `lastTempC`, `lastBattV`, `lastLo`, `tempOk`, `tempI2cErrors`, `telSeq` | SLOW; I²C con `i2cMutex`, snapshot con `stateMutex` |
| Rings | `ecgPush/Peek/Discard`, `ppgPush/Peek/Discard`, `telPush/Peek/DiscardIfSeq` | `ecgRing`, `ppgRing`, `telRing`, índices y `ecgDrops`, `ppgDrops`, `telDrops` | Un `portMUX` por ring; nunca hacer I/O mientras esté tomado |
| BLE/USB | `bleNotify`, `bleText`, `usbWriteFrame`, `send*Frame`, `process*Commands` | `bleConnected`, `usbBinaryMode`, `bleNotifyAttempts`, `bleNotifyRejected`, `framesGenerated`, `framesSubmitted`, `usbFrames`, `usbShortWrites` | COMMS envía normalmente; SLEEP envía evento final con COMMS aparcado; `bleMutex` protege setValue/notify |
| Sleep | `sleepTask`, `performDeepSleep` | `sleepRequested`, `deepSleeping`, `adcParked`, `ppgParked`, `slowParked`, `commsParked`, `sleepAborts`, `sleepFailureLatch` | SLEEP coordina; todos los flags parked bajo `stateMutex` |
| Diagnóstico | `statusText`, `printDiagnostics` | `measuredAdcSps[]`, `measuredPpgSps`, salud, colas, mínimos de stack | COMMS imprime normalmente; son observaciones, no garantías de SPS |

`ESample` y `PSample` contienen RAW y microsegundos absolutos `uint64_t`. `SlowSample` guarda telemetría y una secuencia asignada **al adquirirla**, de modo que se vean snapshots perdidos. `CmdMsg` copia hasta 127 bytes desde el callback BLE: no conserva un puntero a memoria efímera. `StateGuard` usa RAII: si toma el mutex, su destructor lo libera. `pickNextAdcChannel` lo libera antes de esperar/I²C y pone `held=false` para evitar doble Give.

### Diccionario de variables globales

- `ppg` es el driver SparkFun; `prefs` maneja NVS. `bleServer` y `chJson/chCmd/chEcg/chPpg/chTel/chEvt/chHr` se crean en `setupBle()` y viven hasta deinit BLE.
- `i2cMutex`, `bleMutex`, `stateMutex` definen los tres recursos serializados. `cmdQueue` lleva copias de órdenes BLE a COMMS. `adcTaskHandle/ppgTaskHandle/slowTaskHandle/commsTaskHandle/sleepTaskHandle` permiten comprobar creación y margen de stack. `ecgMux/ppgMux/telMux` protegen secciones cortas de rings.
- `bleConnected` lo cambian callbacks BLE; `sleepRequested`, `deepSleeping`, `sleepAbortPending`, `sleepFailureLatch` son señales de coordinación. `adcParked/ppgParked/slowParked/commsParked` son confirmaciones bajo `stateMutex`. `ppgPowerUnverified` impide confirmar falsamente el reposo PPG tras un fallo de configuración. `sessionGeneration` invalida trabajo viejo; `usbBinaryMode` comienza false en cada arranque.
- `runState` controla adquisición. `enEcg/enGsr/enThorax/enAbdomen/enPpg/enTemp/enBattery` habilitan sensores; `streamEcg/streamPpg/streamTelemetry/streamJson` controlan salida; `txUsb/txBle` habilitan transportes; `usbEcgStream/usbPpgStream` habilitan RAW USB; `autoEcgBoost` cambia el objetivo ECG.
- `ecgRateCfg/ppgRateCfg/gsrRateCfg/thoraxRateCfg/abdomenRateCfg` son SPS solicitados, no medidos. `gsrVref` calibra conversión GSR. `adcSlots[4]` guardan vencimientos; `lastAdsChan/lastAdsGain` recuerdan la conversión anterior real para contar cambios de MUX; `adcScheduleDirty` pide reconstrucción.
- `adsOk/ppgOk` reflejan detección/configuración inicial, no salud continua. `tempOk` refleja última lectura MAX30205. `lastEcgV/lastGsrUs/lastThoraxV/lastAbdomenV/lastTempC/lastBattV/lastLo` son últimas lecturas derivadas usadas por JSON/telemetría.
- `ppgHr/ppgRrMs`, `lastBeatTsMs`, `hrHist[4]`, `hrHistCount` pertenecen a la vista previa aproximada de pulso. Se reinician en START y al cambiar tasa PPG.
- `adcSamples[4]/adcErrors[4]`, `adcConversions`, `adcMuxChanges`, `ppgSamples` cuentan adquisición/fallos. `ppgFifoOverflow` es cota inferior del contador físico de 5 bits; `ppgOverflowPrev` es lectura anterior; `ppgSoftwareOverflow` estima pérdidas del ring SparkFun; `ppgI2cErrors` cubre operaciones explícitas PPG; `ppgPowerErrors` cubre verificación SHDN/wake; `tempI2cErrors` cuenta fallos de temperatura.
- `ecgDrops/ppgDrops` cuentan muestras nuevas rechazadas por rings llenos; `telDrops` cuenta telemetría antigua sobrescrita; `cmdDrops` cuenta órdenes BLE rechazadas por cola llena. `ecgPackets/ppgPackets/telPackets/eventPackets` cuentan frames enviados localmente por tipo.
- `framesGenerated` cuenta frames construidos; `framesSubmitted` cuenta frames con solicitud BLE local o escritura USB completa; `bleNotifyAttempts/bleNotifyRejected` son resultados locales; `usbFrames/usbShortWrites` cuentan escrituras USB completas/cortas; `sleepAborts` cuenta barreras sleep fallidas. Nada demuestra recepción móvil.
- `ecgSeq/ppgSeq/evtSeq` avanzan con frames enviados; `telSeq` avanza con cada telemetría adquirida. `sessionStartUs` es origen de reloj de 64 bits; `lastTelemetryMs/lastDiagMs` marcan intervalos; `configDirtySinceMs/configDirty` marcan reintentos NVS; `bootEventPending` hace que solo COMMS emita BOOT después de crear tareas.
- `ecgRing/ppgRing/telRing` guardan muestras; `ecgHead/ecgTail`, `ppgHead/ppgTail`, `telHead/telTail` son índices circulares con su portMUX. Cada ring reserva una celda para distinguir lleno de vacío.

Variables locales importantes: `generation` es sesión capturada por una tarea; `active`/`want` deciden si inicia trabajo; `hwOn` indica estado PPG verificado; `maxLen` limita frame; `canBle` exige suscripción y MTU suficiente; `submitted` indica solicitud local aceptada; `periodUs` estima separación temporal; `parked` reúne confirmaciones sleep. Ninguna garantiza entrega física al móvil.

## 3. Sentencias e invariantes que deben comprobarse

1. `adsRawToVolts()` usa exactamente el rango configurado por `adsGainBits()`: para GSR ambos son ±0,512 V. `calcGsrUs()` descarta valores inválidos/fuera de rango, en lugar de inventar conductancia.
2. `adsStartAndRead()` escribe configuración single-shot, consulta OS y lee conversión. 860 SPS es la tasa máxima configurada en el ADC, **no** la cantidad garantizada de muestras ECG con cuatro canales. Medir `STATUS actual`/`DIAG` en placa.
3. `pickNextAdcChannel()` elige el canal habilitado cuyo plazo vence antes y suelta `stateMutex` antes de esperar o usar I²C. Es planificación flexible. Si va tarde, avanza el slot para no quedar atrapado recuperando plazos viejos.
4. ADC/PPG/SLOW capturan `sessionGeneration` antes de trabajar y lo comprueban bajo `stateMutex` al publicar. START/STOP y cambio de tasa PPG invalidan trabajo antiguo. ADC comprueba además que el canal siga habilitado; PPG comprueba `enPpg`.
5. `START` detiene MAX30102, limpia y verifica punteros de FIFO **física**, y vacía el pequeño ring RAM de SparkFun además de los rings propios. Si falla, deja sesión parada. Impide que muestras anteriores reciban el nuevo origen temporal.
6. `ppgHardware(false)` llama al apagado SparkFun y lee después MODE_CONFIG `0x09`: exige bit SHDN 7. Si falla mutex/lectura devuelve false y sube `ppgPowerErrors`; PPG no confirma reposo. Si falla la configuración inicial, `ppgPowerUnverified` también impide suponer apagado. Sleep aborta a los 2 s. La lectura demuestra el bit, no corriente nula.
7. Builders ECG/PPG usan **13 bytes de cabecera + payload + 2 CRC**. Hacen peek y solo descartan tras llamada BLE local con CCCD habilitado o escritura USB completa. BLE aún puede perder la notificación después: no hay confirmación extremo a extremo.
8. Si el MTU BLE no permite una muestra, `canBle=false`, pero USB puede enviarla. Si ambos sirven, BLE limita el tamaño del frame común. No hay cursores separados por transporte ni retransmisión independiente a un cliente atrasado.
9. SLOW asigna secuencia de telemetría antes de encolarla. `telPeek()` conserva el snapshot si falla envío; `telDiscardIfSeq()` evita descartar el siguiente si el productor sobrescribió el más antiguo durante el envío. `telDrops` cuenta esas sobreescrituras.
10. `usbBinaryMode` es false al arrancar y no se guarda con `saveConfig()`. En BINARY se suprime texto tras la última confirmación; las órdenes USB siguen entrando. `usbWriteFrame()` cuenta escrituras cortas; el cliente debe resincronizar por CRC.
11. `saveConfig()` comprueba `Preferences.begin` y el número de bytes de cada `put*`; en error devuelve `ERR SAVE_NVS` y deja `configDirty=true` para reintentar. NVS usa varias claves separadas: **no hay transacción atómica** frente a corte de alimentación.
12. `performDeepSleep()` para adquisición y avanza generación; después espera cuatro confirmaciones de reposo. La de PPG incluye SHDN verificado. Si pasan 2 s sin todas, cancela, restaura `runState` y comunica `ERR SLEEP_NOT_QUIESCENT`. MAX30205 permanece alimentado; medir consumo real.
13. `STATUS frames_submitted` y `ble_notify_calls` son *solicitudes locales*, no ACK del móvil. Observar también rings, FIFO y saltos de secuencia. `stack_hwm` muestra el mínimo stack libre de FreeRTOS (unidades ESP-IDF); vigilarlo en el bring-up.

## 4. Límites de esta revisión y pruebas pendientes

Este código no valida por sí solo la placa. Se deben medir ECG 250 SPS/boost 500, PPG 200, contención I²C, respuesta óptica, pérdidas FIFO y corriente en sleep. `check()` de SparkFun no entrega un registro completo de errores I²C: `ppg_i2c` cuenta principalmente las operaciones explícitas, no toda la salud del bus PPG. El contador MAX30102 de overflow tiene 5 bits y puede saturarse: `fifo_ovf` es una cota inferior. Los timestamps PPG son estimaciones. BLE notify no tiene ACK del móvil. JSON v4 es solo depuración. El repositorio no incluye cliente Android V1.2.

Primer bring-up: desconectar electrodos/bandas antes de USB; compilar con versiones fijadas de Arduino-ESP32 y SparkFun; cargar, revisar sondas iniciales y `STATUS`, y medir DIAG tras dos intervalos. Probar START/STOP, CRC, MTU 23/24/42/247, USB BINARY con BLE MTU 23, fallos NVS cuando sea posible, bit SHDN y corriente en sleep. Con piel conectada, usar batería y BLE. No es un dispositivo médico.

## Fuentes primarias

- [Datasheet TI ADS1115](https://www.ti.com/lit/ds/symlink/ads1115.pdf): MUX, PGA y modo/tasa de conversión.
- [Datasheet Analog Devices MAX30102](https://www.analog.com/media/en/technical-documentation/data-sheets/max30102.pdf): registros FIFO 0x04–0x06 y MODE_CONFIG.SHDN en 0x09 bit 7.
- [Código SparkFun MAX3010x MAX30105.cpp](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/blob/master/src/MAX30105.cpp): `clearFIFO`, `shutDown`, `wakeUp`, `check`.
- [Referencia Espressif Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html): bytes devueltos por `put*` y errores.
