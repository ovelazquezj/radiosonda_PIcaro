# Ejercicio 02 — BMP280 + GNSS tracker (LR1110)

**Qué demuestra:** un **tracker con sensor ambiental y geolocalización** sobre el LR1110. El
firmware lee un **BMP280 por I²C** (temperatura y presión) y lo envía como payload propio en
**fPort 10**, y además ejecuta **scans GNSS y Wi-Fi** que el LR1110 resuelve como
geolocalización (a través del servicio de geoloc de LoRa Basics Modem, en su propio fPort). Es
el primer ejercicio con **payload de sensor rico decodificado** y con **posicionamiento**.

| | |
|---|---|
| Hardware | Nucleo-L476RG + LR1110 + BMP280 (I²C) |
| Capacidad LR1110 | Radio LoRaWAN + GNSS + Wi-Fi scan |
| ¿Join/ChirpStack? | ✅ Sí (OTAA, LoRaWAN 1.0.4) |
| Binarios | `artifacts/app_lr1110_US_915.bin` · `app_lr1110_EU_868.bin` |
| Payload ambiental | 5 bytes en **fPort 10** → `object` decodificado por MQTT |
| Geolocalización | scans GNSS/Wi-Fi (fPort del servicio de geoloc) |

Credenciales en [`credentials.json`](credentials.json). JoinEUI común `aabbccddeeff0000`.

## Conexión del sensor BMP280 (I²C1)

El BMP280 cuelga del **I²C1** del Nucleo. Configurado a **dirección 0x76** (pin SDO a GND):

| BMP280 | Nucleo (LR1110 shield) | Nota |
|--------|------------------------|------|
| VCC | 3V3 | alimentación 3.3 V |
| GND | GND | común |
| SCL | **PB_8** (D15) | reloj I²C1 |
| SDA | **PB_9** (D14) | datos I²C1 |
| CSB | 3V3 | fuerza modo **I²C** (no SPI) |
| SDO | GND | selecciona dirección **0x76** (a 3V3 sería 0x77) |

Diagrama visual de pines: [`../../bmp280-gnss-tracker/pinout_diagram.html`](../../bmp280-gnss-tracker/pinout_diagram.html).
Detalle de flasheo y alta paso a paso (apartado **1.bis**):
[`../../bmp280-gnss-tracker/FLASH_AND_CHIRPSTACK.md`](../../bmp280-gnss-tracker/FLASH_AND_CHIRPSTACK.md).

## Pasos del ejercicio

> Antes de usar los scripts, hazlos ejecutables una vez:
> `chmod +x provision.sh scripts/*.sh`

### 1. Flashear
Sigue [`../COMMON_FLASH.md`](../COMMON_FLASH.md) con el `.hex`/`.bin` de tu banda
(`app_lr1110_US_915` o `app_lr1110_EU_868`). La región del binario debe coincidir con la del
device profile (US915 con US915, EU868 con EU868).

### 2. Provisionar en ChirpStack (por API)
Setup del token en [`../COMMON_CHIRPSTACK_API.md`](../COMMON_CHIRPSTACK_API.md). Luego:
```bash
export TOKEN="tu_api_key"
./provision.sh us915      # o: ./provision.sh eu868
```
El script crea (idempotente) un **device profile PROPIO por banda**
(`Tracker-BMP280-US915` / `-EU868`), el device y la AppKey (en `nwkKey`, porque es LoRaWAN
1.0.x). Cada banda usa su propio profile para adjuntarle el codec sin mezclarlo con otros
payloads de la aplicación. Al terminar imprime el **device-profile-id**.

### 3. Subir el codec (decoder BMP280)
Adjunta [`payload_decoder.js`](payload_decoder.js) al device profile que imprimió el paso 2:
```bash
./scripts/upload_codec.sh <DEVICE_PROFILE_ID>
```
A partir de ahí los uplinks en fPort 10 llegan con el campo **`object`** ya decodificado.

### 4. Probar el join
Abre la traza serie (115200 8N1) y pulsa **RESET (B2)**. Debe aparecer **`Joined`**. En
ChirpStack el device mostrará *last seen* y verás JoinRequest→JoinAccept + uplinks. En los
uplinks de fPort 10 verás temperatura/presión; en los scans GNSS/Wi-Fi, los frames de geoloc.

### 5. Consumir los datos (base del dashboard)
```bash
# Stream por MQTT (trae 'object' decodificado si subiste el codec):
./scripts/subscribe.sh                    # tracker US915 por defecto
./scripts/subscribe.sh aabbccdd00868001   # tracker EU868

# Estado + métricas por REST (reutiliza el consume.py del ejercicio 01):
export APP="5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7"    # Demos-LR1110
python3 ../01_periodical-uplink/consume.py --state aabbccdd00915001
```

## Formato del payload ambiental (fPort 10)

5 bytes **big-endian** (ver [`payload_decoder.js`](payload_decoder.js)):

| Offset | Campo | Tipo | Escala |
|--------|-------|------|--------|
| 0 | version | uint8 | fijo `0x02` |
| 1-2 | temperatura | int16 **BE** | °C × 100 (con signo) |
| 3-4 | presión | uint16 **BE** | hPa |

El decoder devuelve `{ version, temperature, pressure }`.

**Vector de ejemplo** — 23.45 °C y 1013 hPa:

```
23.45 °C → 2345 = 0x0929      1013 hPa → 0x03F5      version = 0x02
bytes (fPort 10):  02 09 29 03 F5
decodificado:      { version: 2, temperature: 23.45, pressure: 1013 }
```

El decoder solo procesa el fPort 10; los uplinks de geolocalización (otro fPort) pasan de largo
sin romperlo y los resuelve el servicio de geoloc.

## Qué observar
- **Traza UART:** `Joined`, lecturas del BMP280 y disparo de scans GNSS/Wi-Fi.
- **ChirpStack (LoRaWAN frames):** Join + uplinks de sensor (fPort 10) + frames de geoloc.
- **MQTT:** el campo **`object`** (`{version, temperature, pressure}`) de los uplinks de fPort 10
  es la fuente directa para un dashboard;
  topic `application/<APP>/device/aabbccdd00915001/event/up`.

## Notas
- Región del **device profile** = región del **binario** (US915 con US915, EU868 con EU868).
- La AppKey se registra en el campo **`nwkKey`** (LoRaWAN 1.0.x).
- Cada banda tiene su **device profile propio con el codec BMP280 adjunto** para que el `object`
  del MQTT venga decodificado y no se mezcle con otros payloads de la aplicación.

## Archivos
```
payload_decoder.js         codec ChirpStack del payload BMP280 (fPort 10)
provision.sh               alta reproducible por API (device profile + device + keys)
scripts/upload_codec.sh    adjunta el codec al device profile de la banda
scripts/subscribe.sh       stream MQTT de uplinks (tracker US915 por defecto)
credentials.json           DevEUI/AppKey de ambas bandas
artifacts/                 binarios app_lr1110_US_915 / app_lr1110_EU_868 (.bin/.hex/.elf)
```
