#!/usr/bin/env python3
"""Place footprints, outline, netclasses and copper zones on PinaBiosensor Mini."""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.append("/usr/lib/python3/dist-packages")
import pcbnew

BOARD_W = 110.0
BOARD_H = 70.0
PCB = Path("/workspace/hardware/PinaBiosensor_Mini.kicad_pcb")

# mm, rotation degrees. Grouped by function.
PLACE = {
    "U1": (20.0, 9.22, 0),
    "J9": (55.0, 9.0, 0),
    "J10": (6.0, 26.0, 90),
    "J1": (6.5, 46.0, 90),
    "J2": (6.5, 36.0, 90),
    "U2": (46.0, 20.0, 0),
    "C1": (40.5, 20.0, 90),
    "C2": (51.5, 20.0, 90),
    "C3": (46.0, 15.0, 0),
    "FB1": (57.0, 20.0, 90),
    "C4": (62.0, 20.0, 90),
    "NT1": (42.0, 27.0, 0),
    "R1": (32.0, 32.0, 90),
    "R2": (32.0, 36.5, 90),
    "U3": (62.0, 32.0, 0),
    "C6": (55.5, 32.0, 90),
    "R3": (70.0, 26.0, 90),
    "R4": (73.5, 26.0, 90),
    "R5": (70.0, 32.0, 0),
    "C5": (75.5, 32.0, 90),
    "U4": (80.0, 16.0, 90),
    "C7": (80.0, 10.5, 0),
    "C8": (80.0, 22.0, 0),
    "R6": (85.5, 16.0, 90),
    "R7": (38.0, 40.0, 0),
    "R8": (38.0, 43.5, 0),
    "R9": (70.0, 40.0, 90),
    "R10": (73.5, 40.0, 90),
    "R11": (77.0, 40.0, 90),
    "C9": (82.0, 40.0, 90),
    "D1": (85.5, 40.0, 90),
    "J3": (18.0, 62.0, 0),
    "J6": (40.0, 62.0, 0),
    "J7": (62.0, 62.0, 0),
    "J4": (22.0, 50.0, 0),
    "C10": (22.0, 42.0, 0),
    "J5": (48.0, 50.0, 0),
    "C11": (48.0, 42.0, 0),
    "J8": (88.0, 50.0, 0),
    "R12": (70.0, 44.0, 90),
    "R13": (73.5, 44.0, 90),
    "R14": (77.0, 44.0, 90),
    "C12": (82.0, 44.0, 90),
    "D2": (85.5, 44.0, 90),
    "R15": (70.0, 48.5, 90),
    "R16": (73.5, 48.5, 90),
    "R17": (77.0, 48.5, 90),
    "C13": (82.0, 48.5, 90),
    "D3": (85.5, 48.5, 90),
    "R18": (96.0, 44.0, 90),
    "C14": (96.0, 38.0, 90),
    "H1": (3.5, 3.5, 0),
    "H2": (106.5, 3.5, 0),
    "H3": (3.5, 66.5, 0),
    "H4": (106.5, 66.5, 0),
}


def mm(x: float, y: float) -> pcbnew.VECTOR2I:
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def add_rect_edge(board: pcbnew.BOARD, w: float, h: float) -> None:
    pts = [(0, 0), (w, 0), (w, h), (0, h), (0, 0)]
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetStart(mm(x1, y1))
        seg.SetEnd(mm(x2, y2))
        seg.SetWidth(pcbnew.FromMM(0.1))
        board.Add(seg)


def add_zone(board: pcbnew.BOARD, netname: str, layer, outline, name: str) -> None:
    net = board.FindNet(netname)
    if net is None:
        print("missing net", netname)
        return
    zone = pcbnew.ZONE(board)
    zone.SetLayer(layer)
    zone.SetNetCode(net.GetNetCode())
    zone.SetLocalClearance(pcbnew.FromMM(0.2))
    zone.SetMinThickness(pcbnew.FromMM(0.2))
    zone.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    zone.SetThermalReliefGap(pcbnew.FromMM(0.3))
    zone.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.3))
    zone.SetAssignedPriority(0)
    zone.SetZoneName(name)
    cstr = zone.Outline()
    cstr.NewOutline()
    for x, y in outline:
        cstr.Append(mm(x, y))
    board.Add(zone)


def set_netclass(board: pcbnew.BOARD, name: str, width_mm: float, nets: list[str]) -> None:
    settings = board.GetDesignSettings()
    ncs = settings.m_NetSettings
    classes = ncs.GetNetclasses()
    nc = pcbnew.NETCLASS(name)
    nc.SetTrackWidth(pcbnew.FromMM(width_mm))
    nc.SetViaDiameter(pcbnew.FromMM(0.8))
    nc.SetViaDrill(pcbnew.FromMM(0.4))
    nc.SetClearance(pcbnew.FromMM(0.15))
    classes[name] = nc
    assignment = ncs.GetNetclassPatternAssignments()
    for net in nets:
        assignment[net] = name


def main() -> None:
    board = pcbnew.LoadBoard(str(PCB))
    for fp in board.GetFootprints():
        ref = fp.GetReference()
        if ref not in PLACE:
            print("unplaced", ref)
            continue
        x, y, rot = PLACE[ref]
        fp.SetPosition(mm(x, y))
        fp.SetOrientationDegrees(rot)

    # Remove existing edge cuts
    for d in list(board.GetDrawings()):
        if d.GetLayer() == pcbnew.Edge_Cuts:
            board.Remove(d)

    add_rect_edge(board, BOARD_W, BOARD_H)

    # Digital ground left/center; analog right. Overlap at net-tie.
    add_zone(
        board,
        "GNDD",
        pcbnew.F_Cu,
        [(0.4, 0.4), (58.0, 0.4), (58.0, 69.6), (0.4, 69.6)],
        "GNDD_F",
    )
    add_zone(
        board,
        "GNDD",
        pcbnew.B_Cu,
        [(0.4, 0.4), (58.0, 0.4), (58.0, 69.6), (0.4, 69.6)],
        "GNDD_B",
    )
    add_zone(
        board,
        "GNDA",
        pcbnew.F_Cu,
        [(52.0, 0.4), (109.6, 0.4), (109.6, 69.6), (52.0, 69.6)],
        "GNDA_F",
    )
    add_zone(
        board,
        "GNDA",
        pcbnew.B_Cu,
        [(52.0, 0.4), (109.6, 0.4), (109.6, 69.6), (52.0, 69.6)],
        "GNDA_B",
    )

    try:
        set_netclass(board, "Power", 0.45, ["V_BATT", "+3V3", "+5V", "V_ANALOG", "LDO_VIN", "LDO_OUT"])
        set_netclass(board, "Analog", 0.25, [
            "V_REF", "VREF_DIV", "GSR_MID", "GSR_AIN", "GSR_ELEC",
            "TH_MID", "TH_AIN", "TH_ELEC", "AB_MID", "AB_AIN", "AB_ELEC",
            "ECG_OUT", "ECG_AIN", "ECG_LOP", "ECG_LON",
        ])
        set_netclass(board, "I2C", 0.25, ["SDA", "SCL"])
    except Exception as exc:
        print("netclass skipped:", exc)

    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
    board.Save(str(PCB))
    print("Laid out", PCB)


if __name__ == "__main__":
    main()
