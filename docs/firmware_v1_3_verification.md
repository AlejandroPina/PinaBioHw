# Firmware 1.3 verification / Verificación del firmware 1.3

## English

**Static checks in this repository review (2026-09-28):** the V1.3 sketch has balanced C++ delimiters in a lexer that skips comments and strings; the pin/address constants, BLE name, Wi-Fi-off call, ADS GSR PGA bits, CRC-16/CCITT-FALSE vectors and frame/MTU arithmetic remain consistent with V1.2. All new explicit I²C mutex takes have a matching give on the acquired path. V1.2 source remains in the tree. `git diff --check` passes. These checks do **not** compile the sketch.

**MAX30102 rate readback:** the code reads register `0x0A`, sample-rate bits 4:2, and register `0x09`, SHDN bit 7, after both an apply and a rollback. The mapping for 100/200/400 SPS is 1/2/3. This mapping was checked against the [MAX30102 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX30102.pdf) and [SparkFun library source](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/blob/master/src/MAX30105.cpp). A failed readback leaves the result unverified; it is never counted as a successful rollback.

**Still required before use:** compile with a pinned Arduino-ESP32 core, Seeed XIAO ESP32-S3 board definition and SparkFun MAX3010x library. On the fabricated PCBA, inject or observe failed rate writes/readbacks, verify STOP → `SET PPG_RATE` → START, check status and measured PPG SPS, test a failed rollback and the resulting START/sleep refusal, exercise START concurrently with a sleep request, measure ECG SPS after the ADS polling change, and check power consumption. No build, flash or PCBA test was performed in this workspace.

**Fabrication archive:** the current GitHub remote contains only `main`. The original ZIP was recovered from a local archive, copied byte-for-byte to `exports/PinaBio_v1_gerbers.zip`, and its SHA-256 `80ad8ffbf5c0c934477c3629876153e2ca1a08734c8c796fbc24ed5f0dff62f7` matches `exports/archive_v1.0/SHA256SUMS`. The committed BOM/CPL bytes match that same manifest. This review did not regenerate gerbers or establish that the loose `exports/gerbers/` match the paid order.

## Español

**Comprobaciones estáticas de esta revisión (28-09-2026):** delimitadores C++ equilibrados con analizador que ignora comentarios y cadenas; constantes de pines/direcciones, nombre BLE, Wi-Fi apagado, bits PGA GSR, vectores CRC-16/CCITT-FALSE y aritmética frame/MTU coherentes con V1.2. Cada nueva toma explícita del mutex I²C tiene su liberación cuando se adquiere. El código V1.2 sigue en el árbol. `git diff --check` pasa. Estas pruebas **no** compilan el sketch.

**Lectura de tasa MAX30102:** se leen el registro `0x0A`, bits 4:2 de tasa, y `0x09`, bit SHDN 7, tanto después de aplicar como al revertir. La correspondencia 100/200/400 SPS es 1/2/3, contrastada con el [datasheet MAX30102](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX30102.pdf) y el [código SparkFun](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/blob/master/src/MAX30105.cpp). Si falla la lectura, la reversión no se considera verificada.

**Pendiente antes del uso:** compilar con versiones fijadas de Arduino-ESP32, placa Seeed XIAO ESP32-S3 y librería SparkFun MAX3010x. En la PCBA fabricada, provocar u observar fallos de escritura/lectura de tasa, probar STOP → `SET PPG_RATE` → START, comprobar STATUS y SPS PPG medidos, verificar que un rollback fallido bloquea START/sleep, ensayar START junto con una solicitud sleep, medir SPS ECG tras el cambio de sondeo ADS y consumo. Aquí no se compiló ni flasheó ni se probó la PCBA.

**Archivo de fabricación:** el remoto GitHub actual solo tiene `main`. Se recuperó el ZIP original de un archivo local y se copió sin modificar a `exports/PinaBio_v1_gerbers.zip`. Su SHA-256 `80ad8ffbf5c0c934477c3629876153e2ca1a08734c8c796fbc24ed5f0dff62f7` coincide con `exports/archive_v1.0/SHA256SUMS`, igual que los bytes BOM/CPL guardados en Git. Esta revisión no regeneró los gerbers ni demostró que los sueltos de `exports/gerbers/` coincidan con el pedido pagado.
