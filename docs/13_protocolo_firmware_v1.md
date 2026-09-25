# PinaBiosensor V1 - Protocolo de firmware final de prueba

## 1. Interfaces

- BLE: nombre exacto `PinaBiosensor`.
- USB-Serial: 115200 baud.
- Wi-Fi: no usado.
- BLE y USB comparten el mismo procesador de comandos.

## 2. Servicios BLE

Servicio propietario conservado:

- Service: `6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- JSON legacy/debug: `6b1d0002-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- COMMAND: `6b1d0003-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- ECG RAW: `6b1d0004-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- PPG RAW: `6b1d0005-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- TELEMETRY: `6b1d0006-5e8a-4c2f-9b3a-2c7f0e1a4d90`
- EVENT: `6b1d0007-5e8a-4c2f-9b3a-2c7f0e1a4d90`

También se mantiene Heart Rate Service `0x180D / 0x2A37` para compatibilidad. Solo se publica HR/RR cuando existe una detección real en el flujo PPG.

## 3. Formato ECG/PPG

Cabecera de 13 bytes:

| Byte | Campo |
|---:|---|
| 0 | `0xA5` magic |
| 1 | tipo: `1=ECG`, `2=PPG`, `3=telemetry`, `4=event` |
| 2 | versión = 1 |
| 3 | flags |
| 4..5 | sequence `uint16` LE |
| 6..9 | timestamp del primer dato, `uint32` LE, microsegundos desde `START` |
| 10..11 | periodo de muestra estimado, `uint16` LE, microsegundos |
| 12 | número de muestras |
| 13.. | payload |
| final 2 | CRC16-CCITT |

El timestamp es relativo a la sesión y está expresado en microsegundos. Por tanto el `uint32` se reinicia en cada `START` y da vuelta aproximadamente cada 71,6 minutos. Para sesiones normales se usa la diferencia modular de `uint32`.

### ECG

Cada muestra es `int16` raw del ADS1115, little-endian. La ganancia del canal es conocida por el firmware (GAIN_ONE).

### PPG

Cada muestra son 6 bytes: `IR uint24 LE` + `RED uint24 LE`. El MAX30102 original entrega 18 bits útiles; se conservan en un contenedor de 24 bits para evitar pérdida de información.

## 4. Telemetría

Frame tipo `3`, `sample_count=0`. Payload:

- GSR: `uint32`, uS x1000
- temperatura: `int16`, grados C x100
- batería: `uint16`, mV
- tórax: `uint16`, mV
- abdomen: `uint16`, mV
- HR: `uint16`, bpm
- RR: `uint16`, ms
- flags: `uint16`
- ECG queue: `uint16`
- PPG queue: `uint16`
- TELEMETRY queue: `uint16`

La telemetría se genera a 10 Hz como snapshot. No es el stream RAW de ECG/PPG.

## 5. Eventos

Frame tipo `4` con texto corto para eventos como `BOOT`, `START`, `STOP`, `SHUTDOWN`.

## 6. MTU y paquetes

El firmware solicita MTU 247, pero usa el MTU real del peer. Los bloques se reducen automáticamente cuando el peer negocia un MTU menor.

Con el MTU por defecto de 23, el protocolo todavía puede transportar:

- ECG: 3 muestras por paquete.
- PPG: 1 muestra por paquete.

Para PPG RAW práctico se recomienda que la app solicite/negocie MTU >= 64 y preferiblemente 247.

## 7. Scheduler ADS1115

Un único FreeRTOS task es dueño del ADS1115. No se hacen lecturas del ADS desde otras tareas.

Asignación V1:

- AIN0 ECG
- AIN1 GSR
- AIN2 tórax
- AIN3 abdomen

ADS: 860 SPS configurado.

Objetivos iniciales:

- ECG: 250 SPS, con `AUTO_ECG=ON` puede llegar hasta 500 SPS si los otros objetivos permiten margen.
- GSR: 10 SPS.
- Tórax: 20 SPS.
- Abdomen: 20 SPS.

Presupuesto lógico inicial: 650 SPS de objetivos de conversión. Es una reserva de ingeniería, no una medición física del tiempo total del sistema.

El scheduler cambia la lista de canales según `ON/OFF`; los canales desactivados no consumen conversiones.

## 8. GSR

VREF configurable, por defecto 0,500 V.

R fija = 100 kOhm y R serie de PCB = 1 kOhm, que se resta del total estimado.

No se impone una espera fija de 20 ms por un supuesto Cshunt. El settling se debe validar en la PCBA y, si hace falta, convertirlo en un parámetro explícito en una siguiente iteración.

## 9. PPG

MAX30102 a 200 SPS inicialmente, RED+IR, promedio de 1, FIFO y rollover habilitado.

El firmware mantiene un cálculo HR/RR en tiempo real como preview, pero la señal RAW PPG se envía a la app para cálculo de HRV.

No se calcula SpO2.

## 10. USB RAW

El USB-Serial mantiene comandos de texto en el mismo puerto. Si `USB_ECG ON` o `USB_PPG ON` están activos, los frames binarios RAW se escriben directamente en el puerto, sin prefijo de texto. El host debe distinguir comandos de texto y frames por `0xA5` + longitud implícita/parseo CRC. Para laboratorio se recomienda mantener `USB_ECG`/`USB_PPG` en OFF y usar `JSON ON` o comandos de texto, salvo que el host implemente el parser binario.

## 11. Comandos

Disponibles por BLE COMMAND y USB-Serial:

```text
START
STOP
STATUS
HELP
DEFAULTS
SAVE
SLEEP
ECG ON|OFF
GSR ON|OFF
THORAX ON|OFF
ABDOMEN ON|OFF
PPG ON|OFF
TEMP ON|OFF
BAT ON|OFF
ECG_STREAM ON|OFF
PPG_STREAM ON|OFF
TELEM_STREAM ON|OFF
JSON ON|OFF
USB_ECG ON|OFF
USB_PPG ON|OFF
USB ON|OFF
BLE ON|OFF
SET ECG_RATE n
SET PPG_RATE 100|200|400
SET GSR_RATE n
SET THORAX_RATE n
SET ABDOMEN_RATE n
SET AUTO_ECG ON|OFF
SET VREF x.xxxx
```

`USB OFF` y `BLE OFF` controlan la transmisión por esa interfaz; no deshabilitan el parser de comandos interno de la otra interfaz.

## 12. Sleep

D3/GPIO4 LOW durante 40 ms solicita deep sleep. Antes de dormir se apaga el MAX30102 por software y se desinicializa BLE. El wake-up es por GPIO4 HIGH.

## 13. Diagnóstico

Cada 2 s por USB se informa:

- SPS real de cada canal ADS
- profundidad de buffers ECG/PPG/telemetría
- paquetes enviados
- drops ECG/PPG
- errores ADS por canal
- cambios de canal
- MTU negociado
- rechazos por tamaño de notificación

Esto permite ajustar el scheduler con la PCBA real.
