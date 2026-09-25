#!/usr/bin/env python3
"""PinaBio v1.1 RF: U1 al borde, keepout, buses B.Cu (X único) + F.Cu (Y único)."""
from __future__ import annotations
import sys
import pcbnew

PCB = "/workspace/hardware/PinaBiosensor_Mini.kicad_pcb"
U1_X, U1_Y = 20.0, 9.22
KEEP = (12.4, 0.40, 27.6, 6.40)
RIP = (6.8, 0.20, 34.6, 35.40)


def mm(x, y=None):
    if y is None:
        return pcbnew.FromMM(x)
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def tomm(v):
    return pcbnew.ToMM(v)


def add_keepout(board):
    for z in list(board.Zones()):
        if z.GetZoneName() == "ANT_KEEPOUT":
            board.Remove(z)
    z = pcbnew.ZONE(board)
    z.SetIsRuleArea(True)
    z.SetDoNotAllowCopperPour(True)
    z.SetDoNotAllowTracks(True)
    z.SetDoNotAllowVias(True)
    z.SetDoNotAllowPads(False)
    z.SetDoNotAllowFootprints(False)
    z.SetLayerSet(pcbnew.LSET.AllCuMask())
    z.SetZoneName("ANT_KEEPOUT")
    o = z.Outline()
    o.NewOutline()
    x0, y0, x1, y1 = KEEP
    for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
        o.Append(mm(x, y))
    board.Add(z)


def silk(board):
    x0, y0, x1, y1 = KEEP
    pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)]
    for a, b in zip(pts, pts[1:]):
        s = pcbnew.PCB_SHAPE(board)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetLayer(pcbnew.F_SilkS)
        s.SetStart(mm(*a))
        s.SetEnd(mm(*b))
        s.SetWidth(mm(0.12))
        board.Add(s)
    for text, xy, sz in (
        ("RF KEEPOUT", (20.0, 3.35), 0.8),
        ("ANTENA / NO COBRE", (20.0, 4.55), 0.6),
        ("USB-C BORDE", (7.2, 7.4), 0.6),
        ("v1.1 RF", (48.0, 21.5), 0.8),
    ):
        t = pcbnew.PCB_TEXT(board)
        t.SetText(text)
        t.SetLayer(pcbnew.F_SilkS)
        t.SetTextHeight(mm(sz))
        t.SetTextWidth(mm(sz))
        t.SetTextThickness(mm(max(0.1, sz * 0.15)))
        t.SetPosition(mm(*xy))
        board.Add(t)
    for d in board.GetDrawings():
        if isinstance(d, pcbnew.PCB_TEXT) and d.GetText() == "PINABio v1.0":
            d.SetText("PINABio v1.1")
            d.SetPosition(mm(48, 18.5))


def hit(x0, y0, x1, y1, r):
    return not (max(x0, x1) < r[0] or min(x0, x1) > r[2] or max(y0, y1) < r[1] or min(y0, y1) > r[3])


def add_track(board, x0, y0, x1, y1, net, w, layer):
    if abs(x0 - x1) < 0.03 and abs(y0 - y1) < 0.03:
        return
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(mm(x0, y0))
    t.SetEnd(mm(x1, y1))
    t.SetLayer(layer)
    t.SetWidth(mm(w))
    t.SetNetCode(net)
    board.Add(t)


def add_via(board, x, y, net):
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(mm(x, y))
    v.SetViaType(pcbnew.VIATYPE_THROUGH)
    v.SetWidth(mm(0.6))
    v.SetDrill(mm(0.3))
    v.SetNetCode(net)
    board.Add(v)


def run(board, net, w, segs):
    for s in segs:
        if s[0] == "via":
            add_via(board, s[1], s[2], net)
        else:
            add_track(board, s[1], s[2], s[3], s[4], net, w, s[0])


def main():
    board = pcbnew.LoadBoard(PCB)
    u1 = next(fp for fp in board.GetFootprints() if fp.GetReference() == "U1")
    print("old", tomm(u1.GetPosition().x), tomm(u1.GetPosition().y))
    u1.SetPosition(mm(U1_X, U1_Y))
    u1.SetOrientationDegrees(0)
    killed = 0
    for t in list(board.GetTracks()):
        if isinstance(t, pcbnew.PCB_VIA):
            x, y = tomm(t.GetStart().x), tomm(t.GetStart().y)
            if RIP[0] <= x <= RIP[2] and RIP[1] <= y <= RIP[3]:
                board.Remove(t); killed += 1
            continue
        ax, ay = tomm(t.GetStart().x), tomm(t.GetStart().y)
        bx, by = tomm(t.GetEnd().x), tomm(t.GetEnd().y)
        if hit(ax, ay, bx, by, RIP):
            board.Remove(t); killed += 1
    print("ripped", killed)
    add_keepout(board)
    silk(board)
    board.Save(PCB)
    print("saved move")


def route():
    board = pcbnew.LoadBoard(PCB)
    N = {n.GetNetname(): n.GetNetCode() for n in board.GetNetsByName().values()}
    F, B = pcbnew.F_Cu, pcbnew.B_Cu
    p1, p4, p5, p6 = (11.11, 1.60), (11.11, 9.22), (11.11, 11.76), (11.11, 14.30)
    p9, p10, p12, p14, p15 = (28.89, 14.30), (28.89, 11.76), (28.89, 6.68), (28.89, 1.60), (20.00, 20.72)

    # bus X (B.Cu vertical) and channel Y (F.Cu horizontal in ripped band)
    # VBAT_DIV -> (36.68, 13.51 B)  canal este y=19.0 (evita SCL B en y=24.7)
    n, w, bx, hy = N["VBAT_DIV"], 0.25, 7.55, 15.60
    run(board, n, w, [
        (F, *p1, bx, p1[1]), ("via", bx, p1[1]),
        (B, bx, p1[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 34.40, hy), ("via", 34.40, hy),
        (B, 34.40, hy, 36.68, hy), (B, 36.68, hy, 36.68, 13.51),
    ])
    # SLEEP_N: baja a y=16.19 en x=33.2 (oeste del bus ECG_LON)
    n, bx, hy = N["SLEEP_N"], 8.35, 23.50
    run(board, n, 0.25, [
        (F, *p4, bx, p4[1]), ("via", bx, p4[1]),
        (B, bx, p4[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 33.80, hy), ("via", 33.80, hy),
        (B, 33.80, hy, 33.80, 16.19),
        (B, 33.80, 16.19, 53.61, 16.19),
    ])
    # SDA
    n, bx, hy = N["SDA"], 9.15, 29.00
    run(board, n, 0.25, [
        (F, *p5, bx, p5[1]), ("via", bx, p5[1]),
        (B, bx, p5[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 28.22, hy), ("via", 28.22, hy),
        (B, 28.22, hy, 28.22, 30.74),
    ])
    # SCL
    n, bx, hy = N["SCL"], 9.95, 30.20
    run(board, n, 0.25, [
        (F, *p6, bx, p6[1]), ("via", bx, p6[1]),
        (B, bx, p6[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 11.11, hy), ("via", 11.11, hy),
        (B, 11.11, hy, 11.11, 25.08),
    ])
    # ECG_LOP
    n, bx, hy = N["ECG_LOP"], 30.40, 31.20
    run(board, n, 0.25, [
        (F, *p9, bx, p9[1]), ("via", bx, p9[1]),
        (B, bx, p9[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 28.89, hy), ("via", 28.89, hy),
        (B, 28.89, hy, 28.89, 25.08),
    ])
    # ECG_LON: horizontal B solo al este de x=34.5 (no cruza buses +3V3/+5V)
    n, bx, hy = N["ECG_LON"], 31.20, 22.20
    run(board, n, 0.25, [
        (F, *p10, bx, p10[1]), ("via", bx, p10[1]),
        (B, bx, p10[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 35.00, hy), ("via", 35.00, hy),
        (B, 35.00, hy, 66.73, hy),
        (B, 66.73, hy, 66.73, 22.54),
    ])
    # +3V3
    n, bx, hy = N["+3V3"], 32.00, 33.00
    run(board, n, 0.45, [
        (F, *p12, bx, p12[1]), ("via", bx, p12[1]),
        (B, bx, p12[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 26.58, hy), ("via", 26.58, hy),
        (B, 26.58, hy, 26.58, 26.35),
    ])
    # +5V
    n, bx, hy = N["+5V"], 32.80, 34.20
    run(board, n, 0.45, [
        (F, *p14, bx, p14[1]), ("via", bx, p14[1]),
        (B, bx, p14[1], bx, hy), ("via", bx, hy),
        (F, bx, hy, 9.36, hy),
        (F, 9.36, hy, 9.36, 31.91),
    ])
    # V_BATT (B.Cu para no cruzar horizontales F)
    n = N["V_BATT"]
    run(board, n, 0.45, [
        (F, *p15, 20.00, 22.80), ("via", 20.00, 22.80),
        (B, 20.00, 22.80, 20.00, 24.60), ("via", 20.00, 24.60),
    ])

    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    board.Save(PCB)
    print("routed channels")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "route":
        route()
    else:
        main()
