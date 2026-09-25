# PinaBiosensor V1 firmware

## Dónde está en este repo

Sketch Arduino (carpeta = nombre del `.ino`):
`firmware/PinaBiosensor_V1_Firmware_Final/PinaBiosensor_V1_Firmware_Final.ino`

Protocolo: `docs/13_protocolo_firmware_v1.md`

Scheduler ADS1115: `docs/14_ads1115_scheduler_v0.2.md`

El sketch JSON v3 antiguo está en `firmware/archive/PinaBiosensor_Mini/`.

## Qué es

Firmware de ingeniería para la PCBA V1 ya fabricada. Mantiene el hardware documentado y elimina Wi-Fi.

## Hardware esperado

- XIAO ESP32-S3
- ADS1115 0x49: AIN0 ECG, AIN1 GSR, AIN2 tórax, AIN3 abdomen
- MAX30102 0x57: módulo externo compatible con la alimentación disponible, RAW RED+IR
- MAX30205 0x48
- batería en D0/A0/GPIO1 con divisor 47k/47k
- SLEEP_N en D3/GPIO4
- AD8232 LO+ / LO- en D8/D9
- I2C D4/D5

La asignación anterior coincide con la descripción de la PCB V1 proporcionada para el análisis.

## Arquitectura

- `ADC task`: único propietario del ADS1115; scheduler dinámico según sensores activos.
- `PPG task`: vacía la FIFO del MAX30102 y conserva RAW IR+RED.
- `SLOW task`: MAX30205, batería y leads-off.
- `COMMS task`: BLE, USB, buffers de salida y diagnóstico.
- `SLEEP task`: sleep/wake por D3.

## Streams BLE

- ECG RAW: característica dedicada.
- PPG RAW: característica dedicada.
- Telemetría: característica separada.
- COMMAND: escritura desde Android.
- EVENT: eventos.
- JSON legacy/debug: opcional.
- Heart Rate Service 0x180D/0x2A37: HR/RR reales derivados del PPG, solo como preview/compatibilidad.

El BLE usa el nombre `PinaBiosensor` y conserva el servicio propietario de V1.

## Puntos clave del protocolo

Los frames RAW llevan:

- magic
- tipo
- versión
- flags
- sequence
- timestamp en microsegundos desde `START`
- periodo estimado
- número de muestras
- payload
- CRC16-CCITT

ECG usa `int16` raw del ADS1115. PPG usa `uint24 IR + uint24 RED` por muestra para conservar los 18 bits útiles del sensor en un formato compacto.

## Comandos

Los mismos comandos pueden entrar por BLE COMMAND o USB-Serial, por ejemplo:

```text
START
STOP
STATUS
HELP
ECG ON
ECG OFF
PPG ON
PPG OFF
GSR ON
GSR OFF
THORAX ON
THORAX OFF
ABDOMEN ON
ABDOMEN OFF
TEMP ON
TEMP OFF
BAT ON
BAT OFF
ECG_STREAM ON
PPG_STREAM ON
TELEM_STREAM ON
USB_ECG ON
USB_PPG ON
SET ECG_RATE 250
SET ECG_RATE 500
SET PPG_RATE 200
SET GSR_RATE 10
SET THORAX_RATE 20
SET ABDOMEN_RATE 20
SET AUTO_ECG ON
SET VREF 0.5000
SAVE
SLEEP
```

## Parámetros iniciales

- ECG: 250 SPS, auto boost hasta 500 SPS si existe presupuesto.
- GSR: 10 SPS.
- Tórax: 20 SPS.
- Abdomen: 20 SPS.
- PPG: 200 SPS.
- Telemetría: 10 Hz.
- ADS1115 configurado a 860 SPS.

Los valores anteriores son objetivos de firmware, no resultados medidos de una PCBA.

## Validación

Se ha hecho una comprobación sintáctica C++ con stubs de las APIs externas disponibles en el entorno. No se ha realizado una compilación/link real con la toolchain Arduino-ESP32 ni una ejecución sobre XIAO ESP32-S3.

Por tanto, antes de producción debe hacerse:

1. compilar con la versión concreta de Arduino-ESP32 y librerías que se vayan a usar;
2. cargar en una XIAO;
3. ejecutar el checklist de bring-up de la documentación de protocolo;
4. medir SPS, jitter, drops, ruido ECG/PPG y estabilidad I2C.
