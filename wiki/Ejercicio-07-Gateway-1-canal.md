# Ejercicio 07 — Gateway LoRaWAN de 1 canal (TTGO + ESP-1ch-Gateway)

Monta tu **propio gateway LoRaWAN** con una **TTGO ESP32 (SX1276)** y el firmware de terceros
**ESP-1ch-Gateway** para que **ChirpStack reciba los uplinks** de los nodos del proyecto (el LR1110
del [ejercicio 02](Ejercicio-02-BMP280-GNSS) y los TTGO de los ejercicios
[05](Ejercicio-05-TTGO-LoRa32) y [06](Ejercicio-06-TTGO-BMP280)). Es el eslabón que faltaba: hasta
ahora dependías de un gateway ajeno; aquí **la red también es tuya**.

> **Carpeta del ejercicio:** [`specs/exercises/07_esp-1ch-gateway/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/07_esp-1ch-gateway) · **Plataforma:** **infraestructura** — TTGO ESP32 LoRa (SX1276) como gateway *single-channel*

| | |
|---|---|
| Qué demuestra | Un **gateway *single-channel*** que reenvía uplinks a ChirpStack por **Semtech UDP (1700/UDP)** |
| Hardware | **TTGO ESP32 LoRa** (SX1276) — la misma placa de los ejercicios 05/06 |
| ¿Join / ChirpStack? | ✅ (como **infraestructura**): se **registra el gateway** por su EUI real; **no** es un nodo |
| Dato / observable | Ninguno propio — **habilita** que lleguen los uplinks de los nodos 02/05/06 |
| Binario / sketch | **Firmware de terceros** ESP-1ch-Gateway (`src/ESP-sc-gway.ino`) — **no** se redistribuye aquí |

> ℹ️ **Atribución.** ESP-1ch-Gateway es un proyecto **de terceros** de **Maarten Westenberg
> (things4u)**, licencia **MIT** — **no** es de Semtech ni de radiosonda_PIcaro. Este ejercicio solo
> aporta la **guía de integración con ChirpStack** y el script de alta. Descarga el firmware de su
> [repo oficial](https://github.com/things4u/ESP-1ch-Gateway); aquí no se redistribuye su código.

## ⚠️ Qué es (y qué NO es) un gateway de 1 canal

Un gateway "de verdad" lleva un concentrador (SX1301/SX1302) que escucha **8 canales y todos los SF
a la vez**. Este montaje usa **un solo SX1276** → solo escucha **una frecuencia**:

- ✅ **Sirve para aprender** el flujo nodo → gateway → ChirpStack con hardware barato.
- ❌ **No es LoRaWAN-compliant**: escucha 1 de 8 canales → si el nodo salta de canal (hopping/ADR), pierde ~7/8 de los uplinks.
- ❌ **No es para producción**: aguanta bien 2–3 nodos; con más, colisiones y pérdidas.
- ⛔ **Bloqueado en TTN público**; **funciona perfectamente contra tu ChirpStack privado**, que es nuestro caso.

> 🎯 **Elige UNA banda extremo a extremo.** El ejemplo va en **EU868** (canal 0 / 868.1 MHz), pero los
> nodos **TTGO 05/06 son US915**. Dos rutas coherentes: **(a)** gateway EU868 + un nodo **LR1110 en
> EU868** (compila el [ej. 02](Ejercicio-02-BMP280-GNSS) en `EU_868`); **(b)** gateway en **US915**
> (`US902_928`, un canal de la FSB2, 8–15) + los **TTGO 05/06**. Nodo y gateway deben coincidir en banda.

## 📖 El cuerpo del ejercicio: la How-To completa

Todos los pasos —descargar y **fijar el commit** del firmware, configurar `configGway.h`/`configNode.h`,
compilar para **TTGO por USB**, leer el **Gateway EUI real**, dar de alta el gateway en ChirpStack y
**fijar los nodos a 1 canal + 1 SF**— están en la guía paso a paso:

### 👉 **[How-To · Montar tu propio gateway LoRaWAN de 1 canal](How-To-Montar-un-gateway-de-1-canal)**

> **Regla de oro:** para que el gateway oiga a un nodo, **el nodo y el gateway deben coincidir en
> región, frecuencia y (idealmente) SF**. Casi todo lo que "no funciona" se reduce a esto.

## Qué observar

- **Monitor serie del gateway (115200):** WiFi conectada, el **Gateway EUI**, y líneas `rxpk`/`Up` al captar un uplink.
- **Web del gateway** (`http://<IP>:80`): estadísticas, último paquete y ajuste en caliente de SF/canal.
- **ChirpStack → Gateways:** tu gateway en **online**, con *last seen* reciente.
- **ChirpStack → aplicación → device → LoRaWAN frames:** los uplinks de un nodo (p. ej. el fPort 2 del ejercicio 06) **llegando a través de tu gateway**.

## Detalles y configuración

- **Región/canal por defecto:** EU868 · canal 0 (**868.1 MHz**) · **SF9** · BW125.
- **Hacia el Network Server:** Semtech UDP Packet Forwarder → **1700/UDP** al Gateway Bridge de ChirpStack v4.
- **`_PIN_OUT = 4`** para TTGO (Heltec V2 = 5); el default del repo es `1` (ESP8266) → **hay que cambiarlo**.
- **`_STRICT_1CH = 1`** devuelve los downlinks en el mismo canal/SF del uplink (clave para ACK/join en 1 canal).
- **Empieza con ABP**, no con OTAA: en 1 canal el Join Accept en ventana RX es la parte más frágil.
- **Alta por API (idempotente):** `./register_gateway.sh <EUI_REAL>` — usa el **EUI que imprime el gateway**, no uno inventado (ver [COMMON_CHIRPSTACK_API.md](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/COMMON_CHIRPSTACK_API.md) para token e IDs del stack).

## Ficheros del ejercicio

*(El firmware es **externo**; esta carpeta solo trae la guía y los ejemplos de configuración — no hay `artifacts/` ni `sketches/`.)*

- [`README.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/README.md) — guía del ejercicio (misma que la How-To).
- [`VERSIONS.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/VERSIONS.md) — **commit exacto** del firmware (`3b02352`, v6.2.8) + versiones de librerías que compilan.
- [`configGway.example.h`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/configGway.example.h) — `#define` clave (TTGO, EU868, canal 0, SF9, `_STRICT_1CH`).
- [`configNode.example.h`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/configNode.example.h) — WiFi (`wpa[]`), identidad y ubicación del gateway.
- [`register_gateway.sh`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/07_esp-1ch-gateway/register_gateway.sh) — alta del gateway en ChirpStack v4 por API REST (idempotente).

> ◀ [Ejercicio 06 — TTGO + BMP280](Ejercicio-06-TTGO-BMP280) · [Ejercicio 08 — Radiosonda PICARO ▶](Ejercicio-08-Radiosonda-PICARO)
