# Ejercicio 07 — Tu gateway LoRaWAN de 1 canal (TTGO ESP32 + ESP-1ch-Gateway)

> **En una frase:** montas tu **propio gateway** con una **TTGO ESP32 (SX1276)** y ves tu gateway
> **online** en ChirpStack recibiendo los uplinks de tu nodo **a través de él**. Hasta ahora
> dependías de un gateway ajeno; aquí el gateway también es **tuyo**.
> **Plataforma:** TTGO ESP32 LoRa (SX1276) + firmware de terceros **ESP-1ch-Gateway**. **Ejemplo:**
> **EU868**, canal 0 (**868.1 MHz**), **SF9**, `_STRICT_1CH`. Reenvía a ChirpStack por **Semtech UDP
> (1700/UDP)**.

> ℹ️ **Atribución.** ESP-1ch-Gateway es un proyecto **de terceros** de **Maarten Westenberg
> (things4u)**, bajo licencia **MIT** — **no** forma parte de Semtech ni de radiosonda_PIcaro. Este
> ejercicio **solo aporta la guía de integración con ChirpStack** y el script de alta por API
> (aportación propia, Clear BSD). Descarga el firmware desde su repositorio oficial; aquí no se
> redistribuye su código.

## 🎯 Qué vas a conseguir
Tu gateway registrado y **online** en ChirpStack, y los uplinks de tu nodo (p. ej. el TTGO+BMP280 del
[ej.06](../06_ttgo-bmp280/)) llegando **a través de tu gateway**. En el arranque verás por serie la
**línea del Gateway EUI**, y una línea `Up`/`rxpk` cada vez que capta un uplink:

```
Gateway EUI: AABBCCFFFE001122        <-- el tuyo será distinto (deriva de la MAC de tu ESP)
WiFi connected, IP 192.168.1.60
...
Up 868.1 SF9BW125 ... rxpk           <-- una línea así por cada uplink captado
```
*(El formato exacto depende de la versión del firmware; tómalo como orientación.)*

## ⚠️ Qué es (y qué NO es) un gateway de 1 canal
Un gateway "de verdad" lleva un concentrador (SX1301/SX1302) que escucha **8 canales y todos los SF a
la vez**. Este montaje usa **un único SX1276**, así que solo oye **una frecuencia**. Ten presente:

- ✅ **Sirve para aprender** el flujo completo nodo → gateway → ChirpStack con hardware barato.
- ❌ **No es LoRaWAN-compliant** ni para producción: escucha **1 de 8 canales** → si el nodo salta de
  canal (hopping/ADR) **pierde ~7/8 de los uplinks**. Aguanta **2–3 nodos**; con más, colisiones.
- ⛔ **Bloqueado en TTN público**, pero **funciona perfectamente contra tu ChirpStack privado**
  (nuestro caso).
- 🕰️ Firmware **sin mantenimiento desde 2021** (v6.2.8) → puede exigir **fijar librerías antiguas**
  para compilar (ver [`VERSIONS.md`](VERSIONS.md)). Usa **ESP32** (la TTGO ya lo es), no ESP8266.

> **Regla de oro:** para que este gateway oiga a un nodo, **nodo y gateway deben coincidir en región,
> frecuencia y SF**. Casi todo lo que "no funciona" se reduce a esto.

## 📡 Aviso de banda (este curso usa dos bandas)
El gateway de ejemplo está en **EU868**, canal 0. Como es de **1 canal, solo recibe UNA frecuencia**,
así que **solo sirve a nodos de su misma banda fijados a ese canal/SF**:

| Quieres servir a… | Qué hacer |
|---|---|
| Un nodo **EU868** | Úsalo tal cual (868.1 MHz, SF9) y **fija el nodo a ese canal/SF**. |
| Un nodo **US915** (ej.05/06) | Compila el firmware con `#define US902_928 1` y un `_CHANNEL` de la **sub-banda 2 (FSB2, ~903.9–905.3 MHz)**, y **fija el nodo a ese mismo canal/SF**. |
| La **radiosonda US915 del [ej.08](../08_radiosonda_picaro/)** (multicanal) | ⛔ **No funciona** con este gateway de 1 canal (salta por toda la banda) salvo que la fijes a 1 canal. Para ella usa un **gateway multicanal US915**. |

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir:

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Tu `TOKEN`** (API key de ChirpStack) y los IDs del stack → [`../COMMON_CHIRPSTACK_API.md`](../COMMON_CHIRPSTACK_API.md).
- [ ] **Hardware:** TTGO ESP32 LoRa (SX1276) — la misma placa de los ej.05/06 — y su cable USB. El radio ya viene cableado internamente: no sueldas nada. ⚠️ **No la enciendas sin antena.**
- [ ] **Entorno de compilación:** **PlatformIO** (recomendado; el repo trae `platformio.ini`) o **Arduino IDE** con el core **ESP32** → [Wiki: Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación).
- [ ] **Red:** gateway y host de ChirpStack en la **misma LAN** (o rutados), con el **puerto 1700/UDP abierto** en el host de ChirpStack.

> ℹ️ Los `./*.sh` corren en **bash** (Linux/macOS o **WSL** en Windows). Los valores de ejemplo de
> esta guía son **EU868, canal 0 (868.1 MHz), SF9**; para US915 lee el [aviso de banda](#-aviso-de-banda-este-curso-usa-dos-bandas).

## 🪜 Paso a paso

### 1. Descarga y fija el firmware (terceros)
El repositorio **no publica releases ni tags**, así que se ancla por **SHA de commit**:
```bash
git clone https://github.com/things4u/ESP-1ch-Gateway.git
cd ESP-1ch-Gateway
git checkout 3b023527bd23cf33657dc7ffdf5bedaf1b85cdcc   # v6.2.8 (2021-10-18), sin release formal
```
**Salida esperada:** `HEAD is now at 3b02352 ... Version 6.2.8: TTN3`. El sketch principal es
`src/ESP-sc-gway.ino`. Las dependencias van **vendorizadas** en `lib/`; solo `sandeepmistry/LoRa
0.8.0` se resuelve aparte (ver [`VERSIONS.md`](VERSIONS.md)).

### 2. Configura `configGway.h` y `configNode.h`
Toda la configuración vive en esas dos cabeceras de `src/`. Copia/adapta los `#define` de
[`configGway.example.h`](configGway.example.h) (radio/región/canal) y de
[`configNode.example.h`](configNode.example.h) (WiFi/identidad/ubicación). Los imprescindibles:

| Parámetro | Fichero | Valor (ejemplo) | Significado |
|-----------|---------|-----------------|-------------|
| `EU863_870` | `configGway.h` | `1` | Plan **EU868** (para US915: `US902_928 1`) |
| `_CHANNEL` | `configGway.h` | `0` | Canal único → **868.1 MHz** en EU |
| `_SPREADING` | `configGway.h` | `SF9` | SF por defecto (debe coincidir con el nodo) |
| `_STRICT_1CH` | `configGway.h` | `1` | Downlinks (ACK/Join Accept) en el **mismo canal/SF** del uplink — **clave** en 1 canal |
| `_TTNSERVER` | `configGway.h` | `"<IP_DE_CHIRPSTACK>"` | **Servidor → apúntalo a tu ChirpStack** (ruta simple) |
| `_TTNPORT` | `configGway.h` | `1700` | Puerto **UDP** (Semtech UDP / Gateway Bridge) |
| `_SERVER` | `configGway.h` | `1` | Interfaz web de admin en `http://<IP>:80` |
| `wpa[]` | `configNode.h` | `{ "SSID", "clave" }` | Tu **WiFi** (en el repo viene **vacío** → el gateway no conecta) |
| `_LAT`/`_LON`/`_ALT` | `configNode.h` | tu ubicación | ChirpStack la muestra en el mapa |

> 🔧 **Pines del radio (`_PIN_OUT`), env de PlatformIO y TTGO-vs-Heltec:** el detalle está en
> [`VERSIONS.md`](VERSIONS.md). En resumen: para **TTGO** pon **`_PIN_OUT 4`** (el default del repo es
> `1`, de ESP8266); Heltec V2 = `5`.

> **Ruta hacia ChirpStack (elige UNA):** **(a) simple** — pon `_TTNSERVER`/`_TTNPORT` en
> `configGway.h` con la IP de tu ChirpStack y **1700/UDP** (lo recomendado aquí); **(b) secundario** —
> conserva `_TTNSERVER` y **descomenta `_THINGSERVER`/`_THINGPORT` en `configNode.h`** para enviar a
> ChirpStack *además* del primario. No mezcles las dos sin querer.

### 3. Compila, flashea y lee el **Gateway EUI real**
Compila `src/ESP-sc-gway.ino` y flashea por USB. El `platformio.ini` del repo trae un único entorno
(Heltec por OTA); para **TTGO por USB** usa el entorno adaptado de [`VERSIONS.md`](VERSIONS.md)
(`board = ttgo-lora32-v1`, `upload_protocol = esptool`, `-D _PIN_OUT=4`). Abre el **monitor serie a
115200**.
**Salida esperada** (algo así): WiFi conectado y la **línea del Gateway EUI** (lo genera el firmware
a partir de la MAC del ESP; también aparece en `http://<IP>:80`):
```
WiFi connected, IP 192.168.1.60
Gateway EUI: AABBCCFFFE001122     <-- el tuyo será distinto; anótalo
```
> 🔑 **NO inventes el EUI.** Léelo del arranque serie o de la web y **regístralo tal cual** en
> ChirpStack. Si registras un EUI distinto del que emite, ChirpStack lo verá **offline** aunque esté
> enviando.

### 4. Registra el gateway en ChirpStack
ChirpStack recibe por el **Gateway Bridge** con el backend **Semtech UDP (1700/UDP)**, justo lo que
emite este firmware. Solo hay que **registrar el gateway** (una vez):
```bash
# desde specs/exercises/07_esp-1ch-gateway/
export TOKEN="tu_api_key"                  # web ChirpStack -> Tenant -> API keys
./register_gateway.sh AABBCCFFFE001122     # <-- el EUI real del paso 3
```
**Salida esperada:**
```
== Gateway aabbccfffe001122 en tenant f8a271ec-... ==
  creando gateway 'esp-1ch-gateway'...
  creado.
== LISTO ==
   Gateway aabbccfffe001122 registrado en ChirpStack.
```
*(Alternativa por UI: `Gateways → Add gateway`, pega el EUI y guarda.)* En **Gateways** debe pasar a
**online** en segundos si hay tráfico/keep-alive.

### 5. Comprueba que llegan los uplinks
Enciende un nodo que **coincida en banda, canal y SF** con el gateway (ver el [aviso de banda](#-aviso-de-banda-este-curso-usa-dos-bandas)
y la [nota sobre fijar el nodo](#-fijar-un-nodo-multicanal-a-1-canal)).
**Salida esperada:**
- **Serie del gateway:** una línea `Up`/`rxpk` por cada uplink captado.
- **ChirpStack → Gateways:** tu gateway **online**, *last seen* reciente.
- **ChirpStack → tu app → device → LoRaWAN frames:** los uplinks del nodo (p. ej. el BMP280 del
  ej.06 en fPort 2) llegando **por tu gateway**.

### 📌 Fijar un nodo multicanal a 1 canal
Un nodo LoRaWAN normal reparte sus envíos entre 8 canales; como este gateway solo oye uno, hay que
**fijar el nodo a esa misma frecuencia/SF** y **desactivar el ADR/hopping**:
- **Nodos LMIC** (ej.05/06): `LMIC_disableChannel(i)` para todos menos el canal elegido,
  `LMIC_setDrTxpow(DR_SF9, 14)` y `LMIC_setAdrMode(0)`.
- **Nodo LR1110** (ej.02): configura un **canal/SF único** en el stack.

Con `_STRICT_1CH=1` el gateway devuelve los downlinks (ACK/Join Accept) en ese mismo canal.
💡 Si el join OTAA falla, prueba primero **ABP** (el nodo ya está activado y validas antes el camino de datos).

## ✅ Cómo saber que funcionó
- [ ] La serie del gateway imprime la **línea del Gateway EUI** y **WiFi conectado**.
- [ ] En **ChirpStack → Gateways** tu gateway está **online** con *last seen* reciente.
- [ ] Cuando transmite el nodo, la serie del gateway muestra una línea **`Up`/`rxpk`**.
- [ ] En **LoRaWAN frames** del device ves sus uplinks llegando **a través de tu gateway**.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| **Gateway offline** en ChirpStack | EUI mal registrado, 1700/UDP bloqueado, o firmware apuntando a otra IP | Registra el **EUI real** (paso 3); abre **1700/UDP** en el host; en la ruta simple, `_TTNSERVER` = IP de ChirpStack, `_TTNPORT 1700` |
| El gateway ve `rxpk` pero **ChirpStack no lo registra** | EUI distinto del que emite, o puerto cerrado | Mismo arreglo: EUI real + 1700/UDP abierto |
| **No llega nada de un nodo** | El nodo hace **hopping/ADR** o usa **otro SF/canal/banda** | Fija el nodo a ese canal + SF y desactiva ADR ([nota](#-fijar-un-nodo-multicanal-a-1-canal)); comprueba la banda ([aviso](#-aviso-de-banda-este-curso-usa-dos-bandas)) |
| Solo llegan **algunos** uplinks | Esperado en 1 canal si el nodo salta de canal | Fija el nodo; asume la limitación del single-channel |
| **No hace join (OTAA)** | El Join Accept no cae en la ventana RX | Usa `_STRICT_1CH=1`; prueba primero **ABP** |
| El firmware **no compila** | Librerías más nuevas que 2021, o `_PIN_OUT` al default 1 | Fija versiones antiguas y pon `_PIN_OUT 4` (TTGO) — ver [`VERSIONS.md`](VERSIONS.md) |

## ➡️ Navegación
- ⬅️ Vienes de los nodos: [Ejercicio 05](../05_ttgo-lora32/) · [Ejercicio 06 · TTGO+BMP280](../06_ttgo-bmp280/)
- ➡️ Siguiente: [Ejercicio 08 · Radiosonda PICARO](../08_radiosonda_picaro/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Archivos:** [`configGway.example.h`](configGway.example.h) (radio/región/canal/`_STRICT_1CH`) · [`configNode.example.h`](configNode.example.h) (WiFi/identidad/ubicación) · [`register_gateway.sh`](register_gateway.sh) (alta por API, idempotente) · [`VERSIONS.md`](VERSIONS.md) (commit exacto + `_PIN_OUT`/PlatformIO/librerías).
- **Guía común:** [ChirpStack API](../COMMON_CHIRPSTACK_API.md).
- **Terceros:** [ESP-1ch-Gateway (things4u)](https://github.com/things4u/ESP-1ch-Gateway) · [ChirpStack — conectar gateway](https://www.chirpstack.io/docs/guides/connect-gateway.html) · [ChirpStack — Semtech UDP (1700)](https://www.chirpstack.io/docs/chirpstack-gateway-bridge/backends/semtech-udp.html)

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
