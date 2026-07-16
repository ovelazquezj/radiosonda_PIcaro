# Ejercicio 05 — TTGO ESP32 LoRa (nodo de terceros con LMIC)

> **En una frase:** unes a tu ChirpStack un **nodo LoRaWAN que no es nuestro** —una placa **TTGO
> ESP32 LoRa v1** con firmware **Arduino / MCCI LMIC**— y ves sus uplinks decodificados. Es el
> *"bring-your-own-device"* del curso: aquí **sí compilas** (en el Arduino IDE) y flasheas tú.
> **Plataforma:** TTGO ESP32 + radio **SX1276** + OLED SSD1306. **Banda:** US915 (sub-banda 2 / FSB2).

## 🎯 Qué vas a conseguir
Un nodo TTGO que hace **join OTAA**, muestra **`CONECTADO`** en su OLED y envía un paquete de 8 bytes
cada 30 s. Al terminar, en ChirpStack lo verás con *last seen* reciente y, por MQTT, un JSON con el
payload ya **decodificado** en el campo `object`:

```json
{ "deviceInfo": { "devEui": "02389205358e71db" },
  "fPort": 1,
  "data": "mQUAEgEAALE=",
  "object": { "magic": 153, "counter": 5, "timestamp": 274, "checksum_ok": true } }
```

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, el join **no** ocurrirá):

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Tu `TOKEN`** (API key de ChirpStack) — lo creas en el [Ejercicio 00](../00_chirpstack-docker/): web `:8080` → *Tenant* → *API keys*.
- [ ] **Un gateway US915 en la sub-banda 2 (FSB2, canales 8-15)**, encendido y *online* en ChirpStack → [Ejercicio 07](../07_esp-1ch-gateway/) o uno comercial. **⚠️ Sin gateway en FSB2, el nodo se queda en `EV_JOINING` para siempre.**
- [ ] **Hardware:** placa **TTGO ESP32 LoRa v1** (SX1276 + OLED) con **antena** puesta, y su cable USB.
- [ ] **Arduino IDE** con el **core ESP32** instalado y estas **librerías**: **MCCI LoRaWAN LMIC** y **U8g2** → [Wiki: Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación).
- [ ] **Configurada la región de LMIC** editando `arduino_lmic_project_config.h` (paso 1) → [Wiki: Compilar el TTGO en Arduino](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Compilar-el-TTGO-en-Arduino).
- [ ] Para provisionar/consumir: **bash** (Linux/macOS o **WSL** en Windows) y **Python 3**.

> ℹ️ Los `./*.sh` y `python3` corren en **bash**. **Credenciales de este ejercicio** (MSB, para
> ChirpStack): DevEUI `02389205358e71db`, JoinEUI `505246f87143fd8a`,
> AppKey `8ac583dfeec76c81ffd19ccfe76b73bf`. En el sketch los EUIs van **al revés** (LSB) — se explica
> en la [📖 Nota didáctica](#-nota-didáctica--por-qué-fallaba-el-join).

## 📟 Hardware y conexiones
Placa **integrada** — no hay que cablear nada: el SX1276 y el OLED ya vienen conectados al ESP32. El
sketch usa este mapa de pines (definido en `sketches/TTGO_LoRaWAN_v3.ino`), correcto para el TTGO v1:

| Señal | GPIO | | Señal | GPIO |
|---|---|---|---|---|
| SCK  | 5  | | RST  | 14 |
| MISO | 19 | | DIO0 | 26 |
| MOSI | 27 | | DIO1 | 33 |
| NSS (CS) | 18 | | DIO2 | 32 |

OLED SSD1306 por I2C: **SDA=21, SCL=22** (dirección `0x3C`). **⚠️ Nunca enciendas la radio sin antena.**

> 🧪 **¿Prefieres ESP-IDF nativo?** Hay una versión equivalente de este nodo en
> [`esp-idf/`](esp-idf/) — mismo hardware y mismo device en ChirpStack, pero con **ESP-IDF v5** y
> **RadioLib** (en vez de Arduino/LMIC). Se compila con `idf.py build flash monitor`.

## 🪜 Paso a paso

### 1. Instala core + librerías y configura la región de LMIC
En el Arduino IDE, con el **core ESP32** y las librerías **MCCI LMIC** + **U8g2** ya instaladas
([Wiki: Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación)),
edita el archivo de configuración de la librería LMIC, **`arduino_lmic_project_config.h`** (está dentro
de la carpeta de la librería, en `.../MCCI_LoRaWAN_LMIC_library/project_config/`). Deja **solo** la
región US915 y la radio SX1276 activas:
```c
//#define CFG_eu868 1
#define CFG_us915 1          // <- región US915 (nuestro caso)
#define CFG_sx1276_radio 1   // <- la radio del TTGO v1
```
> 🔑 Este paso es el más olvidado: la librería LMIC **se compila por región**. Si dejas `CFG_eu868`,
> el nodo transmitirá en frecuencias europeas y **nunca** hará join contra un gateway US915.
> Detalle completo en [Wiki: Compilar el TTGO en Arduino](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Compilar-el-TTGO-en-Arduino).

### 2. Abre el sketch y confirma la sub-banda
Abre [`sketches/TTGO_LoRaWAN_v3.ino`](sketches/TTGO_LoRaWAN_v3.ino) en el Arduino IDE. Comprueba que en
`setup()` está seleccionada la **sub-banda 2** (índice base-0 = `1`):
```c
LMIC_selectSubBand(1);   // Sub-banda 2 (FSB2) -> canales 8-15 (903.9-905.3 MHz) = la del gateway
```
Las credenciales ya vienen puestas en el sketch (arrays `DEVEUI`/`APPEUI`/`APPKEY`); no toques nada más.

### 3. Selecciona placa y puerto
En el menú **Tools/Herramientas**:
- **Board:** *"TTGO LoRa32-OLED"* (o *"ESP32 Dev Module"* si tu core no la lista).
- **Port:** el `COMx` (Windows) o `/dev/ttyUSB0` (Linux/WSL) donde aparece la placa al enchufarla.

### 4. Compila y sube (Upload)
Pulsa **Upload (→)**. El IDE compila y graba el firmware por USB.

**Salida esperada:** `Done uploading.` (o `Hard resetting via RTS pin...`) en la parte inferior del IDE.

### 5. Abre el Monitor Serie (115200)
Abre **Tools → Serial Monitor** a **115200 baudios**. Verás el banner de arranque y, tras unos
segundos, el **join** y el primer envío.

**Salida esperada** (recortada — los números de timestamp varían):
```
========================================
   TTGO T3 LoRaWAN - EJEMPLO DIDÁCTICO
========================================
📡 CONFIGURACIÓN DE RED:
  ├─ Región:           US915 (América)
  ├─ Sub-banda:        2 (Canales 8-15)
  └─ Intervalo TX:     30 segundos
...
🚀 INICIANDO PROCEDIMIENTO DE JOIN
...
2739: EV_JOINING - Intentando unirse a la red...
  → Enviando Join Request con DevEUI y JoinEUI
  → Esperando Join Accept del Network Server...
128473: ========================================
✅ EV_JOINED - ¡CONECTADO A CHIRPSTACK!
========================================
...
📤 ENVIANDO PAQUETE #1
...
  Payload (HEX):         99 01 00 12 01 00 00 AD
...
📤 EV_TXCOMPLETE - Transmisión completa
  → Uplink enviado exitosamente
  ⏱️  Próximo uplink en 30 segundos
```
En cuanto veas **`EV_JOINED`**, el OLED cambia a **`Estado: CONECTADO`** y en ChirpStack
(device → *LoRaWAN frames*) aparecen JoinRequest → JoinAccept y los primeros uplinks.

### 6. Provisiona el device en ChirpStack
Registra el device (idempotente). El script **autodetecta** tu tenant y busca/crea la aplicación por
nombre, así que solo necesitas el `TOKEN`:
```bash
# desde specs/exercises/05_ttgo-lora32/
export TOKEN="tu_api_key"
./provision.sh                    # app + device profile + device + AppKey
```
**Salida esperada** (algo así):
```
== tenant: f8a271ec-... ==
== Application TTGO-LoRa32 ==
   APP=d19cf2cb-...
== Device profile TTGO-US915 (US915, 1.0.3, OTAA) ==
   DP=150ae510-...
== Device 02389205358e71db ==
   creado
   AppKey fijada en nwkKey
== LISTO. Sube el codec para datos decodificados:  ./scripts/upload_codec.sh 150ae510-... ==
```
Ahora sube el **codec** al device profile para que ChirpStack entregue el `object` decodificado
(usa el `DP=` que imprimió el paso anterior):
```bash
export TOKEN="tu_api_key"         # si abriste otra terminal, vuelve a exportarlo
./scripts/upload_codec.sh 150ae510-0577-4352-9cd3-39c354889474
```
**Salida esperada:** `Codec JS subido al device profile … Los próximos uplinks traerán 'object' decodificado.`

### 7. Consume los datos (la base del dashboard)
```bash
# desde specs/exercises/05_ttgo-lora32/
# a) Stream de uplinks por MQTT (trae 'object' si subiste el codec en el paso 6):
./scripts/subscribe.sh
#    (el script detecta el contenedor mosquitto de ChirpStack automáticamente)

# b) Estado + métricas de enlace por REST (reutiliza el consume.py del ejercicio 01):
export TOKEN="tu_api_key"
export APP="d19cf2cb-5a1b-452b-8cfe-638784e5fb80"
python3 ../01_periodical-uplink/consume.py --state 02389205358e71db
```
**Salida esperada** (MQTT): un JSON por uplink con `object: {magic, counter, timestamp, checksum_ok}`.

## ✅ Cómo saber que funcionó
- [ ] El **OLED** muestra **`Estado: CONECTADO`** y el contador de **Paquetes** subiendo.
- [ ] En el **Monitor Serie** ves **`EV_JOINED`** y luego un **`EV_TXCOMPLETE`** cada 30 s.
- [ ] En **ChirpStack** el device tiene *last seen* reciente y frames en *LoRaWAN frames*.
- [ ] Por **MQTT** llega un JSON con el campo **`object`** decodificado (`counter`, `timestamp`, `checksum_ok`).

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| El OLED se queda en `JOINING...` y la serie repite `EV_JOINING` sin parar | No hay gateway US915 en **FSB2**, o está *offline* | Enciende un gateway en **sub-banda 2** (ej.07) y verifica que está *online* en ChirpStack |
| `EV_JOINING` eterno con gateway OK | Región de LMIC mal compilada, o `selectSubBand` distinto de 1 | Revisa el **paso 1** (`CFG_us915 1`) y el **paso 2** (`LMIC_selectSubBand(1)`) |
| ChirpStack dice *"Unknown device"* / no aparece | El DevEUI que transmite el nodo ≠ el registrado (orden de bytes) | Reejecuta `./provision.sh` (registra el DevEUI real del sketch); ver [📖 Nota didáctica](#-nota-didáctica--por-qué-fallaba-el-join) |
| Llega el uplink pero **`object` vacío** | Falta el codec en el device profile | Ejecuta `./scripts/upload_codec.sh <DP>` (paso 6) y espera al siguiente uplink |
| `subscribe.sh`: *No such container* | ChirpStack no está levantado | Arranca el ej.00 (`docker compose up -d`); el script ya detecta el nombre del contenedor mosquitto |
| Nada por MQTT / REST | `TOKEN`/`APP` sin exportar | `export TOKEN=…` y `export APP=…` en la misma terminal |

## 📤 Los datos (payload)
8 bytes en **fPort 1** (ver [`payload_decoder.js`](payload_decoder.js)):

| Offset | Campo | Tipo | |
|---|---|---|---|
| 0 | magic | uint8 | fijo `0x99` |
| 1-2 | counter | uint16 **LE** | contador de paquetes |
| 3-6 | timestamp | uint32 **LE** | uptime del dispositivo (s) |
| 7 | checksum | uint8 | `suma(bytes 0..6) & 0xFF` |

Vector real capturado: `99 05 00 12 01 00 00 B1` → `counter=5, timestamp=274, checksum_ok=true`.
El campo **`object`** del MQTT es la fuente directa para un dashboard.

## 📖 Nota didáctica — por qué fallaba el join
El valor real de este ejercicio está en el **flujo de depuración**: al principio el join fallaba con
**cero** frames en ChirpStack. El diagnóstico reveló **dos bugs independientes** (y **no** era, como se
suele culpar, el orden de bytes):

### Bug A — sub-banda US915 mal seleccionada
```c
LMIC_selectSubBand(0);   // ❌ sub-banda 1 -> canales 0-7  (902.3-903.7 MHz)
LMIC_selectSubBand(1);   // ✅ sub-banda 2 -> canales 8-15 (903.9-905.3 MHz)  <- la del gateway
```
El gateway escucha **FSB2 (canales 8-15)**. Con `selectSubBand(0)` el TTGO transmitía en canales 0-7 →
el gateway **no lo oía** → cero frames. El sketch incluido (`TTGO_LoRaWAN_v3.ino`) ya usa `selectSubBand(1)`.

### Bug B — credenciales del sketch ≠ credenciales registradas
Los arrays del sketch producían DevEUI `02389205358e71db`, pero en ChirpStack se había registrado otro
device. Aunque el RF funcione, ChirpStack responde *"Unknown device"*. **Solución:** registrar las
credenciales reales del sketch — que es justo lo que hace `./provision.sh` (paso 6).

### El orden de bytes de DevEUI y JoinEUI (la fuente eterna de confusión)
El **mismo** DevEUI/JoinEUI se escribe en **dos órdenes distintos** según dónde: en el **sketch (LMIC)
en LSB** (invertido) y en **ChirpStack en MSB** (tal como se lee). El **AppKey NO** se invierte.

| Campo | En el **sketch** (LMIC, arrays) | En **ChirpStack** (UI/API) |
|---|---|---|
| **DevEUI** | `DB 71 8E 35 05 92 38 02` ← **LSB (invertido)** | `02 38 92 05 35 8E 71 DB` ← **MSB** |
| **JoinEUI/AppEUI** | `8A FD 43 71 F8 46 52 50` ← **LSB (invertido)** | `50 52 46 F8 71 43 FD 8A` ← **MSB** |
| **AppKey** | `8A C5 83 DF … 73 BF` ← **MSB (igual)** | `8A C5 83 DF … 73 BF` ← **MSB (igual)** |

**¿Por qué?** MCCI/Arduino LMIC carga `DevEUI` y `AppEUI` en **little-endian** (así lo esperan
`os_getDevEui()`/`os_getArtEui()`), mientras que LoRaWAN/ChirpStack los representan en **big-endian**.
El `AppKey` va en el mismo orden (MSB) en ambos → **no** se invierte.
> **Regla:** en el array C, DevEUI/AppEUI van **al revés** que en ChirpStack; el AppKey, igual.

### Posible Bug C (si el join se ve en logs pero el device sigue fallando)
En el TTGO v1, el pad **DIO1 (GPIO33) a veces necesita un puente físico** (pad DIO1 del SX1276 →
GPIO33). Sin él, el Join Request **sale** pero el device **no recibe el Join Accept** en la ventana RX.
(En nuestro caso no hizo falta — señal RSSI −31, SNR 10.5.)

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 04 · Wi-Fi Region Detection](../04_wifi-region-detection/)
- ➡️ Siguiente: [Ejercicio 06 · TTGO + BMP280](../06_ttgo-bmp280/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json) (IDs y claves reales, MSB para ChirpStack).
- **Archivos de esta carpeta:** `sketches/TTGO_LoRaWAN_v3.ino` (firmware) · `payload_decoder.js` (codec) · `provision.sh` (alta por API) · `scripts/upload_codec.sh` (adjunta el codec) · `scripts/subscribe.sh` (MQTT sin Python).
- **Guías comunes:** [Flashear/serie](../COMMON_FLASH.md) · [ChirpStack API](../COMMON_CHIRPSTACK_API.md) · Wiki: [Compilar el TTGO](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Compilar-el-TTGO-en-Arduino) · [Dar de alta un TTGO nuevo](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Dar-de-alta-un-TTGO-nuevo).

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
