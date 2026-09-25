# KiCad — guía para Alejandro (cero experiencia)

No vas a “diseñar la placa desde un folio en blanco”. Eso ya está hecho en este repositorio.

La guía vieja de otros LLM (`uploads/PinaBiosensor_Mini_Guia_KiCad_Principiantes_…`) te decía **crear un proyecto nuevo y dibujar**. **No hagas eso.** Tiene errores (pin de apagado, alimentación de sensores, nombres de menús). Aquí abres **lo que ya hay** y solo miras / compruebas.

---

## Qué te genero yo (en cristiano)

Imagina tres papeles del mismo circuito:

| Archivo | Qué es | Tú qué haces |
|---|---|---|
| `hardware/PinaBiosensor_Mini.kicad_pro` | La carpeta-proyecto. El “índice”. | Doble clic. Es lo único que abres al empezar. |
| `…kicad_sch` | El **esquemático**. Hoy es un **mapa en texto** (párrafos), no el dibujo de simbolitos de la carrera. | Lo lees. No es donde se fabrica. |
| `…kicad_pcb` | La **placa de verdad**: cobre, agujeros, sitio de cada pieza. | Aquí pasas el rato. |
| `…net` | Lista de “este pin va con este pin”. | No lo edites a mano. |
| `hardware/footprints.pretty/` | Formas de soldadura raras (zócalo XIAO, pads de los cables). | KiCad las usa solo. |
| `hardware/sym-lib/` | Símbolos nuestros. | Igual. |
| `bom/BOM.csv` | Lista de la compra (qué pieza, cuántas). | Para pedir a JLCPCB / DigiKey. |
| `exports/gerbers/` | Archivos que entiende la fábrica. | **No los subas aún** si el DRC no está limpio. |
| `exports/PinaBiosensor_Mini_pcb.svg` | Dibujo de la placa para verla en el navegador. | Echar un vistazo sin KiCad. |

**Qué no te estoy dando todavía (importante):**

- Un esquemático “bonito” con resistencias dibujadas y cables. Eso se puede hacer después, para que lo entiendas mejor. **La fábrica mira la PCB, no ese dibujo.**
- Una PCB **lista para pagar** sin que nadie mire el DRC. Las pistas las tendió un programa; pueden cruzarse o quedar justas. **No pidas 5 placas el primer día sin esa revisión.**

Si más adelante quieres, el siguiente encargo de KiCad sería: 1) dibujar el esquemático de verdad, 2) limpiar pistas hasta DRC = 0 errores, 3) Gerbers nuevos + PDF para ti.

---

## 0. Instalar KiCad (una vez)

1. Entra en [https://www.kicad.org/download/](https://www.kicad.org/download/).
2. Windows: descarga el instalador. **KiCad 8** (o 9) vale. El proyecto se hizo con formato 7; 8 lo abre.
3. Instala. Cuando pregunte por **librerías / footprints / 3D**, déjalo todo marcado. Si las quitas, faltan piezas.
4. Abre **KiCad**. Verás una ventana gris con iconos grandes: eso es el **gestor de proyectos**. No pulses “Nuevo proyecto”.

---

## 1. Abrir ESTE proyecto

1. En el gestor: **Archivo → Abrir proyecto**.
2. Ve a la carpeta del repo → `hardware` → `PinaBiosensor_Mini.kicad_pro` → Abrir.
3. A la izquierda salen los archivos del proyecto. Los dos que importan:
   - hoja / esquemático
   - placa / PCB

Si KiCad pide “migrar” o “guardar como versión nueva”: acepta y guarda **dentro de `hardware/`**, no en Documentos.

---

## 2. El esquemático (solo lectura)

1. Doble clic en el esquemático.
2. Verás **texto**. Es un resumen del circuito, no el dibujo clásico.
3. Zoom: rueda del ratón. Mover: rueda pulsada o clic medio.
4. No hace falta que dibujes nada. **Cierra** cuando te hayas leído el mapa (alimentación, GSR, I2C, XIAO).

No pulses “Añadir símbolo” ni “Hilo”. Si lo haces sin querer: **Archivo → Revertir** o cierra sin guardar.

---

## 3. La PCB (aquí está la placa)

1. En el gestor, doble clic en **PCB** (`PinaBiosensor_Mini.kicad_pcb`).
2. Verás un rectángulo (~9 × 6 cm) con pads y rayas.
3. Si las zonas de cobre (masas) se ven vacías o raras: pulsa la tecla **`B`** (rellenar zonas). Es normal.
4. Zoom y mover: igual que antes.

### Capas (el “sándwich”)

Arriba a la derecha hay casillas de capas. Para no marearte, deja visibles sobre todo:

- **F.Cu** — cobre de arriba (pistas)
- **B.Cu** — cobre de abajo
- **F.SilkS** — texto blanco (nombres J1, USB…)
- **Edge.Cuts** — el contorno de la placa

Si lo ves todo negro, desmarca capas raras (Adhesive, Courtyard) hasta que se lea.

### Vista 3D (la más útil para ti)

Menú **Ver → Visor 3D** (o Alt+3).

Ahí ves la placa como objeto. Gira con el ratón. Comprueba a ojo:

- El **USB del XIAO** hacia **fuera**, en un borde, no contra el centro.
- En **v1.1** hay un recuadro **RF KEEPOUT** junto al USB: no debe haber cobre bajo el conector USB. El BLE del ESP32-S3 va por el **U.FL + cable** (hacia `BAT+`), no por una cerámica en ese borde.
- **J1** batería: hay sitio para el conector; no está debajo del XIAO.
- **J3** tornillo GSR: en un borde, fácil de cablear.
- Dos grupos de **4 pads** (MAX30102 y MAX30205) con un agujero al lado (brida del cable).

Cierra el visor 3D. Sigues en la PCB.

---

## 4. No romper nada (manos quietas)

| Si pulsas sin querer | Qué pasa | Qué hacer |
|---|---|---|
| Clic en una pieza y arrastras | Se mueve el componente | `Ctrl+Z` ahora mismo |
| Tecla `M` | Mover | Esc, luego `Ctrl+Z` |
| Tecla `X` | Empiezas una pista | Esc |
| `Supr` | Borras | `Ctrl+Z` |

Regla de oro: **si no estás corrigiendo un error de DRC, no guardes.** Mira, `Ctrl+Z`, cierra.

---

## 5. El DRC (el examen antes de pagar)

Esto es “¿la fábrica puede hacer esta placa sin cortocircuitos?”.

1. Menú **Inspeccionar → Verificación de reglas de diseño** (DRC).
2. Dale a **Ejecutar DRC** (o similar).
3. Saldrá una lista. **Errores** = mal. **Avisos** = a veces se pueden vivir.

Qué significan, en cristiano:

- **Clearance / cortocircuito:** dos coppers se tocan y no deberían. Malo.
- **Unconnected / no conectado:** un pad que debería ir a una red se quedó en el aire. Malo.
- **Courtyard overlap:** dos piezas se pisan el “espacio personal”. A veces aceptable, a veces no.

**Hoy es normal que salgan errores.** Las pistas las tiró un script. No significa que el circuito esté mal pensado; significa que **aún no está limpio para JLCPCB**.

Anota cuántos errores hay (captura de pantalla). Eso es lo que hay que limpiar **antes** de pedir placas. Si quieres, en el siguiente paso lo hacemos juntos (tú pegas la lista o una captura).

No intentes “arreglar 40 pistas” el primer día. Es el trabajo más ingrato de KiCad.

---

## 6. Qué debes mirar tú (aunque no sepas KiCad)

Con el visor 3D y el silkscreen (texto blanco):

1. **USB** del zócalo XIAO hacia el borde de la placa.
2. Hay un **pad extra BAT+** junto al zócalo. En la vida real soldarás un **hilo corto** de ese pad al pad BAT de debajo del XIAO (el zócalo de 14 pines **no** lleva la batería).
3. **J1** dice BAT+ y GND. El conector JST no se puede meter al revés si es el bueno; aun así, **rojo = positivo**.
4. El borne GSR **lejos** del XIAO es mejor (menos ruido). Si están pegados, se puede vivir en un prototipo.
5. Los dos sensores I2C **no van soldados en esta placa**: van con cable a esos 4 pads.

---

## 7. Fabricación (solo cuando DRC esté limpio)

**Todavía no.** Cuando lo esté:

1. En PCB: **Archivo → Fabricación → Gerbers** (y taladros / drill).
2. O usas `exports/gerbers/` **regenerados ese día**, no unos viejos.
3. En JLCPCB subes el ZIP y **abres su visor Gerber**. Tienes que ver el contorno, los agujeros y las pistas. Si ves un rectángulo vacío, el ZIP está mal.
4. Pedido típico que hablamos: **5 placas**, **2 montadas** (SMD, incluido ADS1115), **3 vírgenes**. Tú sueldas zócalo, JST, interruptor, borne y cables.

---

## 8. Lo que no tienes que aprender ahora

- Crear librerías.
- Dibujar un esquemático desde cero.
- El auto-enrutador.
- Python / SKiDL (`scripts/generate_kicad.py`). Eso es cómo **yo** regenero el diseño; tú no lo necesitas para abrir KiCad.

---

## 9. Si algo sale raro

- “No encuentro footprints Pina”: abre el proyecto desde `PinaBiosensor_Mini.kicad_pro`, no arrastrando solo el `.kicad_pcb`.
- Todo blanco: tecla `B`; activa F.Cu y B.Cu.
- KiCad en inglés: **Preferences → Language → Spanish**, reinicia.
- Has tocado y no sabes qué: cierra **sin guardar** y vuelve a abrir.

---

## Resumen en una frase

Yo te genero el **proyecto KiCad ya relleno** (placa + lista de piezas + mapa). Tú instalas KiCad, **abres**, miras en **3D**, corres el **DRC** y no pides cobre hasta que esa lista de errores esté vacía. El dibujo “de carrera” del esquemático es un extra, no el primer paso.
