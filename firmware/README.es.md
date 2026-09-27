# Firmware PinaBiosensor 1.3 — Paca v1.0 fabricada

[English](README.md) · [Protocolo 1.3](../docs/firmware_v1_3_protocol.es.md) · [Guía 1.3](../docs/firmware_v1_3_code_guide.es.md) · [Cambios 1.3](CHANGELOG_v1_3.md)

**Sketch vigente:** `PinaBiosensor_Firmware_v1_3/PinaBiosensor_Firmware_v1_3.ino`

**Se conserva por si hay que volver:** `PinaBiosensor_Firmware_v1_2/` con [protocolo 1.2](../docs/firmware_v1_2_protocol.es.md) y [changelog 1.2](CHANGELOG_v1_2.md).

Mismo hardware Paca v1.0, nombre BLE `PinaBiosensor`, Wi-Fi apagado. No es dispositivo médico. Los frames binarios siguen con byte **`0x12`**.

## Compilación y carga

Arduino IDE o CLI, placa **Seeed XIAO ESP32-S3**, core Arduino-ESP32 y SparkFun MAX3010x. Carpeta y `.ino` con el mismo nombre. Electrodes fuera antes de USB.

## Uso

Los mismos comandos que 1.2. `STATUS` empieza por `fw=1.3 proto=0x12`. `STOP` pone a cero la vista previa de pulso. `START` sigue pudiendo devolver `ERR START_PPG_FIFO`.

No hay interlock por software de USB (no hay GPIO de VBUS). Cargar con power bank está bien; piel fuera mientras el cable esté puesto.

## Verificación

1.3 corrige comportamiento de 1.2. Hay que compilar y medir SPS de ECG en la XIAO tras el cambio del sondeo ADS.
