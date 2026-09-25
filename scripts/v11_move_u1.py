#!/usr/bin/env python3
"""Mueve U1, keepout RF, seda v1.1. No borra pistas (SWIG)."""
import pcbnew

PCB = "/workspace/hardware/PinaBiosensor_Mini.kicad_pcb"
U1_X, U1_Y = 20.0, 9.22
KEEP = (12.4, 0.40, 27.6, 6.40)


def mm(x, y=None):
    if y is None:
        return pcbnew.FromMM(x)
    return pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y))


def tomm(v):
    return pcbnew.ToMM(v)


board = pcbnew.LoadBoard(PCB)
u1 = next(fp for fp in board.GetFootprints() if fp.GetReference() == "U1")
print("old", tomm(u1.GetPosition().x), tomm(u1.GetPosition().y))
u1.SetPosition(mm(U1_X, U1_Y))
u1.SetOrientationDegrees(0)

for z in list(board.Zones()):
    if z.GetZoneName() == "ANT_KEEPOUT":
        board.Remove(z)  # hopefully few zones, might still break... skip if issues

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

pcbnew.ZONE_FILLER(board).Fill(board.Zones())
board.Save(PCB)
print("saved U1", U1_X, U1_Y)
