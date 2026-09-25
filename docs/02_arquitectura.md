# Arquitectura — PinaBio v1.0

Una sola placa (antes se llamaba «Completa» en docs; es este mismo diseño). La Mini queda sustituida.

**No es un dispositivo médico.** Sesión con electrodos o bandas: **Bluetooth, USB del PC desconectado**.

## Qué hay

| Sensor | Cómo |
|---|---|
| GSR | Borne J3, igual que antes (0,5 V, 100 kΩ) |
| PPG / pulso | MAX30102 en J4 (pads) |
| Temperatura | MAX30205 en J5 (pads) |
| ECG | Módulo AD8232 en J8 (6 pads). Electrodos van al módulo, no a nuestra PCB |
| Respiración pecho | Goma elástica resistiva, borne J6 |
| Respiración abdomen | Igual, borne J7 |
| Batería | Carga USB-C del XIAO; voltios en A0; % en el teléfono |

Bandas: 0,5 V → 47 kΩ → goma → masa. El op-amp B/C copia el punto medio. El teléfono **calibra** la profundidad (no son litros de aire).

## ADS1115

| Canal | Ganancia | Señal |
|---|---|---|
| AIN0 | ×1 | ECG |
| AIN1 | ×8 | GSR |
| AIN2 | ×1 | Pecho |
| AIN3 | ×1 | Abdomen |

AD8232 SDN unido a V_ANALOG: con SW1 OFF se apaga el ECG junto al analógico. LO+ / LO− → D8 / D9.

## BLE

Nombre BLE `PinaBiosensor` (no es el nombre de marketing). JSON v3 + servicio de pulso 0x180D.
