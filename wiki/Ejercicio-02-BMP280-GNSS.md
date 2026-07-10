# Ejercicio 02 — BMP280 + GNSS tracker (LR1110)

Un **tracker con sensor ambiental y geolocalización**: el firmware lee un **BMP280 por I²C**
(temperatura y presión) y lo envía como payload propio en **fPort 10**, y además lanza **scans
GNSS y Wi-Fi** que el LR1110 resuelve como posición. Es el primer ejercicio con **payload de
sensor rico decodificado** por un codec — la base directa de un dashboard.

> **Carpeta del ejercicio:** [`specs/exercises/02_bmp280-gnss-tracker/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/02_bmp280-gnss-tracker) · **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG** + **BMP280 (I²C)**

| | |
|---|---|
| Qué demuestra | Sensor I²C (BMP280) en payload propio + scans GNSS/Wi-Fi de geolocalización, con codec para datos decodificados |
| Hardware | Nucleo-L476RG + shield LR1110 + **BMP280** en I²C1 (dirección **0x76**) |
| ¿Join / ChirpStack? | **Sí** (OTAA, LoRaWAN 1.0.4). Device profile **propio por banda** con el codec adjunto |
| Dato / observable | **fPort 10** → 5 bytes → `object` `{version, temperature, pressure}` por MQTT; frames de geoloc (otro fPort) |
| Binario / sketch | Ya compilado en `artifacts/`: `app_lr1110_US_915` y `app_lr1110_EU_868` (`.bin`/`.hex`/`.elf`) — solo **flashear** |

## Ruta paso a paso

1. **Requisitos** → [Requisitos e instalación](How-To-Requisitos-e-instalación) · para *solo flashear*, herramienta de grabado + terminal serie (sección **3**) y **ChirpStack levantado** ([Ejercicio 00](Ejercicio-00-ChirpStack)).
2. **Cablear el BMP280** → conéctalo al **I²C1** del Nucleo en dirección **0x76** (ver tabla de pines abajo). Diagrama visual en [`specs/bmp280-gnss-tracker/pinout_diagram.html`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/bmp280-gnss-tracker/pinout_diagram.html).
3. **(Opcional) Recompilar** → [Compilar el firmware](How-To-Compilar-el-firmware) · el binario **ya viene**; recompila solo si cambias credenciales/región/payload (comando en [`COMMON_BUILD.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/COMMON_BUILD.md)).
4. **Flashear** → [Flashear y ver la serie](How-To-Flashear-y-ver-la-serie) · arrastra el `.hex`/`.bin` de tu banda (`app_lr1110_US_915` o `app_lr1110_EU_868`) al disco `NODE_L476RG`. La región del binario debe coincidir con la del device profile.
5. **Provisionar en ChirpStack** → [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack) · o directo: `export TOKEN="tu_api_key"` y `./provision.sh us915` (o `eu868`). Crea un **device profile propio por banda** (`Tracker-BMP280-US915` / `-EU868`), el device y la AppKey (en **`nwkKey`**), e imprime el **device-profile-id**.
6. **Subir el codec** → adjunta el decoder BMP280 al device profile del paso anterior: `./scripts/upload_codec.sh <DEVICE_PROFILE_ID>`. A partir de ahí los uplinks de fPort 10 llegan con el campo **`object`** decodificado (ver [Provisionar en ChirpStack §codec](How-To-Provisionar-en-ChirpStack)).
7. **Verificar el join** → traza serie a **115200 8N1**, pulsa **RESET (B2)**: debe aparecer **`Joined`**. En ChirpStack verás Join + uplinks (fPort 10 con temperatura/presión, y los frames de geoloc).
8. **Consumir los datos** → `./scripts/subscribe.sh` (tracker US915 por defecto; `./scripts/subscribe.sh aabbccdd00868001` para el EU868). El campo `object` es la fuente directa del dashboard.

## Conexión del sensor BMP280 (I²C1)

BMP280 en el **I²C1** del Nucleo, a **dirección 0x76** (SDO a GND):

| BMP280 | Nucleo (shield LR1110) | Nota |
|--------|------------------------|------|
| VCC | 3V3 | alimentación 3.3 V |
| GND | GND | común |
| SCL | **PB_8** (D15) | reloj I²C1 |
| SDA | **PB_9** (D14) | datos I²C1 |
| CSB | 3V3 | fuerza modo **I²C** (no SPI) |
| SDO | GND | dirección **0x76** (a 3V3 sería 0x77) |

## Formato del payload ambiental (fPort 10)

5 bytes **big-endian** (los decodifica `payload_decoder.js`):

| Offset | Campo | Tipo | Escala |
|--------|-------|------|--------|
| 0 | version | uint8 | fijo `0x02` |
| 1–2 | temperatura | int16 **BE** | °C × 100 (con signo) |
| 3–4 | presión | uint16 **BE** | hPa |

**Vector de ejemplo** — 23.45 °C y 1013 hPa → bytes `02 09 29 03 F5` → `{ version: 2, temperature: 23.45, pressure: 1013 }`. El codec **solo** procesa el fPort 10; los uplinks de geolocalización (otro fPort) pasan de largo sin romperlo y los resuelve el servicio de geoloc.

## Qué observar

- **Traza UART:** `Joined`, lecturas del BMP280 y el disparo de los scans GNSS/Wi-Fi.
- **ChirpStack → LoRaWAN frames:** Join + uplinks de sensor (fPort 10) + frames de geoloc.
- **MQTT:** el campo **`object`** (`{version, temperature, pressure}`) de los uplinks de fPort 10 en `application/<APP>/device/aabbccdd00915001/event/up` — la fuente directa del dashboard (si subiste el codec; si no, `object` viene vacío).

## Credenciales y detalles

JoinEUI común: **`aabbccddeeff0000`**. Credenciales completas en el
[`credentials.json`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/02_bmp280-gnss-tracker/credentials.json) de la carpeta.

| Banda | DevEUI | AppKey | Device profile |
|-------|--------|--------|----------------|
| **US915** | `aabbccdd00915001` | `A915A915A915A915A915A915A915A915` | `Tracker-BMP280-US915` |
| **EU868** | `aabbccdd00868001` | `A868A868A868A868A868A868A868A868` | `Tracker-BMP280-EU868` |

- **AppKey en `nwkKey`** (LoRaWAN 1.0.x), no en `appKey`.
- **Cada banda usa su propio device profile** para adjuntarle el codec sin mezclarlo con otros payloads de la aplicación.
- **Orden de bytes (LR1110):** DevEUI/JoinEUI en **MSB**, se pegan **sin invertir** en ChirpStack (al contrario que el TTGO/LMIC). Ver [Dar de alta un LR1110 nuevo](How-To-Dar-de-alta-un-LR1110-nuevo).
- **US915 · sub-banda `us915_1`** en el device profile (ver [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)).

## Ficheros del ejercicio

- `artifacts/` — binarios ya compilados US915 y EU868 (`.bin`/`.hex`/`.elf`).
- `payload_decoder.js` — codec ChirpStack del payload BMP280 (fPort 10).
- `provision.sh` — alta por API (device profile propio por banda + device + AppKey).
- `scripts/upload_codec.sh` — adjunta el codec al device profile de la banda.
- `scripts/subscribe.sh` — stream MQTT de uplinks (tracker US915 por defecto).
- `credentials.json` — DevEUI/AppKey de ambas bandas.
- Detalle en el [README de la carpeta](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/02_bmp280-gnss-tracker).

> ◀ [Ejercicio 01 — Periodical Uplink](Ejercicio-01-Periodical-Uplink) · [Ejercicio 03 — Hardware Modem ▶](Ejercicio-03-Hardware-Modem)
