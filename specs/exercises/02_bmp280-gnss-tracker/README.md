# Ejercicio 02 — BMP280 + GNSS tracker (tu primer payload de sensor)

> **En una frase:** conectas un **sensor BMP280** (temperatura y presión) por I²C al nodo y ves su
> lectura llegar a ChirpStack **ya decodificada** (`{temperature, pressure}`), no como bytes crudos.
> **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG** + sensor **BMP280 (I²C)**.
> **Banda:** US915 (por defecto; hay binario EU868 alternativo). El binario **ya viene compilado**
> en `artifacts/` — aquí **no compilas nada**, solo cableas, flasheas y das de alta.

## 🎯 Qué vas a conseguir
Un **tracker** que se une a tu red LoRaWAN (**join OTAA**) y envía cada cierto tiempo la
**temperatura y la presión** que lee del BMP280 en **fPort 10**. Cuando subas el codec, en ChirpStack
verás el campo **`object`** ya decodificado, listo para un dashboard. Por MQTT recibirás un JSON así:

```json
{ "deviceInfo": { "devEui": "aabbccdd00915001" },
  "fPort": 10,
  "data": "AgkpA/U=",
  "object": { "version": 2, "temperature": 23.45, "pressure": 1013 } }
```

> El LR1110 además ejecuta **scans GNSS y Wi-Fi** (geolocalización) en su propio fPort; esos frames
> también aparecen en ChirpStack y los resuelve el servicio de geoloc. En este ejercicio nos
> centramos en el **payload de sensor** (fPort 10), que es el que decodificamos.

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, no continúes):

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Tu `TOKEN`** (API key de ChirpStack) exportado — lo creas en el [Ejercicio 00](../00_chirpstack-docker/): web `:8080` → *Tenant* → *API keys*.
- [ ] **Un gateway US915** con cobertura (sub-banda 2 / FSB2), encendido y *online* en ChirpStack → [Ejercicio 07](../07_esp-1ch-gateway/) o uno comercial. **Sin gateway el join nunca ocurre.**
- [ ] **Hardware:** Nucleo-L476RG + shield **LR1110**, un módulo **BMP280** (GY-BMP280, 6 pines), **4 cables** dupont y el cable USB.
- [ ] **Herramientas:** una utilidad para flashear + un **terminal serie** (115200 8N1) → [Wiki: Flashear y ver la serie](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Flashear-y-ver-la-serie). Para el consumo, **Python 3** (`pip install requests paho-mqtt`).

> ℹ️ Los `./*.sh` y `python3` corren en **bash** (Linux/macOS o **WSL** en Windows). La primera vez,
> hazlos ejecutables: `chmod +x provision.sh scripts/*.sh`.
> **Credenciales de este ejercicio** (US915): DevEUI `aabbccdd00915001`, JoinEUI `aabbccddeeff0000`,
> AppKey `A915A915…A915`. Para EU868: DevEUI `aabbccdd00868001`, AppKey `A868A868…A868`.
> Completas en [`credentials.json`](credentials.json).

## 📟 Hardware y conexiones
El BMP280 cuelga del **I²C1** del Nucleo. El firmware fija estos pines **en código** (no
configurables por build) y usa la **dirección 0x76** (por eso SDO va a GND):

| Pin del módulo BMP280 | Señal | Nucleo (rótulo serigrafía) | MCU | Motivo |
|-----------------------|-------|----------------------------|-----|--------|
| **VCC / VIN** | 3.3 V | **3V3** (CN6-4) | — | alimentación 3.3 V |
| **GND** | Masa | **GND** (CN6-6) | — | masa común |
| **SCL / SCK** | Reloj I²C | **D15** (CN5-10) | **PB_8** | `I2C1_SCL` |
| **SDA / SDI** | Datos I²C | **D14** (CN5-9) | **PB_9** | `I2C1_SDA` |
| **CSB / CS** | Selección de bus | **3V3** | — | atar a VCC fuerza el **modo I²C** (a GND entraría en SPI) |
| **SDO / SDD** | Bit de dirección | **GND** | — | GND → dirección **0x76** (a 3V3 sería 0x77) |

```
     BMP280 (GY-BMP280)                     STM32 Nucleo-L476RG
   ┌────────────────────┐               ┌──────────────────────────────┐
   │ VCC ───────────────┼──────────────►│ 3V3      (CN6-4)              │
   │ GND ───────────────┼──────────────►│ GND      (CN6-6)             │
   │ SCL ───────────────┼──────────────►│ D15 / PB_8   (CN5-10) I2C1_SCL│
   │ SDA ───────────────┼──────────────►│ D14 / PB_9   (CN5-9)  I2C1_SDA│
   │ CSB ───────────────┼───► 3V3       (fuerza modo I²C)               │
   │ SDO ───────────────┼───► GND       (dirección I²C = 0x76)          │
   └────────────────────┘               └──────────────────────────────┘
```

> **⚠️ Nunca enciendas la radio LR1110 sin antena.** Y usa **3V3, nunca 5V** para el BMP280.
> Cableado corto (< 20 cm) y no cruces SDA/SCL. La mayoría de módulos GY-BMP280 ya traen pull-ups,
> así que normalmente no hacen falta resistencias externas.
> Diagrama visual: [`../../bmp280-gnss-tracker/pinout_diagram.html`](../../bmp280-gnss-tracker/pinout_diagram.html) ·
> detalle completo de pinout y cableado: [`FLASH_AND_CHIRPSTACK.md`](../../bmp280-gnss-tracker/FLASH_AND_CHIRPSTACK.md).

## 🪜 Paso a paso

### 1. Cablea el BMP280
Conecta los **4 cables** según la tabla de arriba (VCC→3V3, GND→GND, SCL→D15/PB_8, SDA→D14/PB_9)
y ata **CSB→3V3** y **SDO→GND**. Hazlo con la placa **desconectada** del USB.

**Salida esperada:** nada aún; solo verifica visualmente que no hay cortos y que SDA/SCL no están
invertidos. Lo confirmarás en el paso 5, cuando la traza serie muestre `BMP280 detected`.

### 2. Flashea el binario
Graba el `.hex` (o `.bin`) de tu banda en la Nucleo. Detalle del método (arrastrar al disco
`NODE_L476RG` o STM32CubeProgrammer) en [`../COMMON_FLASH.md`](../COMMON_FLASH.md). **La región del
binario debe coincidir con la del device profile** (US915 con US915, EU868 con EU868).
- **US915:** `artifacts/app_lr1110_US_915.hex`
- **EU868:** `artifacts/app_lr1110_EU_868.hex`

**Salida esperada:** el LED del ST-LINK parpadea al copiar y la placa se reinicia sola al terminar.

### 3. Provisiona el device en ChirpStack
Registra el device (idempotente: puedes repetirlo sin miedo). El script **autodetecta** tu tenant y
tu aplicación `Demos-LR1110` —los crea si hace falta— y crea un **device profile propio por banda**
(`Tracker-BMP280-US915`) para poder adjuntarle el codec sin mezclarlo con otros payloads:
```bash
# desde specs/exercises/02_bmp280-gnss-tracker/
export TOKEN="tu_api_key"
./provision.sh us915          # o: ./provision.sh eu868
```
**Salida esperada:**
```
== tenant: 52f14cd4-... ==
== application: 5bc22cfa-... ==
== [us915] device profile Tracker-BMP280-US915 ==
  creando device profile Tracker-BMP280-US915 (US915)...
  device profile id: a1b2c3d4-...
== [us915] device aabbccdd00915001 ==
  creado.
== [us915] AppKey (nwkKey en 1.0.x) ==
  AppKey fijada en nwkKey.
== LISTO: aabbccdd00915001 provisionado en US915 (app 5bc22cfa-... , profile a1b2c3d4-...) ==
   Ahora adjunta el codec BMP280 a este device profile:
       ./scripts/upload_codec.sh a1b2c3d4-...
   Y flashea artifacts/app_lr1110_US_915.bin y resetea la placa (B2).
   Exporta tu app para consumir:   export APP=5bc22cfa-...
```
**Apunta el `device profile id`** (lo necesitas en el paso 4) y ejecuta la línea `export APP=…`
(la usarás en el paso 6).

### 4. Sube el codec (decoder BMP280)
Adjunta [`payload_decoder.js`](payload_decoder.js) al device profile que imprimió el paso 3.
**Sin esto, el campo `object` del MQTT viene vacío** y solo verás los bytes crudos:
```bash
# desde specs/exercises/02_bmp280-gnss-tracker/  (usa el device-profile-id del paso 3)
./scripts/upload_codec.sh a1b2c3d4-...
```
**Salida esperada:**
```
Codec JS subido al device profile a1b2c3d4-.... Los próximos uplinks en fPort 10 traerán 'object' decodificado.
```

### 5. Prueba el join
Abre la **traza serie** (115200 8N1) y pulsa el botón **RESET (B2)** de la Nucleo.
**Salida esperada** (algo así — las líneas exactas del BMP280 y `Joined` son las importantes):
```
[INFO] LoRa Basics Modem - Geolocation (BMP280 + GNSS)
[INFO] BMP280 detected (chip-id 0x58)
[INFO] Joining...
[INFO] Joined
[INFO] TXDONE   fPort 10
```
En cuanto veas **`Joined`**, en ChirpStack (device → *LoRaWAN frames*) aparecerán el JoinRequest →
JoinAccept, los uplinks de sensor (fPort 10) y los frames de geoloc (GNSS/Wi-Fi).

> Si en vez de `BMP280 detected` ves `BMP280 not detected - environmental uplinks disabled`, revisa
> el cableado del paso 1 (alimentación 3V3, CSB→3V3, SDO→GND, SDA/SCL sin invertir).

### 6. Consume los datos (la base del dashboard)
```bash
# desde specs/exercises/02_bmp280-gnss-tracker/
# a) Stream de payloads en tiempo real por MQTT (trae 'object' si subiste el codec; Ctrl-C para salir):
export APP=5bc22cfa-...          # el que imprimió provision.sh
./scripts/subscribe.sh                    # tracker US915 por defecto
./scripts/subscribe.sh aabbccdd00868001   # tracker EU868

# b) Estado y métricas de enlace por REST (reutiliza el consume.py del ejercicio 01):
python3 ../01_periodical-uplink/consume.py --state aabbccdd00915001
#    o el stream vía Python:
python3 ../01_periodical-uplink/consume.py --stream
```
**Salida esperada** (MQTT): un JSON por uplink en el topic
`application/<APP>/device/aabbccdd00915001/event/up`, con el campo **`object`**
`{version, temperature, pressure}` ya decodificado.

## ✅ Cómo saber que funcionó
- [ ] La traza serie muestra **`BMP280 detected (chip-id 0x58)`** y luego **`Joined`**.
- [ ] En ChirpStack el device tiene *last seen* reciente y ves **JoinRequest→JoinAccept + uplinks** en *LoRaWAN frames*.
- [ ] Por **MQTT** llega un JSON en **fPort 10** con el campo **`object`** = `{version, temperature, pressure}`.
- [ ] (Opcional) Aparecen además frames de **geolocalización** (GNSS/Wi-Fi) en otro fPort.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| Serie: `BMP280 not detected - environmental uplinks disabled` | Cableado I²C mal, o dirección incorrecta | Revisa VCC→3V3, **CSB→3V3**, **SDO→GND** (dir. 0x76) y que SDA/SCL no estén invertidos |
| El `object` del MQTT viene **vacío** (`{}`) | No subiste el codec al device profile | Ejecuta `./scripts/upload_codec.sh <DP_ID>` (paso 4) con el id del paso 3 |
| Serie con `BME280` / chip-id `0x60` no detectado | Tu módulo es un **BME280**, no un BMP280 (el driver valida 0x58) | Usa un **BMP280** (0x58); el GNSS seguiría funcionando igual |
| La serie se queda en `Joining…` para siempre | No hay gateway US915 con cobertura, o está *offline* | Enciende un gateway (ej.07) y verifica que está *online* en ChirpStack |
| `Joining…` eterno con gateway OK | Región del binario ≠ región del device profile, o claves mal | Usa el binario US915 con profile US915; reejecuta `./provision.sh us915` |
| Caracteres basura en la serie | Baudios mal | Ponlo a **115200 8N1** |
| `subscribe.sh`: *No such container* | ChirpStack no está levantado | Arranca el ej.00 (`docker compose up -d`); el script ya detecta el nombre del contenedor |
| No llega nada por MQTT | `APP` sin exportar o incorrecto | `export APP=<id>` (el que imprimió `provision.sh`) |

## 📤 Los datos (payload)
- **fPort 10:** payload ambiental del BMP280 → **5 bytes big-endian**.
- **Otro fPort:** scans de **geolocalización** GNSS/Wi-Fi (los resuelve el servicio de geoloc; este
  decoder no los toca).

| Offset | Campo | Tipo | Escala |
|--------|-------|------|--------|
| 0 | version | uint8 | fijo `0x02` |
| 1-2 | temperatura | int16 **BE** | °C × 100 (con signo) |
| 3-4 | presión | uint16 **BE** | hPa |

**Vector de ejemplo** — 23.45 °C y 1013 hPa (ver [`payload_decoder.js`](payload_decoder.js)):
```
23.45 °C → 2345 = 0x0929      1013 hPa → 0x03F5      version = 0x02
bytes (fPort 10):  02 09 29 03 F5   (base64 "AgkpA/U=")
decodificado:      { version: 2, temperature: 23.45, pressure: 1013 }
```

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 01 · Periodical Uplink](../01_periodical-uplink/)
- ➡️ Siguiente: [Ejercicio 03 · Hardware Modem](../03_hw-modem/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json) (US915 y EU868).
- **Archivos:** `provision.sh` (alta + device profile por banda) · `payload_decoder.js` (codec fPort 10) · `scripts/upload_codec.sh` (adjunta el codec) · `scripts/subscribe.sh` (MQTT sin Python) · `artifacts/` (binarios).
- **Guías específicas:** [Pinout y flasheo del tracker](../../bmp280-gnss-tracker/FLASH_AND_CHIRPSTACK.md) · [Diagrama de pines](../../bmp280-gnss-tracker/pinout_diagram.html).
- **Guías comunes:** [Flashear](../COMMON_FLASH.md) · [ChirpStack API](../COMMON_CHIRPSTACK_API.md) · [Compilar (opcional)](../COMMON_BUILD.md).

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
