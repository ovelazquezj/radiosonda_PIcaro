# Ejercicio 07 — Gateway LoRaWAN de 1 canal (TTGO ESP32 + ESP-1ch-Gateway)

**Qué demuestra:** montar tu **propio gateway LoRaWAN** con una **TTGO ESP32 (SX1276)** y el
firmware de terceros **ESP-1ch-Gateway** para que **ChirpStack reciba los uplinks** de los nodos
del proyecto (el LR1110 del ejercicio [02](../02_bmp280-gnss-tracker/) y los TTGO de los
ejercicios [05](../05_ttgo-lora32/) y [06](../06_ttgo-bmp280/)). Es el eslabón que faltaba: hasta
ahora dependías de un gateway ajeno; aquí **el gateway también es tuyo**.

| | |
|---|---|
| Hardware | **TTGO ESP32 LoRa** (SX1276) — la misma placa de los ej. 05/06 |
| Firmware | **ESP-1ch-Gateway** — commit `3b02352` (v6.2.8 interno, sin release formal) — *terceros* |
| Tipo | Gateway **single-channel** (un solo canal) — solo demos/educación |
| Protocolo hacia el NS | **Semtech UDP Packet Forwarder** → puerto **1700/UDP** |
| Network Server | **ChirpStack v4** (Gateway Bridge, backend Semtech UDP) |
| Región / canal | EU868 · canal 0 (**868.1 MHz**) · **SF9** · BW125 |
| Admin | Interfaz web integrada en `http://<IP_gateway>:80` |

> ℹ️ **Atribución.** ESP-1ch-Gateway es un proyecto **de terceros** de **Maarten Westenberg
> (things4u)**, bajo licencia **MIT** — **no** forma parte de Semtech ni de radiosonda_PIcaro. Este
> ejercicio **solo aporta la guía de integración con ChirpStack** y el script de alta por API
> (aportación propia de radiosonda_PIcaro, Clear BSD). Descarga el firmware desde su repositorio
> oficial; aquí no se redistribuye su código.

---

## ⚠️ Antes de empezar: qué es (y qué NO es) un gateway de 1 canal

Un gateway LoRaWAN "de verdad" lleva un concentrador (SX1301/SX1302) que escucha **8 canales y
todos los factores de dispersión (SF) a la vez**. Este montaje usa **un único SX1276**, así que
solo puede escuchar **una frecuencia**:

- ✅ **Sirve para aprender** el flujo completo nodo → gateway → ChirpStack con hardware barato.
- ❌ **No es LoRaWAN-compliant**: escucha **1 de 8 canales** → si los nodos saltan de canal (channel
  hopping / ADR), **pierde ~7/8 de los uplinks**.
- ❌ **No es para producción**. Aguanta bien **2–3 nodos**; con más hay colisiones y pérdidas.
- ⛔ **Bloqueado en TTN público** (The Things Network prohíbe gateways de 1 canal). **Pero funciona
  perfectamente contra tu ChirpStack privado**, que es justo nuestro caso.
- 🕰️ Firmware **sin mantenimiento desde 2021** (v6.2.8): puede que tengas que **fijar versiones
  antiguas** de librerías (ArduinoJson 6.x, etc.) para que compile.
- 🧠 En **ESP8266** la RAM (~80 KB) es crítica y el gateway se reinicia bajo carga → **usa ESP32**
  (la TTGO ya lo es).

> **Regla de oro del ejercicio:** para que este gateway oiga a un nodo, **el nodo y el gateway
> deben coincidir en región, frecuencia y (idealmente) SF**. Todo lo demás que "no funcione"
> suele reducirse a esto.

---

## 1. Requisitos y descarga del firmware

- **Placa:** TTGO ESP32 LoRa (SX1276). En placas TTGO/Heltec el radio **ya viene cableado
  internamente** al ESP32: no tienes que soldar nada.
- **Entorno:** **Arduino IDE** con soporte ESP32, o **PlatformIO** (recomendado; el repo trae
  `platformio.ini`).
- **Firmware (terceros):** clona el repositorio oficial
  **https://github.com/things4u/ESP-1ch-Gateway** y **fija el commit probado** (el repo **no publica
  releases ni tags**, así que se ancla por SHA — ver [`VERSIONS.md`](VERSIONS.md)). El sketch
  principal es `src/ESP-sc-gway.ino`.
- **Dependencias:** casi todas van **vendorizadas** en `lib/` del propio repo (ArduinoJson,
  WiFiManager, TinyGPS++, Streaming, Time, OLED SSD1306, aes, gBase64…), así que al fijar el commit
  quedan congeladas; la única que PlatformIO descarga aparte es `sandeepmistry/LoRa 0.8.0`. Ver
  [`VERSIONS.md`](VERSIONS.md).
- **Red:** el gateway y el host de ChirpStack deben verse por IP (misma LAN o rutado), con el
  **puerto 1700/UDP** abierto en el host de ChirpStack.

```bash
git clone https://github.com/things4u/ESP-1ch-Gateway.git
cd ESP-1ch-Gateway
git checkout 3b023527bd23cf33657dc7ffdf5bedaf1b85cdcc   # v6.2.8 (2021-10-18), sin release formal
# Compila con PlatformIO (recomendado; env de TTGO por USB en VERSIONS.md) o abre
# src/ESP-sc-gway.ino en Arduino IDE.
```

---

## 2. Configuración (`configGway.h` y `configNode.h`)

Toda la configuración vive en dos cabeceras dentro de `src/`. Los `#define` clave y sus valores
para nuestro caso (TTGO ESP32, EU868, canal 0, apuntando a ChirpStack) están recogidos en
[`configGway.example.h`](configGway.example.h) — cópialos/adáptalos sobre tu `configGway.h`.

**`configGway.h` — parámetros imprescindibles:**

| Parámetro | Valor para este ejercicio | Significado |
|-----------|---------------------------|-------------|
| `_PIN_OUT` | **`4`** | Pines del radio para **TTGO** (Heltec V2 = `5`). **Default del repo = `1` (ESP8266) → cámbialo** |
| `EU863_870` | `1` | Plan de frecuencias **EU868** (usa `US902_928` si tu región es US915) |
| `_CHANNEL` | `0` | Canal que se escucha → **868.1 MHz** en EU |
| `_SPREADING` | `SF9` | Factor de dispersión por defecto |
| `_CAD` | `1` | Escucha **todos los SF** en esa frecuencia (Channel Activity Detection) |
| `_STRICT_1CH` | `1` | Devuelve los **downlinks en el mismo canal/SF** del uplink (clave para ACK/join) |
| `_TTNSERVER` | `"<IP_DE_CHIRPSTACK>"` | **Servidor primario → apúntalo a tu ChirpStack** (lo más simple) |
| `_TTNPORT` | `1700` | Puerto **UDP** (Semtech UDP / Gateway Bridge) |
| `_SERVER` | `1` | Activa la **interfaz web** de administración (`http://<IP>:80`) |

> 🧩 **TTGO vs Heltec (y el `board` de PlatformIO).** El repo original compila para **Heltec**
> (`board = heltec_wifi_lora_32`), pero **los pines del radio los decide `_PIN_OUT`** (GPIOs fijados
> en `loraModem.h`), **no** el `board` de PlatformIO. El firmware soporta las dos: **`_PIN_OUT 4` =
> TTGO**, **`_PIN_OUT 5` = Heltec V2**; solo difieren en DIO1/DIO2 (usados para el CAD), así que usa
> el número correcto. El env activo del repo **no** fija `_PIN_OUT` → cae al default `1` (ESP8266);
> ponlo a `4` (ver [`VERSIONS.md`](VERSIONS.md)).

**`configNode.h` — WiFi, identidad y ubicación** (ver [`configNode.example.h`](configNode.example.h)):

- **WiFi:** rellena el array `wpa[]` con tu SSID/clave, p.ej. `wpas wpa[] = { { "MI_SSID", "MI_CLAVE" } };`
  (en el repo viene **vacío**). Alternativa: portal por AP con `_WIFIMANAGER=1`.
- **Identidad:** `_DESCRIPTION` (nombre), `_EMAIL`, `_PLATFORM` (pon `"ESP32"` para la TTGO).
- **Ubicación:** `_LAT` / `_LON` / `_ALT` (ChirpStack la muestra en el mapa).

> **Dos rutas para enviar a ChirpStack** (elige una): **(a)** *simple* — pon `_TTNSERVER` (en
> `configGway.h`) con la IP de tu ChirpStack; **(b)** *como secundario* — deja `_TTNSERVER` como
> esté y **descomenta `_THINGSERVER`/`_THINGPORT` en `configNode.h`**. En ambos casos, **1700/UDP**.

---

## 3. Flashear y leer el **Gateway EUI real**

1. Compila y flashea `src/ESP-sc-gway.ino`. **Ojo:** el `platformio.ini` del repo trae **un único
   entorno activo** (`[env:Gateway_38]`) para **Heltec por OTA**; para **TTGO por USB** usa el
   entorno adaptado de [`VERSIONS.md`](VERSIONS.md) (`board = ttgo-lora32-v1`,
   `upload_protocol = esptool`, `upload_port = COMx`). Con Arduino IDE: placa *TTGO LoRa32*, puerto COM correcto.
2. Abre el **monitor serie a 115200**. En el arranque el firmware **genera el Gateway EUI (8 bytes)
   a partir de la MAC del ESP** y lo imprime (también aparece en la web `http://<IP>:80`).

> 🔑 **NO inventes el EUI.** Léelo del arranque serie o de la web y **regístralo tal cual** en
> ChirpStack. Si registras un EUI distinto del que emite el gateway, ChirpStack lo verá **offline**
> aunque el gateway esté enviando.

---

## 4. Dar de alta el gateway en ChirpStack v4

ChirpStack recibe por el **ChirpStack Gateway Bridge** con el **backend Semtech UDP (1700/UDP)**,
que es exactamente lo que emite este firmware. Solo tienes que **registrar el gateway** (una vez):

**Opción A — por la UI:** `Gateways → Add gateway`, pega el **Gateway EUI** (paso 3), ponle nombre
y guárdalo. Debe pasar a **online** en segundos si hay tráfico/keep-alive.

**Opción B — por API REST (reproducible):**
```bash
export TOKEN="tu_api_key"           # Tenant -> API keys en la web de ChirpStack
./register_gateway.sh AABBCCDDEEFF0011     # <-- el EUI real que imprime el gateway
```
Ver setup del token e IDs del stack en [`../COMMON_CHIRPSTACK_API.md`](../COMMON_CHIRPSTACK_API.md).

---

## 5. Fijar los nodos a **1 canal + 1 SF** (paso crítico)

Un nodo LoRaWAN normal reparte sus envíos entre 8 canales y varios SF. Como el gateway solo
escucha **uno**, hay que **fijar cada nodo** a esa misma frecuencia/SF (ej.: **868.1 MHz, SF9**):

- **Nodos TTGO (Arduino/LMIC — ej. 05/06):** deshabilita todos los canales salvo el 0 y fija el
  data rate (para que no haya ADR/hopping):
  ```c
  for (int i = 1; i < 9; i++) LMIC_disableChannel(i);   // deja solo el canal 0 (868.1 MHz)
  LMIC_setDrTxpow(DR_SF9, 14);                            // SF9 fijo
  LMIC_setAdrMode(0);                                     // sin ADR
  ```
- **Nodo LR1110 (ej. 02, Nucleo-L476RG):** configura el stack para transmitir en esa **misma
  frecuencia y SF única** (sin salto de canal). En EU868 el canal por defecto 868.1/SF ya suele
  coincidir; verifica en la traza UART la frecuencia real de cada uplink.
- **`_STRICT_1CH = 1`** hace que el gateway **devuelva los downlinks en el mismo canal/SF del
  uplink** (en vez de las ventanas RX1/RX2 estándar). Es lo que permite que lleguen **ACKs,
  downlinks y el Join Accept** a un nodo que solo escucha un canal.

> 💡 **Empieza con ABP**, no con OTAA. En un gateway de 1 canal el join OTAA (ida + Join Accept en
> ventana RX) es la parte más frágil. Con **ABP** el nodo ya está "activado" y validas antes el
> camino de datos. Cuando el enlace ABP funcione, prueba OTAA con `_STRICT_1CH=1`.

---

## 6. Qué observar

- **Monitor serie del gateway:** arranque, WiFi conectado, **Gateway EUI**, y líneas de
  `Up`/`rxpk` cada vez que capta un uplink.
- **Web del gateway** (`http://<IP>:80`): estadísticas, último paquete, y ajuste en caliente de
  SF/canal/nivel de debug.
- **ChirpStack → Gateways:** el gateway en **online** con *last seen* reciente.
- **ChirpStack → tu aplicación → device → LoRaWAN frames:** los uplinks del nodo (fPort 10 del
  BMP280, etc.) llegando **a través de tu gateway**.

---

## 7. Solución de problemas

| Síntoma | Causa probable | Arreglo |
|---------|----------------|---------|
| El gateway ve `rxpk` pero **ChirpStack no lo registra** | EUI mal registrado, o 1700/UDP bloqueado | Registra el **EUI real** (paso 3); abre **1700/UDP** en el host de ChirpStack |
| **Gateway offline** en ChirpStack | `_THINGSERVER`/`_THINGPORT` mal, firewall, o sin tráfico | Revisa IP/puerto, ping al host, firewall del 1700/UDP |
| **No llega nada de un nodo** | El nodo hace **hopping/ADR** o usa **otro SF/canal** | Fija el nodo a canal 0 + SF9 (paso 5); desactiva ADR |
| Solo llegan **algunos** uplinks | Es lo esperado en 1 canal si el nodo salta de canal | Fija el nodo; asume la limitación del single-channel |
| **No hace join (OTAA)** | Join Accept no llega a la ventana RX | Usa `_STRICT_1CH=1`; prueba primero **ABP** |
| El firmware **no compila** | Librerías más nuevas que 2021 | Fija versiones antiguas (ArduinoJson 6.x, WiFiManager) |

---

## 8. Enlaces útiles

- Repositorio (terceros): **https://github.com/things4u/ESP-1ch-Gateway**
- Guía de configuración (things4u): https://things4u.github.io/Projects/SingleChannelGateway/UserGuide/3_Configuration.html
- ChirpStack — conectar un gateway: https://www.chirpstack.io/docs/guides/connect-gateway.html
- ChirpStack — backend Semtech UDP (puerto 1700): https://www.chirpstack.io/docs/chirpstack-gateway-bridge/backends/semtech-udp.html
- Issue de integración con ChirpStack: https://github.com/things4u/ESP-1ch-Gateway/issues/98

---

## Archivos

```
README.md                  este documento (guía del ejercicio)
configGway.example.h       #define clave para configGway.h (TTGO, EU868, canal 0, SF9, _STRICT_1CH)
configNode.example.h       WiFi (wpa[]), identidad y ubicacion del gateway (para configNode.h)
register_gateway.sh        alta del gateway en ChirpStack v4 por API REST (idempotente)
VERSIONS.md                commit exacto del firmware + versiones de librerias que compilan
```
