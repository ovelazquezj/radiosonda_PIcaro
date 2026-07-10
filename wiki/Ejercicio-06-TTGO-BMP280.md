# Ejercicio 06 — TTGO ESP32 LoRa + BMP280 (envío cada minuto)

Conecta un **sensor real BMP280** (temperatura + presión) a un nodo **TTGO ESP32 LoRa v1** por I²C
y envía sus lecturas por **LoRaWAN OTAA cada 60 s**. Es la evolución del ejercicio 05: del payload
de prueba pasamos a **datos de sensor** listos para un dashboard, con el `object` decodificado por
ChirpStack.

> **Carpeta del ejercicio:** [`specs/exercises/06_ttgo-bmp280/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/06_ttgo-bmp280) · **Plataforma:** radio **SX1276** sobre **TTGO ESP32 LoRa v1** + **BMP280**

| | |
|---|---|
| Qué demuestra | Un **sensor I²C** real (BMP280) enviando **T + P** por OTAA **cada 60 s** a ChirpStack |
| Hardware | TTGO ESP32 LoRa v1 (SX1276) + OLED + **BMP280** (GY-BMP280, I²C) |
| ¿Join / ChirpStack? | ✅ **Sí** (OTAA, LoRaWAN 1.0.3), **provisionado** — app `TTGO-BMP280` |
| Dato / observable | **5 bytes en fPort 2** → `temperature` (int16 BE, °C×100) + `pressure` (u16 BE, hPa) |
| Binario / sketch | [`sketches/TTGO_BMP280_v1.ino`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/06_ttgo-bmp280/sketches/TTGO_BMP280_v1.ino) (Arduino/MCCI LMIC) |

**Región:** US915, **sub-banda 2** (canales 8-15) → sketch `LMIC_selectSubBand(1)`; device profile `us915_1` en ChirpStack. **Cadencia:** 1 uplink cada 60 s.

> 🇪🇺 **Viene configurado para US915.** Para **EU868**: en `arduino_lmic_project_config.h` pon
> `#define CFG_eu868 1` y **quita** la línea `LMIC_selectSubBand(...)` (EU868 no tiene sub-bandas).
> Ver [Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).

## Ruta paso a paso

1. **Prepara el entorno** → [Requisitos e instalación § Arduino IDE + librerías](How-To-Requisitos-e-instalación).
   Igual que el 05 **más** las librerías del sensor: **Adafruit BMP280** (+ **Adafruit Unified Sensor**).
2. **Cablea el BMP280** (ver *Credenciales y detalles → Pinout* abajo).
   Va al bus I²C del TTGO (**SDA=GPIO21, SCL=GPIO22**, compartido con la OLED). El módulo **trae pull-ups integrados**: no hacen falta resistencias.
3. **Compila y flashea el sketch** → [Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).
   Abre `TTGO_BMP280_v1.ino`, deja `CFG_us915 1` / `CFG_sx1276_radio 1` y **verifica `LMIC_selectSubBand(1)`**. Pon los EUIs en el **orden LMIC** (ver nota de bytes).
4. **Provisiona el device en ChirpStack** → [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack).
   Registra el device en **MSB** y pon el device profile en **`us915_1`**. Reproducible con `./provision.sh` + `scripts/upload_codec.sh` (codec fPort 2).
5. **Consume los datos** → [Provisionar en ChirpStack § Consumir por MQTT](How-To-Provisionar-en-ChirpStack).
   El `object` del MQTT (`{version, temperature, pressure}`) alimenta el dashboard. Para estado/métricas por REST, **reutiliza el `consume.py` del [ejercicio 01](Ejercicio-01-Periodical-Uplink)** (`--state aabbccdd60915001`).

## Qué observar

- **OLED + monitor serie (115200):** `EV_JOINING` → `EV_JOINED` y, después, **cada 60 s** un uplink con T y P.
- **ChirpStack → aplicación `TTGO-BMP280` → device → LoRaWAN frames:** uplinks en **fPort 2** cada minuto.
- **Ejemplo de payload:** `01 09 29 03 F5` → `{version:1, temperature:23.45, pressure:1013}` (verificado).

## Credenciales y detalles

Credenciales (MSB, para ChirpStack) — completas en el [`credentials.json`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/06_ttgo-bmp280/credentials.json):

| Campo | Valor (MSB, para ChirpStack) |
|-------|------------------------------|
| DevEUI | `aabbccdd60915001` |
| JoinEUI | `aabbccddeeff0000` |
| AppKey (→ `nwkKey` en 1.0.x) | `60156015601560156015601560156015` |

> 📌 **Orden de bytes (mismo criterio que el ejercicio 05).** En el **sketch (LMIC)** DevEUI/JoinEUI
> van en **LSB (invertido)**; en **ChirpStack en MSB**. El **AppKey NO se invierte**. Ej.: DevEUI en
> el sketch = `01 50 91 60 DD CC BB AA`, en ChirpStack = `AA BB CC DD 60 91 50 01`. Confundirlo →
> `Unknown device` y nunca hace join. Detalle en
> [Compilar el TTGO en Arduino § Orden de bytes](How-To-Compilar-el-TTGO-en-Arduino).

**Pinout BMP280 ↔ TTGO** (comparte el bus I²C de la OLED; OLED=0x3C, BMP280=0x76, no colisionan):

| Pin BMP280 | TTGO ESP32 | Motivo |
|------------|-----------|--------|
| VCC / VIN | **3V3** | Alimentación (3.3 V) |
| GND | **GND** | Masa común |
| SDA / SDI | **GPIO21** | Datos I²C (mismo bus que la OLED) |
| SCL / SCK | **GPIO22** | Reloj I²C (mismo bus que la OLED) |
| CSB / CS | **3V3** | Atar a VCC fuerza **modo I²C** (si no, entra en SPI) |
| SDO / SDD | **GND** | GND → dirección **0x76** (la del sketch); a 3V3 sería 0x77 |

*Pines de radio ya cableados por el módulo (no tocar): SCK=5, MISO=19, MOSI=27, NSS=18, RST=14, DIO0=26, DIO1=33, DIO2=32. (DIO1=GPIO33 puede requerir un puente físico para la ventana RX en algunas placas.)*

## Ficheros del ejercicio

- [`README.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/06_ttgo-bmp280/README.md) — pinout completo, nota de bytes y provisión.
- `sketches/TTGO_BMP280_v1.ino` — firmware (BMP280 + LMIC, sub-banda 2, uplink 60 s).
- `payload_decoder.js` — codec ChirpStack (fPort 2). · `provision.sh` — alta reproducible por API. · `scripts/upload_codec.sh`, `scripts/subscribe.sh`. · `credentials.json` — IDs (MSB y LMIC/LSB) y payload.

> ◀ [Ejercicio 05 — TTGO LoRa32](Ejercicio-05-TTGO-LoRa32) · [Ejercicio 07 — Gateway de 1 canal ▶](Ejercicio-07-Gateway-1-canal)
