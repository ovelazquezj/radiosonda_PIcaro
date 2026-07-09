# Ejercicio 06 — TTGO ESP32 LoRa + BMP280 (envío cada minuto)

**Qué demuestra:** integrar un **sensor real BMP280** (temperatura + presión) a un nodo
**TTGO ESP32 LoRa v1** por I²C y enviar sus lecturas por **LoRaWAN OTAA cada 60 segundos**.
Es la evolución del ejercicio 05: del payload de prueba pasamos a **datos de sensor** listos para
un dashboard, con el `object` decodificado por ChirpStack.

| | |
|---|---|
| Hardware | TTGO ESP32 LoRa v1 (SX1276) + OLED + **BMP280** |
| Firmware | `sketches/TTGO_BMP280_v1.ino` (Arduino/MCCI LMIC) |
| Región | US915 (sub-banda 2, canales 8-15) |
| Cadencia | **1 uplink cada 60 s** |
| Payload | 5 bytes en **fPort 2** → `temperature`, `pressure` |
| ¿ChirpStack? | ✅ (OTAA 1.0.3) — provisionado |

## 📌 Nota didáctica CLAVE: el orden de bytes de DevEUI y JoinEUI

**El mismo DevEUI/JoinEUI se escribe en DOS órdenes distintos según dónde:** en el **sketch (LMIC)
en LSB** (little-endian, byte menos significativo primero) y en **ChirpStack en MSB** (big-endian,
tal como se lee). El **AppKey NO** se invierte: va en MSB en ambos lados.

| Campo | En el **sketch** (LMIC, arrays `os_getDevEui`/`os_getArtEui`) | En **ChirpStack** (UI/API) |
|-------|--------------------------------------------------------------|----------------------------|
| **DevEUI** | `01 50 91 60 DD CC BB AA`  ← **LSB (invertido)** | `AA BB CC DD 60 91 50 01`  ← **MSB** |
| **JoinEUI/AppEUI** | `00 00 FF EE DD CC BB AA`  ← **LSB (invertido)** | `AA BB CC DD EE FF 00 00`  ← **MSB** |
| **AppKey** | `60 15 60 15 … 60 15`  ← **MSB (igual)** | `60 15 60 15 … 60 15`  ← **MSB (igual)** |

**¿Por qué?** La librería **MCCI/Arduino LMIC** carga `DevEUI` y `AppEUI` en **little-endian**
(así lo esperan `os_getDevEui()`/`os_getArtEui()`), mientras que la especificación LoRaWAN y
ChirpStack los representan en **big-endian**. Por eso, para el **mismo** dispositivo, los EUIs se
ven "al revés" entre el código y ChirpStack. El `AppKey` se transmite en el mismo orden (MSB) en
ambos, así que **no** se invierte.

> **Error típico:** copiar el DevEUI de ChirpStack (MSB) directo al array del sketch sin invertir
> (o viceversa) → ChirpStack registra un DevEUI que no coincide con el que transmite el nodo →
> `Unknown device` y nunca hace join. **Regla:** en el array C, DevEUI/AppEUI van **al revés** que
> en ChirpStack; el AppKey, igual.

## 🔌 Conexión de pines (BMP280 ↔ TTGO ESP32 LoRa v1)

El BMP280 va por **I²C, compartiendo el bus del OLED** (SDA=GPIO21, SCL=GPIO22). El OLED es 0x3C y
el BMP280 0x76 → no colisionan.

| Pin BMP280 (GY-BMP280) | Señal | TTGO ESP32 | Motivo |
|------------------------|-------|-----------|--------|
| **VCC / VIN** | 3.3 V | **3V3** | Alimentación (el módulo trabaja a 3.3 V) |
| **GND** | Masa | **GND** | Masa común |
| **SDA / SDI** | Datos I²C | **GPIO21** | mismo bus que el OLED |
| **SCL / SCK** | Reloj I²C | **GPIO22** | mismo bus que el OLED |
| **CSB / CS** | Selección | **3V3** | atar a VCC fuerza **modo I²C** (si no, entra en SPI) |
| **SDO / SDD** | Dir. I²C | **GND** | GND → dirección **0x76** (la del sketch). A 3V3 sería 0x77 |

```
     BMP280 (GY-BMP280)                 TTGO ESP32 LoRa v1
   ┌────────────────────┐            ┌────────────────────────┐
   │ VCC ───────────────┼───────────►│ 3V3                    │
   │ GND ───────────────┼───────────►│ GND                    │
   │ SDA ───────────────┼───────────►│ GPIO21  (SDA, bus OLED)│
   │ SCL ───────────────┼───────────►│ GPIO22  (SCL, bus OLED)│
   │ CSB ───────────────┼───► 3V3    (fuerza I²C)             │
   │ SDO ───────────────┼───► GND    (dirección 0x76)         │
   └────────────────────┘            └────────────────────────┘
```

**Pines de radio (ya usados por el módulo LoRa, NO tocar):** SCK=5, MISO=19, MOSI=27, NSS=18,
RST=14, DIO0=26, DIO1=33, DIO2=32. *(DIO1=GPIO33 puede requerir un puente físico en algunas placas
TTGO para la ventana RX.)*

## Credenciales

| | ChirpStack (MSB) |
|---|---|
| DevEUI | `aabbccdd60915001` |
| JoinEUI | `aabbccddeeff0000` |
| AppKey (→ `nwkKey`) | `60156015601560156015601560156015` |

(Los arrays en orden LMIC están arriba en la nota de bytes y en `credentials.json`.)

## Provisión en ChirpStack (ya hecha por API)

| Recurso | Valor |
|---------|-------|
| Application | `TTGO-BMP280` — `8b11760e-6cfb-48e1-9ec2-f76da193e060` |
| Device profile | `TTGO-BMP280-US915` — `f46ea82a-7608-4a23-a744-d1bbbcc4befa` (codec adjunto) |
| Topic MQTT | `application/8b11760e-6cfb-48e1-9ec2-f76da193e060/device/aabbccdd60915001/event/up` |

Reproducible:
```bash
export TOKEN="tu_api_key"
./provision.sh                                             # app + profile + device + keys
./scripts/upload_codec.sh f46ea82a-7608-4a23-a744-d1bbbcc4befa   # codec para datos decodificados
```

## Pasos del ejercicio

1. **Librerías Arduino:** *MCCI LoRaWAN LMIC*, *U8g2*, *Adafruit BMP280* (+ *Adafruit Unified Sensor*).
   Config LMIC: `#define CFG_us915 1` y `#define CFG_sx1276_radio 1`.
2. **Cablea el BMP280** según la tabla de pines (SDA=21, SCL=22, CSB→3V3, SDO→GND).
3. **Flashea** `sketches/TTGO_BMP280_v1.ino` desde el Arduino IDE (placa "TTGO LoRa32-OLED").
4. **Observa** el OLED y el monitor serie (115200): `EV_JOINING` → `EV_JOINED` → cada 60 s un uplink
   con T y P.
5. **Consume los datos** (dashboard):
   ```bash
   ./scripts/subscribe.sh      # MQTT: verás 'object' con {version, temperature, pressure}
   ```

## Payload y decoder

5 bytes en **fPort 2** (ver [`payload_decoder.js`](payload_decoder.js)):

| Offset | Campo | Tipo | |
|--------|-------|------|--|
| 0 | version | uint8 | `0x01` |
| 1-2 | temperature | int16 **BE** | °C × 100 |
| 3-4 | pressure | uint16 **BE** | hPa |

Ejemplo: `01 09 29 03 F5` → `{version:1, temperature:23.45, pressure:1013}` (verificado).

## Consumir para dashboard

El campo **`object`** del MQTT (`{temperature, pressure}`) es la fuente directa del dashboard.
Para estado/métricas por REST reutiliza el `consume.py` del ejercicio 01:
```bash
export APP="8b11760e-6cfb-48e1-9ec2-f76da193e060"
python3 ../01_periodical-uplink/consume.py --state aabbccdd60915001
```

## Archivos
```
sketches/TTGO_BMP280_v1.ino    firmware (BMP280 + LMIC, sub-banda 2, uplink 60s)
payload_decoder.js             codec ChirpStack (fPort 2)
provision.sh                   alta reproducible por API
scripts/upload_codec.sh        adjunta el codec al device profile
scripts/subscribe.sh           stream MQTT de uplinks
credentials.json               IDs, credenciales (MSB y LMIC/LSB), payload
```
