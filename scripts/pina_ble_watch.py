#!/usr/bin/env python3
"""Watch PinaBiosensor BLE JSON v3 on a PC (no Android)."""
import asyncio
import sys

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    print("pip install bleak")
    sys.exit(1)

NAME = "PinaBiosensor"
CHAR = "6b1d0002-5e8a-4c2f-9b3a-2c7f0e1a4d90"


def on_notify(_handle, data: bytearray):
    print(data.decode("utf-8", errors="replace"))


async def main():
    print(f"Buscando {NAME}…")
    dev = await BleakScanner.find_device_by_name(NAME, timeout=20.0)
    if not dev:
        print("No aparece. ¿XIAO encendida, cerca, DEMO_BLE o firmware v3?")
        return
    print("Conectando", dev)
    async with BleakClient(dev) as client:
        await client.start_notify(CHAR, on_notify)
        print("JSON cada ~200 ms. Ctrl+C para salir.")
        while True:
            await asyncio.sleep(1)


if __name__ == "__main__":
    asyncio.run(main())
