# Firmware PinaBiosensor 1.2 — protocolo (español)

Esta especificación corresponde únicamente a la PinaBio Paca v1.0 fabricada y al sketch `PinaBiosensor_Firmware_v1_2.ino`. Es biofeedback experimental, no un dispositivo médico. La versión **0x12** cambia el byte de V1.1 (0x11); conserva el formato binario y los UUID. El receptor debe rechazar una versión desconocida antes de interpretar el payload.

## Transportes y GATT

- Nombre anunciado: exactamente `PinaBiosensor`.
- Servicio propietario: `6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90`.
- JSON de depuración `...0002`, COMMAND `...0003`, ECG `...0004`, PPG `...0005`, TELEMETRY `...0006`, EVENT `...0007`; todos comparten `-5e8a-4c2f-9b3a-2c7f0e1a4d90`.
- Servicio estándar HRS `0x180D`, Heart Rate Measurement `0x2A37`, Body Sensor Location `0x2A38`. El pulso es una vista previa detectada en IR, sin valor clínico ni HRV calculada en el micro.
- COMMAND acepta escritura de texto ASCII/UTF-8 y notifica respuestas de texto. Una respuesta larga se divide en notificaciones consecutivas, sin cabecera adicional de fragmentación. USB CDC a 115200 baudios acepta órdenes acabadas en CR/LF.

Las notificaciones BLE **no tienen ACK de aplicación**. `ble_notify_calls` cuenta llamadas a la API local, no recepción en el móvil. `Serial.write` por USB también puede escribir menos bytes; `usb_short` cuenta ese fallo. El cliente debe resincronizar por magic, longitud y CRC y vigilar saltos de secuencia. El único ring de adquisición se consume cuando al menos un transporte solicitado acepta el envío local: BLE y USB simultáneos no tienen retransmisión independiente.
El firmware comprueba la suscripción CCCD de cada característica antes de considerar BLE utilizable. Un cliente desconectado o sin suscribir no consume el ring por una llamada notify sin efecto.

## Frame binario

Los enteros de varios bytes son little-endian. Cabecera fija de **13 bytes**, payload y CRC de dos bytes.

| Offset | Bytes | Significado |
|---:|---:|---|
| 0 | 1 | Magic `0xA5` |
| 1 | 1 | Tipo: 1 ECG, 2 PPG, 3 telemetría, 4 evento |
| 2 | 1 | Versión `0x12` |
| 3 | 1 | Flags; ahora cero |
| 4 | 2 | Secuencia `uint16` por tipo, vuelta en 65536 |
| 6 | 4 | Microsegundos desde START módulo 2³² |
| 10 | 2 | Periodo estimado de muestra en µs; cero en telemetría/evento |
| 12 | 1 | Cantidad ECG/PPG, longitud de texto en evento, cero en telemetría |

CRC **CRC-16/CCITT-FALSE**: inicio `0xFFFF`, polinomio `0x1021`, MSB primero, sin reflexión ni XOR final. Cubre desde magic hasta el último byte del payload; excluye el propio CRC y se transmite little-endian. Vectores: byte `00` → `0xE1F0` (en cable `F0 E1`); ASCII `123456789` → `0x29B1`. Longitud = `13 + payload + 2`.

### Payloads

- ECG: `count` valores `int16` RAW de ADS1115 AIN0, PGA ±4,096 V. Longitud `15 + 2×count`; count 1–80.
- PPG: `count` pares IR `uint24` + RED `uint24` little-endian. Solo 18 bits de origen tienen contenido. Longitud `15 + 6×count`; count 1–30. Los tiempos de FIFO se estiman desde lectura y tasa configurada; no existen marcas físicas por muestra.
- Telemetría: payload fijo de 24 bytes, frame de 39, count cero. Offsets relativos: GSR µS×1000 `uint32` en 0; temperatura °C×100 `int16` en 4; batería mV `uint16` en 6; tórax mV en 8; abdomen mV en 10; HR bpm en 12; RR ms en 14; flags en 16; profundidad ring ECG en 18; PPG en 20; telemetría en 22. Bits de flags: 0 ADS presente, 1 PPG presente, 2 última temperatura válida, 3 ECG habilitado, 4 GSR, 5 tórax, 6 abdomen, 7 leads-off. Los tres primeros describen salud en la adquisición, no son una prueba continua de I²C.
- Evento: `count` bytes de texto, máximo 50; longitud `15 + count`. `BOOT` puede generarse antes de conectar BLE. `SHUTDOWN` requiere más que MTU 23 y es un envío de mejor esfuerzo.

`START` reinicia secuencias ECG/PPG/evento y fija un origen temporal interno de 64 bits. El timestamp transmitido vuelve aproximadamente cada 71,6 min; el cliente resta módulo 2³². La secuencia de telemetría se asigna al adquirirla, no al enviarla; un snapshot perdido u sobrescrito deja un salto visible. Una secuencia de frames no revela todas las muestras perdidas dentro de rings: consultar también `drops`, `fifo_ovf`, `sw_ovf`.

## MTU y elección de transporte

El firmware solicita MTU 247, pero usa el negociado. Valor máximo de notificación ATT = `MTU − 3`. Muestras máximas = `floor((valor_max − 15) / bytes_por_muestra)`, además de los límites 80 ECG y 30 PPG. MTU 23 permite dos ECG y ningún PPG; PPG necesita **24**, telemetría **42**, evento `SHUTDOWN` **26**. Si BLE no admite ni una muestra y USB binario sí está habilitado, se construye un frame para USB y se omite BLE. Si ambos sirven, el límite BLE fija el tamaño común. Un MTU BLE pequeño ya no bloquea PPG USB.

## Comandos y modos USB

`START`, `STOP`, `STATUS`, `HELP`, `DEFAULTS`, `SAVE`, `SLEEP`; `ECG`, `GSR`, `THORAX`, `ABDOMEN`, `PPG`, `TEMP`, `BAT`, `ECG_STREAM`, `PPG_STREAM`, `TELEM_STREAM`, `JSON`, `USB_ECG`, `USB_PPG`, `USB`, `BLE` seguidos de `ON|OFF`; `SET ECG_RATE n`, `SET PPG_RATE 100|200|400`, `SET GSR_RATE n`, `SET THORAX_RATE n`, `SET ABDOMEN_RATE n`, `SET AUTO_ECG ON|OFF`, `SET VREF 0.45..0.55`; `USB_MODE TEXT|BINARY`.

USB siempre arranca en TEXT; ese modo no se guarda. `USB_MODE BINARY` envía una última confirmación textual y después suprime texto/DIAG/JSON en USB mientras emite frames. Sigue aceptando órdenes; `USB_MODE TEXT` restaura el texto. `USB_ECG` y `USB_PPG` necesitan BINARY. `SAVE` responde `ERR SAVE_NVS` si falla alguna escritura NVS. `DIAG` aparece automáticamente cada intervalo: **no es comando**. JSON `v=4` es depuración optativa, sin compatibilidad con el cliente Android JSON antiguo.

## Sesión y sleep

`START` cierra la generación anterior, vacía rings, comprueba que se limpió la FIFO física MAX30102, reinicia origen temporal y reanuda adquisición. Si falla el borrado FIFO deja adquisición parada y responde `ERR START_PPG_FIFO`. `STOP` invalida trabajos en curso. Al deshabilitar un canal, no se publica una conversión/lote iniciado antes de OFF. `SET PPG_RATE` y `DEFAULTS` exigen antes `STOP` (`ERR STOP_REQUIRED`). Cambiar tasa PPG reconfigura el sensor físico, limpia FIFO/ring e invalida adquisición en curso.

`SLEEP` o D3/GPIO4 LOW durante 40 ms solicita deep sleep. El coordinador para adquisición, espera ADC/SLOW/COMMS y que PPG confirme el bit de apagado. Si no ocurre en 2 s, cancela y responde `ERR SLEEP_NOT_QUIESCENT`; no entra en deep sleep. MAX30205 sigue alimentado por la PCB fija. El nivel de wake en GPIO4 es el contrario al de entrada. Al despertar reinicia y USB vuelve a TEXT.
