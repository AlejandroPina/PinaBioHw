# Firmware v1.1 — PinaBio Paca v1.0 fabricada

Sketch: `PinaBiosensor_Firmware_v1_1.ino`. Esta entrega solo corresponde a la PCB V1.0 fabricada. El protocolo vigente está en `docs/13_protocolo_firmware_v1_1.md`.

## Hardware verificado

- XIAO ESP32-S3. ADS1115 `0x49`: AIN0 ECG, AIN1 GSR, AIN2 tórax, AIN3 abdomen.
- MAX30102 `0x57`; MAX30205 `0x48`.
- D0/A0/GPIO1 batería con divisor 47 kΩ / 47 kΩ; D3/GPIO4 SLEEP_N activo bajo; D8/GPIO7 LO+; D9/GPIO8 LO−; D4/GPIO5 SDA; D5/GPIO6 SCL.
- BLE `PinaBiosensor`, UUID V1 conservados. Wi-Fi apagado.

## Instalación y uso

Arduino IDE: usar la placa Seeed XIAO ESP32-S3 y la biblioteca SparkFun MAX3010x Pulse and Proximity Sensor Library. Guardar el sketch dentro de una carpeta con el mismo nombre, `PinaBiosensor_Firmware_v1_1`. USB Serial a 115200. Tras arrancar, `STATUS` muestra objetivos y tasas reales. `START` abre una sesión limpia; `STOP` la cierra. `USB_MODE BINARY` cambia el puerto USB a solo frames; `USB_MODE TEXT` vuelve a respuestas y diagnóstico. Los comandos siguen disponibles por USB y BLE en ambos modos. Las preferencias de sensores, tasas y streams se guardan con `SAVE`; el modo USB vuelve a texto en cada arranque.

Objetivos iniciales: ECG 250 SPS (`AUTO_ECG OFF`), GSR 10, tórax 20, abdomen 20, PPG 200. `SET AUTO_ECG ON` permite hasta 500 SPS de ECG según presupuesto lógico. `STATUS`/`DIAG` muestran la tasa medida; la placa real determinará la capacidad alcanzable. Los timestamps internos son de 64 bits. El campo transmitido sigue siendo `uint32` de microsegundos desde `START`, con vuelta modular aproximadamente a los 71,6 minutos.

## Verificación pendiente en placa

Esta entrega tuvo revisión estática y comprobación de invariantes del formato; **no se ha compilado con Arduino-ESP32 ni flasheado en una XIAO** en este entorno. Antes de usarla con la PCBA, compilar con la versión de Arduino-ESP32 y SparkFun MAX3010x seleccionadas, verificar que `STATUS` detecta los tres dispositivos, medir SPS/jitter, comprobar CRC y secuencia en MTU 23/64/247, probar FIFO y `START`/`STOP` repetidos, y confirmar sleep/wake GPIO4.

La placa no está aislada para USB. Con electrodos o bandas sobre la piel, usar solo BLE y batería; desconectar contacto corporal para cargar o flashear por USB. No es un dispositivo médico.
