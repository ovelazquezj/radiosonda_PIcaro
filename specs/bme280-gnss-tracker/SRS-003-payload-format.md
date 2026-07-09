# SRS-003 — Formato de Payload Ambiental y Decoder

## Objetivo

Definir un formato de payload **compacto, endianness fija y auto-descrito por fport** para los
datos del BME280, y entregar un **decoder** (JavaScript, estilo Chirpstack/TTN) que lo interprete.

> El paquete GNSS lo genera y formatea el propio servicio del LBM; **este SRS solo cubre el
> payload ambiental**. Ver SRS-004 para cómo se envían ambos.

## Requisitos funcionales

- **REQ-003-1 — fports:**
  - Payload ambiental → **fport 10**.
  - Paquete GNSS → el fport que fija el servicio (`smtc_modem_gnss_set_port`, por defecto del stack). No reutilizar el 10.
- **REQ-003-2 — Estructura del payload ambiental (7 bytes, big-endian):**

  | Offset | Campo | Tipo | Codificación | Unidad |
  |--------|-------|------|--------------|--------|
  | 0 | version | uint8 | fijo `0x01` | — |
  | 1–2 | temperature | int16 | `°C × 100` | 0.01 °C |
  | 3–4 | humidity | uint16 | `%RH × 100` | 0.01 %RH |
  | 5–6 | pressure | uint16 | `hPa` (offset −0, rango 300–1100 cabe en uint16) | 1 hPa |

  - Ejemplo: 23.45 °C, 48.20 %RH, 1013 hPa → `01 09 29 12 D4 03 F5`.
- **REQ-003-3** — La codificación se hace en C en la app (SRS-004) mediante una función pura:
  ```c
  // Devuelve el nº de bytes escritos (7). buffer debe tener capacidad >= 7.
  uint8_t bme280_payload_encode( const bme280_data_t* in, uint8_t* buffer );
  ```
- **REQ-003-4** — Big-endian explícito (MSB primero), sin depender del endianness del MCU.
- **REQ-003-5 — Decoder:** entregar `docs/payload_decoder.js` con función
  `decodeUplink(input)` que, para fport 10, devuelva `{ data: { version, temperature, humidity, pressure } }`
  con los valores físicos ya escalados. Debe ignorar/derivar a GNSS los demás fports sin romper.
- **REQ-003-6** — El decoder debe ser el **inverso exacto** de `bme280_payload_encode` (round-trip
  del ejemplo de REQ-003-2 reproduce 23.45 / 48.20 / 1013).

## Fuera de alcance

- Compresión, cifrado a nivel de aplicación, o CayenneLPP (se usa formato propio compacto).
- Decodificación del paquete GNSS (lo hace el solucionador de geolocalización, no este decoder).

## Checklist de aceptación

- [ ] **A1** `bme280_payload_encode` existe y produce 7 bytes big-endian (REQ-003-2/3/4).
- [ ] **A2** Test de vector: encode(23.45, 48.20, 1013) == `01 09 29 12 D4 03 F5` (verificable a mano o con un test unitario host).
- [ ] **A3** `docs/payload_decoder.js` con `decodeUplink` para fport 10 (REQ-003-5).
- [ ] **A4** Round-trip decoder(encode(x)) ≈ x dentro de la resolución de codificación (REQ-003-6).
- [ ] **A5** fport ambiental (10) ≠ fport GNSS (REQ-003-1).
