# Changelog — Firmware v1.1 para Paca v1.0

- Corregido PGA de GSR a ±0,512 V (`0x0800`) y conversión RAW→V correspondiente.
- Corregidos cálculos de frames a cabecera de 13 bytes y CRC de 2; evento y telemetría ahora miden correctamente 13+texto+2 y 24 bytes de payload.
- ECG/PPG inspeccionan la cola sin consumirla; solo descartan muestras tras una solicitud de envío válida o escritura USB completa. Un ring lleno descarta la muestra nueva y aumenta drops.
- Inicialización explícita de la biblioteca MAX3010x con `begin`; contadores separados de overflow FIFO hardware detectable y pérdida del buffer interno de la biblioteca.
- `String` en callback BLE, BLE/HRS serializados bajo un mismo mutex.
- START/STOP incrementan generación de sesión; ADC, PPG y telemetría descartan trabajo terminado después de un cambio de generación. Estado y configuración compartidos se protegen en las rutas de adquisición y comandos.
- Modo USB texto/binario independiente; se evita mezclar diagnóstico/JSON/respuestas con frames.
- Timestamps internos de 64 bits; V1.1 conserva campo de 32 bits relativo a START en el frame. PPG marca tiempo estimado de FIFO.
- DIAG inicial establece línea base; STATUS informa objetivos, tasas medidas, colas, pérdidas, errores y contadores.
- Sleep para la adquisición, envía evento final, suspende tareas, apaga PPG y desinicializa BLE antes del deep sleep. Comando SLEEP funciona sin tener que mantener GPIO4 bajo.
- Configuración guardada incluye flags de stream y transporte. AUTO_ECG comienza apagado: ECG tiene objetivo inicial real de 250 SPS; boost puede subir a 500 según presupuesto.
- Se conserva únicamente el hardware V1 fabricado, nombre BLE y UUID existentes; Wi-Fi se apaga en setup.

**Límites conocidos:** no hay ACK de aplicación para BLE notify. El contador de overflow FIFO puede subestimar pérdidas si el contador hardware se satura. La tasa real ADS/PPG y la estabilidad dependen de la PCBA. No se ha hecho compilación ni prueba física en esta entrega.
