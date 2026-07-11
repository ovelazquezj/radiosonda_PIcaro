# Ejercicio 08 — 🎈 Radiosonda PICARO (LilyGo T-Beam → ChirpStack)

Arma una **"radiosonda"** con una **LilyGo T-Beam (ESP32, radio SX1276)** que hace **join OTAA** a una
red **LoRaWAN/ChirpStack**, lee sus **sensores integrados (GPS y batería)** y **envía la telemetría**
por radio a través de un gateway, imprimiéndolo por **Monitor Serie** y **OLED**. Pensada para empezar
de cero: lo único que editas es **`config_lorawan.h`**.

> **Carpeta del ejercicio:** [`specs/exercises/08_radiosonda_picaro/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/08_radiosonda_picaro) · **Plataforma:** **LilyGo T-Beam** (ESP32 + radio **SX1276** + GPS), firmware **Arduino / RadioLib**

| | |
|---|---|
| Qué demuestra | Join OTAA + telemetría de **sensores integrados** (GPS + batería) hacia ChirpStack |
| Hardware | **LilyGo T-Beam** (ESP32/SX1276) + antena LoRa; *(opcional)* batería 18650 + OLED; un **gateway US915** reportando |
| ¿Join / ChirpStack? | ✅ **Sí** (OTAA, LoRaWAN **1.0.x**); codec `chirpstack/decoder.js` al device profile |
| Dato / observable | **15 bytes** big-endian: GPS (lat/lon/alt/sats), batería (mV/%) y flags → `object` decodificado |
| Binario / sketch | [`radiosonda_picaro.ino`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/08_radiosonda_picaro/radiosonda_picaro.ino) (Arduino IDE + **RadioLib 7.6.x**) |

**Parámetros de red (ya fijados en el código):** Región **US915** · **sub-banda 2** (canales 8–15) ·
**OTAA** · Clase **A** · LoRaWAN **1.0.x** (solo se usa AppKey).

> 📖 **La guía completa paso a paso está en el
> [README del ejercicio](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/08_radiosonda_picaro)**
> (instalar Arduino IDE + ESP32, librerías, editar credenciales, compilar, flashear, dar de alta en
> ChirpStack y ver los datos). Esta página es el índice/resumen.

## Ruta paso a paso

1. **Arduino IDE + ESP32** — instala el IDE y el soporte ESP32 (Boards Manager `esp32`, versión 3.x o 2.0.x).
2. **Librerías** — **RadioLib 7.6.x**, **TinyGPSPlus**, **XPowersLib**, **U8g2** (`WiFi`/`SPI`/`Wire`/`FS` ya vienen con ESP32).
3. **Credenciales** — edita **`config_lorawan.h`**: `PICARO_DEV_EUI`, `PICARO_JOIN_EUI`, `PICARO_APP_KEY` (todo **MSB**, `0x…`). Es lo único que tocas.
4. **Compilar** — tarjeta **"ESP32 Dev Module"** y, **importante**, **Partition Scheme = `Huge APP`** (el programa es grande); Upload Speed `921600`. Pulsa *Verify*.
5. **Flashear** — conecta la T-Beam, elige el **puerto COM** (instala el driver CP210x/CH9102 si hace falta) y pulsa *Upload*.
6. **Monitor Serie (115200)** — pulsa **RST**; **copia el bloque "DATOS PARA DAR DE ALTA EN CHIRPSTACK"** (DevEUI/JoinEUI/AppKey) y observa el **`JOIN EXITOSO`**.
7. **Dar de alta en ChirpStack** — device profile **US915 / 1.0.x / OTAA / Clase A** y pega el `decoder.js` en su **Codec**; crea la *Application*, añade el device (DevEUI/JoinEUI) y pon el **AppKey en *Application key***. *(Referencia general: [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack).)*
8. **Ver los datos** — device → **Events** → evento `up` → campo **`object`** con `latitude`, `longitude`, `altitude_m`, `satellites`, `battery_v`, `battery_pct` y flags.

## Qué observar

- **Monitor Serie (115200):** el banner de arranque, `Radio OK`, el bloque de credenciales, **`JOIN EXITOSO`** + `DevAddr`, y cada `Uplink #N` con **RSSI/SNR**.
- **GPS:** puede tardar **varios minutos** en fijar (arranque en frío); mientras, envía lat/lon = 0 — es normal. Colócalo cerca de una ventana o al aire libre.
- **ChirpStack:** **LoRaWAN frames** (JoinRequest → JoinAccept) y **Events** con los uplinks; el `object` ya decodificado.

## Estructura del payload (15 bytes, big-endian)

Definido en `radiosonda_picaro.ino` (`buildPayload`) y leído en `chirpstack/decoder.js` — **si cambias uno, cambia el otro**. El uplink va en **fPort 10** (datarate inicial DR3 en US915, con ADR).

| Byte(s) | Campo | Tipo | Codificación |
|:---:|---|---|---|
| 0 | `status` | uint8 | bit0 = GPS con fix · bit1 = cargando · bit2 = USB conectado |
| 1–4 | `latitud` | int32 | grados × 1 000 000 |
| 5–8 | `longitud` | int32 | grados × 1 000 000 |
| 9–10 | `altitud` | int16 | metros |
| 11 | `satélites` | uint8 | número de satélites |
| 12–13 | `batería_mv` | uint16 | milivolts |
| 14 | `batería_pct` | uint8 | 0–100 (255 = desconocido) |

## Credenciales y detalles

- En **`config_lorawan.h`** (todo **MSB**, sin invertir): `PICARO_DEV_EUI` (`0x` + 16 hex), `PICARO_JOIN_EUI` (por defecto todo ceros), `PICARO_APP_KEY` (16 bytes). Deben ser **idénticas** en la placa y en ChirpStack.
- **LoRaWAN 1.0.x:** en ChirpStack el AppKey va en **Application key** (no hay "Network key").
- **US915, sub-banda 2** (device profile US915). El intervalo (`PICARO_UPLINK_INTERVAL_S`, 30 s) es ajustable; para **EU868** cambia `PICARO_REGION` (ver §14 del README).
- ⚠️ **La AppKey es secreta:** no la subas a internet ni la compartas.

## Ficheros del ejercicio

- `radiosonda_picaro.ino` — programa principal (objetos, payload, `buildPayload`, `setup()`/join, `loop()`/envío).
- `config_lorawan.h` — ⭐ credenciales (DevEUI/JoinEUI/AppKey) y región/intervalo — **lo único que editas**.
- `utilities.h` — pines de la placa (perfil *T-Beam SX1276*).
- `LoRaBoards.h` / `LoRaBoards.cpp` — "motor" de la placa (enciende PMU, GPS y OLED) — no se edita.
- `chirpstack/decoder.js` — codec para pegar en ChirpStack.
- Guía completa en el [README de la carpeta](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/08_radiosonda_picaro).

> *Basado en los ejemplos oficiales de LilyGo (RadioLib LoRaWAN). Licencia MIT. Radio SX1276 · US915 · OTAA · LoRaWAN 1.0.x.*

> ◀ [Ejercicio 07 — Gateway de 1 canal](Ejercicio-07-Gateway-1-canal)
