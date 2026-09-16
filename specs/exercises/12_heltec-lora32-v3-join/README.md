# Ejercicio 12 — Join OTAA con Heltec WiFi LoRa 32 (V3 / V4) y ChirpStack, desde cero

> **En una frase:** haces que una **Heltec WiFi LoRa 32** se una por **OTAA** al servidor LoRaWAN del
> curso (ChirpStack en `lns.pi-caro.org`) con un DevEUI, JoinEUI y AppKey que **genera ChirpStack**, y
> ves el join y cada paquete en el **Monitor Serie** (con todo el detalle) y en la **OLED** de la placa.
> **Plataforma:** ESP32-S3 + radio **SX1262** + OLED, firmware **Arduino IDE / RadioLib**.
> **Banda:** US915 (sub-banda 2). **Activación:** OTAA · **Clase A** · **LoRaWAN 1.0.x**.
> **Compatible con la V3 y la V4** de la placa.

Este ejercicio es **autocontenido**: no necesitas haber hecho ningún otro. Solo asume dos cosas:

1. Tienes **acceso al LNS del curso** (`https://lns.pi-caro.org`) con el usuario y contraseña de tu equipo.
2. Tienes **Arduino IDE 2.x instalado** (si no: <https://www.arduino.cc/en/software>, instalación por defecto).

Todo lo demás (obtener los archivos, configurar Arduino IDE para la placa, librerías, ChirpStack,
credenciales, compilar, flashear y verificar) está aquí, paso a paso.

---

## 🎯 Qué vas a conseguir

Una Heltec que arranca, muestra en la OLED `JOIN OTAA intento 1`, recibe el Join-Accept, pasa a
`CONECTADO (JOIN OK)` con su DevAddr, y desde entonces envía cada 30 s un paquete de 9 bytes con
contador, tiempo encendida y voltaje de batería. En el Monitor Serie verás cada Join-Request, cada
uplink con frecuencia, datarate, potencia y fCnt, y cada downlink con RSSI/SNR. En ChirpStack el device
aparece con *last seen* reciente y el payload decodificado, algo así:

```json
{ "deviceInfo": { "devEui": "a1b2c3d4e5f60718" },
  "fPort": 1,
  "data": "SAAHAAABDg+c",
  "object": { "counter": 7, "uptime_s": 270, "vbat_mv": 3996, "vbat_v": 4.0,
              "on_battery": true, "battery_pct": 77 } }
```

> ⚠️ **Sobre las "salidas esperadas" de esta guía:** están escritas a partir del código y de la
> documentación de RadioLib, **no de una captura real**. Los textos que imprime el sketch son exactos;
> los números (RSSI, tiempos, DevAddr) son "algo así".

---

## 🧰 Lo que necesitas tener a mano

- [ ] **Placa Heltec WiFi LoRa 32 V3 o V4** con su **antena de 915 MHz enroscada**. ⚠️ **Nunca la enciendas sin antena:** puedes dañar la radio.
- [ ] **Cable USB de datos** (muchos cables de carga no llevan datos: si el PC no detecta nada, prueba otro cable).
- [ ] **PC** con Windows, macOS o Linux, **Arduino IDE 2.x** instalado y conexión a Internet.
- [ ] **Usuario y contraseña del LNS** (`equipoNN@pi-caro.org`, en la ficha de tu equipo).
- [ ] *(Opcional)* batería LiPo con conector de 2 pines para ver el voltaje real. Sin batería el sketch reporta `0` = "USB".
- [ ] *(Opcional, Paso 11)* `git` para clonar el repo; si no lo tienes, hay opción de descarga ZIP.

### ¿V3 o V4? Identifica tu placa antes de empezar

| Mira esto | V3 | V4 |
|---|---|---|
| Serigrafía en la placa | `WiFi LoRa 32(V3)` | `WiFi LoRa 32(V4)` o `V4.2` / `V4.3` |
| Conectores extra | solo batería | batería, **panel solar** y **GNSS** (dos conectores pequeños más) |
| Chip USB | un chip aparte (CP2102) junto al USB-C | ninguno: el USB entra directo al ESP32-S3 |
| En el Administrador de dispositivos (Windows) | *Silicon Labs CP210x USB to UART Bridge (COMx)* | *USB Serial Device (COMx)* o *USB JTAG/serial debug unit* |

Apunta cuál es: la necesitarás en los Pasos 2, 4 y 6.

---

## 🪜 Paso a paso

### Paso 1. Obtén los archivos del ejercicio

**Opción A — con git (recomendada).** Abre una terminal (Windows: *PowerShell*; macOS/Linux: *Terminal*):

```bash
# Windows (PowerShell)
mkdir C:\dev -Force
cd C:\dev
git clone https://github.com/ovelazquezj/radiosonda_PIcaro.git

# macOS / Linux
mkdir -p ~/dev && cd ~/dev
git clone https://github.com/ovelazquezj/radiosonda_PIcaro.git
```

**Salida esperada:** termina con `Resolving deltas: 100% ..., done.` y aparece la carpeta
`radiosonda_PIcaro`.

**Opción B — sin git.** Entra a <https://github.com/ovelazquezj/radiosonda_PIcaro>, botón verde
**Code ▸ Download ZIP**, y descomprime el ZIP en `C:\dev\` (Windows) o `~/dev/`. La carpeta se llamará
`radiosonda_PIcaro-master`; renómbrala a `radiosonda_PIcaro` para que las rutas de esta guía coincidan.

**Dónde está el ejercicio.** Dentro del repo, la carpeta es:

```
radiosonda_PIcaro/
└── specs/
    └── exercises/
        └── 12_heltec-lora32-v3-join/          <-- este ejercicio
            ├── README.md                      <-- esta guía
            ├── credentials.json               <-- molde para anotar tus credenciales
            ├── payload_decoder.js             <-- codec que pegarás en ChirpStack (Paso 5)
            ├── scripts/subscribe.sh           <-- opcional: escuchar por MQTT (Paso 11)
            └── sketches/
                └── heltec_lora32_join/        <-- carpeta del sketch de Arduino
                    ├── heltec_lora32_join.ino <-- el programa (NO se edita)
                    ├── config_lorawan.h       <-- LO ÚNICO QUE EDITAS (Paso 6)
                    └── board_heltec.h         <-- pines de la V3 y la V4 (NO se edita)
```

> 🔑 Arduino IDE exige que el archivo `.ino` esté dentro de una carpeta **con el mismo nombre**
> (`heltec_lora32_join/heltec_lora32_join.ino`). Ya viene así: **no muevas ni renombres** el `.ino`
> solo. Si quieres una copia para trabajar, copia la **carpeta completa** `heltec_lora32_join`.

### Paso 2. Configura Arduino IDE para la placa (core ESP32)

Arduino IDE no conoce el ESP32-S3 de fábrica: hay que añadir el "core" de Espressif.

**2.1 URL del gestor de placas.** En Arduino IDE: **File ▸ Preferences** (Windows/Linux) o
**Arduino IDE ▸ Settings** (macOS). En el campo **Additional boards manager URLs** pega:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Si ya había otra URL, sepáralas con una coma. **OK**.

**2.2 Instala el core.** **Tools ▸ Board ▸ Boards Manager…** (o el icono de placa en la barra
izquierda). Busca `esp32`, localiza **esp32 by Espressif Systems**, elige la versión **3.x** más reciente
e **Install**. Tarda varios minutos (descarga ~300 MB).

**Salida esperada:** en la consola de abajo, `Platform esp32:esp32@3.x.x installed`.

**2.3 Driver USB (solo V3, solo Windows).** La V3 usa el chip CP2102. Windows 10/11 suele instalar el
driver solo al conectar la placa. Si en **Administrador de dispositivos** aparece con un signo de
admiración, instala el driver *CP210x Universal Windows Driver* de Silicon Labs:
<https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers>. La **V4** no necesita driver
(USB nativo). En macOS/Linux no hace falta nada en ninguna de las dos.

**2.4 Conecta la placa y localiza el puerto.** Conecta la Heltec por USB. En Arduino IDE,
**Tools ▸ Port** debe mostrar un puerto nuevo:

| Sistema | V3 | V4 |
|---|---|---|
| Windows | `COMx` (Silicon Labs CP210x) | `COMx` (USB Serial Device) |
| macOS | `/dev/cu.usbserial-XXXX` o `/dev/cu.SLAB_USBtoUART` | `/dev/cu.usbmodemXXXX` |
| Linux | `/dev/ttyUSB0` | `/dev/ttyACM0` |

Si no aparece ninguno: cambia de cable USB (que sea de datos), prueba otro puerto del PC y, en la V4,
mantén pulsado **PRG** mientras conectas el cable.

> Linux: si el IDE dice *Permission denied* al subir, añade tu usuario al grupo del puerto:
> `sudo usermod -aG dialout $USER` y vuelve a iniciar sesión.

### Paso 3. Instala las dos librerías

**Tools ▸ Manage Libraries…** (o el icono de libros 📚). Busca e instala:

| Busca | Instala | Versión | Para qué |
|---|---|---|---|
| `RadioLib` | **RadioLib** by Jan Gromes | **7.x** (la más reciente) | Radio SX1262 y pila LoRaWAN |
| `U8g2` | **U8g2** by oliver | la más reciente | Pantalla OLED |

Si el IDE pregunta *"Install dependencies?"*, acepta **Install all**.

**Salida esperada:** en la consola, `Installed RadioLib@7.x.x` e `Installed U8g2@2.x.x`.

### Paso 4. Abre el sketch y selecciona la placa con sus opciones

**4.1 Abre el sketch.** **File ▸ Open…** y navega hasta
`C:\dev\radiosonda_PIcaro\specs\exercises\12_heltec-lora32-v3-join\sketches\heltec_lora32_join\heltec_lora32_join.ino`
(o la ruta equivalente en `~/dev/`). Arduino abrirá **tres pestañas**: `heltec_lora32_join.ino`,
`board_heltec.h` y `config_lorawan.h`.

**4.2 Selecciona la placa.** **Tools ▸ Board ▸ esp32 ▸ Heltec WiFi LoRa 32(V3)**. Esta entrada sirve
**para las dos versiones** (V3 y V4): el chip es el mismo ESP32-S3 y las diferencias las resuelve el
código con la constante del Paso 6. Si tu core lista *Heltec WiFi LoRa 32(V4)*, puedes usarla para la
V4; las opciones de abajo son las mismas.

**4.3 Ajusta las opciones.** Al elegir la placa, el menú **Tools** muestra más opciones. Déjalas
**exactamente** así:

| Opción (menú Tools) | V3 | V4 |
|---|---|---|
| **USB CDC On Boot** | `Disabled` | **`Enabled`** ← sin esto la V4 no muestra nada en el Monitor Serie |
| **Upload Speed** | `921600` | `921600` |
| **Partition Scheme** | **`Huge APP (3MB No OTA/1MB SPIFFS)`** | **`Huge APP (3MB No OTA/1MB SPIFFS)`** |
| **Flash Size** | `8MB (64Mb)` | `8MB (64Mb)` (funciona aunque la V4 tenga 16 MB) |
| **Erase All Flash Before Sketch Upload** | `Disabled` | `Disabled` |
| **Port** | el puerto del Paso 2.4 | el puerto del Paso 2.4 |

> **¿Por qué "Huge APP"?** RadioLib + LoRaWAN + U8g2 no caben en la partición por defecto; sin esto la
> compilación falla con *"text section exceeds available space"*.

### Paso 5. Genera las credenciales en ChirpStack (consola web)

Vas a crear **un device profile** (una vez por equipo) y **un device** (uno por placa). ChirpStack es el
que **genera** el DevEUI, el JoinEUI y la AppKey; tú solo los copias.

**5.1 Entra.** <https://lns.pi-caro.org> → usuario `equipoNN@pi-caro.org` y contraseña de tu ficha.
Al entrar verás el menú de la izquierda con **Dashboard, Device profiles, Gateways, Applications…**
Estás dentro de tu **tenant** (tu equipo); no puedes ver los de los demás.

**5.2 Comprueba que hay un gateway online.** Menú **Gateways**: al menos uno debe mostrar
**Online** (verde) y *last seen* de hace segundos. Los gateways son compartidos por todo el curso y
están en **US915 sub-banda 2**. ⚠️ Si no hay ninguno online, avisa al instructor: sin gateway el join
no puede ocurrir.

**5.3 Crea el device profile.** Menú **Device profiles ▸ Add device profile** y rellena:

| Pestaña **General** | Valor |
|---|---|
| Name | `Heltec-Join-US915` |
| Region | `US915` |
| MAC version | `LoRaWAN 1.0.3` |
| Regional parameters revision | `A` |
| ADR algorithm | `Default ADR algorithm (LoRa only)` |
| Flush queue on activate | ✅ |
| Expected uplink interval (secs) | `60` |
| Device-status request frequency (req/day) | `24` |

| Pestaña **Join (OTAA / ABP)** | Valor |
|---|---|
| Device supports OTAA | ✅ |

| Pestaña **Class B** / **Class C** | Valor |
|---|---|
| Device supports Class-B / Class-C | ❌ (déjalas sin marcar) |

| Pestaña **Codec** | Valor |
|---|---|
| Payload codec | `JavaScript functions` |
| Codec functions | abre `payload_decoder.js` de la carpeta del ejercicio con el Bloc de notas, **copia todo** y pégalo aquí, sustituyendo el texto de ejemplo |

Pulsa **Submit**. **Salida esperada:** vuelves a la lista y aparece `Heltec-Join-US915`.

> Si tu usuario no tiene el botón *Add device profile*, pide al instructor que lo cree en tu tenant.

**5.4 Crea (o elige) la aplicación.** Menú **Applications**. Si ya existe `radiosonda`, entra en ella.
Si prefieres una propia: **Add application** → Name `heltec-join` → **Submit**.

**5.5 Crea el device.** Dentro de la aplicación, pestaña **Devices ▸ Add device**:

| Campo | Qué hacer |
|---|---|
| Name | `heltec-<tu nombre>` (sin espacios) |
| Description | lo que quieras |
| **Device EUI (EUI64)** | pulsa el icono **🔄 (generar)** que está a la derecha del campo. Aparecen 16 caracteres hexadecimales |
| **Join EUI (EUI64)** | pulsa también su icono **🔄 (generar)** |
| Device profile | `Heltec-Join-US915` |
| Device is disabled | ❌ |

**Antes de pulsar Submit**, comprueba que el botón **MSB / LSB** de cada campo EUI está en **MSB** y
**copia los dos valores** a un bloc de notas (o a `credentials.json`). Luego **Submit**.

**5.6 Genera la AppKey.** ChirpStack te lleva al device recién creado; entra en la pestaña
**OTAA keys**. En **Application key** pulsa el icono **🔄 (generar)**: aparecen 32 caracteres
hexadecimales. **Cópiala** al bloc de notas y pulsa **Submit**.

Ahora tienes tus tres valores. Ejemplo (los tuyos serán distintos):

```
DevEUI  a1b2c3d4e5f60718
JoinEUI 70b3d57ed00512f3
AppKey  8ac583dfeec76c81ffd19ccfe76b73bf
```

> 🔐 La **AppKey es secreta**: no la pegues en chats ni la subas a ningún repositorio.

### Paso 6. Copia la versión de la placa y las credenciales a `config_lorawan.h`

En Arduino IDE, pestaña **`config_lorawan.h`**. Edita **solo** estas cuatro líneas:

```c
#define HELTEC_BOARD_VERSION   3        // 3 = V3, 4 = V4  (lo apuntaste al principio)

#define PICARO_DEV_EUI    0xA1B2C3D4E5F60718ULL   // tu DevEUI
#define PICARO_JOIN_EUI   0x70B3D57ED00512F3ULL   // tu JoinEUI
#define PICARO_APP_KEY    { 0x8A, 0xC5, 0x83, 0xDF, 0xEE, 0xC7, 0x6C, 0x81, \
                            0xFF, 0xD1, 0x9C, 0xCF, 0xE7, 0x6B, 0x73, 0xBF }   // tu AppKey
```

**Regla de conversión, de izquierda a derecha y sin invertir nada:**

| ChirpStack muestra (MSB) | En `config_lorawan.h` se escribe |
|---|---|
| `a1b2c3d4e5f60718` (DevEUI) | `0xA1B2C3D4E5F60718ULL` → `0x` + los 16 caracteres + `ULL` |
| `70b3d57ed00512f3` (JoinEUI) | `0x70B3D57ED00512F3ULL` |
| `8ac583dfeec76c81ffd19ccfe76b73bf` (AppKey) | `{ 0x8A, 0xC5, 0x83, ... , 0xBF }` → cada **par** de caracteres con `0x` delante, separados por comas, 16 en total |

Mayúsculas o minúsculas dan igual. Guarda con **Ctrl+S**. Anota también los valores en
`credentials.json` para no perderlos.

### Paso 7. Compila y sube

1. Pulsa **Verify (✓)** para compilar sin subir. La primera vez tarda 2-4 minutos.
   **Salida esperada:** `Sketch uses ... bytes (xx%) of program storage space.` sin líneas en rojo.
2. Pulsa **Upload (→)**. Verás `Connecting....`, luego `Writing at 0x... (100 %)` y al final
   `Hard resetting via RTS pin...` (V3) o `Leaving...` (V4).

Si en la **V4** el IDE se queda en `Connecting......_____....` y falla: mantén pulsado **PRG**, pulsa y
suelta **RST**, suelta **PRG**, y vuelve a pulsar **Upload**. Después de subir, pulsa **RST** una vez.

### Paso 8. Mira el join en el Monitor Serie

**Tools ▸ Serial Monitor** (o el icono de lupa arriba a la derecha). En la barra del monitor selecciona
**115200 baud**. Pulsa **RST** en la placa. Verás, algo así:

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

**Comprueba** que las tres líneas del bloque `CREDENCIALES` coinciden **carácter a carácter** con lo
que muestra ChirpStack. Si una difiere, el join fallará: corrige `config_lorawan.h` y vuelve a subir.

Después, cada 30 s:

```
=============== UPLINK #1 ===============
  counter=1  uptime=12 s  vbat=0 mV (USB, sin bateria)
  Payload (hex, 9 bytes): 48 00 01 00 00 00 0C 00 00
  fPort=1  fCnt que va a usar=0
  TX OK: 904.500 MHz, DR3 SF7/125k, 14 dBm, fCnt=0, nbTrans=1, tiempo en aire 62 ms
  Sin downlink (normal en Clase A si el servidor no tiene nada que decir).
  Siguiente uplink en 30 s.
```

Cada 5 uplinks se pide uno **confirmado** y verás el downlink con ACK:

```
=============== UPLINK #5 (CONFIRMADO) ===============
  ...
  DOWNLINK en RX1: 926.300 MHz, DR13 SF7/500k, RSSI -52 dBm, SNR 8.8 dB, fCnt=0, fPort=0 [ACK]
  Downlink sin datos de aplicacion (solo MAC/ACK).
```

Si el join falla, el sketch imprime **qué revisar** según el código de error y reintenta cada 15 s.

### Paso 9. Mira la OLED

La pantalla resume lo mismo, en cuatro líneas bajo el nombre de la placa:

```
Heltec LoRa32 V3            Heltec LoRa32 V3            Heltec LoRa32 V3
────────────────            ────────────────            ────────────────
JOIN OTAA  intento 1        CONECTADO (JOIN OK)         TX #5  fCnt 4
DevEUI ..F60718             DevAddr 01A3B7C2            Bat USB       up 150s
esperando JoinAccept        DR3 SF7/125k                DL RSSI -52 SNR  8.8
US915 sub-banda 2           US915 SB2 JoinAccept        RX1 ACK
```

### Paso 10. Verifica en ChirpStack

En la consola, **Applications ▸ tu aplicación ▸ tu device**:

- Arriba, **Last seen** debe decir *a few seconds ago*.
- Pestaña **LoRaWAN frames**: verás `JoinRequest` seguido de `JoinAccept` y después
  `UnconfirmedDataUp` cada 30 s (y `ConfirmedDataUp` + `UnconfirmedDataDown` cada 5).
- Pestaña **Events**: cada evento `up` trae `object` con `counter`, `uptime_s`, `vbat_mv`… Si `object`
  no aparece, el codec del Paso 5.3 no se guardó.
- Pestaña **Device metrics**: RSSI y SNR con los que el gateway oye a tu placa.

### Paso 11. (Opcional) Escucha los uplinks por MQTT

El servidor publica cada uplink decodificado en un broker MQTT con TLS. Necesitas `mosquitto_sub`
(paquete `mosquitto-clients` en Linux/WSL, `brew install mosquitto` en macOS), el usuario y contraseña
**MQTT** de tu ficha (son distintos de los de la web) y el UUID de la aplicación (está en la URL de la
consola cuando entras en ella: `.../applications/<uuid>/...`).

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

---

## ✅ Cómo saber que funcionó

- [ ] El Monitor Serie muestra `JOIN EXITOSO ... llego el Join-Accept` y un `DevAddr` distinto de cero.
- [ ] La OLED muestra `CONECTADO (JOIN OK)` y luego `TX #n` con el fCnt subiendo.
- [ ] En ChirpStack, el device tiene *last seen* de hace segundos y en **LoRaWAN frames** se ven
      `JoinRequest`, `JoinAccept` y `UnconfirmedDataUp` cada 30 s.
- [ ] En **Events**, cada `up` trae el `object` con `counter`, `uptime_s` y `vbat_mv`.
- [ ] En el uplink #5, el Monitor Serie muestra `DOWNLINK en RX1 ... [ACK]`.

## 🛠️ Si algo falla

| Síntoma | Causa probable | Arreglo |
|---|---|---|
| *Tools ▸ Board* no lista `esp32` | El core no se instaló (Paso 2.2) o la URL del 2.1 está mal | Repite 2.1 y 2.2; la URL debe terminar en `package_esp32_index.json` |
| No aparece ningún puerto | Cable solo de carga, driver CP210x (V3) o puerto USB | Cambia el cable; instala el driver (2.3); en V4 conecta con PRG pulsado |
| `text section exceeds available space` | Partition Scheme por defecto | **Partition Scheme = Huge APP** (Paso 4.3) |
| `RadioLib.h: No such file` / `U8g2lib.h: No such file` | Faltan librerías | Paso 3 |
| `Connecting......_____` y falla al subir (V4) | La placa no entró en modo de carga | PRG pulsado + RST + soltar PRG, y Upload de nuevo |
| El Monitor Serie no muestra nada (V4) | **USB CDC On Boot** en `Disabled` | Ponlo en `Enabled` y vuelve a subir |
| El Monitor Serie muestra símbolos raros | Baudios distintos de 115200 | Selecciona 115200 en el monitor |
| `RADIO NO ENCONTRADO` al arrancar | `HELTEC_BOARD_VERSION` mal o la placa no es una Heltec LoRa 32 | Revisa la constante y la placa |
| Se queda en `JOIN OTAA intento n` con `NO LLEGO EL JOIN-ACCEPT` | Gateway offline, o llaves que no coinciden con ChirpStack | Paso 5.2; compara el bloque `CREDENCIALES` con la consola byte a byte |
| `MIC INCORRECTO` | AppKey copiada mal (un byte cambiado o en LSB) | Vuelve a copiarla en MSB; si generaste otra en ChirpStack, actualiza la placa |
| ChirpStack muestra el `JoinRequest` pero la placa no recibe el Accept | Placa pegada al gateway (satura) o gateway lento | Aléjala 2-3 m; en V4 no subas la potencia |
| Consola: *"DevNonce already used"* | Borraste y recreaste el device; los DevNonce de la placa ya se usaron | En `config_lorawan.h` pon `PICARO_RESET_NONCES 1`, sube, regresa a `0`, sube de nuevo |
| OLED negra | Vext no encendido o versión de placa incorrecta | Revisa `HELTEC_BOARD_VERSION`; el sketch pone GPIO36 en BAJO |
| Voltaje de batería absurdo (V4) | Polaridad del control del ADC invertida | Confirma `HELTEC_BOARD_VERSION 4` |
| Uplinks OK pero sin `object` en ChirpStack | El codec no está en el device profile o el fPort no es 1 | Paso 5.3, pestaña Codec |
| `SIN CANAL DISPONIBLE` | ADR dejó un datarate donde el payload no cabe | Espera; con ADR la red corrige. Si persiste, baja `PICARO_UPLINK_DR` a 2 |

---

## 📟 Hardware: pines y diferencias V3 / V4

Placa integrada, no hay que cablear nada. Los pines viven en `board_heltec.h` y son los mismos en V3 y
V4 para la radio y la pantalla:

| Señal SX1262 | GPIO | | Señal OLED / otros | GPIO |
|---|---|---|---|---|
| NSS (CS) | 8 | | OLED SDA | 17 |
| SCK | 9 | | OLED SCL | 18 |
| MOSI | 10 | | OLED RST | 21 |
| MISO | 11 | | **Vext** (alimenta la OLED, activo en BAJO) | 36 |
| RST | 12 | | ADC batería | 1 |
| BUSY | 13 | | Control del divisor de batería | 37 |
| DIO1 (IRQ) | 14 | | LED (solo V3) | 35 |

Lo que **sí cambia**, y el código resuelve solo con `HELTEC_BOARD_VERSION`:

| | V3 | V4 (4.2 y 4.3) |
|---|---|---|
| USB | chip CP2102 | USB nativo del ESP32-S3 |
| Flash / PSRAM | 8 MB / no | 16 MB / 2 MB |
| Control del ADC de batería (GPIO37) | mide con **BAJO** | mide con **ALTO** |
| Amplificador de RF | no | sí: GC1109 (4.2) o KCT8103L (4.3), pines 7/2/46/5 |
| Potencia por defecto del SX1262 | 14 dBm | **2 dBm** (el amplificador suma ~11 dB) |

> 🔑 En la V4 **no subas la potencia a ciegas**: 22 dBm al amplificador salen como ~28 dBm y saturan
> el gateway si está a pocos metros. El valor por defecto deja unos 13 dBm en la antena.

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

Aquí el **servidor** es el dueño de las identidades, que es el flujo real de producción. Un DevEUI
generado al azar no colisiona con otro equipo, la AppKey nunca viaja por correo ni por chat (solo de
la consola a tu `config_lorawan.h`), y el JoinEUI generado te obliga a comprobar que **los tres**
valores coincidan: si uno solo difiere, el join falla y el Monitor Serie te dice cuál sospechar
(`MIC INCORRECTO` = AppKey; `NO LLEGO EL JOIN-ACCEPT` = DevEUI/JoinEUI desconocidos, gateway o
sub-banda).

## 📎 Referencia

- **Credenciales:** [`credentials.json`](credentials.json), molde para anotar lo que generó ChirpStack.
- **Archivos de esta carpeta:**
  - `sketches/heltec_lora32_join/heltec_lora32_join.ino` — la lógica (join, uplinks, Serie, OLED).
  - `sketches/heltec_lora32_join/config_lorawan.h` — **lo único que editas**: versión de placa y llaves.
  - `sketches/heltec_lora32_join/board_heltec.h` — pines de V3 y V4.
  - `payload_decoder.js` — codec para el device profile.
  - `scripts/subscribe.sh` — escucha MQTT del servidor del curso.
- **Para saber más (no hace falta para este ejercicio):** [Wiki del proyecto](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki) · [Índice de ejercicios](../README.md) · [Documentación de RadioLib](https://jgromes.github.io/RadioLib/)

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
> 🧩 **Código:** basado en los ejemplos LoRaWAN de RadioLib, bajo **licencia MIT**.
