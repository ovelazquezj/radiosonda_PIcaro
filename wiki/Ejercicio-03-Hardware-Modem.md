# Ejercicio 03 — Hardware Modem (LR1110 controlado por host)

Muestra la arquitectura **host + módem**: aquí el LR1110 no es un nodo autónomo, sino un **módem
LoRaWAN** que un **host externo** gobierna por **UART + 3 GPIO**, fijando en **runtime** el DevEUI,
JoinEUI, AppKey **y la región**. Es el laboratorio para explicar cómo se **embebe** un módulo
LoRaWAN dentro de otro producto que "habla" su protocolo de comandos.

> **Carpeta del ejercicio:** [`specs/exercises/03_hw-modem/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/03_hw-modem) · **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG**

| | |
|---|---|
| Qué demuestra | Arquitectura **host ↔ módem**: el host controla el LR1110 por UART + GPIO y fija credenciales/región en runtime |
| Hardware | Nucleo-L476RG + shield LR1110, **+ un host** (MCU/PC) que hable el protocolo |
| ¿Join / ChirpStack? | **Sí**, pero con las credenciales que cargue el host. **Ninguna baked-in** en el binario |
| Dato / observable | GPIO **EVENT (PC_5)** en cada evento; los uplinks que pida el host, por MQTT |
| Binario / sketch | Ya compilado en `artifacts/`: `hw-modem_lr1110_multiband.bin` (multi-banda US915+EU868) — solo **flashear** |

> ⚠️ **Ejercicio de arquitectura, no de demo autónoma.** El ejercicio **no incluye un host
> ejecutable**: sin un anfitrión que hable el protocolo de comandos, la placa **queda a la espera y
> no hace nada sola** (no se une a la red por sí misma). Para una demo de conectividad *end-to-end*
> "que se vea sola", usa el [Ejercicio 01 — Periodical Uplink](Ejercicio-01-Periodical-Uplink),
> cuyo binario ya trae credenciales y región compiladas.

## Arquitectura (host ↔ módem)

El host manda el `.bin` de este ejercicio a modo de **módem**: le habla por UART y se coordina con
3 GPIO de handshake.

- **UART (115200 8N1):** transporta los comandos y sus respuestas.
- **COMMAND (PC_6, entrada al módem):** el host la asserta para anunciar que va a enviar un comando.
- **EVENT (PC_5, salida del módem):** el módem la sube para avisar de que hay un evento pendiente (`JOINED`, `TXDONE`, downlink…); el host responde con `CMD_GET_EVENT`.
- **BUSY (PC_8, salida del módem):** el módem está procesando y no acepta otro comando.

Flujo mínimo que debe ejecutar el host: `set_deveui → set_joineui → set_nwkkey → set_region →
join_network → esperar EVENT=JOINED → request_uplink`. El host decide **qué DevEUI/AppKey y qué
región** usar, y ese mismo DevEUI es el que se da de alta en ChirpStack.

## Ruta paso a paso

1. **Requisitos** → [Requisitos e instalación](How-To-Requisitos-e-instalación) · herramienta de grabado + terminal serie (sección **3**) y **ChirpStack levantado** ([Ejercicio 00](Ejercicio-00-ChirpStack)).
2. **(Opcional) Recompilar** → [Compilar el firmware](How-To-Compilar-el-firmware) · el binario **ya viene**; recompila solo si quieres tu variante (comando de este ejercicio en [`COMMON_BUILD.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/COMMON_BUILD.md)). Es **multi-banda**: la región se elige después, por comando del host.
3. **Flashear** → [Flashear y ver la serie](How-To-Flashear-y-ver-la-serie) · arrastra `hw-modem_lr1110_multiband.hex` (o `.bin`) al disco `NODE_L476RG`.
4. **Conectar y controlar por host** → necesitas un host que hable el protocolo de `cmd_parser.h` (la herramienta de host de Semtech para hw-modem, o un script propio que maneje UART + las líneas COMMAND/EVENT/BUSY). **Este ejercicio no lo incluye** — es la pieza que tú aportas para la demo end-to-end.
5. **Registrar el DevEUI en ChirpStack** → [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack) · como las credenciales las define el host, `provision.sh` es **parametrizable**; pásale las mismas que cargará el host, p.ej.: `DEVEUI=aabbccdd10930001 JOINEUI=aabbccddeeff0000 APPKEY=1030…1030 ./provision.sh` (AppKey en **`nwkKey`**).
6. **Consumir los datos** → por MQTT igual que 01: `./scripts/subscribe.sh` (toda la app) o `./scripts/subscribe.sh aabbccdd10930001` (solo el DevEUI del host).

## El protocolo de comandos (lo esencial)

El host se comunica con opcodes de 1 byte (lista completa en `lbm_examples/hw_modem/cmd_parser.h`):

| Orden | Comando | Opcode | Qué hace |
|-------|---------|--------|----------|
| 1 | `CMD_SET_DEV_EUI` | `0x0B` | Fija el DevEUI |
| 2 | `CMD_SET_JOIN_EUI` | `0x09` | Fija el JoinEUI (=AppEUI) |
| 3 | `CMD_SET_NWKKEY` | `0x0C` | Fija la AppKey (*nwkkey* en 1.0.x) |
| 4 | `CMD_SET_REGION` | `0x01` | Selecciona la región (US915 / EU868) |
| 5 | `CMD_JOIN_NETWORK` | `0x03` | Lanza el join OTAA |
| — | *(esperar)* | — | El módem sube **EVENT**; el host lee con `CMD_GET_EVENT` (`0x05`) hasta ver `JOINED` |
| 6 | `CMD_REQUEST_UPLINK` | `0x04` | Envía un uplink |

Definición de pines en `lbm_examples/modem_pinout.h`; handshake de GPIO en `lbm_examples/hw_modem/hw_modem.c`.

## Qué observar

- **GPIO EVENT (PC_5):** pulso alto cada vez que hay un evento (JOINED, TXDONE…) que el host debe leer con `CMD_GET_EVENT`.
- **ChirpStack → LoRaWAN frames:** JoinRequest → JoinAccept y los uplinks que dispare el host (con las credenciales que este haya cargado).
- **MQTT:** un JSON por uplink en `application/<APP>/device/<DEVEUI>/event/up`.

## Credenciales y detalles

- **Sin credenciales baked-in:** el `credentials.json` de la carpeta refleja que DevEUI/JoinEUI/AppKey son **`null`** — los fija el host en runtime. Ver [`credentials.json`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/03_hw-modem/credentials.json).
- **Región del device profile = región que seleccione el host** con `CMD_SET_REGION`. El profile por defecto de `provision.sh` es **US915** (`a177f6fe-…`); para EU868 pásale un DP de esa banda.
- **AppKey en `nwkKey`** (LoRaWAN 1.0.x). Registra en ChirpStack **exactamente** el DevEUI/AppKey que cargará el host, o el join falla.
- **Orden de bytes (LR1110):** DevEUI/JoinEUI en **MSB**, sin invertir (ver [Dar de alta un LR1110 nuevo](How-To-Dar-de-alta-un-LR1110-nuevo)).
- **US915 · sub-banda `us915_1`** en el device profile (ver [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)).

## Ficheros del ejercicio

- `artifacts/hw-modem_lr1110_multiband.bin` (`.hex`/`.elf`) — binario multi-banda (US915+EU868), **sin** credenciales.
- `provision.sh` — alta **parametrizable** del DevEUI/AppKey del host en ChirpStack.
- `scripts/subscribe.sh` — stream MQTT de uplinks de la app.
- `credentials.json` — refleja que **no** hay credenciales baked-in (las fija el host).
- Detalle en el [README de la carpeta](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/03_hw-modem).

> ◀ [Ejercicio 02 — BMP280 + GNSS](Ejercicio-02-BMP280-GNSS) · [Ejercicio 04 — Wi-Fi Region Detection ▶](Ejercicio-04-Wi-Fi-Region-Detection)
