#!/usr/bin/env python3
"""Borra bloques (segment)/(via) cuyos uuid están en /tmp/kill_uuids.json."""
import json
import re
from pathlib import Path

PCB = Path("/workspace/hardware/PinaBiosensor_Mini.kicad_pcb")
kill = set(json.loads(Path("/tmp/kill_uuids.json").read_text()))
text = PCB.read_text()
n = len(text)
i = 0
out = []
removed = 0
while i < n:
    if text.startswith("\t(segment", i) or text.startswith("\t(via", i):
        depth = 0
        j = i
        while j < n:
            if text[j] == "(":
                depth += 1
            elif text[j] == ")":
                depth -= 1
                if depth == 0:
                    j += 1
                    if j < n and text[j] == "\n":
                        j += 1
                    break
            j += 1
        block = text[i:j]
        uu = re.search(r'\(uuid "([^"]+)"\)', block)
        if uu and uu.group(1) in kill:
            removed += 1
            i = j
            continue
        out.append(block)
        i = j
        continue
    out.append(text[i])
    i += 1
PCB.write_text("".join(out))
print("removed blocks", removed, "of", len(kill))
