#!/usr/bin/env python3
"""v1.1: lista UUIDs de pistas en la zona del XIAO (sin board.Remove, que rompe SWIG)."""
import json
import pcbnew

PCB = "/workspace/hardware/PinaBiosensor_Mini.kicad_pcb"
RIP = (6.8, 0.20, 34.6, 35.40)


def tomm(v):
    return pcbnew.ToMM(v)


def hit(x0, y0, x1, y1, r):
    return not (max(x0, x1) < r[0] or min(x0, x1) > r[2] or max(y0, y1) < r[1] or min(y0, y1) > r[3])


board = pcbnew.LoadBoard(PCB)
kill = []
for t in board.GetTracks():
    if isinstance(t, pcbnew.PCB_VIA):
        x, y = tomm(t.GetPosition().x), tomm(t.GetPosition().y)
        if RIP[0] <= x <= RIP[2] and RIP[1] <= y <= RIP[3]:
            kill.append(t.m_Uuid.AsString())
        continue
    ax, ay = tomm(t.GetStart().x), tomm(t.GetStart().y)
    bx, by = tomm(t.GetEnd().x), tomm(t.GetEnd().y)
    if hit(ax, ay, bx, by, RIP):
        kill.append(t.m_Uuid.AsString())
print("kill", len(kill))
open("/tmp/kill_uuids.json", "w").write(json.dumps(kill))
