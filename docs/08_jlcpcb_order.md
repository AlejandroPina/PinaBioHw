[English](#) | [Español](08_pedido_jlcpcb.md)

# JLCPCB order — PinaBio v1.0 (Paca)

For: **Alejandro**. You do not need KiCad. Do not open or edit the fab files.

Product: **PinaBio v1.0** (nickname **Paca**, after Alejandro’s grandmother). On the phone, Bluetooth is still **`PinaBiosensor`** (do not change it).

A **v1.1** board (XIAO USB at the edge) exists for a **future** order only. For **this** paid order use **`PinaBio_v1_*`**, not `PinaBio_v1.1_*`. See `docs/10_pinabio_v1.1_rf.md`.

This is **not** a medical device. With electrodes or bands on skin: **Bluetooth only**. **Never** PC USB and skin at the same time.

Fabrication files live on branch **`kicad-completa`**. A OneDrive copy may be stale.

---

## 0. What you get

One order. The factory makes **5 PCBs** and solders parts on **2** of them.

- **2 assembled boards** (resistors, capacitors, chips, and connectors the factory can place).
- **3 bare boards** (green PCB only). Spares or practice.

This does **not** include the XIAO module, MAX/AD8232 sensors, battery, bands, or the box switch. You buy those (section 7).

---

## 1. The 3 files

You need **exactly 3 files** (names as of September 2026):

| Role | Exact name | Path |
|------|------------|------|
| Copper / drills / silk | `PinaBio_v1_gerbers.zip` | `exports/PinaBio_v1_gerbers.zip` |
| Parts list for the factory | `PinaBio_v1_bom_jlc.csv` | `exports/PinaBio_v1_bom_jlc.csv` |
| Where each part sits | `PinaBio_v1_cpl.csv` | `exports/PinaBio_v1_cpl.csv` |

Identical copies still exist under the old “Completa” names. Prefer **`PinaBio_v1_*`**. If you already started a quote with Completa files, keep that same trio — do not mix.

**Do not** upload `exports/gerbers/` files named `PinaBiosensor_Mini-...`. That is a **different** (Mini) board.

These three files are **not** on `main`. Open **`kicad-completa`**, then `exports`. Copy the three `PinaBio_v1_*` files to Desktop or Downloads.

- Do not unzip `PinaBio_v1_gerbers.zip` to “fix” anything. Upload the **whole ZIP**.
- Do not save the CSVs from Excel if it wants to convert numbers or commas.

---

## 2. Sign up at JLCPCB

1. Open **https://jlcpcb.com**.
2. **Sign up** / **Register**, confirm email if asked, then **Log in**.

---

## 3. Upload gerbers and check the viewer

1. **Quote now** (sometimes **Instant Quote**).
2. Upload **`PinaBio_v1_gerbers.zip`**.
3. Wait for the board viewer.

Checklist:

- Size about **110 × 70 mm**. If it shows ~50×30, you uploaded the Mini — cancel.
- If engineering says the Gerber is **110×80**: that is the **panel** with 5 mm rails (5+70+5). Reply that the **finished board is 110×70 mm**. Do not approve an 80 mm tall PCB.
- Silk must show **J9** (box switch, 4 pins) and **J10** (battery meter, 2 pins).
- **No SW1** on the board. The switch is in the **box**, wired to J9.
- Rectangle, 4 corner screw holes.
- Text **PINA** / **PINABio v1.0** (no “Paca” on the PCB).

Then **Next** / **Confirm**.

---

## 4. PCB options

| Field | Set this |
|-------|----------|
| **PCB Qty** | **5** |
| Layers | **2** |
| PCB Thickness | **1.6 mm** |
| PCB Color | **Green** |
| Surface Finish | **HASL** (LeadFree HASL is fine). ENIG is optional and costs more. |
| Copper Weight | **1 oz** if shown |

No panel. Loose boards, qty **5**. **Do not pay yet.** Assembly is lower on the same page.

---

## 5. Assembly (PCBA): yes, 2 boards

1. **PCB Assembly** → ON.
2. **PCBA Qty**: **2**.
3. Side: **Top side**.
4. Start with **Economic**. If the site blocks through-hole connectors (JST, terminals, socket), switch to **Standard**.
5. **Confirm** / **Next**.

---

## 6. BOM + CPL, matching, DNP, polarity

1. BOM: `PinaBio_v1_bom_jlc.csv`.
2. CPL: `PinaBio_v1_cpl.csv`.
3. **Process BOM&CPL**.

Two BOM lines may need a manual match or **Do not place**:

- **U1** — XIAO **socket** (not the module). Example LCSC **C2932672** (check it is 2.54 mm female 1×7). If unsure, DNP U1 and solder at home.
- **J3, J6, J7** — 2-pin screw terminals, **5.08 mm**. Example **C475082**. Compare the photo to the board. If the body will not fit, DNP and buy separately.

Factory **should** place: resistors, capacitors, ferrite FB1, diodes D1 D2 D3, chips **U2 U3 U4**, connectors **J1 J2 J9 J10**, and U1 / J3 / J6 / J7 if the service allows.

**Do not place:** XIAO module, MAX30102, MAX30205, AD8232, pads **J4 J5 J8**, **SW1** (does not exist), holes **H1–H4**, jumper copper **NT1**.

Polarity in the assembly viewer before pay: **U2 U3 U4** pin 1, diodes D1–D3, JST latches toward the edge as drawn. U2 and U3 CPL rotation is **180°** (KiCad PCB stays 0°) so LCSC pin1 matches the silk triangle; **do not set U3 back to 0°**; **do not rotate U4**. If a part is still 180° wrong, **do not pay** — fix the CPL file, do not eyeball Excel. The cart 3D viewer may ignore CPL rotation until DFM; confirm the Part Placement table, not only the purple body.

Some 1206 resistors (and U2–U4) are **Extended** parts. That surcharge is normal.

---

## 7. Parts you buy yourself

For each board you actually use (start with the 2 assembled ones):

| What | Notes |
|------|--------|
| Seeed **XIAO ESP32-S3** | Socket. S3, not RP2040. |
| 2× female 1×7 2.54 mm | Only if factory skipped U1. |
| 5.08 mm 2P terminals | Only if J3/J6/J7 were DNP. |
| **MAX30102** | Pulse; wires to J4. |
| **MAX30205** | Temperature; J5. |
| **AD8232** | Lab ECG; J8. Not a hospital ECG. |
| **1S LiPo with protection** | JST to J1. Pin 1 +, pin 2 GND. |
| Panel **DPDT** + 4 wires | To **J9**. No SW1 on the PCB. |
| Battery meter (optional) | Plug into **J10**. |
| Chest/abdomen stretch bands | Terminals J6 and J7. |
| GSR electrodes | Terminal J3. |
| AD8232 electrodes | On the ECG module, not the main board. |
| M3 screws + box | Four corner holes. |

Phone BLE name: **`PinaBiosensor`**. Board silk: PinaBio v1.0.

---

## 8. Pay, wait, first power-up

Cart: **5** PCBs, **2** assembled, green, 1.6 mm. Ship to your address, pay.

### First power-up (safety)

1. **Nobody** wearing electrodes, bands, or ECG.
2. **PC USB unplugged** until you have inspected the dry board. Then you may program the XIAO over USB **with no skin**.
3. If the XIAO is not fitted: power off, insert it matching silk (USB toward the drawn edge).
4. **No** battery and **no** sensors: look for solder bridges and crooked chips.
5. USB **only** to the XIAO for a few seconds: no burning smell, no instant-hot chip. If it smells: USB out. Do not throw a LiPo in household trash.
6. Box switch wires to **J9** (4 wires). Until then analog power may not match the final design.
7. Battery **only** with protection, on **J1**, correct polarity. **Never** USB and skin together.
8. Skin session: **Bluetooth only**, computer USB **out**.

Firmware and the app are a later step. This order only builds **PinaBio v1.0** hardware.
