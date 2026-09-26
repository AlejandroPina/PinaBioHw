# Firmware PinaBiosensor 1.2 — Paca v1.0 fabricada

[English](README.md) · [Protocolo](../docs/firmware_v1_2_protocol.es.md) · [Guía de revisión](../docs/firmware_v1_2_code_guide.es.md) · [Verificación](../docs/firmware_v1_2_verification.md) · [Cambios](CHANGELOG_v1_2.md)

El único firmware vigente es `PinaBiosensor_Firmware_v1_2/PinaBiosensor_Firmware_v1_2.ino`. Usa Seeed XIAO ESP32-S3 en la Paca v1.0 fabricada: ADS1115 `0x49`, MAX30102 `0x57`, MAX30205 `0x48`; I²C D4/D5, sleep D3/GPIO4, leads-off D8/D9, batería D0/A0. No incorpora hardware futuro. Nombre BLE `PinaBiosensor` y UUID conservados; Wi-Fi apagado. No es un dispositivo médico.

## Compilación y carga

Usar Arduino IDE o Arduino CLI con placa **Seeed XIAO ESP32-S3**, versión fijada de Arduino-ESP32 y SparkFun MAX3010x Pulse and Proximity Sensor Library. Elegir variante y opciones USB CDC del XIAO físico. Carpeta y `.ino` deben tener el mismo nombre. No hay paquete de placa ni lockfile en el repo. Hace falta compilar y cargar de verdad antes de afirmar compatibilidad. El callback BLE emplea `auto` para `getValue()` y acepta la diferencia String/std::string entre versiones del core.

Desconectar electrodos y bandas del cuerpo antes de usar USB para programar o diagnosticar. Con piel conectada, solo batería y BLE. El USB de esta placa no está aislado.

## Uso

- Serial 115200: revisar sondas iniciales y enviar `STATUS`. `DIAG` aparece automáticamente cada 2 s; el primer paso fija referencia. No es un comando.
- `STOP`, `START`, `STATUS` prueban una sesión nueva. `START` verifica el borrado FIFO MAX30102; en fallo devuelve `ERR START_PPG_FIFO` y deja adquisición parada.
- Objetivos iniciales: ECG 250 SPS, GSR 10, tórax 20, abdomen 20, PPG 200. `SET AUTO_ECG ON` permite objetivo lógico ECG hasta 500. Manda la tasa real medida.
- `USB_MODE BINARY` confirma en texto y después solo emite frames por USB. `USB_ECG ON` y `USB_PPG ON` activan esas salidas. Las órdenes siguen entrando en BINARY; `USB_MODE TEXT` restaura texto. Cada arranque vuelve a TEXT.
- `SAVE` comprueba escrituras NVS y devuelve `ERR SAVE_NVS` en fallo. Guarda sensores, tasas y streams, no el modo USB.
- `SLEEP` o D3/GPIO4 LOW solicita deep sleep. Se espera a tareas y apagado PPG verificado; si no se logra en 2 s cancela con `ERR SLEEP_NOT_QUIESCENT`.
- `STATUS` muestra objetivos, SPS medidos, salud, drops, cota inferior de pérdida FIFO, solicitudes de transporte/escrituras cortas, abortos sleep y margen mínimo de stack. Una llamada BLE notify no prueba recepción en el móvil.

Las guías enlazadas detallan cada módulo y grupo de variables, offsets, mutex, sentencias importantes y límites. El cliente debe verificar versión `0x12`, CRC y secuencias. No existe app Android V1.2 en el repositorio.

## Estado de verificación

Los cambios tuvieron segunda revisión estática de buffers, rutas de mutex y carreras de sleep/transporte. No había toolchain Arduino compatible ni PCBA física aquí; **el sketch no se ha compilado ni flasheado en este espacio**. Antes de llamarlo validado, compilar y hacer bring-up real: medir SPS, jitter, pérdidas ring/FIFO, MTU, bit SHDN PPG, corriente en sleep y margen de stack.
