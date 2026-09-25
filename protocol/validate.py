#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sample = json.loads((ROOT / "protocol" / "ejemplo.json").read_text())
required = ["v", "ms", "gsr_uS", "t_c", "hr", "rr_ms", "ir", "batt_v", "ok", "lo", "rt_v", "ra_v", "ecg_mv"]
missing = [k for k in required if k not in sample]
if missing:
    raise SystemExit(f"faltan claves: {missing}")
if sample["v"] != 3:
    raise SystemExit("v debe ser 3")
print("protocolo v3 OK:", json.dumps(sample, separators=(",", ":")))
