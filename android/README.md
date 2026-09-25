# App Android — PinaBio v1.0

Nombre en pantalla: **PinaBio**. Paquete `com.pinabiosensor.mini` (no cambiar).

BLE: busca `PinaBiosensor`. JSON v3. HRV en el teléfono.

## Probar ahora (sin placa de fábrica)

1. En Android Studio abre la carpeta `android/`.
2. Instala en un teléfono real (el emulador casi nunca tiene BLE).
3. **Demo en vivo**: datos de mentira a 5 Hz, mismas gráficas.
4. **Escanear BLE**: XIAO con este firmware, USB **sin** electrodos en la piel.
   - Para probar BLE **sin** ADS/MAX: en el `.ino` pon `#define DEMO_BLE 1`, sube, luego vuelve a `0` para la PCB real.

PC: `pip install bleak` y `python3 scripts/pina_ble_watch.py`.

USB del PC nunca con electrodos.
