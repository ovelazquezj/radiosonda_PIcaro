# Ejercicio 03 — Hardware Modem (LR1110 controlado por host)

**Qué demuestra:** la arquitectura **host + módem**. Aquí el LR1110 (sobre la Nucleo-L476RG) no
es un nodo autónomo: se comporta como un **módem LoRaWAN** que un **host externo** gobierna por
**UART + 3 GPIO**. El host fija en **runtime** DevEUI, JoinEUI, AppKey **y la región**, lanza el
join y pide los uplinks. Es el laboratorio para explicar cómo se integra un módulo LoRaWAN dentro
de otro producto (un MCU/PC anfitrión que "habla" el protocolo de comandos).

| | |
|---|---|
| Hardware | Nucleo-L476RG + LR1110 |
| Capacidad LR1110 | Radio LoRaWAN multi-banda (**US915 + EU868**) |
| Credenciales | **Ninguna baked-in** — las fija el host por comando en runtime |
| ¿Join/ChirpStack? | ✅ Sí (con las credenciales que configure el host) |
| Control | UART (115200 8N1) + 3 GPIO de handshake |
| Binario | `artifacts/hw-modem_lr1110_multiband.bin` |
| Dato para dashboard | los uplinks que pida el host, vía MQTT |

> ⚠️ **Necesita un host.** Sin un anfitrión que hable el protocolo, la placa **queda a la espera y
> no hace nada sola** (no se une a la red por sí misma). Para conectividad "que se vea sola" sin
> montar un host, usa el **ejercicio 01 (Periodical Uplink)**, cuyo binario trae credenciales y
> región compiladas.

## Arquitectura (host ↔ módem)

```
   HOST (MCU/PC)                     LR1110 hw-modem (Nucleo-L476RG)
   ┌───────────┐   UART TX/RX      ┌──────────────────────────────┐
   │ tu script │◄────────────────►│  cmd_parser.c  (comandos)     │──► RF LoRaWAN ─► ChirpStack
   │  o herr.  │   COMMAND (in) ──►│  PC_6                         │
   │  Semtech  │   EVENT   (out)◄──│  PC_5  (hay evento que leer)  │
   │           │   BUSY    (out)◄──│  PC_8  (módem ocupado)        │
   └───────────┘                   └──────────────────────────────┘
```

- **UART:** transporta los comandos y las respuestas (payload de cada comando).
- **COMMAND (PC_6, entrada al módem):** el host la asserta para anunciar que va a enviar un comando.
- **EVENT (PC_5, salida del módem):** el módem la sube para avisar al host de que hay un evento
  pendiente (p.ej. `JOINED`, `TXDONE`, downlink). El host responde con `CMD_GET_EVENT`.
- **BUSY (PC_8, salida del módem):** indica que el módem está procesando y no acepta otro comando.

Definición de pines en `lbm_examples/modem_pinout.h`; lista completa de comandos en
`lbm_examples/hw_modem/cmd_parser.h`; handshake de GPIO en `lbm_examples/hw_modem/hw_modem.c`.

## El protocolo de comandos (lo esencial)

El host se comunica con opcodes de 1 byte (ver `cmd_parser.h`). Los relevantes para poner el
módem en línea:

| Orden | Comando | Opcode | Qué hace |
|-------|---------|--------|----------|
| 1 | `CMD_SET_DEV_EUI` | `0x0B` | Fija el DevEUI |
| 2 | `CMD_SET_JOIN_EUI` | `0x09` | Fija el JoinEUI (=AppEUI) |
| 3 | `CMD_SET_NWKKEY` | `0x0C` | Fija la AppKey (llamada *nwkkey* en 1.0.x) |
| 4 | `CMD_SET_REGION` | `0x01` | Selecciona la región (US915 / EU868) |
| 5 | `CMD_JOIN_NETWORK` | `0x03` | Lanza el join OTAA |
| — | *(esperar)* | — | El módem sube **EVENT**; el host lee con `CMD_GET_EVENT` (`0x05`) hasta ver `JOINED` |
| 6 | `CMD_REQUEST_UPLINK` | `0x04` | Envía un uplink de datos |

> El host es el que decide **qué DevEUI/AppKey y qué región** usar. Ese mismo DevEUI es el que hay
> que dar de alta en ChirpStack (ver `provision.sh`).

## Pasos del ejercicio

### 1. Flashear
Sigue [`../COMMON_FLASH.md`](../COMMON_FLASH.md) con `artifacts/hw-modem_lr1110_multiband.bin`
(o el `.hex`). El firmware es multi-banda: la región se elige después, por comando.

### 2. Conectar y controlar por host
Necesitas un **host que hable el protocolo** de `cmd_parser.h`:
- la **herramienta de host de Semtech** para el hardware modem, o
- un **script propio** (Python/C) que abra el UART, maneje las líneas COMMAND/EVENT/BUSY y envíe
  los opcodes en el orden de la tabla anterior.

Flujo mínimo del host: `set_deveui → set_joineui → set_nwkkey → set_region → join_network →`
`esperar EVENT=JOINED → request_uplink`.

### 3. Registrar en ChirpStack el DevEUI que use el host (por API)
Setup del token en [`../COMMON_CHIRPSTACK_API.md`](../COMMON_CHIRPSTACK_API.md). Como las
credenciales las define el host, `provision.sh` es **parametrizable** — pásale las mismas que
cargará el host:
```bash
export TOKEN="tu_api_key"
# Con variables de entorno:
DEVEUI=aabbccdd10930001 JOINEUI=aabbccddeeff0000 APPKEY=10301030103010301030103010301030 ./provision.sh
# ...o como argumentos posicionales:
./provision.sh aabbccdd10930001 aabbccddeeff0000 10301030103010301030103010301030
```
El script crea (idempotente) el device en la app `Demos-LR1110` con el device profile **US915**
`a177f6fe-…` y fija la AppKey en `nwkKey` (LoRaWAN 1.0.x).

> Región del device profile = región que seleccione el host con `CMD_SET_REGION`. El profile por
> defecto de este script es US915; para EU868 pasa un `DP` de esa región (ver `COMMON_CHIRPSTACK_API`).

### 4. Consumir los datos
Igual que en 01/05 — por MQTT:
```bash
./scripts/subscribe.sh                 # todos los devices de la app Demos-LR1110
./scripts/subscribe.sh aabbccdd10930001   # solo el DevEUI del host
```

## Qué observar
- **GPIO EVENT (PC_5):** pulso alto cada vez que hay un evento (JOINED, TXDONE…) que el host debe leer.
- **ChirpStack (LoRaWAN frames):** JoinRequest→JoinAccept y los uplinks que dispare el host.
- **MQTT:** un JSON por uplink en `application/<APP>/device/<DEVEUI>/event/up`.

## Recomendación docente
Usa este ejercicio para **explicar la arquitectura** host-módem (cómo se embebe un módulo LoRaWAN
en un producto y se controla por serie). Para una demo de conectividad **autónoma** que "se vea
sola" sin escribir un host, apóyate en el **ejercicio 01**.

## Archivos
```
artifacts/hw-modem_lr1110_multiband.bin   binario multi-banda (US915+EU868), sin credenciales
provision.sh                              alta parametrizable del DevEUI del host en ChirpStack
scripts/subscribe.sh                      stream MQTT de uplinks de la app
credentials.json                          refleja que NO hay credenciales baked-in (las fija el host)
```
