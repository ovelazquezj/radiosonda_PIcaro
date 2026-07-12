# Ejercicio 03 — Hardware Modem (LR1110 controlado por host) · 🧩 CONCEPTUAL / AVANZADO

> **En una frase:** entiendes la arquitectura **host + módem**: el LR1110 deja de ser un nodo
> autónomo y pasa a ser un **módem LoRaWAN** que un **host externo** gobierna por **UART + 3 GPIO**
> (el host fija DevEUI/AppKey/región en runtime, lanza el join y pide los uplinks).
> **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG**. **Banda:** multi-banda
> (US915 + EU868; la elige el host). El binario **ya viene compilado** en `artifacts/`.
>
> **⚠️ Ejercicio conceptual/avanzado.** Este ejemplo **necesita un host** que hable el protocolo de
> comandos por UART, y **ese host NO se entrega aquí**. Sin él, la placa **queda a la espera y no
> hace nada sola** (no se une a la red por sí misma). Léelo para **entender la arquitectura**; para
> ver LoRaWAN "funcionando solo" vuelve al **[Ejercicio 01](../01_periodical-uplink/)**, cuyo binario
> trae credenciales y región compiladas.

## 🎯 Qué vas a conseguir
Comprender **cómo se embebe un módulo LoRaWAN dentro de otro producto**: un MCU/PC anfitrión (el
*host*) que "habla" con el módem por una línea serie y tres GPIO de handshake. Al terminar sabrás:

- Qué papel juega cada línea física (**UART**, **COMMAND**, **EVENT**, **BUSY**).
- La **secuencia de comandos** (opcodes) que pone el módem en línea: fijar credenciales → región →
  join → esperar `JOINED` → pedir uplink.
- Cómo **registrar en ChirpStack** el DevEUI/AppKey que cargará el host (con `provision.sh`).

```
   HOST (MCU/PC)                     LR1110 hw-modem (Nucleo-L476RG)
   ┌───────────┐   UART TX/RX      ┌──────────────────────────────┐
   │ tu script │◄────────────────►│  cmd_parser.c  (comandos)     │──► RF LoRaWAN ─► ChirpStack
   │  o herr.  │   COMMAND (in) ──►│  PC_6                         │
   │  Semtech  │   EVENT   (out)◄──│  PC_5  (hay evento que leer)  │
   │           │   BUSY    (out)◄──│  PC_8  (módem ocupado)        │
   └───────────┘                   └──────────────────────────────┘
```

> **Nota docente:** no hay una "salida final observable" garantizada como en los demás ejercicios,
> porque depende de un host que tú aportes. Los pasos 1, 3 y 4 sí se pueden ejecutar; el paso 2
> (control por host) es **conceptual** salvo que dispongas de la herramienta de host de Semtech o
> escribas tu propio script.

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir:

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Tu `TOKEN`** (API key de ChirpStack) exportado — lo creas en el [Ejercicio 00](../00_chirpstack-docker/): web `:8080` → *Tenant* → *API keys*.
- [ ] **Un gateway US915** *online* en ChirpStack *(solo si vas a intentar el join con un host real)* → [Ejercicio 07](../07_esp-1ch-gateway/) o uno comercial.
- [ ] **Hardware:** Nucleo-L476RG + shield **LR1110** y su cable USB.
- [ ] **(Avanzado) Un host** que hable el protocolo de `cmd_parser.h`: la **herramienta de host de Semtech** para el hardware modem, **o** un script propio (Python/C) que maneje UART + las líneas COMMAND/EVENT/BUSY. **No se incluye en este repo.**
- [ ] **Herramientas:** una utilidad para flashear + un **terminal serie** (115200 8N1) → [Wiki: Flashear y ver la serie](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Flashear-y-ver-la-serie).

> ℹ️ Los `./*.sh` y `python3` corren en **bash** (Linux/macOS o **WSL** en Windows). La primera vez,
> hazlos ejecutables: `chmod +x provision.sh scripts/*.sh`.
> **Credenciales:** este binario **no lleva ninguna compilada** — las fija el host en runtime. En los
> ejemplos usamos DevEUI `aabbccdd10930001`, JoinEUI `aabbccddeeff0000`, AppKey `10301030…1030`
> (ver [`credentials.json`](credentials.json)).

## 📟 Hardware y conexiones
Para flashear y ver la traza basta con el **USB del ST-LINK**. Para **controlar el módem** conectas
un host a la línea serie del hardware modem (**PC_10/PC_11**, distinta de la UART de depuración
PA_2/PA_3) y a los tres GPIO de handshake. Pines fijados en `lbm_examples/modem_pinout.h`:

| Señal | Nucleo (módem) | Dirección | Se conecta al host |
|-------|----------------|-----------|--------------------|
| **COMMAND** | **PC_6** | host → módem | GPIO **salida** del host (anuncia que va a enviar un comando) |
| **EVENT** | **PC_5** | módem → host | GPIO **entrada** del host (hay un evento que leer) |
| **BUSY** | **PC_8** | módem → host | GPIO **entrada** del host (módem ocupado, no acepta comandos) |
| **UART TX del módem** | **PC_10** | módem → host | **RX** del host *(cruzado)* |
| **UART RX del módem** | **PC_11** | host → módem | **TX** del host *(cruzado)* |
| **GND** | **GND** | común | **GND** del host (masa común obligatoria) |

- **UART (PC_10/PC_11, 115200 8N1):** transporta los comandos y sus respuestas. **Se cruza**: TX de
  uno va al RX del otro.
- **COMMAND (PC_6):** el host la asserta para avisar de que va a enviar un comando.
- **EVENT (PC_5):** el módem la sube para avisar al host de que hay un evento pendiente (`JOINED`,
  `TXDONE`, downlink…). El host responde leyéndolo con `CMD_GET_EVENT` (`0x05`).
- **BUSY (PC_8):** indica que el módem está procesando y no acepta otro comando.

> Handshake de GPIO en `lbm_examples/hw_modem/hw_modem.c`; lista completa de comandos en
> `lbm_examples/hw_modem/cmd_parser.h`. **⚠️ Nunca enciendas la radio LR1110 sin antena.**

## 🪜 Paso a paso

### 1. Flashea el binario
Graba `artifacts/hw-modem_lr1110_multiband.bin` (o el `.hex`) en la Nucleo. Método (arrastrar al
disco `NODE_L476RG` o STM32CubeProgrammer) en [`../COMMON_FLASH.md`](../COMMON_FLASH.md). El firmware
es **multi-banda**: la región **no** está compilada, se elige después por comando.

**Salida esperada:** el LED del ST-LINK parpadea al copiar y la placa se reinicia sola. Tras el
reset, la placa **espera comandos** por UART: sin host, no verás actividad de red (es lo normal).

### 2. Controla el módem con un host *(conceptual — requiere host propio)*
Necesitas un **host que hable el protocolo** de `cmd_parser.h` (herramienta de Semtech o script
propio). El flujo mínimo, en orden, usando los opcodes de la [tabla de más abajo](#-el-protocolo-de-comandos-opcodes):

```
set_dev_eui  (0x0B)  → set_join_eui (0x09) → set_nwkkey (0x0C)
→ set_region (0x01)  → join_network (0x03)
→ (esperar EVENT alto; leer con get_event 0x05 hasta ver JOINED)
→ request_uplink (0x04)
```

**Salida esperada** (si tienes host): un **pulso alto en EVENT (PC_5)** tras el join, y al leerlo
con `CMD_GET_EVENT` obtienes el evento `JOINED`; después, cada uplink genera otro EVENT (`TXDONE`).

> ⚠️ Las credenciales que cargue el host (paso 2) deben ser **exactamente** las que registres en
> ChirpStack (paso 3). Si difieren, el join se rechaza por **MIC inválido**.

### 3. Registra en ChirpStack el DevEUI que usará el host
Como las credenciales las define el host, `provision.sh` es **parametrizable**: pásale las mismas que
cargará el host. El script **autodetecta** tu tenant, la app `Demos-LR1110` y un device profile
**US915** (los crea si hacen falta):
```bash
# desde specs/exercises/03_hw-modem/
export TOKEN="tu_api_key"
# con variables de entorno:
DEVEUI=aabbccdd10930001 JOINEUI=aabbccddeeff0000 APPKEY=10301030103010301030103010301030 ./provision.sh
# ...o como argumentos posicionales:
./provision.sh aabbccdd10930001 aabbccddeeff0000 10301030103010301030103010301030
```
**Salida esperada:**
```
== tenant: 52f14cd4-... ==
== application: 5bc22cfa-... ==
== device profile US915: a177f6fe-... ==
== hw-modem: device aabbccdd10930001 (app Demos-LR1110, profile US915 a177f6fe-...) ==
  creado.
== AppKey (nwkKey en 1.0.x) ==
  AppKey fijada en nwkKey.
== LISTO: aabbccdd10930001 provisionado (JoinEUI aabbccddeeff0000) ==
   Exporta tu app para consumir:   export APP=5bc22cfa-...
   Ahora el host debe cargar EXACTAMENTE estas credenciales en el módem:
     CMD_SET_DEV_EUI=aabbccdd10930001  CMD_SET_JOIN_EUI=aabbccddeeff0000  CMD_SET_NWKKEY=10301030103010301030103010301030
   ...luego CMD_SET_REGION (US915) -> CMD_JOIN_NETWORK -> esperar JOINED -> CMD_REQUEST_UPLINK.
```
Ejecuta la línea `export APP=…` (la usarás en el paso 4).

> Región del device profile = región que seleccione el host con `CMD_SET_REGION`. El profile por
> defecto es **US915**; para EU868 exporta un `DP` de esa región (ver [`../COMMON_CHIRPSTACK_API.md`](../COMMON_CHIRPSTACK_API.md)).

### 4. Consume los datos
Igual que en el ejercicio 01, por MQTT (solo verás algo si un host llega a disparar uplinks):
```bash
# desde specs/exercises/03_hw-modem/
export APP=5bc22cfa-...                    # el que imprimió provision.sh
./scripts/subscribe.sh                     # todos los devices de la app
./scripts/subscribe.sh aabbccdd10930001    # solo el DevEUI del host
```
**Salida esperada** (MQTT): un JSON por uplink en el topic
`application/<APP>/device/aabbccdd10930001/event/up`.

## ✅ Cómo saber que funcionó
- [ ] **Entiendes la arquitectura:** sabes qué hacen UART, COMMAND (PC_6), EVENT (PC_5) y BUSY (PC_8).
- [ ] Tras flashear, la placa **arranca y espera comandos** (sin host, no hace join — es lo esperado).
- [ ] `provision.sh` deja el DevEUI dado de alta en ChirpStack con su AppKey en `nwkKey`.
- [ ] *(Solo con host)* tras `join_network` ves **EVENT (PC_5)** alto y, al leerlo, el evento `JOINED`;
      en ChirpStack aparecen **JoinRequest→JoinAccept** y los uplinks que dispares.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| El host **no recibe respuesta** por UART | TX/RX no cruzados, baudios ≠ 115200, o falta **GND común** | Cruza TX↔RX (PC_10↔RX host, PC_11↔TX host), pon 115200 8N1, une las masas |
| **EVENT (PC_5) nunca sube** | El host no assertó COMMAND (PC_6) o no envió `CMD_JOIN_NETWORK`, o no hay gateway con cobertura | Revisa el handshake de GPIO y el orden de opcodes; enciende un gateway US915 *online* |
| Join rechazado / **`invalid MIC`** en ChirpStack | El host cargó **DevEUI/JoinEUI/AppKey distintos** de los dados de alta | Usa en el host **exactamente** los del `provision.sh` (DevEUI, JoinEUI y AppKey idénticos) |
| `Joining…` eterno con claves OK | Región del host (`CMD_SET_REGION`) ≠ región del device profile | Igualalas (host US915 ↔ profile US915) |
| El módem responde `CMD_RC_BUSY` | Enviaste un comando con **BUSY (PC_8)** alto | Espera a que BUSY baje antes de enviar el siguiente comando |
| No hay traza serie de depuración | Baudios mal, o COM equivocado | 115200 8N1 en el VCP del ST-LINK (la de depuración es PA_2/PA_3) |
| No llega nada por MQTT | `APP` sin exportar, o ningún host ha disparado uplinks todavía | `export APP=<id>`; recuerda que sin host no hay uplinks |

## 📤 Los datos (payload)
Aquí "los datos" son el **protocolo de comandos**: el host habla con opcodes de 1 byte (ver
`lbm_examples/hw_modem/cmd_parser.h`). Los relevantes para poner el módem en línea:

<a id="-el-protocolo-de-comandos-opcodes"></a>

| Orden | Comando | Opcode | Qué hace |
|-------|---------|--------|----------|
| 1 | `CMD_SET_DEV_EUI` | `0x0B` | Fija el DevEUI |
| 2 | `CMD_SET_JOIN_EUI` | `0x09` | Fija el JoinEUI (=AppEUI) |
| 3 | `CMD_SET_NWKKEY` | `0x0C` | Fija la AppKey (llamada *nwkkey* en 1.0.x) |
| 4 | `CMD_SET_REGION` | `0x01` | Selecciona la región (US915 / EU868) |
| 5 | `CMD_JOIN_NETWORK` | `0x03` | Lanza el join OTAA |
| — | *(esperar)* | — | El módem sube **EVENT**; el host lee con `CMD_GET_EVENT` (`0x05`) hasta ver `JOINED` |
| 6 | `CMD_REQUEST_UPLINK` | `0x04` | Envía un uplink de datos |

Cuando un uplink llega a ChirpStack, se publica por MQTT como un JSON en
`application/<APP>/device/<DEVEUI>/event/up` (los bytes de datos los define el host en el
`CMD_REQUEST_UPLINK`; este ejercicio no adjunta un codec, así que verás el payload crudo en `data`).

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 02 · BMP280 + GNSS](../02_bmp280-gnss-tracker/)
- ➡️ Siguiente: [Ejercicio 04 · Wi-Fi region detection](../04_wifi-region-detection/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json) (refleja que **no** hay credenciales baked-in — las fija el host).
- **Archivos:** `provision.sh` (alta parametrizable del DevEUI del host) · `scripts/subscribe.sh` (MQTT sin Python) · `artifacts/` (binario multi-banda).
- **Código de referencia:** `lbm_examples/hw_modem/cmd_parser.h` (opcodes) · `lbm_examples/hw_modem/hw_modem.c` (handshake GPIO) · `lbm_examples/modem_pinout.h` (pines).
- **Guías comunes:** [Flashear](../COMMON_FLASH.md) · [ChirpStack API](../COMMON_CHIRPSTACK_API.md) · [Compilar (opcional)](../COMMON_BUILD.md).

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
