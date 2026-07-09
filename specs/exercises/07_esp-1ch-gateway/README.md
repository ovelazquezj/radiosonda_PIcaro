# Ejercicio 07 — Gateway LoRaWAN de 1 canal (TTGO ESP32 + ESP-1ch-Gateway)

**Qué demuestra:** montar tu **propio gateway LoRaWAN** con una **TTGO ESP32 (SX1276)** y el
firmware de terceros **ESP-1ch-Gateway** para que **ChirpStack reciba los uplinks** de los nodos
del proyecto (el LR1110 del ejercicio [02](../02_bmp280-gnss-tracker/) y los TTGO de los
ejercicios [05](../05_ttgo-lora32/) y [06](../06_ttgo-bmp280/)). Es el eslabón que faltaba: hasta
ahora dependías de un gateway ajeno; aquí **el gateway también es tuyo**.

| | |
|---|---|
| Hardware | **TTGO ESP32 LoRa** (SX1276) — la misma placa de los ej. 05/06 |
| Firmware | **ESP-1ch-Gateway** v6.2.8 (Arduino IDE o PlatformIO) — *software de terceros* |
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
  **https://github.com/things4u/ESP-1ch-Gateway**. El sketch principal es
  `src/ESP-sc-gway.ino`.
- **Red:** el gateway y el host de ChirpStack deben verse por IP (misma LAN o rutado), con el
  **puerto 1700/UDP** abierto en el host de ChirpStack.

```bash
git clone https://github.com/things4u/ESP-1ch-Gateway.git
cd ESP-1ch-Gateway
# Abre src/ESP-sc-gway.ino en Arduino IDE, o usa PlatformIO.
```

---

## 2. Configuración (`configGway.h` y `configNode.h`)

Toda la configuración vive en dos cabeceras dentro de `src/`. Los `#define` clave y sus valores
para nuestro caso (TTGO ESP32, EU868, canal 0, apuntando a ChirpStack) están recogidos en
[`configGway.example.h`](configGway.example.h) — cópialos/adáptalos sobre tu `configGway.h`.

**`configGway.h` — parámetros imprescindibles:**

| Parámetro | Valor para este ejercicio | Significado |
|-----------|---------------------------|-------------|
| `_PIN_OUT` | **`4`** | Mapa de pines de **TTGO/Heltec ESP32** (SX1276 ya cableado) |
| `EU863_870` | `1` | Plan de frecuencias **EU868** (usa `US902_928` si tu región es US915) |
| `_CHANNEL` | `0` | Canal que se escucha → **868.1 MHz** en EU |
| `_SPREADING` | `SF9` | Factor de dispersión por defecto |
| `_CAD` | `1` | Escucha **todos los SF** en esa frecuencia (Channel Activity Detection) |
| `_STRICT_1CH` | `1` | Devuelve los **downlinks en el mismo canal/SF** del uplink (clave para ACK/join) |
| `_THINGSERVER` | `"<IP_DE_CHIRPSTACK>"` | **Servidor secundario → tu ChirpStack** |
| `_THINGPORT` | `1700` | Puerto **UDP** del servidor (Semtech UDP) |
| `_SERVER` | `1` | Activa la **interfaz web** de administración (`http://<IP>:80`) |

**`configNode.h` — identidad y red:**

- `AP_NAME` / `AP_PASSWD`: tu **WiFi** (o activa `_WIFIMANAGER` para configurar por portal).
- `_LAT` / `_LON` / `_ALT`: **ubicación** del gateway (ajústala a la real; ChirpStack la muestra en el mapa).

> Puedes dejar `_TTNSERVER` como venga (servidor primario); lo que hace llegar los datos a
> ChirpStack es el **secundario** (`_THINGSERVER` + `_THINGPORT 1700`).

---

## 3. Flashear y leer el **Gateway EUI real**

1. Compila y flashea `src/ESP-sc-gway.ino` (Arduino IDE: placa *TTGO LoRa32* / *ESP32 Dev Module*;
   o `pio run -t upload`).
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
07_README.md               este documento (guía del ejercicio)
configGway.example.h       #define clave para configGway.h (TTGO, EU868, canal 0, → ChirpStack)
register_gateway.sh        alta del gateway en ChirpStack v4 por API REST (idempotente)
```
