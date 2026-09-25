#!/usr/bin/env python3
"""Naive Manhattan router so the board is electrically connected (zones handle GND)."""
from __future__ import annotations

import sys
from collections import defaultdict

sys.path.append("/usr/lib/python3/dist-packages")
import pcbnew

PCB = "/workspace/hardware/PinaBiosensor_Mini.kicad_pcb"

SKIP = {"GNDA", "GNDD"}
WIDTH = {
    "V_BATT": 0.45,
    "+3V3": 0.45,
    "+5V": 0.45,
    "V_ANALOG": 0.45,
    "LDO_VIN": 0.45,
    "LDO_OUT": 0.45,
}


def add_seg(board, start, end, netcode, width):
    if start.x == end.x and start.y == end.y:
        return
    t = pcbnew.PCB_TRACK(board)
    t.SetStart(start)
    t.SetEnd(end)
    t.SetLayer(pcbnew.F_Cu)
    t.SetWidth(pcbnew.FromMM(width))
    t.SetNetCode(netcode)
    board.Add(t)


def manhattan(board, a, b, netcode, width):
    mid = pcbnew.VECTOR2I(b.x, a.y)
    add_seg(board, a, mid, netcode, width)
    add_seg(board, mid, b, netcode, width)


def main():
    board = pcbnew.LoadBoard(PCB)
    by_net = defaultdict(list)
    for fp in board.GetFootprints():
        for pad in fp.Pads():
            name = pad.GetNetname()
            if not name or name in SKIP:
                continue
            by_net[name].append(pad.GetPosition())

    # drop existing tracks
    for t in list(board.GetTracks()):
        board.Remove(t)

    for name, pts in by_net.items():
        if len(pts) < 2:
            continue
        net = board.FindNet(name)
        if net is None:
            continue
        w = WIDTH.get(name, 0.25)
        pts = sorted(pts, key=lambda p: (p.x, p.y))
        for a, b in zip(pts, pts[1:]):
            manhattan(board, a, b, net.GetNetCode(), w)

    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
    board.Save(PCB)
    print("routed nets:", len(by_net))


if __name__ == "__main__":
    main()
