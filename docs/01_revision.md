# Revisión de la arquitectura original (LLM → v1.1)

La dirección del Mini (GSR + PPG + temperatura, sin ECG ni respiración) es razonable. Estos puntos **sí** había que cambiarlos antes de fabricar.

## Errores o riesgos reales

1. **Back-power por I2C.** Si el ADS1115 (y los sensores) se alimentan de V_ANALOG y los pull-up de I2C van a 3.3 V del XIAO, con el interruptor “off” y el USB conectado el micro sigue vivo y mete corriente en chips apagados por los diodos ESD de SDA/SCL. **v1.1:** ADS1115, MAX30102 y MAX30205 se alimentan de `+3V3`. V_ANALOG queda para GSR, V_REF y el MCP6004.

2. **GPIO3 es pin de strapping del ESP32-S3.** Si SW1 lo pone a GND al apagar, un reset o un USB con el interruptor off puede impedir el arranque. **v1.1:** *sleep* en **D3 / GPIO4**.

3. **4.7 µF colgado de la salida del MCP6004.** Ese op-amp no está pensado para capacitancias grandes; oscila y ensucia V_REF. **v1.1:** R5 = 47 Ω entre salida y C5; la realimentación se toma *antes* de R5.

4. **R_series 47 Ω no es protección ESD.** Frente a la piel (decenas de kΩ) no hace casi nada y no sujeta un transitorio. **v1.1:** 1 kΩ + TVS 5 V (D1) en el electrodo.

5. **Canales ADC sin usar.** Dejar AIN0/2/3 al aire capta ruido. **v1.1:** a GNDA.

6. **Condensadores del ADP150.** El datasheet pide ~1 µF en VIN y VOUT; el BYP lleva 1 nF. El filtro ferrita+10 µF va *después*.

7. **MAX30102 a V_ANALOG con GND digital.** Los pulsos del LED vuelven por el net-tie y se cuelan en el GSR. **v1.1:** VCC del módulo a `+3V3`.

8. **Pad BAT del XIAO no está en el zócalo 2×7.** Hay que un hilo del pad 15 de la PCB al pad BAT+ del módulo. Sin eso no hay carga ni batería.

9. **USB + electrodos.** No hay aislamiento de paciente. Ver `04_seguridad.md`.

10. **Guía KiCad:** “etiqueta jerárquica global” no existe; se usan **etiquetas globales** o símbolos de *power*. El net-tie tiene que estar también en el esquemático, no solo en la PCB.

## Lo que se mantuvo porque está bien pensado

- XIAO en zócalo, carga por el propio módulo, SW1 no corta la ruta de carga.
- V_REF 56k/10k → 0.50 V desde 3.3 V.
- Ganancia ×8 del ADS1115 (±0.512 V) para GSR.
- Planos GNDA/GNDD y un solo net-tie.
- Canales B/C/D del MCP6004 en seguidor a masa.
- Terminal de tornillo para GSR y pads soldados para I2C.

## Qué no pretende ser esta placa

No sustituye un diseño sanitario (IEC 60601). Es un prototipo de laboratorio personal, con corriente de GSR muy baja (~5 µA máx. si la piel fuera un cortocircuito a través de 100 kΩ @ 0.5 V).
