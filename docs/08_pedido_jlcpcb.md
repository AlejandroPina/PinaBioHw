[English](08_jlcpcb_order.md) | [Español](#)

# Guía de pedido JLCPCB — PinaBio v1.0 (Paca)

Para: **Alejandro**. No hace falta KiCad. No abras ni edites los archivos de fabricación.

Producto: **PinaBio v1.0** (apodo **Paca**, homenaje a la abuela de Alejandro). En el teléfono, el Bluetooth sigue llamándose **`PinaBiosensor`** (no lo cambies).

Hay una placa **v1.1** (USB del XIAO al borde) para **un pedido más adelante**. Para **este** pedido usa **`PinaBio_v1_*`**, no `PinaBio_v1.1_*`. Ver `docs/10_pinabio_v1.1_rf.md`.

Los archivos de fabricación están en la rama **`kicad-completa`**. Una copia en OneDrive puede estar desactualizada.

---

## 0. Qué tendrás al final

Un solo pedido. La fábrica fabrica **5 placas** y suelda componentes en **2** de ellas.

- **2 placas ensambladas** (con resistencias, condensadores, chips y conectores soldados por la fábrica).
- **3 placas vírgenes** (solo el circuito impreso verde, sin piezas). Sirven de recambio o para practicar.

Eso **no** incluye el módulo XIAO, los sensores MAX/AD8232, la batería, las bandas ni el interruptor de la caja. Esos los compras tú (apartado 7).

Esto **no** es un aparato médico. No diagnostica. Con electrodos o bandas en la piel: **solo Bluetooth**. El USB del PC **nunca** a la vez que la piel.

---

## 1. Dónde están los 3 archivos (Windows)

Necesitas **exactamente 3 archivos**. Nombres actuales (septiembre 2026):

| Para qué | Nombre exacto | Dónde (Windows) |
|----------|----------------|-----------------|
| Cobre / taladros / seda (la placa) | `PinaBio_v1_gerbers.zip` | `exports\PinaBio_v1_gerbers.zip` |
| Lista de piezas para la fábrica | `PinaBio_v1_bom_jlc.csv` | `exports\PinaBio_v1_bom_jlc.csv` |
| Dónde va cada pieza | `PinaBio_v1_cpl.csv` | `exports\PinaBio_v1_cpl.csv` |

Hay **copias idénticas** con el nombre antiguo «Completa» (es **la misma placa**):

- `exports\PinaBiosensor_Completa_gerbers.zip`
- `exports\PinaBiosensor_Completa_bom_jlc.csv`
- `exports\PinaBiosensor_Completa_cpl.csv`

Usa preferiblemente los de **PinaBio_v1_***. Si ya empezaste un presupuesto con los Completa, no mezcles: sigue con el mismo trío.

**No uses** la carpeta `exports\gerbers\` con archivos `PinaBiosensor_Mini-...`. Eso es **otra placa** (Mini), no PinaBio v1.0.

### Aviso importante: carpeta / rama equivocada

Estos tres archivos **no están** en la versión principal (`main`) del proyecto. En `main` solo hay gerbers sueltos de la Mini.

Tienes que abrir la versión **`kicad-completa`**. Si en Cursor solo ves `main`, **no vas a encontrar** `PinaBio_v1_gerbers.zip`.

### Si tienes Cursor en el PC y la carpeta del proyecto abierta

1. En el Explorador de Windows (o en el árbol de archivos de Cursor, a la izquierda), entra en la carpeta del proyecto.
2. Abre **`exports`**.
3. Copia estos tres a Escritorio o Descargas, para tenerlos a mano:
   - `PinaBio_v1_gerbers.zip`
   - `PinaBio_v1_bom_jlc.csv`
   - `PinaBio_v1_cpl.csv`

### Si usas el navegador: cursor.com/codebase

1. Abre el navegador y entra en **https://cursor.com/codebase**.
2. Abre el proyecto **PinaBioHw** (el nombre puede verse como PinaBio / PinaBioHw).
3. Arriba o a un lado, si ves un selector de versión/rama, elige **`kicad-completa`**. Si no aparece y solo ves `main`, **para**: esa vista no tiene el ZIP bueno.
4. Entra en la carpeta **`exports`**.
5. Descarga los tres archivos de la tabla. En Cursor web suele haber un botón de **descargar** al pulsar el archivo o al pasar el ratón (icono de flecha hacia abajo). Guárdalos en **Descargas**.
6. Si ves un botón **Download** / **Descargar** de todo el proyecto, úsalo, descomprime el ZIP grande en el PC, y luego entra en `exports\` y saca los tres archivos.

### Si estás en Origin (la web del código)

Si ves un botón **Download** o **Download ZIP**:

1. Asegúrate de estar viendo **`kicad-completa`**, no `main`.
2. Pulsa **Download** / **Download ZIP**.
3. Descomprime el archivo en el PC (clic derecho → Extraer todo).
4. Entra en `exports\` y copia los tres archivos `PinaBio_v1_*`.

### Lo que no debes hacer

- **No abras KiCad.** No hace falta.
- **No descomprimas** `PinaBio_v1_gerbers.zip` para “arreglar” nada. A JLCPCB se sube **el ZIP entero**.
- **No edites** los CSV en Excel si te pide “convertir a números” o cambia comas. Si los abres, **no guardes**. Mejor copiarlos tal cual.

---

## 2. Registrarse en JLCPCB

1. Abre **https://jlcpcb.com** en el navegador (Chrome o Edge).
2. Arriba a la derecha pulsa **Sign up** / **Register** (Registrarse).
3. Crea cuenta con correo. Confirma el correo si te lo piden.
4. Vuelve a **https://jlcpcb.com** e inicia sesión (**Log in**).

Idioma: arriba suele haber un selector. Puedes dejarlo en inglés; abajo se indica el texto del botón.

---

## 3. Subir gerbers y revisar el visor

1. En la portada, pulsa **Quote now** (a veces **Instant Quote**). Te lleva a la página de pedido.
2. En la zona grande de subir archivos, pulsa o arrastra **`PinaBio_v1_gerbers.zip`**.
3. Espera a que termine de leer el ZIP (unos segundos). Sale un visor de la placa.

**Lista de comprobación en el visor (antes de seguir):**

- Tamaño aproximado **110 × 70 mm** (11 cm × 7 cm). Si pone 50×30 o similar, **has subido la Mini**: cancela y sube `PinaBio_v1_gerbers.zip` de `kicad-completa`.
- En la seda (texto blanco) debe verse **J9** (interruptor de caja, 4 pines) y **J10** (medidor de batería, 2 pines).
- **No debe haber SW1** (no hay interruptor deslizante en la placa). El interruptor va en la **caja**, cableado a J9.
- Forma rectangular, 4 agujeros de tornillo en las esquinas.
- Texto **PINA** / **PINABio v1.0** (no pone «Paca» en la placa: no cabe).

Si el visor se ve al revés o raro, no edites el ZIP. Revisa que subiste el archivo correcto. Si 110×70 y J9/J10 están, sigue.

Pulsa **Next** / **Confirm** cuando el visor esté bien.

---

## 4. Opciones de la placa (PCB)

Deja lo que JLCPCB detecte si coincide con esto. Si no, cámbialo a mano:

| Campo (inglés) | Pon esto |
|----------------|----------|
| **PCB Qty** (cantidad) | **5** |
| Layers (capas) | **2** |
| PCB Thickness | **1.6 mm** |
| PCB Color | **Green** (verde) |
| Surface Finish | **HASL** (estaño; vale **LeadFree HASL**). ENIG (oro) es más caro y no hace falta. |
| Copper Weight | **1 oz** (si sale) |
| Remove Mark / Order Number | puedes dejar que pongan el número de pedido en la placa (no molesta) |

No actives panel (varias placas unidas) ni agujeros chapados raros. Placa **suelta**, cantidad **5**.

**No pagues todavía.** El ensamblaje se elige en la misma página, más abajo.

---

## 5. Ensamblaje (PCBA): sí, 2 placas

1. Baja hasta **PCB Assembly**.
2. Activa el interruptor (**ON** / Yes).
3. **PCBA Qty** / Assembly Qty: pon **2** (solo 2 de las 5 se sueldan).
4. Lado: **Top side** (todo va arriba).
5. Empieza por **Economic**. Si la web **bloquea** conectores de agujero (THT: JST, bornes, zócalo) o dice que Economic no admite esos pines, cambia a **Standard** y sigue.
6. Pulsa **Confirm** / **Next**.

Tooling holes / confirmación de taladros: acepta lo que recomiende la fábrica si el visor de la placa sigue viéndose 110×70.

Si un ingeniero (p. ej. Lulu) dice que el Gerber mide **110×80** y el pedido **110×70**: responde que la **placa acabada es 110×70 mm**. Los **80 mm** son el **panel** con rieles de ~5 mm a cada lado (5+70+5) y las líneas V-CUT a 5 mm y 75 mm. **No** pidas que fabriquen una placa de 80 mm de alto.

---

## 5b. Dónde ver el diseño que van a fabricar

No está en Cursor ni en el correo de Lulu. Está en tu cuenta:

1. Entra en **https://jlcpcb.com** e inicia sesión.
2. Arriba a la derecha, tu cuenta → **Order History** (historial). Directo: **https://jlcpcb.com/user-center/orders**
3. Abre el pedido **SMT026091261919**.

Ahí verás, según el momento:

- **View PCB** / visor Gerber / **Order details** — la placa (cobre y seda). Debe medir **110 × 70**, texto **PINABio v1.0**, **J9** y **J10**, **sin SW1**.
- **Confirm production file** / **Download production file** — el archivo **después** de que el ingeniero lo prepare. Las fotos pequeñas de esa página **no bastan** (lo dice JLCPCB). Descarga el ZIP, descomprímelo y mira la carpeta **`ok`** (no `yg`: eso es lo que subiste tú). El recorte de placa es la capa **`ko`**: **110 × 70**, no 80.
- Piezas soldadas: en el mismo pedido, visor **SMT** / **Part Placement** / ensamblaje. Revisa **U2 U3 U4** contra el triángulo de seda.

Si el estado ya dice **confirmed production file** / revisado, el diseño final es ese visor + la carpeta **`ok`**. Si aún sale un botón **Yes, please proceed to production**, no lo pulses hasta ver **110 × 70** y los chips bien.

El dibujo rojo **V-CUT 80 mm** del correo es el **panel** (rieles), no la Paca.

---

## 6. Subir BOM + CPL, emparejar, DNP, polaridad

1. En **BOM file** sube `PinaBio_v1_bom_jlc.csv`.
2. En **CPL** / **CPL file** / Pick and Place sube `PinaBio_v1_cpl.csv`.
3. Pulsa **Process BOM&CPL** (o **Next** si ya procesa solo).

### Emparejado

La web rellena códigos de almacén (números tipo C1848). Revisa que casi todas las líneas tengan pieza.

Dos líneas del BOM **van en blanco a propósito** (las emparejas a mano o las marcas para no soldar si no hay pieza clara):

- **U1** — zócalo del XIAO (no el módulo). Busca 2 tiras hembra 1×7, paso 2,54 mm. Candidato de ejemplo: **C2932672** (comprueba en la ficha que es hembra 2,54 mm). Si no estás seguro, marca U1 como **no soldar** y lo sueldas tú en casa.
- **J3, J6, J7** — bornes de tornillo 2 pines, paso **5,08 mm**. Candidato: **C475082** (KF128R-5.08-2P). **Compara la foto** con el hueco de la placa. Si el cuerpo no encaja, **no sueldes en fábrica**; cómpralos aparte.

Si una línea sale en rojo (sin stock): pulsa **Search**, elige otra pieza **igual de valor y tamaño 1206** (o el encapsulado que ponga), o marca **Do not place** / no soldar esa referencia y la compras tú.

### Qué SÍ debe soldar la fábrica

Resistencias, condensadores, ferrita FB1, diodos D1 D2 D3, chips **U2 U3 U4**, conectores **J1 J2 J9 J10**, y si el servicio lo permite: zócalo **U1** y bornes **J3 J6 J7**.

### Qué NO debe soldar (DNP)

Marca **Do not place** / no ensamblar si la web los propone:

- Módulo **XIAO** (solo el zócalo, nunca el cerebro soldado a pelo si quieres poder cambiarlo).
- Módulos **MAX30102**, **MAX30205**, **AD8232**.
- Pads **J4, J5, J8** (solo cobre; no hay conector de fábrica).
- **SW1** (no existe en esta placa).
- Agujeros **H1–H4** y el puente de cobre **NT1** (no son piezas).

### Polaridad (visor de componentes)

Antes de pagar, abre el visor de piezas montadas. Mira sobre todo:

- **U2** (regulación 3,3 V, 5 patas): el triángulo de seda es pad 1 (lado de 3 patas). En el CPL, U2 va a **180°** (el PCB en KiCad sigue a 0°): misma corrección LCSC que U3; el morado debe coincidir con el triángulo, no con el lado de 2 patas.
- **U3** (chip largo 14 patas): muesca o pata 1 al mismo lado que el dibujo. En el CPL, U3 ya va a **180°** (el PCB en KiCad sigue a 0°): el pin1 del modelo LCSC es opuesto al triángulo de seda; en Part Placement el morado debe coincidir con el triángulo. **No vuelvas U3 a 0°.**
- **U4** (chip pequeño 10 patas): igual, pata 1. **No gires U4.**
- **D1 D2 D3**: orientación del diodo según la seda.
- Conectores JST: el enganche del plástico hacia el borde, como en el dibujo.

Si una pieza está girada 180°, **no pagues**: en la tabla de CPL suele haber un ángulo; dímelo y se corrige el archivo. No gires “a ojo” en Excel.

Varias resistencias 1206 son **Extended** (un recargo de carga de carrete). Es normal. U2, U3 y U4 también son Extended.

Pulsa **Next** hasta el resumen de precio.

---

## 7. Piezas que compras tú (no van en este pedido de fábrica)

Para **cada** placa que quieras usar de verdad (empiezas con las 2 ensambladas):

| Qué | Notas |
|-----|--------|
| Seeed **XIAO ESP32-S3** (el módulo) | Se clava en el zócalo. Comprueba que es S3, no RP2040. |
| Zócalo 2× hembra 1×7 2,54 mm | Solo si la fábrica **no** soldó U1. |
| Bornes 5,08 mm 2P | Solo si J3/J6/J7 no salieron de fábrica. |
| Módulo **MAX30102** | Pulso; cables a pads J4. |
| Módulo **MAX30205** | Temperatura; pads J5. |
| Módulo **AD8232** | ECG de laboratorio; pads J8. **No** es un electrocardiógrafo de hospital. |
| Batería **LiPo 1S** **con placa de protección** | Conector JST hacia J1. Polaridad: pin 1 +, pin 2 masa. |
| Interruptor **DPDT de panel** + 4 hilos | A **J9**. No hay SW1 en la placa. |
| Medidor de batería (opcional) | Enchufe en **J10**. |
| Bandas de goma resistivas pecho y abdomen | A bornes J6 y J7. |
| Electrodos GSR | A borne J3. |
| Electrodos del AD8232 | Al módulo ECG, no a la placa grande. |
| Tornillos M3 + caja | Los 4 agujeros de esquina. |

El Bluetooth en el móvil se llama **PinaBiosensor**. En la placa pone PinaBio v1.0.

---

## 8. Pagar, esperar, y el primer encendido

1. Revisa el carrito: **5** placas, **2** ensambladas, color verde, 1,6 mm.
2. Elige envío (el barato tarda más; el rápido cuesta más).
3. Dirección de España / la tuya. Paga (tarjeta).
4. Espera el correo de producción. Si la fábrica escribe por un conector o un chip sin stock, no improvises: responde o pregunta antes de aceptar un recambio raro.
5. Al llegar: **2** con piezas y **3** desnudas.

### Primer encendido (seguridad)

1. **Nadie** con electrodos, bandas ni ECG puestos.
2. **USB del PC desconectado** hasta que hayas mirado la placa en seco. Luego puedes programar el XIAO por USB **sin piel**.
3. Si el XIAO no viene puesto: apaga todo, clávalo en el zócalo **en el sentido de la seda** (USB hacia el borde que indica el dibujo).
4. **Sin** batería y **sin** sensores: mira que no hay estaño uniendo pistas, ni chips torcidos.
5. Enchufa **solo USB** al XIAO, unos segundos: no debe oler a quemado ni calentarse un chip al instante. Si huele mal: USB fuera, no tires la placa a la basura normal (batería aparte, punto limpio).
6. El interruptor de caja se cablea a **J9** (4 hilos). Hasta entonces la parte analógica puede no encenderse como en el diseño final.
7. Batería **solo** con protección, en **J1**, polaridad correcta. No USB y piel a la vez **nunca**.
8. Sesión con piel: **solo Bluetooth**, USB del ordenador **fuera**.

Este pedido solo fabrica el hardware de **PinaBio v1.0**. El firmware vigente está documentado aparte en `firmware/`.
