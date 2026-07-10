# How-To · Montar tu propio gateway LoRaWAN de 1 canal (ejercicio 07)

Hasta ahora tus nodos (LR1110, TTGO) dependían de **un gateway ajeno** para llegar a ChirpStack.
En este ejercicio montas **tu propio gateway** con una **TTGO ESP32 (SX1276)** y el firmware de
terceros **ESP-1ch-Gateway**. Es el eslabón que cierra el círculo: **la red también es tuya**.

Ficheros de apoyo en el repo: **[`specs/exercises/07_esp-1ch-gateway/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/07_esp-1ch-gateway)**
(`README.md`, `configGway.example.h`, `configNode.example.h`, `register_gateway.sh`, `VERSIONS.md`).

> ℹ️ **Atribución.** ESP-1ch-Gateway es un proyecto **de terceros** de **Maarten Westenberg
> (things4u)**, licencia **MIT** — no es de Semtech ni de radiosonda_PIcaro. Aquí solo aportamos la
> **guía de integración con ChirpStack** y el script de alta. Descarga el firmware de su repo oficial.

---

## 0 · Antes de empezar: qué es (y qué NO es) un gateway de 1 canal

Un gateway LoRaWAN "de verdad" lleva un **concentrador** (SX1301/SX1302) que escucha **8 canales y
todos los factores de dispersión (SF) a la vez**. Aquí usamos **un solo SX1276**, que solo puede
sintonizar **una frecuencia**. Consecuencias, sin rodeos:

- ✅ **Perfecto para aprender** el flujo completo nodo → gateway → ChirpStack con hardware barato.
- ❌ **No es LoRaWAN-compliant**: escucha **1 de 8 canales**. Si un nodo salta de canal (hopping/ADR),
  **pierde ~7/8 de los uplinks**.
- ❌ **No es para producción**: aguanta bien **2–3 nodos**; con más, colisiones y pérdidas.
- ⛔ **Bloqueado en The Things Network público** (TTN prohíbe gateways de 1 canal). **Pero funciona
  perfectamente contra tu ChirpStack privado**, que es justo nuestro caso.
- 🕰️ Firmware **sin mantenimiento desde 2021**: por eso fijamos una versión concreta (ver Paso 1).

> 🔑 **Regla de oro de este ejercicio.** Para que el gateway oiga a un nodo, **el nodo y el gateway
> deben coincidir en región, frecuencia y (idealmente) SF**. **Casi todo lo que "no funciona" se
> reduce a esto.** Tenlo presente en cada paso.

### Requisitos

- **Placa:** una **TTGO ESP32 LoRa (SX1276)** — la misma de los ejercicios 05/06. En las TTGO el
  radio **ya viene cableado** internamente: no sueldas nada.
- **ChirpStack v4 corriendo** (ejercicio 00) con su **Gateway Bridge** escuchando el backend
  **Semtech UDP en 1700/UDP**.
- **Red:** el gateway y el host de ChirpStack deben verse por IP (misma LAN), con el **puerto
  1700/UDP abierto** en el host de ChirpStack.
- Entorno de compilación: **PlatformIO** (recomendado) o **Arduino IDE** con soporte ESP32
  ([Requisitos e instalación](How-To-Requisitos-e-instalación)).

---

## Paso 1 · Descargar el firmware y **fijar la versión**

El repo **no publica releases ni tags**, así que anclamos un **commit concreto** (probado). Casi
todas las librerías van **vendorizadas** dentro de `lib/` del repo → al fijar el commit quedan
congeladas; la única que se descarga aparte es `sandeepmistry/LoRa`.

```bash
git clone https://github.com/things4u/ESP-1ch-Gateway.git
cd ESP-1ch-Gateway
git checkout 3b023527bd23cf33657dc7ffdf5bedaf1b85cdcc   # v6.2.8 interno (2021-10-18)
```

El sketch principal es **`src/ESP-sc-gway.ino`**; la configuración vive en **`src/configGway.h`** y
**`src/configNode.h`**. Detalle de versiones y librerías en
[`VERSIONS.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/VERSIONS.md).

> ⚠️ **No uses `git pull` a master a ciegas.** El firmware es antiguo; versiones más nuevas de
> librerías pueden romper la compilación. Quédate en el commit fijado.

---

## Paso 2 · Configurar `configGway.h` y `configNode.h`

Copia los valores de los ejemplos del repo
([`configGway.example.h`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/configGway.example.h)
y [`configNode.example.h`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/configNode.example.h))
sobre tus ficheros reales.

**`configGway.h` — radio, región y servidor:**

| Parámetro | Valor | Significado |
|-----------|-------|-------------|
| `_PIN_OUT` | **`4`** | Pines del radio para **ESP32/TTGO** (Heltec V2 = `5`). **Default del repo = `1` (ESP8266) → hay que cambiarlo** |
| `EU863_870` | `1` | Plan **EU868** (usa `US902_928` si tu región es US915) |
| `_CHANNEL` | `0` | Canal único que se escucha → **868.1 MHz** en EU |
| `_SPREADING` | `SF9` | Factor de dispersión (debe coincidir con el del nodo) |
| `_CAD` | `1` | Escucha **todos los SF** en esa frecuencia (recomendado) |
| `_STRICT_1CH` | `1` | Devuelve los **downlinks en el mismo canal/SF** del uplink (ACK/join) |
| `_TTNSERVER` | `"<IP_DE_CHIRPSTACK>"` | **Servidor primario → apúntalo a tu ChirpStack** (lo más simple) |
| `_TTNPORT` | `1700` | Puerto **UDP** (Semtech UDP / Gateway Bridge) |
| `_SERVER` | `1` | Interfaz web de administración en `http://<IP>:80` |

> 🧩 **TTGO vs Heltec — y por qué el `board` de PlatformIO no manda aquí.** El repo original compila
> para **Heltec** (`board = heltec_wifi_lora_32`), pero **los pines del radio LoRa no los decide el
> `board` de PlatformIO, sino `_PIN_OUT`** (van por número de GPIO en `loraModem.h`). El firmware
> soporta las dos: **`_PIN_OUT 4` = TTGO**, **`_PIN_OUT 5` = Heltec V2**. Comparten los pines SPI
> (SS=18, SCK=5, MISO=19, MOSI=27, RST=14, DIO0=26) y **solo difieren en DIO1/DIO2**, que este
> firmware usa para el **CAD** → poner el número equivocado **puede romper la recepción**. Ojo: el
> entorno activo del repo **no** fija `_PIN_OUT`, así que cae al default **`1` (ESP8266)**; para TTGO
> añade `-D _PIN_OUT=4` a `build_flags` o ponlo en `configGway.h`.

**`configNode.h` — WiFi, identidad y ubicación:**

- **WiFi (obligatorio):** rellena el array `wpa[]` (viene **vacío**):
  ```c
  wpas wpa[] = { { "MI_SSID", "MI_CLAVE_WIFI" } };
  ```
- **Identidad:** `_DESCRIPTION` (nombre del gateway), `_EMAIL`, `_PLATFORM` (pon **`"ESP32"`**).
- **Ubicación:** `_LAT` / `_LON` / `_ALT` (ChirpStack la muestra en el mapa).

> 💡 **Dónde apuntar a ChirpStack.** El servidor **primario** (`_TTNSERVER`/`_TTNPORT`) está en
> **`configGway.h`**; el **secundario** (`_THINGSERVER`/`_THINGPORT`) está en **`configNode.h`**
> (comentado). Para un único Network Server, lo simple es poner `_TTNSERVER` = la IP de tu
> ChirpStack. En **ambos casos el puerto es 1700/UDP**.

---

## Paso 3 · Compilar y flashear (TTGO por USB)

⚠️ **Nota importante:** el `platformio.ini` del repo trae **un único entorno activo**
(`[env:Gateway_38]`) pensado para una placa **Heltec** y **subida por OTA** (WiFi). Para **TTGO por
USB** añade tu propio entorno al `platformio.ini` (está también en
[`VERSIONS.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/VERSIONS.md)):

```ini
[env:ttgo_usb]
platform = espressif32
board = ttgo-lora32-v1        ; o -v2 / -v21 segun tu placa
framework = arduino
lib_deps =
  sandeepmistry/LoRa @ 0.8.0
build_flags =
  -D _PIN_OUT=4              ; 4 = TTGO (Heltec V2 = 5); sin esto cae al default 1 (ESP8266)
  -D _WIFIMANAGER=0
  -D _OLED=1
  -D _DUSB=1
  -D _STRICT_1CH=1
upload_protocol = esptool
upload_port = COM5            ; <-- tu puerto (Windows COMx; Linux /dev/ttyUSB0)
upload_speed = 921600
monitor_speed = 115200
```

```bash
pio run -e ttgo_usb -t upload     # compila + flashea por USB
pio device monitor -b 115200      # abre el monitor serie
```

**Arduino IDE (alternativa):** instala el core ESP32 y la librería **`LoRa` de sandeepmistry
`0.8.0`**; usa las librerías que trae el repo en `lib/`. Placa *TTGO LoRa32*, puerto COM correcto,
**Upload**.

---

## Paso 4 · Leer el **Gateway EUI real** (nota didáctica clave)

En el arranque, el firmware **genera el Gateway EUI (8 bytes) a partir de la MAC del ESP** y lo
imprime por serie (y en la web `http://<IP>:80`). Busca una línea tipo:

```
Gateway ID: AA BB CC DD EE FF 00 11   (ejemplo)
```

> 🔑 **Fallo clásico nº1: inventarse el EUI.** Si registras en ChirpStack un EUI distinto del que
> emite el gateway, ChirpStack lo verá **offline** aunque el gateway esté enviando. **Copia el EUI
> real** del serial/web y úsalo tal cual en el Paso 5.

---

## Paso 5 · Dar de alta el gateway en ChirpStack v4

ChirpStack recibe por su **Gateway Bridge** con backend **Semtech UDP (1700/UDP)**, que es justo lo
que emite este firmware. Solo hay que **registrar el gateway una vez**:

**Opción A · por la web:** `Gateways → Add gateway`, pega el **Gateway EUI** (Paso 4), ponle nombre
y guarda. Debe pasar a **online** en segundos si hay tráfico.

**Opción B · por API REST (reproducible):**
```bash
export TOKEN="tu_api_key"                    # ChirpStack web -> Tenant -> API keys
./register_gateway.sh AABBCCDDEEFF0011 "Gateway TTGO aula"   # el EUI real del Paso 4
```
El script [`register_gateway.sh`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/register_gateway.sh)
es idempotente (no falla si el gateway ya existe). Token e IDs del stack:
[How-To Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack).

---

## Paso 6 · Fijar los nodos a **1 canal + 1 SF** (el paso crítico)

Recuerda la **regla de oro**: un nodo normal reparte sus envíos entre 8 canales y varios SF; como el
gateway solo escucha **uno**, hay que **fijar cada nodo** a esa misma frecuencia/SF (ej. **868.1 MHz,
SF9**).

- **Nodos TTGO (Arduino/LMIC — ejercicios 05/06):** deja **solo el canal 0** y fija el data rate:
  ```c
  for (int i = 1; i < 9; i++) LMIC_disableChannel(i);   // deja solo el canal 0 (868.1 MHz)
  LMIC_setDrTxpow(DR_SF9, 14);                            // SF9 fijo
  LMIC_setAdrMode(0);                                     // sin ADR
  ```
  (⚠️ En **US915** ojo: `LMIC_selectSubBand(1)` habilita **8** canales (8–15), **no uno**, así que un
  gateway de 1 canal seguiría perdiendo ~7/8. Para single-channel real, además del subband deja
  **solo un canal** deshabilitando los otros 7. Ver
  [Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).)
- **Nodo LR1110 (ejercicio 02, Nucleo):** configura el stack para transmitir en esa **misma
  frecuencia y SF única**, sin salto de canal. Verifica en la traza UART la frecuencia real de cada
  uplink.
- **`_STRICT_1CH = 1`** hace que el gateway **devuelva los downlinks en el mismo canal/SF** del
  uplink (en vez de las ventanas RX1/RX2 estándar). Es lo que permite que lleguen **ACKs, downlinks
  y el Join Accept** a un nodo que solo escucha un canal.

> 💡 **Empieza con ABP, no con OTAA.** En un gateway de 1 canal, el join OTAA (ida + Join Accept en
> ventana RX) es la parte más frágil. Con **ABP** el nodo ya está "activado" y validas antes el
> camino de datos. Cuando el enlace ABP funcione, prueba OTAA con `_STRICT_1CH=1`.

> 🔑 **Fallo clásico nº2: dejar el nodo con hopping/ADR.** Síntoma: "no llega nada" o "llegan solo
> algunos uplinks". Casi siempre es el nodo saltando de canal o usando otro SF. Fíjalo al canal 0 +
> SF9 y desactiva ADR.

---

## Paso 7 · Verificar el flujo completo

- **Monitor serie del gateway (115200):** WiFi conectada, **Gateway EUI**, y líneas `rxpk`/`Up`
  cada vez que capta un uplink.
- **Web del gateway (`http://<IP>:80`):** estadísticas, último paquete y ajuste en caliente de
  SF/canal/debug.
- **ChirpStack → Gateways:** tu gateway en **online**, con *last seen* reciente.
- **ChirpStack → tu aplicación → device → LoRaWAN frames:** los uplinks del nodo (p. ej. el fPort 10
  del BMP280) **llegando a través de tu gateway**. 🎉

---

## Solución de problemas

| Síntoma | Causa probable | Arreglo |
|---------|----------------|---------|
| Gateway **offline** en ChirpStack | `_TTNSERVER`/puerto mal, firewall, o EUI mal registrado | Revisa IP/puerto, abre **1700/UDP**, registra el **EUI real** (Paso 4) |
| El gateway ve `rxpk` pero ChirpStack **no lo registra** | 1700/UDP bloqueado o EUI equivocado | Abre el puerto en el host; verifica el EUI |
| **No llega nada** de un nodo | El nodo hace **hopping/ADR** o usa **otro SF/canal** | Fíjalo a canal 0 + SF9; desactiva ADR (Paso 6) |
| Llegan **solo algunos** uplinks | Es lo esperado en 1 canal si el nodo salta | Fija el nodo; asume la limitación del single-channel |
| **No hace join (OTAA)** | El Join Accept no cae en la ventana RX | Usa `_STRICT_1CH=1`; prueba primero **ABP** |
| El firmware **no compila** | Librerías más nuevas que 2021 | Quédate en el commit fijado; usa `sandeepmistry/LoRa 0.8.0` |

---

## Limitaciones (recordatorio para el aula)

- No es un gateway conforme: sirve para **demostrar el concepto** y unos pocos nodos, no para simular
  una red real.
- **Todos los nodos deben compartir canal y SF** con el gateway.
- Escalabilidad muy baja (2–3 nodos); posibles reinicios bajo carga (en ESP8266 es peor → usa ESP32).
- No lo conectes a **TTN público** (bloqueado); tu **ChirpStack privado** es el camino correcto.

## Enlaces

- Repositorio (terceros): <https://github.com/things4u/ESP-1ch-Gateway>
- ChirpStack — conectar un gateway: <https://www.chirpstack.io/docs/guides/connect-gateway.html>
- ChirpStack — backend Semtech UDP (1700): <https://www.chirpstack.io/docs/chirpstack-gateway-bridge/backends/semtech-udp.html>

> Cierre: **[Glosario y recursos](Glosario-y-recursos)** →
