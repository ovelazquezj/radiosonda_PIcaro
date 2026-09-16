# Ejercicio 12 — Heltec WiFi LoRa 32 (V3 / V4): Join OTAA con credenciales generadas en ChirpStack

> **En una frase:** haces que una **Heltec WiFi LoRa 32** se una por **OTAA** al ChirpStack del curso
> (`lns.pi-caro.org`) usando un DevEUI, JoinEUI y AppKey que **genera ChirpStack**, y ves el join y cada
> paquete tanto en el **Monitor Serie** (con todo el detalle) como en la **OLED** de la placa.
> **Plataforma:** ESP32-S3 + radio **SX1262** + OLED SSD1306, firmware **Arduino IDE / RadioLib**.
> **Banda:** US915 (sub-banda 2). **Activación:** OTAA · **Clase A** · **LoRaWAN 1.0.x**.
> **Compatible con la V3 y la V4** de la placa (se elige con una constante).

## 🎯 Qué vas a conseguir
Una Heltec que arranca, muestra en la OLED `JOIN OTAA intento 1`, recibe el Join-Accept, pasa a
`CONECTADO (JOIN OK)` con su DevAddr, y desde entonces envía cada 30 s un paquete de 9 bytes con
contador, tiempo encendida y voltaje de batería. En el Monitor Serie verás cada Join-Request, cada
uplink con su frecuencia, datarate, potencia y fCnt, y cada downlink con su RSSI/SNR. En ChirpStack, el
device muestra *last seen* reciente y el payload llega decodificado, algo así:

```json
{ "deviceInfo": { "devEui": "a1b2c3d4e5f60718" },
  "fPort": 1,
  "data": "SAAHAAABDg+c",
  "object": { "counter": 7, "uptime_s": 270, "vbat_mv": 3996, "vbat_v": 4.0,
              "on_battery": true, "battery_pct": 77 } }
```

> ⚠️ **Salidas esperadas de este README:** están escritas a partir del código y de la documentación de
> RadioLib, **no de una captura real**. Los textos que imprime el sketch son exactos; los números (RSSI,
> tiempos, DevAddr) son "algo así". Cuando lo pruebes, corrígelas con lo que veas.

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, el join **no** ocurrirá):

- [ ] **Acceso a la consola del curso** → <https://lns.pi-caro.org> con el usuario de tu equipo
      (`equipoNN@pi-caro.org`, contraseña en la ficha que te dio el instructor).
- [ ] **Un gateway US915 sub-banda 2 online** en esa consola. Los del curso son compartidos: míralo en
      *Gateways* → debe decir *Online*. **⚠️ Sin gateway online el nodo se queda en `JOIN OTAA` para siempre.**
- [ ] **Hardware:** una **Heltec WiFi LoRa 32 V3 o V4** con su **antena de 915 MHz puesta** y cable USB de datos.
      Batería opcional (sin batería el voltaje se reporta como 0 = "USB").
- [ ] **Arduino IDE 2.x** con el **core ESP32 de Espressif (3.x)** y las librerías **RadioLib** (7.x) y
      **U8g2** → [Wiki: Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación).
- [ ] Para el Paso 8 (opcional): **bash** (Linux/macOS o WSL), `mosquitto_sub` y `jq`.

> ℹ️ En este ejercicio **no hay `provision.sh`**: las credenciales se crean **a mano en la consola** y se
> copian a la placa. Esa es la práctica: entender qué campo de ChirpStack va a qué constante del código.

## 📟 Hardware y conexiones
Placa integrada — no hay que cablear nada. **⚠️ Nunca enciendas la radio sin antena.**

Los pines viven en `sketches/heltec_lora32_join/board_heltec.h` y son los mismos en V3 y V4 para la radio
y la pantalla:

| Señal SX1262 | GPIO | | Señal OLED / otros | GPIO |
|---|---|---|---|---|
| NSS (CS) | 8 | | OLED SDA | 17 |
| SCK | 9 | | OLED SCL | 18 |
| MOSI | 10 | | OLED RST | 21 |
| MISO | 11 | | **Vext** (alimenta la OLED, activo en BAJO) | 36 |
| RST | 12 | | ADC batería | 1 |
| BUSY | 13 | | Control del divisor de batería | 37 |
| DIO1 (IRQ) | 14 | | LED (solo V3) | 35 |

Lo que **sí cambia** entre versiones, y el código resuelve solo con `HELTEC_BOARD_VERSION`:

| | V3 | V4 (4.2 y 4.3) |
|---|---|---|
| USB | chip CP2102 (puerto "Silicon Labs CP210x") | USB nativo del ESP32-S3 |
| Flash / PSRAM | 8 MB / no | 16 MB / 2 MB |
| Control del ADC de batería (GPIO37) | mide con **BAJO** | mide con **ALTO** |
| Amplificador de RF | no | sí: GC1109 (4.2) o KCT8103L (4.3), pines 7/2/46/5 |
| Potencia por defecto del SX1262 | 14 dBm | **2 dBm** (el amplificador suma ~11 dB) |

> 🔑 En la V4 **no subas la potencia a ciegas**: 22 dBm al amplificador salen como ~28 dBm y saturan
> el gateway si está a pocos metros. El valor por defecto deja unos 13 dBm en la antena, de sobra para un salón.

## 🪜 Paso a paso

### 1. Instala Arduino IDE, el core ESP32 y las dos librerías
1. Arduino IDE 2.x desde <https://www.arduino.cc/en/software>.
2. **File ▸ Preferences ▸ Additional boards manager URLs** →
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
3. **Tools ▸ Board ▸ Boards Manager** → instala **esp32 by Espressif Systems** (3.x).
4. **Tools ▸ Manage Libraries** → instala:

| Librería | Autor | Nota |
|---|---|---|
| **RadioLib** | Jan Gromes | 7.x (escrito con la API de 7.6/7.7). Radio SX1262 + pila LoRaWAN. |
| **U8g2** | oliver | Pantalla OLED SSD1306. |

### 2. Genera las credenciales en ChirpStack (consola web)
Entra a <https://lns.pi-caro.org> con el usuario de tu equipo. Vas a crear **un device profile** (una vez)
y **un device** (uno por placa).

**2.1 Device profile.** *Device profiles ▸ Add device profile*:

| Campo | Valor |
|---|---|
| Name | `Heltec-Join-US915` |
| Region | `US915` |
| MAC version | `LoRaWAN 1.0.3` |
| Regional parameters revision | `A` |
| ADR algorithm | `Default ADR algorithm (LoRa only)` |
| Expected uplink interval | `60` |
| Device supports OTAA | ✅ |
| Class B / Class C | ❌ |

Pestaña **Codec** → *Payload codec* = `JavaScript functions` → pega **todo** el contenido de
[`payload_decoder.js`](payload_decoder.js) → **Submit**.

> Si tu usuario no puede crear device profiles (el botón no aparece), pide al instructor que lo cree en tu
> tenant; el resto del ejercicio no cambia.

**2.2 Device.** *Applications ▸ `radiosonda`* (o crea una aplicación nueva) ▸ *Add device*:

| Campo | Qué hacer |
|---|---|
| Name | `heltec-<tu nombre>` |
| **Device EUI (EUI64)** | pulsa el icono de **generar** (🔄, a la derecha del campo) |
| **Join EUI (EUI64)** | pulsa el icono de **generar** también |
| Device profile | `Heltec-Join-US915` |

**Antes de dar Submit**, copia el DevEUI y el JoinEUI a un bloc de notas: el botón **MSB/LSB** debe estar
en **MSB** (así se copian a la placa). Submit.

**2.3 AppKey.** ChirpStack te lleva a la pestaña **OTAA keys**: en **Application key** pulsa **generar**,
cópiala al bloc de notas y **Submit**.

Ya tienes tus tres valores. Ejemplo (los tuyos serán distintos):

```
DevEUI  a1b2c3d4e5f60718
JoinEUI 70b3d57ed00512f3
AppKey  8ac583dfeec76c81ffd19ccfe76b73bf
```

### 3. Copia las credenciales y la versión de la placa a `config_lorawan.h`
Abre [`sketches/heltec_lora32_join/heltec_lora32_join.ino`](sketches/heltec_lora32_join/heltec_lora32_join.ino)
en Arduino IDE (abre las tres pestañas). Ve a la pestaña **`config_lorawan.h`** y edita **solo** esto:

```c
#define HELTEC_BOARD_VERSION   3        // 3 = V3, 4 = V4

#define PICARO_DEV_EUI    0xA1B2C3D4E5F60718ULL   // "a1b2c3d4e5f60718" -> 0x + 16 dígitos + ULL
#define PICARO_JOIN_EUI   0x70B3D57ED00512F3ULL   // igual
#define PICARO_APP_KEY    { 0x8A, 0xC5, 0x83, 0xDF, 0xEE, 0xC7, 0x6C, 0x81, \
                            0xFF, 0xD1, 0x9C, 0xCF, 0xE7, 0x6B, 0x73, 0xBF }   // 16 bytes, mismo orden
```

Regla de conversión: **de izquierda a derecha, sin invertir nada**. ChirpStack en MSB y RadioLib en MSB
hablan el mismo idioma. (En el ejercicio 05 con LMIC sí había que invertir; aquí no.)

### 4. Selecciona la placa y sus opciones
**Tools ▸ Board ▸ esp32 ▸ "Heltec WiFi LoRa 32(V3)"** sirve para la V3 y también para la V4 si tu core no
lista la V4. Luego, en el mismo menú **Tools**:

| Opción | V3 | V4 |
|---|---|---|
| USB CDC On Boot | Disabled | **Enabled** (sin esto no verás nada en el Monitor Serie) |
| Upload Speed | 921600 | 921600 |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** | **Huge APP** |
| Flash Size | 8MB | 8MB (o 16MB si la placa lo ofrece) |
| Port | el `COMx` / `ttyUSBx` de la placa | el `COMx` / `ttyACMx` de la placa |

> **¿Por qué "Huge APP"?** RadioLib + LoRaWAN + U8g2 no caben en la partición por defecto: sale
> *"text section exceeds available space"*.

### 5. Compila y sube
Botón **Upload (→)**. Si en la V4 el IDE no encuentra el puerto al subir, mantén **PRG/BOOT**, pulsa
**RST**, suelta **BOOT** y vuelve a intentar.

**Salida esperada de la compilación:** `Sketch uses ... bytes (xx%) of program storage space.` sin errores.

### 6. Mira el join en el Monitor Serie
**Tools ▸ Serial Monitor**, **115200 baud**. Pulsa **RST** en la placa. Verás, algo así:

```
##################################################
#  EJERCICIO 12 - JOIN OTAA  (Heltec WiFi LoRa 32) #
##################################################
[info] Sketch v1.0 | placa: Heltec LoRa32 V3 (HELTEC_BOARD_VERSION=3) | RadioLib 7.7.1
[info] Uplink cada 30 s en fPort 1, DR inicial 3, confirmado cada 5, potencia 14 dBm

[1/4] Encendiendo la placa (Vext, OLED, bateria)...
[placa] Heltec LoRa32 V3: Vext=GPIO36 (ON), OLED SDA=17 SCL=18 RST=21 @0x3C
[placa] Radio SX1262: NSS=8 SCK=9 MOSI=10 MISO=11 RST=12 BUSY=13 DIO1=14
[placa] Bateria: ADC=GPIO1, control=GPIO37 (activo en BAJO), divisor x4.9
      Bateria: no detectada (alimentacion por USB)

[2/4] Inicializando radio SX1262 (TCXO 1.8 V, DIO2 = conmutador RF)...
      Radio OK.

[3/4] Configurando OTAA (LoRaWAN 1.0.x: solo AppKey)...

========== CREDENCIALES (deben ser IGUALES en ChirpStack) ==========
  DevEUI  (MSB): A1 B2 C3 D4 E5 F6 07 18
  JoinEUI (MSB): 70 B3 D5 7E D0 05 12 F3
  AppKey  (MSB): 8A C5 83 DF EE C7 6C 81 FF D1 9C CF E7 6B 73 BF
  Region: US915   Sub-banda: 2 (canales 8-15)   LoRaWAN: 1.0.x   Clase: A
====================================================================

[i] Primer arranque: sin contadores de union guardados.
[i] PICARO_FORCE_FRESH_JOIN=1: se hara un JOIN REAL por aire.

[4/4] JOIN OTAA: enviando Join-Request y esperando Join-Accept...
      (RX1 a los 5 s, RX2 a los 6 s; cada intento tarda ~7 s)
      Intento #1: Join-Request (DevNonce nuevo) ...
      JOIN EXITOSO en 6120 ms: llego el Join-Accept.

---------- DATOS DE LA SESION ----------
  DevAddr (lo asigno ChirpStack): 01A3B7C2
  Tipo de sesion : NUEVA (Join-Accept por aire)
  Join-Accept    : RSSI -48 dBm, SNR 9.5 dB
  Datarate uplink: DR3 SF7/125k (ADR activo, la red puede cambiarlo)
  Potencia TX    : 14 dBm en el SX1262
  fCnt uplink    : 0
  Ahora en la consola de ChirpStack: device -> LoRaWAN frames -> JoinRequest / JoinAccept
----------------------------------------
```

Y después, cada 30 s:

```
=============== UPLINK #1 ===============
  counter=1  uptime=12 s  vbat=0 mV (USB, sin bateria)
  Payload (hex, 9 bytes): 48 00 01 00 00 00 0C 00 00
  fPort=1  fCnt que va a usar=0
  TX OK: 904.500 MHz, DR3 SF7/125k, 14 dBm, fCnt=0, nbTrans=1, tiempo en aire 62 ms
  Sin downlink (normal en Clase A si el servidor no tiene nada que decir).
  Siguiente uplink en 30 s.
```

Cada 5 uplinks se pide uno **confirmado**; ahí verás el downlink con ACK:

```
=============== UPLINK #5 (CONFIRMADO) ===============
  ...
  DOWNLINK en RX1: 926.300 MHz, DR13 SF7/500k, RSSI -52 dBm, SNR 8.8 dB, fCnt=0, fPort=0 [ACK]
  Downlink sin datos de aplicacion (solo MAC/ACK).
```

Si el join falla, el sketch te dice **qué revisar** en cada código (llaves, sub-banda, gateway) y reintenta
cada 15 s.

### 7. Mira la OLED
La pantalla resume lo mismo en cuatro líneas bajo el nombre de la placa:

```
Heltec LoRa32 V3            Heltec LoRa32 V3            Heltec LoRa32 V3
────────────────            ────────────────            ────────────────
JOIN OTAA  intento 1        CONECTADO (JOIN OK)         TX #5  fCnt 4
DevEUI ..F60718             DevAddr 01A3B7C2            Bat USB       up 150s
esperando JoinAccept        DR3 SF7/125k                DL RSSI -52 SNR  8.8
US915 sub-banda 2           US915 SB2 JoinAccept        RX1 ACK
```

Si la OLED queda negra: es casi siempre **Vext** (GPIO36 debe estar en BAJO; el sketch lo hace) o una
`HELTEC_BOARD_VERSION` equivocada.

### 8. (Opcional) Escucha los uplinks por MQTT
El servidor publica cada uplink decodificado en el broker del curso. Con las credenciales MQTT de tu
ficha y el UUID de la aplicación (está en la URL de la consola):

```bash
# desde specs/exercises/12_heltec-lora32-v3-join/
export MQTT_USER=equipoNN MQTT_PASS='...' APP_ID='<uuid>' DEV_EUI='a1b2c3d4e5f60718'
./scripts/subscribe.sh
```

**Salida esperada** (una línea por uplink):
```
{"time":"2026-09-15T18:02:11Z","fCnt":7,"fPort":1,"dr":3,"rssi":-47,"snr":9.2,"gw":"2cf7f11153100079",
 "data":"SAAHAAABDg+c","object":{"counter":7,"uptime_s":270,"vbat_mv":3996,"vbat_v":4,"on_battery":true,"battery_pct":77}}
```

## ✅ Cómo saber que funcionó
- [ ] En el Monitor Serie aparece `JOIN EXITOSO ... llego el Join-Accept` y un `DevAddr` distinto de cero.
- [ ] La OLED muestra `CONECTADO (JOIN OK)` y luego `TX #n` con el fCnt subiendo.
- [ ] En ChirpStack, el device tiene *last seen* de hace segundos y en **LoRaWAN frames** se ven
      `JoinRequest`, `JoinAccept` y luego `UnconfirmedDataUp` cada 30 s.
- [ ] En la pestaña **Events** de ChirpStack, cada `up` trae el `object` con `counter`, `uptime_s` y `vbat_mv`.
- [ ] En el uplink #5, el Monitor Serie muestra `DOWNLINK en RX1 ... [ACK]`.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| `RADIO NO ENCONTRADO` al arrancar | Placa que no es Heltec LoRa 32, o `HELTEC_BOARD_VERSION` mal | Revisa la placa y la constante en `config_lorawan.h` |
| Se queda en `JOIN OTAA intento n` con `NO LLEGO EL JOIN-ACCEPT` | Gateway offline, o sub-banda distinta, o llaves distintas a las de ChirpStack | Consola → Gateways *Online*. Compara DevEUI/JoinEUI/AppKey del Monitor Serie con la consola, byte a byte |
| `MIC INCORRECTO` | AppKey copiada mal (un byte cambiado o en LSB) | Vuelve a copiarla en MSB. Si generaste otra en ChirpStack, actualiza la placa |
| ChirpStack muestra el JoinRequest pero la placa no recibe el Accept | El gateway no transmitió a tiempo (RX1 = 5 s) o la placa está pegada al gateway y satura | Aléjala 2-3 m del gateway; en V4 no subas la potencia |
| Consola: *"DevNonce already used"* | Borraste y recreaste el device; los DevNonce de la placa ya se usaron | `PICARO_RESET_NONCES 1`, flashea una vez, regresa a 0, flashea de nuevo |
| El Monitor Serie no muestra nada (V4) | **USB CDC On Boot** en *Disabled* | Ponlo en *Enabled* y vuelve a subir |
| El Monitor Serie no muestra nada (V3) | Baudios distintos de 115200, o driver CP210x sin instalar | Ajusta baudios / instala el driver de Silicon Labs |
| OLED negra | Vext no encendido o versión de placa incorrecta | Revisa `HELTEC_BOARD_VERSION`; el sketch pone GPIO36 en BAJO |
| `text section exceeds available space` al compilar | Partición por defecto | **Partition Scheme = Huge APP** |
| Voltaje de batería absurdo (V4) | Polaridad del control del ADC (GPIO37) invertida | Confirma `HELTEC_BOARD_VERSION 4` |
| Uplinks OK pero sin `object` en ChirpStack | El codec no está en el device profile o el fPort no es 1 | Pega `payload_decoder.js` en la pestaña Codec del profile |
| `SIN CANAL DISPONIBLE` | ADR dejó un datarate donde el payload no cabe, o dwell time | Espera; con ADR la red corrige. Si persiste, baja `PICARO_UPLINK_DR` a 2 |

## 📤 Los datos (payload)
- **fPort:** 1
- **Formato (9 bytes, big-endian):**

| Byte(s) | Campo | Tipo | Significado |
|---|---|---|---|
| 0 | `magic` | uint8 | `0x48` ('H'). El codec rechaza paquetes que no lo traigan |
| 1..2 | `counter` | uint16 | número de paquete desde el arranque |
| 3..6 | `uptime_s` | uint32 | segundos encendida |
| 7..8 | `vbat_mv` | uint16 | voltaje de batería en mV; `0` = sin batería (USB) |

- **Decodificación:** [`payload_decoder.js`](payload_decoder.js). Ejemplo: `48 0007 0000010e 0f9c` →
  `{ counter: 7, uptime_s: 270, vbat_mv: 3996, vbat_v: 4.0, on_battery: true, battery_pct: 77 }`.

## 📖 Nota didáctica — por qué "generar en ChirpStack" y no "inventar en la placa"
En el ejercicio 08 la placa traía credenciales de fábrica y tú las dabas de alta en ChirpStack. Aquí es al
revés, y es el flujo real de producción: el **servidor** es el dueño de las identidades. Un DevEUI generado
al azar no colisiona con otro equipo, la AppKey nunca viaja por correo ni por chat (solo del servidor a tu
`config_lorawan.h`), y el JoinEUI generado te obliga a comprobar que **los tres** valores coincidan: si
uno solo difiere, el join falla y el Monitor Serie te dice cuál sospechar (`MIC INCORRECTO` = AppKey;
`NO LLEGO EL JOIN-ACCEPT` = DevEUI/JoinEUI desconocidos, gateway o sub-banda).

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 11 · Dashboard contra servidor remoto](../11_dashboard-servidor-remoto/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json) (molde; rellénalo con lo que generó ChirpStack).
- **Archivos de esta carpeta:**
  - `sketches/heltec_lora32_join/heltec_lora32_join.ino` — la lógica (join, uplinks, Serie, OLED).
  - `sketches/heltec_lora32_join/config_lorawan.h` — **lo único que editas**: versión de placa y llaves.
  - `sketches/heltec_lora32_join/board_heltec.h` — pines de V3 y V4.
  - `payload_decoder.js` — codec para el device profile.
  - `scripts/subscribe.sh` — escucha MQTT del servidor remoto.
- **Guías comunes:** [ChirpStack API](../COMMON_CHIRPSTACK_API.md) · [Ejercicio 11 (MQTT con TLS)](../11_dashboard-servidor-remoto/)

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
> 🧩 **Código:** basado en los ejemplos LoRaWAN de RadioLib, bajo **licencia MIT**.
