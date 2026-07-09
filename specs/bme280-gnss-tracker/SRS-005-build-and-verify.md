# SRS-005 — Compilación del binario y verificación E2E

## Objetivo

Producir el binario del tracker (BME280 + GNSS, radio LR1110) y verificar de forma reproducible
que satisface el proyecto, sin regresión sobre el binario base.

## Herramientas (ya presentes en el entorno)

- GCC ARM: `arm-none-eabi-gcc` **13.2.1** (recomendada por el repo).
- `make`, `cmake`, `ninja`.

## Requisitos funcionales

- **REQ-005-1 — Build del tracker:** debe compilar desde la carpeta de la aplicación de geolocalización.
  Comando de referencia (ajustar al target real de esa app):
  ```bash
  make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110
  ```
  Resultado: un `.elf` y `.bin` en el directorio de build de la app.
- **REQ-005-2 — Sin regresión:** el binario **base** de geolocalización (sin sensor) debe seguir
  compilando; si la integración se hizo sobre una copia/target nuevo, verificar ambos.
- **REQ-005-3 — Sin warnings nuevos:** la compilación no introduce warnings adicionales respecto al
  base (comparar salida). Warnings preexistentes del stack se ignoran.
- **REQ-005-4 — Credenciales:** documentar que DevEUI/JoinEUI/AppKey se ponen en
  `main_geolocation/example_options.h` antes de un flasheo real; el build no debe hardcodear secretos en `specs/`.
- **REQ-005-5 — Verificación estática mínima (sin hardware):**
  - `nm`/`grep` confirma que `bme280_read`, `bme280_payload_encode`, `smtc_hal_i2c_read_buffer`
    están enlazados en el `.elf` (no eliminados por el linker).
  - Todos los símbolos `smtc_modem_*` referenciados resuelven contra `lbm_lib`.
- **REQ-005-6 — Verificación E2E (con hardware, guía manual):** documentar en un
  `docs/verification.md` los pasos: conectar BME280 al I²C de la Nucleo, flashear (`ninja flash` /
  `make flash`), observar por la UART de traza (`SMTC_HAL_TRACE_*`):
  1. join correcto,
  2. `bme280_init` == OK y chip-id 0x60,
  3. uplink fport 10 con 7 bytes cada `SENSOR_UPLINK_PERIOD_S`,
  4. eventos `GNSS_SCAN_DONE`/`GNSS_TERMINATED` y su uplink,
  5. el Network Server decodifica el fport 10 con `docs/payload_decoder.js`.

## Entregables

- Binario `.bin`/`.elf` del tracker.
- `docs/verification.md` con el procedimiento anterior y una tabla PASS/FAIL.

## Checklist de aceptación

- [ ] **A1** El build del tracker termina con código 0 y genera `.elf` + `.bin` (REQ-005-1).
- [ ] **A2** El build base de geolocalización sigue en verde (REQ-005-2).
- [ ] **A3** Diff de warnings = 0 nuevos (REQ-005-3).
- [ ] **A4** `arm-none-eabi-nm <elf> | grep -E "bme280_read|bme280_payload_encode|smtc_hal_i2c_read_buffer"` devuelve símbolos (REQ-005-5).
- [ ] **A5** Sin secretos reales commiteados en `specs/` ni en el decoder (REQ-005-4).
- [ ] **A6** `docs/verification.md` existe con los 5 pasos E2E (REQ-005-6).
- [ ] **A7** (Con HW, opcional pero recomendado) evidencia de log UART mostrando join + uplink fport 10 + GNSS.

## Nota de cierre para el agente

Cuando A1–A6 estén en PASS, marca la **Definición de "hecho" global** de SRS-000. El paso A7
(hardware) queda como verificación de campo del usuario si no hay banco de pruebas disponible.
