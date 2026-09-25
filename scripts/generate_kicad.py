#!/usr/bin/env python3
"""Generate KiCad netlist and unplaced PCB for PinaBio v1.0 (historical filenames Mini)."""
from __future__ import annotations

import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HW = ROOT / "hardware"

os.environ["KICAD7_SYMBOL_DIR"] = "/usr/share/kicad/symbols"
os.environ["KICAD7_FOOTPRINT_DIR"] = "/usr/share/kicad/footprints"
os.environ["KICAD_SYMBOL_DIR"] = "/usr/share/kicad/symbols"

from skidl import (  # noqa: E402
    KICAD7,
    ERC,
    Net,
    Part,
    generate_netlist,
    generate_pcb,
    lib_search_paths,
    set_default_tool,
)
set_default_tool(KICAD7)
lib_search_paths[KICAD7].insert(0, str(HW / "sym-lib"))
lib_search_paths[KICAD7].append("/usr/share/kicad/symbols")

FP_R = "Resistor_SMD:R_1206_3216Metric"
FP_C = "Capacitor_SMD:C_1206_3216Metric"
FP_FB = "Inductor_SMD:L_0805_2012Metric"
FP_TVS = "Diode_SMD:D_SOD-323"
FP_HOLE = "MountingHole:MountingHole_3.2mm_M3"
FP_TB = "TerminalBlock_MetzConnect:TerminalBlock_MetzConnect_Type101_RT01602HBWC_1x02_P5.08mm_Horizontal"
FP_JST = "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal"


def resistor(value: str, ref: str) -> Part:
    p = Part("Device", "R", footprint=FP_R)
    p.value = value
    p.ref = ref
    return p


def cap(value: str, ref: str) -> Part:
    p = Part("Device", "C", footprint=FP_C)
    p.value = value
    p.ref = ref
    return p


def tvs(ref: str) -> Part:
    d = Part("Device", "D_TVS")
    d.ref, d.value, d.footprint = ref, "PESD5V0S1UL", FP_TVS
    return d


def body_terminal(ref: str, value: str, elec: Net, gnda: Net) -> None:
    j = Part("Connector_Generic", "Conn_01x02")
    j.ref, j.value, j.footprint = ref, value, FP_TB
    j[1] += elec
    j[2] += gnda


def build() -> None:
    vbat = Net("V_BATT")
    v5 = Net("+5V")
    v3v3 = Net("+3V3")
    vanalog = Net("V_ANALOG")
    ldo_in = Net("LDO_VIN")
    ldo_out = Net("LDO_OUT")
    vref = Net("V_REF")
    vref_div = Net("VREF_DIV")
    gnda = Net("GNDA")
    gndd = Net("GNDD")
    sda = Net("SDA")
    scl = Net("SCL")
    mid = Net("GSR_MID")
    elec = Net("GSR_ELEC")
    ain1 = Net("GSR_AIN")
    th_mid = Net("TH_MID")
    th_elec = Net("TH_ELEC")
    th_ain = Net("TH_AIN")
    ab_mid = Net("AB_MID")
    ab_elec = Net("AB_ELEC")
    ab_ain = Net("AB_AIN")
    ecg_out = Net("ECG_OUT")
    ecg_ain = Net("ECG_AIN")
    lo_p = Net("ECG_LOP")
    lo_n = Net("ECG_LON")
    vbat_div = Net("VBAT_DIV")
    sleep = Net("SLEEP_N")
    alert = Net("ADS_ALERT")

    xiao = Part("Pina", "XIAO_ESP32S3")
    xiao.ref = "U1"
    xiao.footprint = "Pina:XIAO_ESP32S3_Socket"
    xiao["1"] += vbat_div
    xiao["4"] += sleep
    xiao["5"] += sda
    xiao["6"] += scl
    xiao["9"] += lo_p   # D8
    xiao["10"] += lo_n  # D9
    xiao["12"] += v3v3
    xiao["13"] += gndd
    xiao["14"] += v5
    xiao["15"] += vbat
    for p in ("2", "3", "7", "8", "11"):
        xiao[p].do_erc = False

    j_batt = Part("Connector_Generic", "Conn_01x02")
    j_batt.ref, j_batt.value, j_batt.footprint = "J1", "LiPo_JST-PH", FP_JST
    j_batt[1] += vbat
    j_batt[2] += gndd

    j_meter = Part("Connector_Generic", "Conn_01x02")
    j_meter.ref, j_meter.value, j_meter.footprint = "J10", "BATT_METER", FP_JST
    j_meter[1] += vbat
    j_meter[2] += gndd

    j_chg = Part("Connector_Generic", "Conn_01x02")
    j_chg.ref, j_chg.value, j_chg.footprint = "J2", "5V_CHARGE", FP_JST
    j_chg[1] += v5
    j_chg[2] += gndd

    # Panel DPDT in the box (replaces onboard SW1). 4 used pins:
    # 1=LDO_VIN  2=V_BATT  4=GND  5=SLEEP_N  (same nets as old SW1)
    sw = Part("Pina", "SW_DPDT_ONOFF")
    sw.ref = "J9"
    sw.value = "INT_CAJA_DPDT"
    sw.footprint = "Button_Switch_THT:SW_CK_JS202011CQN_DPDT_Straight"
    sw["2"] += vbat
    sw["1"] += ldo_in
    sw["3"].do_erc = False
    sw["5"] += sleep
    sw["6"].do_erc = False
    sw["4"] += gndd

    ldo = Part("Pina", "ADP150-3.3")
    ldo.ref = "U2"
    ldo.footprint = "Package_TO_SOT_SMD:SOT-23-5"
    ldo["VIN"] += ldo_in
    ldo["EN"] += ldo_in
    ldo["GND"] += gndd
    ldo["VOUT"] += ldo_out

    cin = cap("1uF", "C1"); cin[1] += ldo_in; cin[2] += gndd
    cout = cap("1uF", "C2"); cout[1] += ldo_out; cout[2] += gndd
    cbyp = cap("1nF", "C3"); cbyp[1] += ldo["BYP"]; cbyp[2] += gndd

    fb = Part("Device", "FerriteBead")
    fb.ref, fb.value, fb.footprint = "FB1", "600R@100MHz", FP_FB
    fb[1] += ldo_out
    fb[2] += vanalog
    clc = cap("10uF", "C4"); clc[1] += vanalog; clc[2] += gnda

    nt = Part("Device", "NetTie_2")
    nt.ref, nt.footprint = "NT1", "NetTie:NetTie-2_SMD_Pad2.0mm"
    nt[1] += gnda
    nt[2] += gndd

    r1 = resistor("47k 1%", "R1"); r1[1] += vbat; r1[2] += vbat_div
    r2 = resistor("47k 1%", "R2"); r2[1] += vbat_div; r2[2] += gndd

    r3 = resistor("56k 0.1%", "R3"); r3[1] += vanalog; r3[2] += vref_div
    r4 = resistor("10k 0.1%", "R4"); r4[1] += vref_div; r4[2] += gnda

    mcp = Part("Amplifier_Operational", "MCP6004")
    mcp.ref = "U3"
    mcp.footprint = "Package_SO:SOIC-14_3.9x8.7mm_P1.27mm"
    mcp["4"] += vanalog
    mcp["11"] += gnda
    # A: V_REF buffer
    mcp["3"] += vref_div
    r5 = resistor("47", "R5")
    mcp["1"] += r5[1]
    mcp["2"] += mcp["1"]
    r5[2] += vref
    c5 = cap("4.7uF", "C5"); c5[1] += vref; c5[2] += gnda
    # B: thorax band buffer
    mcp["5"] += th_mid
    mcp["6"] += mcp["7"]
    # C: abdomen band buffer
    mcp["10"] += ab_mid
    mcp["9"] += mcp["8"]
    # D: unused, tied
    mcp["12"] += gnda
    mcp["13"] += mcp["14"]
    c6 = cap("100nF", "C6"); c6[1] += vanalog; c6[2] += gnda

    ads = Part("Analog_ADC", "ADS1115IDGS")
    ads.ref = "U4"
    ads.footprint = "Package_SO:TSSOP-10_3x3mm_P0.5mm"
    ads["VDD"] += v3v3
    ads["GND"] += gnda
    ads["ADDR"] += v3v3
    ads["SCL"] += scl
    ads["SDA"] += sda
    ads["AIN0"] += ecg_ain
    ads["AIN1"] += ain1
    ads["AIN2"] += th_ain
    ads["AIN3"] += ab_ain
    ads["ALERT/RDY"] += alert
    r6 = resistor("10k", "R6"); r6[1] += v3v3; r6[2] += alert
    c7 = cap("100nF", "C7"); c7[1] += v3v3; c7[2] += gnda
    c8 = cap("1uF", "C8"); c8[1] += v3v3; c8[2] += gnda

    r7 = resistor("2.2k", "R7"); r7[1] += v3v3; r7[2] += sda
    r8 = resistor("2.2k", "R8"); r8[1] += v3v3; r8[2] += scl

    # GSR
    r9 = resistor("100k", "R9"); r9[1] += vref; r9[2] += mid
    r10 = resistor("470", "R10"); r10[1] += mid; r10[2] += ain1
    r11 = resistor("1k", "R11"); r11[1] += mid; r11[2] += elec
    body_terminal("J3", "GSR", elec, gnda)
    c9 = cap("100nF", "C9"); c9[1] += elec; c9[2] += gnda
    d1 = tvs("D1"); d1[1] += elec; d1[2] += gnda

    # Thorax stretch: 47k from 0.5 V, rubber to GNDA
    r12 = resistor("47k 1%", "R12"); r12[1] += vref; r12[2] += th_mid
    r13 = resistor("470", "R13"); r13[1] += mcp["7"]; r13[2] += th_ain
    r14 = resistor("1k", "R14"); r14[1] += th_mid; r14[2] += th_elec
    body_terminal("J6", "RESP_THORAX", th_elec, gnda)
    c12 = cap("100nF", "C12"); c12[1] += th_elec; c12[2] += gnda
    d2 = tvs("D2"); d2[1] += th_elec; d2[2] += gnda

    r15 = resistor("47k 1%", "R15"); r15[1] += vref; r15[2] += ab_mid
    r16 = resistor("470", "R16"); r16[1] += mcp["8"]; r16[2] += ab_ain
    r17 = resistor("1k", "R17"); r17[1] += ab_mid; r17[2] += ab_elec
    body_terminal("J7", "RESP_ABDOMEN", ab_elec, gnda)
    c13 = cap("100nF", "C13"); c13[1] += ab_elec; c13[2] += gnda
    d3 = tvs("D3"); d3[1] += ab_elec; d3[2] += gnda

    j_ppg = Part("Connector_Generic", "Conn_01x04")
    j_ppg.ref, j_ppg.value = "J4", "MAX30102"
    j_ppg.footprint = "Pina:SensorCable_4P_Anchor"
    j_ppg[1] += v3v3
    j_ppg[2] += gndd
    j_ppg[3] += sda
    j_ppg[4] += scl
    c10 = cap("100nF", "C10"); c10[1] += v3v3; c10[2] += gndd

    j_tmp = Part("Connector_Generic", "Conn_01x04")
    j_tmp.ref, j_tmp.value = "J5", "MAX30205"
    j_tmp.footprint = "Pina:SensorCable_4P_Anchor"
    j_tmp[1] += v3v3
    j_tmp[2] += gndd
    j_tmp[3] += sda
    j_tmp[4] += scl
    c11 = cap("100nF", "C11"); c11[1] += v3v3; c11[2] += gndd

    j_ecg = Part("Connector_Generic", "Conn_01x06")
    j_ecg.ref, j_ecg.value = "J8", "AD8232"
    j_ecg.footprint = "Pina:SensorCable_6P_Anchor"
    j_ecg[1] += vanalog
    j_ecg[2] += gnda
    j_ecg[3] += ecg_out
    j_ecg[4] += lo_p
    j_ecg[5] += lo_n
    j_ecg[6] += vanalog  # SDN high while analog rail is on
    r18 = resistor("1k", "R18"); r18[1] += ecg_out; r18[2] += ecg_ain
    c14 = cap("100nF", "C14"); c14[1] += ecg_ain; c14[2] += gnda

    for ref in ("H1", "H2", "H3", "H4"):
        h = Part("Mechanical", "MountingHole", footprint=FP_HOLE)
        h.ref = ref
        h.value = "M3"


def main() -> None:
    os.chdir(HW)
    build()
    ERC()
    generate_netlist(file_=str(HW / "PinaBiosensor_Mini.net"))
    fp_libs = [
        str(HW / "footprints.pretty"),
        "/usr/share/kicad/footprints",
    ]
    generate_pcb(
        file_=str(HW / "PinaBiosensor_Mini.kicad_pcb"),
        fp_libs=fp_libs,
    )
    print("Wrote hardware files to", HW)


if __name__ == "__main__":
    main()
