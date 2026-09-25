# Guía de KiCad para Principiantes — PinaBiosensor Mini

Esta guía asume que **no has usado KiCad nunca**, pero que sí entiendes conceptos de electrónica (aunque estén algo oxidados). No es un manual genérico de KiCad: está escrita paso a paso para diseñar exactamente la placa descrita en `PinaBiosensor_Mini_Arquitectura_Hardware.md`.

---

## 1. Conceptos previos (el "vocabulario" de KiCad)

Antes de abrir el programa, cuatro ideas que necesitas tener claras:

- **Esquemático vs. PCB**: son dos vistas distintas del mismo circuito. El **esquemático** es el dibujo conceptual con símbolos (como los que usabas en la carrera: resistencias, chips, líneas de conexión). La **PCB** es el diseño físico real: rectángulos de cobre (pads), pistas, y la forma física de la placa. KiCad mantiene ambas sincronizadas.
- **Símbolo vs. Footprint (huella)**: el símbolo es el dibujo esquemático de un componente (por ejemplo, un rectángulo con patillas para el ADS1115). El footprint es la geometría física que se solda en la PCB (los pads reales, con sus medidas en milímetros). Un mismo símbolo puede tener varios footprints posibles (por ejemplo, el mismo chip en encapsulado DIP o SOIC).
- **Red (Net)**: un conjunto de pines que están eléctricamente conectados entre sí (por ejemplo, todos los puntos que forman "GNDA" son una sola red). Cuando dibujas una línea entre dos pines en el esquemático, estás diciendo "estos pines pertenecen a la misma red".
- **ERC y DRC**: dos verificadores automáticos. **ERC** (Electrical Rules Check) revisa el esquemático (pines sin conectar, conflictos de dirección de señal). **DRC** (Design Rules Check) revisa la PCB física (pistas demasiado juntas, espaciados no fabricables). Ejecutar ambos antes de dar por bueno un diseño es obligatorio, no opcional.

---

## 2. Instalación

1. Descarga KiCad (versión 8.x o superior) desde **kicad.org** — hay instalador para Windows, macOS y Linux.
2. Durante la instalación, acepta también instalar las **librerías estándar** (símbolos, footprints y modelos 3D) — es la opción por defecto, no la desmarques.
3. Abre KiCad. Verás el **Gestor de Proyectos** (KiCad Project Manager) — es la ventana desde la que se crean y organizan los proyectos.

---

## 3. Crear el proyecto

1. Gestor de Proyectos → `Archivo` → `Nuevo Proyecto`.
2. Nombra el proyecto, por ejemplo `PinaBiosensor_Mini`, y elige una carpeta. KiCad creará varios archivos (`.kicad_pro`, `.kicad_sch`, `.kicad_pcb`) — no los edites a mano, siempre desde el programa.
3. Haz doble clic en el icono de **Editor de Esquemáticos** dentro del proyecto. Empezamos aquí.

---

## 4. Librerías que vas a necesitar

Todas las librerías estándar de KiCad vienen instaladas, pero dos componentes de este proyecto no están en las librerías oficiales y hay que añadirlos como **librerías de terceros**:

- **XIAO ESP32-S3**: busca en internet "Seeed XIAO ESP32S3 KiCad library" — Seeed Studio publica una librería oficial en GitHub con símbolo y footprint del zócalo `Conn_01x07` ya orientado correctamente.
- **MAX30102 (módulo/breakout)**: si usas un módulo comercial (breakout) en vez del chip desnudo, busca "MAX30102 breakout KiCad footprint" — normalmente solo necesitas un footprint de 4 pads en línea, no hace falta el símbolo exacto del fabricante: puedes usar un símbolo genérico de 4 pines (`Conn_01x04`) y renombrar los pines a VCC/GND/SDA/SCL.
- **MAX30205**: igual que el anterior, si usas el chip en SOIC-8 tienes footprint estándar en la librería `Package_SO`; si usas un breakout, aplica el mismo truco del `Conn_01x04`.

**Cómo añadir una librería de terceros** (lo harás 1-2 veces):
1. Descarga el archivo `.zip` del repositorio y descomprímelo.
2. En el Editor de Esquemáticos: `Preferencias` → `Gestionar Símbolos` → `Añadir librería existente` → apunta al archivo `.kicad_sym`.
3. Repite lo mismo para footprints en `Gestionar Footprints`, apuntando al archivo `.pretty` (una carpeta con extensión `.pretty`).

---

## 5. Dibujar el esquemático, bloque a bloque

Trabaja siguiendo el documento de arquitectura, bloque por bloque, ejecutando ERC después de cada uno. Es mucho más fácil depurar 5 errores que 50.

### 5.1 Colocar el XIAO
- Herramienta "Colocar símbolo" (icono de chip con un `+`, o tecla `A`). Busca el símbolo del XIAO ESP32-S3.
- Colócalo en el centro del lienzo. Es el punto de referencia del resto del circuito.

### 5.2 Alimentación
- Coloca símbolos de "power flags" para las redes globales (`VBAT`, `+3V3`, `V_ANALOG`, `GNDA`, `GNDD`): en KiCad, en vez de dibujar cables largos por todo el esquemático, se usan **etiquetas globales** (`Colocar` → `Etiqueta jerárquica global`, o el atajo con la tecla que asignes) con el mismo nombre de red — dos pines con la misma etiqueta global están conectados aunque no tengan ninguna línea dibujada entre ellos. Es la forma limpia de manejar redes de alimentación.
- Coloca el LDO ADP150 (búscalo por nombre en el buscador de símbolos), la ferrita, el condensador de 10µF, y conéctalos según el documento de arquitectura: V_BATT → LDO → filtro LC → V_ANALOG.
- Divisor del monitor de batería: dos resistencias de 47kΩ en serie entre V_BATT y GND, con el punto medio a una etiqueta local que luego unirás al pin A0 del XIAO.
- Divisor V_REF: resistencias de 56kΩ y 10kΩ desde V_ANALOG, punto medio a la entrada no inversora del canal A del MCP6004.

### 5.3 MCP6004 (op-amp)
- Coloca el símbolo del MCP6004 (viene en la librería `Amplifier_Operational`).
- Canal A: entrada no inversora = punto medio del divisor V_REF; realimentación directa de salida a entrada inversora (configuración seguidor); condensador de 4.7µF de la salida a GNDA.
- Canales B, C, D (no usados): entrada no inversora a GNDA, realimentación de salida a entrada inversora igual que el canal A (ver §3.4 del documento de arquitectura — es obligatorio, no opcional, para evitar oscilaciones).
- No olvides los pines de alimentación del propio chip (V+ a V_ANALOG, V- a GNDA) — es un error muy común olvidar alimentar el símbolo del op-amp porque "parece" que solo tiene los pines de señal.

### 5.4 ADS1115
- Símbolo en librería `Analog_ADC` o similar (búscalo por nombre "ADS1115").
- `ADDR` → V_ANALOG (fija dirección `0x49`).
- `SDA`/`SCL` → bus I2C, con las resistencias pull-up de 2.2kΩ hacia el 3.3V digital del XIAO.
- `AIN1` → nodo de salida del divisor de GSR (ver siguiente bloque). Los demás canales (`AIN0`, `AIN2`, `AIN3`) quedan sin conectar — esto SÍ está permitido en un ADC (a diferencia del op-amp), pero ERC probablemente te avisará con un "warning" de pin sin conectar: es esperado, puedes marcarlo como "no conectado intencionadamente" con el símbolo de flag correspondiente (`Colocar` → `Marcador de no conexión`, una X pequeña) para que ERC no lo repita en cada verificación.

### 5.5 Circuito de GSR
- Resistencia `R_FIXED` (100kΩ) desde V_REF (salida del buffer del MCP6004) hasta el Nodo_MID.
- Desde el Nodo_MID: `R_limit` (470Ω) hacia `AIN1` del ADS1115.
- `R_series` (47Ω) desde el Nodo_MID hacia el terminal de tornillo de los electrodos.
- `C_shunt` (100nF) entre el terminal de tornillo y GNDA.
- Símbolo del terminal de tornillo: busca `Conn_01x02` con footprint de terminal de tornillo (lo verás también como "Terminal Block" en las librerías de footprints, `TerminalBlock_...`).

### 5.6 MAX30102 y MAX30205
- Para cada uno: símbolo `Conn_01x04` (o el símbolo específico si usaste la librería del fabricante), pines renombrados VCC/GND/SDA/SCL.
- VCC → V_ANALOG, GND → GNDD, SDA/SCL → mismo bus I2C que el ADS1115.
- Direcciones I2C (0x57 y 0x48) no se "dibujan" en el esquemático — son fijas de fábrica en estos sensores, no hace falta ningún pin de configuración de dirección como sí ocurría con el ADS1115.

### 5.7 Interruptor SW1 y bloque de apagado
- Símbolo de interruptor de doble contacto (o dos interruptores simples, `SW_Push` o `SW_SPST`, si no encuentras uno de doble contacto integrado — puedes representarlo con dos símbolos independientes que se accionan físicamente a la vez).
- Contacto 1: en serie en la línea V_BATT → entrada del LDO.
- Contacto 2: entre GPIO3 del XIAO y GNDD, con una resistencia de pull-up hacia 3.3V (o usa el pull-up interno del ESP32-S3 activado por firmware, como hace el `.ino` con `INPUT_PULLUP` — en ese caso no necesitas la resistencia física).

### 5.8 J_CHARGE y J_BATT
- Igual que en la versión completa: conectores directos a los pads BAT del XIAO, sin pasar por el interruptor.

---

## 6. Ejecutar ERC

`Inspeccionar` → `Verificación de reglas eléctricas`. Revisa la lista de errores/warnings uno a uno. Los más habituales para principiantes:
- **Pin de alimentación sin conducir**: olvidaste conectar V+ o V- de algún chip (op-amp, sensores).
- **Pin de entrada flotante**: alguna entrada de señal quedó sin conectar y sin marcar como "no conectada intencionadamente".

No pases a la PCB hasta que ERC esté limpio (0 errores; los warnings de "no conectado intencionadamente" que marcaste tú mismo son aceptables).

---

## 7. Asignar Footprints

`Herramientas` → `Asignar Footprints`. Para cada símbolo del esquemático, eliges qué footprint físico le corresponde. Usa la lista de la sección §4 de este documento y la tabla del BOM de la arquitectura hardware como referencia. Para las resistencias/condensadores normales, usa el tamaño SMD `0805` (buen equilibrio entre "soldable a mano" y "no ocupa demasiado sitio") salvo que prefieras `1206` si tu pulso no está muy entrenado todavía — es perfectamente válido y más cómodo para volver a soldar después de 30 años.

---

## 8. Pasar a la PCB

1. Abre el **Editor de PCB** desde el gestor de proyectos.
2. `Herramientas` → `Actualizar PCB desde Esquemático`. Verás aparecer todos los footprints amontonados en una esquina, unidos por líneas finas amarillas ("ratsnest") que representan las conexiones pendientes de enrutar.
3. Define el **contorno de la placa**: capa `Edge.Cuts`, dibuja un rectángulo (o la forma que prefieras) con la herramienta de líneas.
4. Distribuye los footprints dentro del contorno, agrupando por bloque funcional: zona de alimentación cerca del XIAO y el LDO; zona analógica (MCP6004, ADS1115, GSR) agrupada y algo alejada del XIAO; pads de sensores I2C cerca del borde de la placa (para que el cable salga cómodo).

---

## 9. Planos de masa (GNDA / GNDD)

1. `Colocar` → `Zona` (o el icono de polígono relleno).
2. Dibuja el polígono cubriendo la zona analógica, asígnalo a la red `GNDA`.
3. Repite para `GNDD` en la zona digital.
4. Entre ambas zonas, coloca el **Net-Tie** (búscalo en footprints como `NetTie-2_...`) cerca del LDO y el XIAO, tal como indica el documento de arquitectura.
5. Recuerda rellenar las zonas (`B` o el botón de "rellenar todas las zonas") para que el cobre se genere realmente — hasta que no lo haces, el plano es solo un contorno vacío.

---

## 10. Ancho de pistas (Net Classes)

`Archivo` → `Configuración de la Placa` → `Clases de Red`. Crea (o edita la clase `Default`) tres clases con los anchos indicados en el documento de arquitectura:
- Alimentación: 0.4–0.5 mm
- Analógicas: 0.25 mm
- I2C: 0.25 mm

Luego, en el esquemático o en la PCB, asigna cada red a su clase correspondiente (`Archivo` → `Configuración de la Placa` → `Clases de Red` → pestaña de asignación de redes por patrón de nombre, por ejemplo `V_*` a la clase de alimentación).

---

## 11. Enrutar las pistas

Herramienta de ruta (tecla `X` para pistas normales). Sigue el ratsnest amarillo. Consejos:
- Enruta primero alimentación y GND (ya cubierto por los planos, así que en realidad son pocas pistas explícitas).
- Mantén las pistas I2C cortas, paralelas entre sí, y sobre el plano GNDD.
- Aleja las pistas analógicas (Nodo_MID, V_REF) de las pistas I2C, tal como indica el documento de arquitectura — el I2C conmuta a alta frecuencia y puede acoplar ruido en la señal de GSR si van paralelas y cercanas.

---

## 12. Verificación final: DRC

`Inspeccionar` → `Verificación de reglas de diseño`. Revisa espaciados mínimos, pistas sin conectar, y solapamientos. No envíes la placa a fabricar hasta que DRC esté limpio.

---

## 13. Generar los ficheros de fabricación (Gerbers)

1. `Archivo` → `Fabricación` → `Gerbers`.
2. Selecciona todas las capas de cobre, máscara de soldadura, serigrafía y `Edge.Cuts`.
3. Genera también el archivo de taladros (`Drill Files`).
4. Comprime todo en un `.zip` — es lo que subirás directamente a un fabricante de PCBs (JLCPCB, PCBWay, etc.).

Con esto tienes el ciclo completo: esquemático → footprints → PCB → planos → rutado → verificación → fabricación, aplicado paso a paso a esta placa concreta.
