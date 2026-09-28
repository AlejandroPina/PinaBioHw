English | Español

Versión en español: README.es.md

PinaBio v1.0 (Paca)

The paid JLCPCB order is v1.0 (exports/PinaBio_v1_*). A v1.1 layout (XIAO USB at the board edge) is ready for a later spin: exports/PinaBio_v1.1_* and docs/10_pinabio_v1.1_rf.md. Same circuit. Do not mix v1.1 files into the current order.

PinaBio is a biofeedback board (skin conductance, pulse, temperature, ECG, breathing). It is not a medical device and does not diagnose anything. The nickname Paca is a tribute to Alejandro’s grandmother.

Phone Bluetooth advertises as PinaBiosensor. Keep this BLE name for firmware clients.

What the board is

One PCB, about 110 × 70 mm, microcontroller Seeed XIAO ESP32-S3 in a socket:





GSR (skin)



Pulse / HRV (MAX30102) — raw PPG for external HRV analysis



Temperature (MAX30205)



ECG (AD8232 module — lab toy, not a hospital ECG)



Chest + abdomen breathing (stretch bands that change resistance)

Older notes call this same board “Completa”. KiCad files stay named PinaBiosensor_Mini.* on purpose (renaming breaks the project). Silk says PINA / PINABio v1.0 (no “Paca” on copper — it does not fit).

Charge the XIAO from its USB-C. With electrodes or bands on skin: Bluetooth only. Never USB from a PC and skin at the same time.

Fabrication files live on branch kicad-completa (exports/PinaBio_v1_*.zip / .csv). A copy on OneDrive may be old.

Safety (read this)

This design is not USB-isolated. Full text: docs/04_seguridad.md.





Never USB to a computer while GSR, bands, or ECG electrodes are on the body.



Charge / flash firmware with skin disconnected.



A session with skin = BLE only.



LiPo with a protection board only. JST J1: pin 1 +, pin 2 ground.



Not for diagnosis, not for use on wounds, not a medical product.



Order PCBs (JLCPCB)

5 bare boards, 2 assembled (PCBA). Step-by-step: English order guide · Español.

Upload these three files from kicad-completa:





exports/PinaBio_v1_gerbers.zip



exports/PinaBio_v1_bom_jlc.csv



exports/PinaBio_v1_cpl.csv

Do not use the Mini gerbers in exports/gerbers/PinaBiosensor_Mini-....

Firmware

Board sketch (Arduino, XIAO ESP32-S3):

firmware/PinaBiosensor_Firmware_v1_3/PinaBiosensor_Firmware_v1_3.ino

Firmware 1.2 is kept at firmware/PinaBiosensor_Firmware_v1_2/ for rollback.

BLE name: PinaBiosensor. Current protocol: docs/firmware_v1_3_protocol.en.md (binary byte still 0x12, same as 1.2). Instructions: firmware/README.md. Changes: firmware/CHANGELOG_v1_3.md.

Firmware debug JSON is v4. Binary frame details are in the protocol specification.

Open the PCB in KiCad

See docs/03_guia_kicad.md. Short version:





Install KiCad 8 with libraries. Do not create a new project.



Open hardware/PinaBiosensor_Mini.kicad_pro.



The schematic is a text map. The board you order is the PCB.



Suggested fab (summary)





JLCPCB / PCBWay, 2 layers, FR4 1.6 mm, green mask, HASL or ENIG.



Factory SMT: 1206, SOT-23-5, SOIC-14, ADS1115 (VSSOP-10, very small).



You fit: XIAO, MAX/AD8232 modules, battery, box switch on J9, optional meter on J10.

Design BOM: bom/BOM.csv.

More docs





docs/02_arquitectura.md — architecture



docs/04_seguridad.md — safety (EN + ES)



docs/firmware_v1_3_protocol.en.md — BLE UUIDs and current frames (byte 0x12)



docs/firmware_v1_2_protocol.en.md — V1.2 frames (kept for rollback)



docs/08_jlcpcb_order.md — order (English)



docs/08_pedido_jlcpcb.md — pedido (español)



docs/09_caja_interruptor_medidor.md — box DPDT (4 wires) and J10

