# Ejercicio 05 — TTGO ESP32 LoRa32 (nodo de terceros con LMIC)

Integra un **nodo LoRaWAN de terceros** (TTGO ESP32 LoRa v1 + SX1276 + OLED, firmware
Arduino/**MCCI LMIC**) contra el mismo ChirpStack. A diferencia de los ejercicios 01–04, aquí **el
binario no es nuestro** (*bring-your-own-device*): el valor está en el **flujo de depuración** de un
join que fallaba y en cómo se provisiona y consume por API.

> **Carpeta del ejercicio:** [`specs/exercises/05_ttgo-lora32/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/05_ttgo-lora32) · **Plataforma:** radio **SX1276** sobre **TTGO ESP32 LoRa v1**

| | |
|---|---|
| Qué demuestra | Un **nodo de terceros** (Arduino/LMIC) uniéndose por **OTAA** y enviando uplinks a ChirpStack |
| Hardware | TTGO ESP32 LoRa v1 (SX1276) + OLED SSD1306 |
| ¿Join / ChirpStack? | ✅ **Sí** (OTAA, LoRaWAN 1.0.3), **provisionado** — app `TTGO-LoRa32` |
| Dato / observable | **8 bytes en fPort 1** → `counter` (u16 LE) + `timestamp` (u32 LE), decodificados por codec |
| Binario / sketch | [`sketches/TTGO_LoRaWAN_v3.ino`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/05_ttgo-lora32/sketches/TTGO_LoRaWAN_v3.ino) (Arduino/MCCI LMIC) |

**Región:** US915, **sub-banda 2** (canales 8-15) → en el sketch `LMIC_selectSubBand(1)`; en ChirpStack el device profile usa la región `us915_1`.

> 🇪🇺 **Viene configurado para US915.** Para **EU868**: en `arduino_lmic_project_config.h` pon
> `#define CFG_eu868 1` y **quita** la línea `LMIC_selectSubBand(...)` — EU868 no tiene sub-bandas
> (3 canales fijos 868.1/.3/.5). Detalle en [Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).

## Ruta paso a paso

1. **Prepara el entorno** → [Requisitos e instalación § Arduino IDE + librerías](How-To-Requisitos-e-instalación).
   Este nodo **no usa el toolchain ARM**: instala **Arduino IDE + ESP32 + MCCI LMIC + U8g2**.
2. **Compila y flashea el sketch** → [Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).
   Abre `TTGO_LoRaWAN_v3.ino`, configura LMIC (`CFG_us915 1`, `CFG_sx1276_radio 1`) y **verifica `LMIC_selectSubBand(1)`** (sub-banda 2, la del gateway del lab). Pon los EUIs en el **orden LMIC** (ver *Credenciales y detalles*).
3. **Provisiona el device en ChirpStack** → [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack).
   Registra el device con las credenciales **en MSB**; **alinea la sub-banda** poniendo el device profile en **`us915_1`** (si no, el join no cuaja). En el repo es reproducible con `./provision.sh` + `scripts/upload_codec.sh` (sube el codec al profile).
4. **Consume los datos** → [Provisionar en ChirpStack § Consumir por MQTT](How-To-Provisionar-en-ChirpStack).
   El `object` del MQTT (`{counter, timestamp, checksum_ok}`) es la fuente del dashboard. Para estado/métricas por REST, **reutiliza el `consume.py` del [ejercicio 01](Ejercicio-01-Periodical-Uplink)** (`--state 02389205358e71db`).

## Qué observar

- **OLED + monitor serie (115200):** la secuencia `EV_JOINING` → `EV_JOINED` y, después, los envíos periódicos.
- **ChirpStack → Gateways:** el gateway con *last seen* reciente al empezar a recibir.
- **ChirpStack → aplicación `TTGO-LoRa32` → device → LoRaWAN frames:** los uplinks llegando en **fPort 1**.
- **Vector real capturado:** `99 05 00 12 01 00 00 B1` → `counter=5, timestamp=274, checksum_ok=true`.

## Credenciales y detalles

Credenciales (MSB, tal cual van en ChirpStack) — completas en el [`credentials.json`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/05_ttgo-lora32/credentials.json):

| Campo | Valor (MSB, para ChirpStack) |
|-------|------------------------------|
| DevEUI | `02389205358e71db` |
| JoinEUI/AppEUI | `505246f87143fd8a` |
| AppKey (→ `nwkKey` en 1.0.x) | `8ac583dfeec76c81ffd19ccfe76b73bf` |

> 📌 **Orden de bytes (la causa nº1 de fallos de join).** El **mismo** DevEUI/JoinEUI se escribe en
> **DOS órdenes distintos**: en el **sketch (LMIC) en LSB (invertido)** y en **ChirpStack en MSB**.
> El **AppKey NO se invierte** (MSB en ambos). Ej.: DevEUI en el sketch = `DB 71 8E 35 05 92 38 02`,
> en ChirpStack = `02 38 92 05 35 8E 71 DB`. Regla: **DevEUI/JoinEUI al revés en el array C; AppKey igual.**
> Detalle en [Compilar el TTGO en Arduino § Orden de bytes](How-To-Compilar-el-TTGO-en-Arduino).

**Lo que enseña este ejercicio (bugs reales, no era el orden de bytes):**
- **Bug A — sub-banda mal:** con `LMIC_selectSubBand(0)` el TTGO transmitía en canales 0-7 y el gateway (FSB2, canales 8-15) **no lo oía** → cero frames. El `v3.ino` ya usa `selectSubBand(1)`.
- **Bug B — credenciales del sketch ≠ registradas:** el sketch producía DevEUI `02389205358e71db` pero en ChirpStack había otro → `Unknown device`. Se registran las credenciales reales del sketch (arriba).
- **Posible Bug C (si join en logs pero device falla):** en el TTGO v1 el **DIO1=GPIO33 suele necesitar un puente físico** para recibir el Join Accept en la ventana RX (aquí no hizo falta).

## Ficheros del ejercicio

- [`README.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/05_ttgo-lora32/README.md) — la historia completa (bugs A/B/C), payload y provisión.
- `sketches/TTGO_LoRaWAN_v3.ino` — firmware del nodo (sub-banda correcta).
- `payload_decoder.js` — codec ChirpStack (fPort 1). · `provision.sh` — alta reproducible por API. · `scripts/upload_codec.sh`, `scripts/subscribe.sh`. · `credentials.json` — IDs y credenciales reales.

> ◀ [Ejercicio 04 — Wi-Fi Region Detection](Ejercicio-04-Wi-Fi-Region-Detection) · [Ejercicio 06 — TTGO + BMP280 ▶](Ejercicio-06-TTGO-BMP280)
