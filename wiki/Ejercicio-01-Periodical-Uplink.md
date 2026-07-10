# Ejercicio 01 — Periodical Uplink (conectividad LoRaWAN)

El laboratorio base de conectividad: el LR1110 hace **join OTAA** y envía **uplinks periódicos**
(más uno extra al pulsar el botón azul **B1**). Es el primer contacto de un nodo con ChirpStack y
el punto de partida antes de añadir sensores.

> **Carpeta del ejercicio:** [`specs/exercises/01_periodical-uplink/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/01_periodical-uplink) · **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG**

| | |
|---|---|
| Qué demuestra | Join OTAA + uplinks periódicos (keep-alive), con un uplink extra al pulsar **B1** |
| Hardware | Nucleo-L476RG + shield LR1110 |
| ¿Join / ChirpStack? | **Sí** (OTAA). Se provisiona por API y se consume por REST/MQTT |
| Dato / observable | Payload keep-alive por MQTT: **fPort 101** (periódico) y **fPort 102** (botón B1) |
| Binario / sketch | Ya compilado en `artifacts/`: `periodical-uplink_lr1110_us915` y `..._eu868` (`.bin`/`.hex`/`.elf`) — solo hay que **flashear** |

## Ruta paso a paso

1. **Requisitos** → [Requisitos e instalación](How-To-Requisitos-e-instalación) · para *solo flashear* basta con la herramienta de grabado + terminal serie (sección **3**) y **ChirpStack levantado** ([Ejercicio 00](Ejercicio-00-ChirpStack)). El toolchain ARM solo hace falta si vas a recompilar.
2. **(Opcional) Recompilar** → [Compilar el firmware](How-To-Compilar-el-firmware) · el binario **ya viene** en `artifacts/`; solo recompila si quieres cambiar credenciales, región o periodo (comando de este ejercicio en [`COMMON_BUILD.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/COMMON_BUILD.md)).
3. **Flashear** → [Flashear y ver la serie](How-To-Flashear-y-ver-la-serie) · arrastra el `.hex` de **tu banda** (`periodical-uplink_lr1110_us915.hex` o `..._eu868.hex`) al disco `NODE_L476RG`. La región del binario debe coincidir con la del *device profile*.
4. **Provisionar en ChirpStack** → [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack) · o directo con el script de la carpeta: `export TOKEN="tu_api_key"` y `./provision.sh us915` (o `./provision.sh eu868`). Crea de forma idempotente el *device profile* de la banda, el device y la AppKey (en el campo **`nwkKey`**).
5. **Verificar el join** → abre la traza serie (**115200 8N1**) y pulsa **RESET (B2)**: debe aparecer **`Joined`**. En ChirpStack el device pasa a *last seen* y verás Join + uplinks. Pulsa **B1** → uplink extra.
6. **Consumir los datos** → stream MQTT con `python3 consume.py --stream` (o `./scripts/subscribe.sh`), y estado/métricas por REST con `python3 consume.py --state aabbccdd10915001`.

## Qué observar

- **Traza UART:** `Joined` al unirse y un `TXDONE` en cada uplink periódico (y al pulsar B1).
- **ChirpStack → LoRaWAN frames:** la secuencia **JoinRequest → JoinAccept** y luego los uplinks periódicos.
- **MQTT:** un JSON por uplink en `application/<APP>/device/aabbccdd10915001/event/up`. Fíjate en el **fPort**: **101** para el keep-alive periódico y **102** para el que dispara el botón **B1** (no es fPort 2).
- Este ejemplo **no** lleva payload de sensor rico decodificado; para `object` con temperatura/presión, ve al [Ejercicio 02](Ejercicio-02-BMP280-GNSS).

## Credenciales y detalles

JoinEUI común a las dos bandas: **`aabbccddeeff0000`**. Credenciales completas en el
[`credentials.json`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/01_periodical-uplink/credentials.json) de la carpeta.

| Banda | DevEUI | AppKey | Device profile |
|-------|--------|--------|----------------|
| **US915** | `aabbccdd10915001` | `10151015101510151015101510151015` | región US915 |
| **EU868** | `aabbccdd10868001` | `10681068106810681068106810681068` | región EU868 |

- **AppKey en `nwkKey`:** en LoRaWAN **1.0.x** la AppKey se registra en el campo **`nwkKey`** de ChirpStack (no en `appKey`).
- **Orden de bytes (LR1110):** el DevEUI/JoinEUI van en **MSB** (big-endian), **tal cual** el `credentials.json` y **tal cual** se pegan en ChirpStack — **sin invertir** (eso es cosa del TTGO/LMIC). Ver [Dar de alta un LR1110 nuevo](How-To-Dar-de-alta-un-LR1110-nuevo).
- **US915 · sub-banda:** device profile en **`us915_1`** (canales 8–15) para que el join cuaje (ver [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)).

## Ficheros del ejercicio

- `artifacts/` — binarios ya compilados US915 y EU868 (`.bin`/`.hex`/`.elf`), listos para flashear.
- `provision.sh` — alta reproducible por API (device profile + device + AppKey), toma la banda como argumento.
- `consume.py` — consumo de datos: `--state` (REST: estado y métricas de enlace) y `--stream` (MQTT: payloads en vivo).
- `scripts/subscribe.sh` — stream MQTT de uplinks sin dependencias de Python.
- `credentials.json` — DevEUI/AppKey de ambas bandas.
- Detalle en el [README de la carpeta](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/01_periodical-uplink).

> ◀ [Ejercicio 00 — ChirpStack](Ejercicio-00-ChirpStack) · [Ejercicio 02 — BMP280 + GNSS ▶](Ejercicio-02-BMP280-GNSS)
