# SRS-000 — Visión, Alcance y Restricciones Globales

## Contexto del sistema

El repositorio **SWL2001** es la pila LoRa Basics Modem (LBM) de Semtech. La aplicación de
partida es la de geolocalización:

- App base: `lbm_applications/3_geolocation_on_lora_edge/`
- Bucle de eventos y scans GNSS: `main_geolocation/main_geolocation.c`
- API pública del módem: `lbm_lib/smtc_modem_api/smtc_modem_api.h`
- API de geolocalización/GNSS: `lbm_lib/smtc_modem_api/smtc_modem_geolocation_api.h`
- HAL de placa STM32L4 (SPI/GPIO/UART/RTC, **sin I²C**): `lbm_applications/3_geolocation_on_lora_edge/smtc_hal_l4/`

## Objetivo (una frase)

Añadir un sensor **BMP280** por **I²C** a la app de geolocalización LR1110 para que cada ciclo
envíe por LoRaWAN **datos ambientales** (temperatura y presión) **y** un **paquete GNSS**.

## Actores y hardware

- **MCU:** STM32L476 (Nucleo-L476RG).
- **Radio:** LR1110 (EVK).
- **Sensor:** Bosch **BMP280** (temperatura + presión, sin humedad; chip-id `0x58`),
  dirección I²C `0x76` (SDO a GND) o `0x77` (SDO a VDD).
- **Bus:** I²C1 del STM32L476. Pines a definir en SRS-001 evitando colisión con los del radio (SPI).

## Restricciones globales (aplican a todos los SRS)

- **RESTR-1 — Portabilidad:** el código de aplicación y de driver del sensor no puede llamar
  directamente a la ST HAL (`HAL_I2C_*`). Debe pasar por la capa `smtc_hal_i2c` (SRS-001).
- **RESTR-2 — No modificar `lbm_lib/`:** la librería del módem es intocable. Todo el trabajo
  vive en la carpeta de la aplicación (`3_geolocation_on_lora_edge/`) y en `specs/`.
- **RESTR-3 — Estilo de código:** seguir el estilo del repo — cabecera de licencia Clear BSD
  Semtech en cada `.c/.h` nuevo, guardas `#ifndef`, comentarios estilo Doxygen, snake_case.
- **RESTR-4 — Bloqueo:** las lecturas del sensor deben ser cortas (≤ pocos ms) para no romper
  el planificador del radio. Nada de `delay` largos bloqueantes en el bucle principal.
- **RESTR-5 — Sin dependencias externas:** no añadir librerías de terceros; el driver BMP280 se
  escribe en el repo (o se adapta el driver de referencia de Bosch como archivo local).
- **RESTR-6 — Trazas:** usar `SMTC_HAL_TRACE_*` (de `smtc_hal_dbg_trace.h`), no `printf`.

## Glosario

| Término | Significado |
|---------|-------------|
| **LBM** | LoRa Basics Modem (la pila de este repo) |
| **HAL** | Capa de abstracción de hardware específica de MCU/placa |
| **Uplink** | Trama LoRaWAN dispositivo → gateway |
| **fport** | Puerto de aplicación LoRaWAN (1–223) que identifica el tipo de payload |
| **GNSS scan** | Captura de satélites del LR1110 cuyo resultado se envía al solucionador de posición |
| **Ciclo** | Iteración periódica: leer sensor → scan GNSS → enviar uplinks |

## Definición de "hecho" (Definition of Done global)

El proyecto está completo cuando **todas** estas condiciones son verdad:

- [ ] SRS-001 a SRS-005 con sus checklists en PASS.
- [ ] El binario del tracker compila sin errores ni warnings nuevos con GCC ARM 13.2.1.
- [ ] El binario base de geolocalización **sigue** compilando (no hay regresión).
- [ ] Existe un decoder de payload (SRS-003) que reproduce los valores del sensor.
- [ ] Cada archivo nuevo tiene cabecera de licencia y guardas de include.
- [ ] Cada `REQ-ID` tiene su ítem correspondiente en una checklist de aceptación.

## Fuera de alcance

- Downlinks de configuración remota del periodo de muestreo.
- Escaneo Wi-Fi (solo GNSS en este proyecto).
- Gestión de energía avanzada del sensor más allá del modo forzado del BMP280.
- Soporte de radios que no sean LR1110.
