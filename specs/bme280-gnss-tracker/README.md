# SRS — Nodo Tracker Ambiental + GNSS (BMP280 + LR1110)

> **AJUSTE (v2):** el sensor pasó de **BME280** a **BMP280** por decisión del usuario.
> BMP280 mide **temperatura + presión** (SIN humedad): chip-id `0x58`, payload de 5 bytes.
> Donde estas specs digan "BME280 / humedad / 7 bytes / chip-id 0x60", léase la variante BMP280.
> Bandas objetivo: **US_915** y **EU_868**, cada una como build independiente con credenciales propias.

Conjunto de especificaciones para desarrollo **SDD (Spec-Driven Development)** pensadas para
ser implementadas por el agente **Claude Code** sobre el repositorio **LoRa Basics Modem (SWL2001)**.

> Estas specs **no** siguen IEEE-830. Cada documento está escrito como un contrato
> ejecutable: contexto → requisitos con ID verificable → interfaces exactas → criterios de
> aceptación en forma de checklist. El agente implementa un SRS a la vez y no avanza al
> siguiente hasta que la checklist de aceptación pasa.

## Objetivo del proyecto

Convertir la aplicación de geolocalización (`lbm_applications/3_geolocation_on_lora_edge`)
en un **nodo tracker** que, en cada ciclo:

1. Lee **temperatura, humedad y presión** de un sensor **BME280** por **I²C**.
2. Realiza un **scan GNSS** (servicio de geolocalización del LR1110).
3. Envía por LoRaWAN **dos uplinks**: el payload ambiental y el paquete GNSS.

Radio objetivo: **LR1110**. MCU objetivo: **Nucleo-L476RG (STM32L476)**.

## Cómo usar estas specs con Claude Code (flujo SDD)

Implementar en orden. Para cada SRS, pedir al agente:

```
Lee specs/bme280-gnss-tracker/SRS-00X-*.md y su sección "Archivos que debes leer primero".
Implementa TODOS los requisitos REQ-* del documento.
Al terminar, recorre la "Checklist de aceptación" y marca cada ítem como PASS/FAIL con evidencia
(comando ejecutado o archivo:línea). No pases al siguiente SRS si algún ítem está en FAIL.
```

## Orden de implementación

| # | Documento | Entregable | Depende de |
|---|-----------|-----------|-----------|
| 0 | [SRS-000-overview.md](SRS-000-overview.md) | Alcance, glosario, restricciones globales, definición de "hecho" | — |
| 1 | [SRS-001-i2c-hal.md](SRS-001-i2c-hal.md) | HAL I²C para STM32L4 (`smtc_hal_i2c.[ch]`) | SRS-000 |
| 2 | [SRS-002-bme280-driver.md](SRS-002-bme280-driver.md) | Driver del BME280 portable | SRS-001 |
| 3 | [SRS-003-payload-format.md](SRS-003-payload-format.md) | Formato de payload + decoder del Network Server | SRS-000 |
| 4 | [SRS-004-app-integration.md](SRS-004-app-integration.md) | Integración sensor+GNSS en el bucle de eventos | SRS-002, SRS-003 |
| 5 | [SRS-005-build-and-verify.md](SRS-005-build-and-verify.md) | Compilación del binario y verificación E2E | SRS-004 |

## Regla de oro para el agente

- **No inventar APIs.** Toda función del stack (`smtc_modem_*`) debe existir ya en
  `lbm_lib/smtc_modem_api/`. Si una firma no existe, es un FAIL: reportar, no improvisar.
- **No romper lo existente.** El binario base de geolocalización debe seguir compilando.
- **Trazabilidad:** cada commit/cambio referencia el `REQ-ID` que satisface.
