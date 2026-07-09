# SRS-004 — Integración en la app (sensor + GNSS en el bucle de eventos)

## Objetivo

Integrar la lectura del BME280 y el envío de su payload dentro del bucle de eventos existente de
la app de geolocalización, **manteniendo** el flujo de scan GNSS intacto.

## Archivos que debes leer primero

- `lbm_applications/3_geolocation_on_lora_edge/main_geolocation/main_geolocation.c`
  (bucle `smtc_modem_get_event` / `switch` de eventos; ~487 líneas).
- API de eventos y uplink: `lbm_lib/smtc_modem_api/smtc_modem_api.h`
  (`smtc_modem_request_uplink`, `smtc_modem_get_event`, `smtc_modem_alarm_start_timer`).
- API GNSS: `lbm_lib/smtc_modem_api/smtc_modem_geolocation_api.h`
  (`smtc_modem_gnss_scan`, `smtc_modem_gnss_set_port`).

## Anclajes reales del código (no inventar)

En `main_geolocation.c` ya existen estos manejadores dentro del `switch (current_event.event_type)`:

- `SMTC_MODEM_EVENT_JOINED` — tras el join.
- `SMTC_MODEM_EVENT_ALARM` — temporizador periódico (aquí se hace un uplink keep-alive).
- `SMTC_MODEM_EVENT_GNSS_SCAN_DONE` — resultado del scan disponible.
- `SMTC_MODEM_EVENT_GNSS_TERMINATED` — reprograma el siguiente scan con `smtc_modem_gnss_scan(...)`.
- `SMTC_MODEM_EVENT_TXDONE` — uplink transmitido.

## Requisitos funcionales

- **REQ-004-1 — Init del sensor:** en el arranque (evento `RESET`/`JOINED`, antes del primer uplink)
  llamar `smtc_hal_i2c_init(...)` y `bme280_init(...)`. Si `bme280_init` falla, registrar un
  `SMTC_HAL_TRACE_WARNING` y continuar (el nodo sigue haciendo GNSS aunque no haya sensor).
- **REQ-004-2 — Muestreo + uplink ambiental:** en el manejador de `SMTC_MODEM_EVENT_ALARM`
  (o donde hoy se hace el keep-alive), sustituir/añadir:
  1. `bme280_read(&data)`,
  2. `bme280_payload_encode(&data, buf)`,
  3. `smtc_modem_request_uplink(stack_id, 10 /*fport*/, false, buf, 7)`.
  Si `bme280_read` falla, **no** enviar payload ambiental (omitir ese uplink) y loguear WARNING.
- **REQ-004-3 — GNSS intacto:** el flujo `GNSS_SCAN_DONE` → `GNSS_TERMINATED` →
  `smtc_modem_gnss_scan(...)` **no se modifica** en su lógica; el envío del paquete GNSS lo sigue
  gestionando el servicio de geolocalización (`smtc_modem_gnss_send_mode`).
- **REQ-004-4 — Cadencia:** mantener el timer de alarma existente
  (`smtc_modem_alarm_start_timer`) para el ciclo ambiental; reprogramarlo al final del manejador
  de `ALARM` para que sea periódico. Definir `SENSOR_UPLINK_PERIOD_S` (p.ej. 60 s) como `#define`.
- **REQ-004-5 — Coexistencia de uplinks:** el uplink ambiental (fport 10) y el paquete GNSS
  (su fport) no deben usar el mismo fport (coherente con SRS-003 REQ-003-1). Confirmar/fijar el
  puerto GNSS con `smtc_modem_gnss_set_port` si hace falta evitar colisión con 10.
- **REQ-004-6 — Sin bloqueo:** la lectura del sensor ocurre dentro del manejador de evento y debe
  ser corta (RESTR-4). Prohibido `while(1)`/delays largos.
- **REQ-004-7 — Includes/build:** añadir `#include "bme280.h"` y `#include "smtc_hal_i2c.h"`;
  asegurar que ambos `.c` están en el build de la app.

## Estrategia de integración recomendada (para el agente)

1. Copiar/derivar de `main_geolocation.c` en lugar de reescribirlo desde cero, para no perder el
   flujo GNSS ya probado.
2. Introducir una función local `send_environmental_uplink(uint8_t stack_id)` que encapsule
   read+encode+request_uplink, y llamarla desde el manejador de `ALARM`.
3. No tocar la sección de credenciales ni la de configuración de región (viene de `example_options.h`).

## Checklist de aceptación

- [ ] **A1** Se llama a `smtc_hal_i2c_init` y `bme280_init` una vez en el arranque (REQ-004-1).
- [ ] **A2** Existe la ruta read→encode→`smtc_modem_request_uplink(..., 10, ..., 7)` en `ALARM` (REQ-004-2).
- [ ] **A3** El manejador GNSS (`SCAN_DONE`/`TERMINATED`/`gnss_scan`) sigue presente y sin cambios de lógica (REQ-004-3).
- [ ] **A4** El timer de alarma se reprograma con `SENSOR_UPLINK_PERIOD_S` (REQ-004-4).
- [ ] **A5** fport 10 ≠ fport GNSS confirmado en código (REQ-004-5).
- [ ] **A6** Fallo de sensor no cuelga el firmware ni impide el GNSS (revisar rutas de error, REQ-004-1/2).
- [ ] **A7** Ambos `.c` (i2c hal, bme280) están en el build (REQ-004-7).
- [ ] **A8** Ninguna función usada de `smtc_modem_*` es inventada — todas existen en `lbm_lib/smtc_modem_api/` (grep de cada símbolo).
