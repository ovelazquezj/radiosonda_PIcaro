# Ejercicio 06 — TTGO ESP32 + BMP280 (tu primer sensor real)

> **En una frase:** conectas un **sensor BMP280** (temperatura + presión) a un nodo **TTGO ESP32
> LoRa v1** por I²C y envías sus lecturas por **LoRaWAN OTAA cada 60 s**, ya **decodificadas** por
> ChirpStack. Es el salto del payload de prueba (ej.05) a **datos listos para un dashboard**.
> **Plataforma:** radio **SX1276** sobre placa **TTGO ESP32 LoRa v1** (+ OLED), firmware
> **Arduino/MCCI LMIC**. **Banda:** US915 (sub-banda 2 / FSB2). Aquí **compilas y flasheas** el
> sketch tú mismo desde el Arduino IDE.

## 🎯 Qué vas a conseguir
Un nodo que se **une** a tu red LoRaWAN y envía cada minuto la **temperatura** y la **presión** que
lee del BMP280. Al terminar, en ChirpStack verás el device con *last seen* reciente y, por MQTT, un
JSON con el campo **`object` ya decodificado** (la fuente directa de un dashboard):

```json
{ "deviceInfo": { "devEui": "aabbccdd60915001" },
  "fPort": 2,
  "object": { "version": 1, "temperature": 23.45, "pressure": 1013 } }
```

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, no continúes):

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Tu `TOKEN`** (API key de ChirpStack) — lo creas en el [Ejercicio 00](../00_chirpstack-docker/): web `:8080` → *Tenant* → *API keys*.
- [ ] **Un gateway US915 (sub-banda 2 / FSB2)**, encendido y *online* en ChirpStack. Lo ideal es un gateway **multicanal** US915; también sirve el del [Ejercicio 07](../07_esp-1ch-gateway/) **configurado para US915** (es de **1 canal**: entonces tendrías que fijar también este nodo a ese canal — ver su [aviso de banda](../07_esp-1ch-gateway/)). **Sin gateway, el nodo se queda en `EV_JOINING` para siempre.**
- [ ] **Hardware:** TTGO ESP32 LoRa v1 (SX1276 + OLED) · módulo **BMP280 (GY-BMP280)** · **6 cables** dupont hembra-hembra · cable USB. ⚠️ **No enciendas la placa sin su antena** conectada.
- [ ] **Herramientas:** **Arduino IDE** con el **core ESP32** instalado y estas librerías (Gestor de librerías): **MCCI LoRaWAN LMIC**, **U8g2**, **Adafruit BMP280** (+ **Adafruit Unified Sensor**) → [Wiki: Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación).

> ℹ️ Los `./*.sh` y `python3` corren en **bash** (Linux/macOS o **WSL** en Windows).
> **Credenciales de este ejercicio** (ChirpStack, MSB): DevEUI `aabbccdd60915001`,
> JoinEUI `aabbccddeeff0000`, AppKey `60156015…6015`. En el **sketch** van en otro orden —lo explica
> la [nota de bytes](#-orden-de-bytes-lsb-en-el-sketch-msb-en-chirpstack) al final.
> Detalle completo en [`credentials.json`](credentials.json).

## 📟 Hardware y conexiones

El BMP280 va por **I²C, compartiendo el bus del OLED** (SDA=GPIO21, SCL=GPIO22). El OLED responde en
`0x3C` y el BMP280 en `0x76` → no colisionan.

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

> **Pines de radio (ya usados por el módulo LoRa, NO tocar):** SCK=5, MISO=19, MOSI=27, NSS=18,
> RST=14, DIO0=26, DIO1=33, DIO2=32. *(DIO1=GPIO33 puede requerir un puente físico en algunas placas
> TTGO para la ventana RX.)*

## 🪜 Paso a paso

### 1. Provisiona el device en ChirpStack
Registra la aplicación, el device profile, el device y sus claves (idempotente: puedes repetirlo).
Luego **sube el codec** para que ChirpStack entregue el `object` decodificado. El script
**autodetecta** tu tenant y tu aplicación por nombre —los crea si hace falta—, así que solo necesitas
el `TOKEN`:
```bash
# desde specs/exercises/06_ttgo-bmp280/
export TOKEN="tu_api_key"
./provision.sh                                                   # app + profile + device + keys
./scripts/upload_codec.sh f46ea82a-7608-4a23-a744-d1bbbcc4befa   # codec (datos decodificados)
```
**Salida esperada:**
```
== tenant: f8a271ec-... ==
== Application TTGO-BMP280 ==
   APP=8b11760e-...
== Device profile TTGO-BMP280-US915 (US915, 1.0.3, OTAA) ==
   DP=f46ea82a-...
== Device aabbccdd60915001 ==
   creado
   AppKey fijada en nwkKey
== LISTO. Sube el codec:  ./scripts/upload_codec.sh f46ea82a-... ==
Codec JS subido al device profile f46ea82a-....
```

### 2. Configura la región de LMIC
La librería **MCCI LMIC** decide su banda en un fichero aparte, **no** en el sketch. En tu carpeta de
librerías de Arduino (`Documentos/Arduino/libraries/`) abre y edita:
`MCCI_LoRaWAN_LMIC_library/project_config/lmic_project_config.h` *(el comentario del sketch lo llama
`arduino_lmic_project_config.h`; es el mismo fichero)*. Deja activas **exactamente estas dos líneas**
y **comenta** el resto de bandas (`CFG_eu868`, etc.):
```c
#define CFG_us915 1          // banda US915 (comenta CFG_eu868 y las demás)
#define CFG_sx1276_radio 1   // radio del TTGO
```
**Salida esperada:** ninguna todavía; es solo edición. Si te la saltas, el sketch **no compilará**
o compilará para la banda equivocada y **nunca hará join**.

### 3. Cablea el BMP280
Conéctalo según la tabla de la sección [📟 Hardware y conexiones](#-hardware-y-conexiones):
**SDA=21, SCL=22, CSB→3V3, SDO→GND** (dirección `0x76`).
**Salida esperada:** nada aún; lo verifica el paso 4 (`BMP280 OK`).

### 4. Compila y flashea el sketch
Abre [`sketches/TTGO_BMP280_v1.ino`](sketches/TTGO_BMP280_v1.ino) en el Arduino IDE, elige la placa
**"TTGO LoRa32-OLED"** y el puerto COM correcto, y pulsa *Upload*. Luego abre el **Monitor Serie a
115200**.
**Salida esperada** (arranque):
```
== TTGO + BMP280 (OTAA US915) ==
BMP280 OK
```
> Si en su lugar ves `ERROR: BMP280 no detectado (...)`, revisa el cableado del paso 3 (SDO→GND para
> `0x76`, CSB→3V3, SDA/SCL en 21/22).

### 5. Observa el join y los uplinks
En cuanto arranca, el nodo hace **join** y empieza a enviar cada 60 s. Mira el **Monitor Serie** y el
**OLED**.
**Salida esperada** (serie, algo así):
```
EV_JOINING
EV_JOINED
Encolado: T=23.45C P=1013.0hPa -> 01092903F5
EV_TXCOMPLETE
```
En cuanto veas **`EV_JOINED`**, en ChirpStack (device → *LoRaWAN frames*) aparecerán JoinRequest →
JoinAccept y los uplinks. En el **OLED** leerás `Joined!` y luego, en cada envío, `T:`, `P:` y el
contador `TX:`. Cada 60 s se repite `Encolado…` → `EV_TXCOMPLETE`.

### 6. Consume los datos (la base del dashboard)
```bash
# desde specs/exercises/06_ttgo-bmp280/
# a) Stream de payloads en tiempo real por MQTT (Ctrl-C para salir):
./scripts/subscribe.sh                      # usa el mosquitto del contenedor de ChirpStack

# b) Estado y métricas de enlace por REST (reutiliza el consume.py del ej.01):
export TOKEN="tu_api_key"                    # el REST necesita tu API key
export APP="8b11760e-6cfb-48e1-9ec2-f76da193e060"
python3 ../01_periodical-uplink/consume.py --state aabbccdd60915001
```
**Salida esperada** (MQTT): un JSON por uplink en el topic
`application/<APP>/device/aabbccdd60915001/event/up`, con el campo **`object`** decodificado:
```json
{ "deviceInfo": { "devEui": "aabbccdd60915001" },
  "fPort": 2,
  "object": { "version": 1, "temperature": 23.45, "pressure": 1013 } }
```

## ✅ Cómo saber que funcionó
- [ ] La traza serie muestra **`BMP280 OK`** en el arranque.
- [ ] Muestra **`EV_JOINING` → `EV_JOINED`** y luego, cada 60 s, un **`Encolado…` → `EV_TXCOMPLETE`**; el OLED muestra `Joined!` y las lecturas `T/P/TX`.
- [ ] En ChirpStack el device tiene *last seen* reciente y ves **JoinRequest→JoinAccept + uplinks** en *LoRaWAN frames*.
- [ ] Por **MQTT** llega un JSON con el campo **`object`** = `{version, temperature, pressure}`.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| Serie: `ERROR: BMP280 no detectado` | Dirección/cableado I²C del BMP280 | **SDO→GND** (0x76), **CSB→3V3**, **SDA=21 / SCL=22**; revisa 3V3 y GND |
| Se queda en `EV_JOINING` para siempre | No hay gateway US915 (FSB2) *online*, o el nodo transmite en otra sub-banda | Enciende un gateway US915 FSB2 (ej.07 configurado para US915, o comercial) y verifica que está *online*; el sketch ya usa `LMIC_selectSubBand(1)` = FSB2 |
| `EV_JOIN_FAILED` o join eterno con gateway OK | DevEUI/JoinEUI copiados **sin invertir** al array del sketch, o AppKey mal | En el sketch, DevEUI/AppEUI van en **LSB** (al revés que ChirpStack) y el AppKey en **MSB** — ver la [nota de bytes](#-orden-de-bytes-lsb-en-el-sketch-msb-en-chirpstack) |
| No compila (`CFG_us915`/región) | Falta editar el fichero de LMIC | Deja `#define CFG_us915 1` y `#define CFG_sx1276_radio 1` en `lmic_project_config.h` (paso 2) |
| Caracteres basura en la serie | Baudios mal | Ponlo a **115200 8N1** |
| Llega el uplink pero **`object` vacío** | El codec no está subido al device profile | `./scripts/upload_codec.sh f46ea82a-7608-4a23-a744-d1bbbcc4befa` |
| `subscribe.sh`: *no encuentro el contenedor mosquitto* | ChirpStack no está levantado | Arranca el ej.00 (`docker compose up -d`); el script ya detecta el nombre del contenedor |

## 📤 Los datos (payload)
5 bytes en **fPort 2** (ver [`payload_decoder.js`](payload_decoder.js)):

| Offset | Campo | Tipo | |
|--------|-------|------|--|
| 0 | version | uint8 | `0x01` |
| 1-2 | temperature | int16 **BE** | °C × 100 |
| 3-4 | pressure | uint16 **BE** | hPa |

Ejemplo: `01 09 29 03 F5` → `{version:1, temperature:23.45, pressure:1013}` (verificado). El campo
**`object`** del MQTT es la fuente directa del dashboard.

## 🔤 Orden de bytes: LSB en el sketch, MSB en ChirpStack
El **mismo** DevEUI/JoinEUI se escribe en **dos órdenes distintos según dónde**: en el **sketch
(LMIC) en LSB** (byte menos significativo primero) y en **ChirpStack en MSB** (tal como se lee). El
**AppKey NO** se invierte: va en MSB en ambos lados.

| Campo | En el **sketch** (LMIC, arrays `os_getDevEui`/`os_getArtEui`) | En **ChirpStack** (UI/API) |
|-------|--------------------------------------------------------------|----------------------------|
| **DevEUI** | `01 50 91 60 DD CC BB AA`  ← **LSB (invertido)** | `AA BB CC DD 60 91 50 01`  ← **MSB** |
| **JoinEUI/AppEUI** | `00 00 FF EE DD CC BB AA`  ← **LSB (invertido)** | `AA BB CC DD EE FF 00 00`  ← **MSB** |
| **AppKey** | `60 15 60 15 … 60 15`  ← **MSB (igual)** | `60 15 60 15 … 60 15`  ← **MSB (igual)** |

> **Error típico:** copiar el DevEUI de ChirpStack (MSB) directo al array del sketch sin invertir (o
> viceversa) → ChirpStack registra un DevEUI que no coincide con el que transmite el nodo →
> `Unknown device` y nunca hace join. **Regla:** en el array C, DevEUI/AppEUI van **al revés** que en
> ChirpStack; el AppKey, **igual**.

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 05 · TTGO ESP32 LoRa (nodo LMIC)](../05_ttgo-lora32/)
- ➡️ Siguiente: [Ejercicio 07 · Gateway de 1 canal](../07_esp-1ch-gateway/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json) (MSB de ChirpStack **y** arrays LMIC/LSB).
- **Archivos:** `sketches/TTGO_BMP280_v1.ino` (firmware) · `payload_decoder.js` (codec fPort 2) · `provision.sh` (alta por API) · `scripts/upload_codec.sh` (adjunta el codec) · `scripts/subscribe.sh` (MQTT sin Python).
- **Guías comunes:** [Flashear](../COMMON_FLASH.md) · [ChirpStack API](../COMMON_CHIRPSTACK_API.md).

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
