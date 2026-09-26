# Protocolo PinaBiosensor v1.1

## Transportes

BLE anuncia `PinaBiosensor`. El servicio propietario y características conservan los UUID V1:

- Servicio `6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- JSON `6b1d0002-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- COMMAND `6b1d0003-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- ECG `6b1d0004-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- PPG `6b1d0005-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- TELEMETRY `6b1d0006-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- EVENT `6b1d0007-5e8a-4c2f-9b3a-2c7f0e1a4d90`

Se mantiene HRS `0x180D`, Heart Rate Measurement `0x2A37` y Body Sensor Location `0x2A38`. HRS es una vista previa del pulso detectado, no HRV. COMMAND acepta texto UTF-8/ASCII por write y emite respuestas por notify. USB a 115200 acepta comandos terminados en CR/LF. Los mensajes de respuesta BLE largos pueden dividirse en varias notificaciones consecutivas.

## Frame binario

Los enteros multibyte son little-endian. Cabecera fija de **13 bytes**:

| Offset | Longitud | Campo |
|---:|---:|---|
| 0 | 1 | Magic `0xA5` |
| 1 | 1 | Tipo: 1 ECG, 2 PPG, 3 telemetría, 4 evento |
| 2 | 1 | Versión `0x11` (V1.1) |
| 3 | 1 | Flags, reservado = 0 |
| 4 | 2 | Secuencia `uint16` por tipo, reiniciada en START |
| 6 | 4 | `uint32` microsegundos desde START, módulo 2³² |
| 10 | 2 | Periodo estimado entre muestras, microsegundos (0 en telemetría/eventos) |
| 12 | 1 | Cantidad de muestras ECG/PPG; en eventos, longitud de texto; en telemetría, 0 |

Después va el payload y al final CRC16-CCITT-FALSE (`poly=0x1021`, inicio `0xFFFF`, sin reflexión ni XOR final), `uint16` little-endian, calculado desde magic hasta el último byte de payload. Longitud total = `13 + payload + 2`.

- ECG: `count × int16` raw del ADS1115, PGA ±4,096 V. Longitud `15 + 2×count`.
- PPG: `count × (IR uint24 + RED uint24)`; cada componente contiene 18 bits útiles. Longitud `15 + 6×count`. Timestamps PPG reconstruidos a partir del tiempo de lectura de FIFO y tasa configurada: **estimaciones**, no marcas de hardware por muestra.
- Telemetría: payload fijo de **24 bytes**, frame total 39. Offsets relativos al payload: GSR `uint32` en µS×1000 (0), temperatura `int16` °C×100 (4), batería `uint16` mV (6), tórax `uint16` mV (8), abdomen `uint16` mV (10), HR `uint16` bpm (12), RR `uint16` ms (14), flags `uint16` (16), profundidad ECG `uint16` (18), PPG `uint16` (20), telemetría `uint16` (22). Flags: bits 0 ADS presente, 1 PPG presente, 2 temperatura válida, 3 ECG activo, 4 GSR activo, 5 tórax activo, 6 abdomen activo, 7 leads-off.
- Evento: texto ASCII/UTF-8 corto, `count` bytes (máximo 50). Longitud `15 + count`.

El timestamp del primer dato se serializa desde un reloj interno de 64 bits. El host debe calcular diferencias modulares de 32 bits. Las secuencias permiten detectar huecos de frames, pero una notificación BLE no confirma recepción por la aplicación. Los contadores de `STATUS` de frames enviados significan llamadas de envío a la API o escrituras USB completas, no entrega confirmada al móvil.

## MTU y límites

Payload ATT máximo = MTU negociado − 3. Muestras máximas = `floor((MTU−3−15)/bytesPorMuestra)`, limitadas además por el buffer del firmware (80 ECG, 30 PPG). MTU 23 permite 2 ECG y 0 PPG con una cabecera real de 13 bytes y CRC de 2; PPG necesita **MTU mínimo 24**. Telemetría requiere MTU mínimo 42. El firmware solicita 247, pero usa el valor negociado. A MTU 247 caben 80 ECG (límite interno) y 30 PPG (límite interno). Los datos no salen del ring ECG/PPG hasta que se construye un frame válido y se solicita su envío por un transporte disponible.

## Comandos

`START`, `STOP`, `STATUS`, `HELP`, `DEFAULTS`, `SAVE`, `SLEEP`; `ECG`, `GSR`, `THORAX`, `ABDOMEN`, `PPG`, `TEMP`, `BAT`, `ECG_STREAM`, `PPG_STREAM`, `TELEM_STREAM`, `JSON`, `USB_ECG`, `USB_PPG`, `USB`, `BLE` seguidos de `ON|OFF`; `SET ECG_RATE n`, `SET PPG_RATE 100|200|400`, `SET GSR_RATE n`, `SET THORAX_RATE n`, `SET ABDOMEN_RATE n`, `SET AUTO_ECG ON|OFF`, `SET VREF 0.45..0.55`. V1.1 añade `USB_MODE TEXT|BINARY`.

`USB_MODE BINARY` emite una última confirmación textual y después solo frames binarios por USB. Las respuestas a comandos USB en ese modo siguen disponibles por BLE; el comando `USB_MODE TEXT` devuelve la salida textual. `USB_ECG/USB_PPG` requieren además el modo binario. JSON `v=4` es de depuración; no equivale a versiones anteriores de JSON. En modo texto no se mezclan frames binarios. Telemetría y eventos pueden salir por USB binario si está activada la transmisión USB.

## Sesión, pérdidas y diagnóstico

`START` incrementa la generación de sesión, limpia buffers y reinicia secuencias. Una conversión iniciada antes de `START` o `STOP` se descarta. Los ring buffers llenos descartan la muestra nueva y aumentan `drops`. `fifo_ovf` cuenta incrementos detectados en el contador de overflow de 5 bits del MAX30102; puede subestimar una saturación prolongada. `sw_ovf` indica pérdida dentro del buffer de la biblioteca SparkFun. El primer intervalo DIAG solo establece línea base y no muestra SPS artificial.

## Sleep/wake

D3/GPIO4 bajo 40 ms inicia sleep; al entrar con GPIO4 bajo, despierta al liberarlo (nivel alto). SLEEP con GPIO4 alto entra en deep sleep y despierta cuando GPIO4 baja. El wake reinicia el firmware y no continúa una sesión anterior.
