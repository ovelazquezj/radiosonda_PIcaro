# Ejercicio 13 — Estación meteorológica LoRaWAN con Heltec CubeCell (HTCC-AB01), desde cero

> **En una frase:** conviertes una **Heltec CubeCell HTCC-AB01** en un nodo LoRaWAN de **bajo consumo**
> que hace **join OTAA** al ChirpStack del curso (`lns.pi-caro.org`) y envía cada minuto un paquete con
> batería, temperatura, humedad, presión, luz y lluvia. Se construye **por etapas**: hoy el join, y
> después cada sensor se **cablea y activa con una constante**.
> **Plataforma:** ASR6501 (Cortex-M0+) + radio **SX1262**, firmware **Arduino IDE + CubeCell Development
> Framework** de Heltec (trae su propia pila LoRaWAN). **Banda:** US915 (sub-banda 2). **Activación:** OTAA ·
> **Clase A** · **LoRaWAN 1.0.3**.

Este ejercicio es **autocontenido**: no necesitas haber hecho ningún otro. Solo asume dos cosas:

1. Tienes **acceso al LNS del curso** (`https://lns.pi-caro.org`) con un usuario que ve el device.
2. Tienes **Arduino IDE 2.x instalado** (si no: <https://www.arduino.cc/en/software>, instalación por defecto).

> ✅ **Verificado en hardware el 2026-09-17** con la placa del curso: join a la primera, DevAddr
> asignado, uplinks cada 60 s, ADR bajando la potencia de 20 a 10 dBm. **Todas las salidas de esta
> guía son capturas reales** del Monitor Serie y de los registros del servidor.

---

## 🎯 Qué vas a conseguir

**Hoy (etapa 0):** la CubeCell arranca, hace join, y cada 60 s envía 11 bytes con el voltaje de
batería; los campos de los sensores viajan como "no disponible". En el Monitor Serie ves el join,
cada uplink con su frecuencia, datarate y potencia, y los downlinks con RSSI/SNR. En ChirpStack, el
device muestra *last seen* reciente y el payload decodificado:

```json
{ "deviceInfo": { "devEui": "623851f518c92135" },
  "fPort": 2,
  "data": "AL5//3//////////",
  "object": { "fport": 2,
              "sensores": { "dht11": false, "bmp280": false, "bh1750": false, "mhrd": false },
              "vbat_mv": 3800, "vbat_v": 3.8, "battery_pct": 55 } }
```

**Después (etapas 1 a 5):** cableas el DHT11, el BMP280, el BH1750 y el módulo de lluvia MH-RD, uno a
uno, y el mismo `object` se va llenando con `temp_dht_c`, `humedad_pct`, `presion_hpa`, `lux`,
`lluvia_pct`… Por último le pones **batería, módulo de carga USB-C y celda solar** para dejarla
funcionando sola.

---

## 🧰 Lo que necesitas tener a mano

- [ ] **Heltec CubeCell HTCC-AB01** (V1.2) con su **antena de 915 MHz enroscada**. ⚠️ **Nunca la enciendas sin antena.**
- [ ] **Cable USB de datos** (micro-USB). Si el PC no detecta nada, prueba otro cable.
- [ ] **PC** con Windows, macOS o Linux, **Arduino IDE 2.x** y conexión a Internet.
- [ ] **Usuario y contraseña del LNS** con acceso al tenant donde vive el device.
- [ ] *(Etapa 0, opcional)* batería LiPo de 3.7 V con conector de 2 pines (la placa la carga por USB).
- [ ] *(Etapas 1–4)* DHT11, BMP280, BH1750, MH-RD, cables Dupont hembra-hembra y una protoboard pequeña.
- [ ] *(Etapa 5)* batería LiPo 3.7 V (1000–2000 mAh, con conector SH1.25-2 o para soldar), **módulo de carga USB-C TP4056** (el de 6 pines: IN+/IN−, B+/B−, OUT+/OUT−, con protección DW01), **celda solar de 6 V** (1–2 W), cable con conector SH1.25-2, multímetro.
- [ ] *(Paso 10, opcional)* `git` para clonar; si no lo tienes, hay opción de descarga ZIP.

### Conoce la placa

| Elemento | Dónde está / qué hace |
|---|---|
| Chip USB | **CP2102** (puente USB-serie). En Windows aparece como *Silicon Labs CP210x USB to UART Bridge (COMx)* |
| Botón **RST** | reinicia la placa |
| Botón **USER** | pulsador de usuario (no se usa en este ejercicio) |
| LED **RGB** | la pila LoRaWAN lo usa: **rojo** = transmitiendo, **azul** = ventana RX1, **amarillo** = RX2, **morado** = join conseguido, **verde** = downlink recibido |
| Conector **batería** (2 pines) | LiPo 3.7 V; se carga desde el USB |
| Conector **solar** (2 pines) | panel de 5–7 V (opcional) |
| Pin **Vext** | 3.3 V **conmutados** para alimentar sensores: el firmware los enciende solo para medir |
| Pines **SDA / SCL** | bus I2C (BMP280 y BH1750) |
| Pin **ADC** | entrada analógica (salida AO del MH-RD) |
| Pines **GPIO1 … GPIO5** | entradas/salidas digitales (DHT11 en GPIO5, DO del MH-RD en GPIO1) |

---

## 🪜 Paso a paso — Etapa 0: el join

### Paso 1. Obtén los archivos del ejercicio

**Opción A — con git (recomendada).** En una terminal (Windows: *PowerShell*; macOS/Linux: *Terminal*):

```bash
# Windows (PowerShell)
mkdir C:\dev -Force
cd C:\dev
git clone https://github.com/ovelazquezj/radiosonda_PIcaro.git

# macOS / Linux
mkdir -p ~/dev && cd ~/dev
git clone https://github.com/ovelazquezj/radiosonda_PIcaro.git
```

**Salida esperada:** termina con `Resolving deltas: 100% ..., done.` y aparece la carpeta `radiosonda_PIcaro`.

**Opción B — sin git.** <https://github.com/ovelazquezj/radiosonda_PIcaro> → botón verde **Code ▸ Download
ZIP** → descomprime en `C:\dev\` (o `~/dev/`) y renombra `radiosonda_PIcaro-master` a `radiosonda_PIcaro`.

**Dónde está el ejercicio:**

```
radiosonda_PIcaro/
└── specs/
    └── exercises/
        └── 13_cubecell-estacion-meteo/        <-- este ejercicio
            ├── README.md                      <-- esta guía
            ├── credentials.json               <-- credenciales y datos del device
            ├── payload_decoder.js             <-- codec que pegarás en ChirpStack (Paso 8)
            ├── scripts/subscribe.sh           <-- opcional: escuchar por MQTT (Paso 10)
            └── sketches/
                └── cubecell_meteo/            <-- carpeta del sketch de Arduino
                    ├── cubecell_meteo.ino     <-- el programa (NO se edita)
                    ├── config_lorawan.h       <-- credenciales y red (Paso 5)
                    └── config_sensores.h      <-- qué sensores hay conectados (Etapas 1-4)
```

> 🔑 Arduino IDE exige que el `.ino` esté en una carpeta **con el mismo nombre**
> (`cubecell_meteo/cubecell_meteo.ino`). Ya viene así: no muevas ni renombres el `.ino` solo.

### Paso 2. Instala el paquete de placas CubeCell en Arduino IDE

Arduino IDE no conoce la CubeCell de fábrica. Heltec publica un "core" que trae el compilador, el
cargador **y la pila LoRaWAN**.

**2.1 URL del gestor de placas.** **File ▸ Preferences** (macOS: **Arduino IDE ▸ Settings**) → campo
**Additional boards manager URLs** → pega:

```
https://resource.heltec.cn/download/package_CubeCell_index.json
```

Si ya había otra URL, sepáralas con una coma. **OK**.

**2.2 Instala el core.** **Tools ▸ Board ▸ Boards Manager…** → busca `CubeCell` → **CubeCell Development
Framework** by Heltec Automation → versión **1.4.0** (o la más reciente) → **Install**. Descarga unos
100 MB (el compilador ARM va incluido).

**Salida esperada:** en la consola, `Platform CubeCell:CubeCell@1.4.0 installed`.

**2.3 Driver USB (solo Windows).** La CubeCell usa el chip **CP2102**. Windows 10/11 suele instalar el
driver solo al conectarla. Si en el *Administrador de dispositivos* aparece con un signo de admiración,
instala el *CP210x Universal Windows Driver*: <https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers>.
En macOS y Linux no hace falta nada.

**2.4 Conecta la placa y localiza el puerto.** Conecta la CubeCell por USB. **Tools ▸ Port** debe mostrar:

| Sistema | Puerto |
|---|---|
| Windows | `COMx` — *Silicon Labs CP210x* (en la placa del curso fue **COM7**) |
| macOS | `/dev/cu.usbserial-XXXX` o `/dev/cu.SLAB_USBtoUART` |
| Linux | `/dev/ttyUSB0` |

> Linux: si al subir dice *Permission denied*, ejecuta `sudo usermod -aG dialout $USER` y vuelve a iniciar sesión.

### Paso 3. Librerías

**Ninguna que instalar.** La pila LoRaWAN, el driver del BMP280 y el del BH1750 vienen **dentro del
paquete CubeCell**; el DHT11 y el MH-RD se leen sin librería. Si el IDE sugiere instalar algo al abrir
el sketch, no hace falta.

### Paso 4. Abre el sketch y selecciona la placa con sus opciones

**4.1 Abre el sketch.** **File ▸ Open…** →
`C:\dev\radiosonda_PIcaro\specs\exercises\13_cubecell-estacion-meteo\sketches\cubecell_meteo\cubecell_meteo.ino`
(o la ruta equivalente en `~/dev/`). Se abren **tres pestañas**: `cubecell_meteo.ino`,
`config_lorawan.h` y `config_sensores.h`.

**4.2 Selecciona la placa.** **Tools ▸ Board ▸ CubeCell ▸ CubeCell-Board（HTCC-AB01）**.

**4.3 Ajusta las opciones.** Al elegir la placa, el menú **Tools** muestra las opciones de la pila LoRaWAN.
**Esto es lo más importante del ejercicio:** la región, la clase y el modo de activación **no están en el
código**, están aquí. Déjalas **exactamente** así:

| Opción (menú Tools) | Valor | Por qué |
|---|---|---|
| **LORAWAN_REGION** | `REGION_US915` | la banda del curso |
| **LORAWAN_CLASS** | `CLASS_A` | nodo de bajo consumo |
| **LORAWAN_DEVEUI** | `CUSTOM` | usamos el DevEUI de `config_lorawan.h`, no el del chip |
| **LORAWAN_NETMODE** | `OTAA` | activación por aire |
| **LORAWAN_ADR** | `ON` | la red ajusta datarate y potencia |
| **LORAWAN_UPLINKMODE** | `UNCONFIRMED` | el sketch pide confirmado cada 5 uplinks por su cuenta |
| **LORAWAN_Net_Reservation** | `OFF` | join real en cada arranque (queremos verlo) |
| **LORAWAN_AT_SUPPORT** | `OFF` | sin comandos AT: el Monitor Serie queda limpio |
| **LORAWAN_RGB** | `ACTIVE` | el LED muestra el estado |
| **LORAWAN_PREAMBLE_LENGTH** | `8(default)` | |
| **LoRaWan Debug Level** | `Freq` | imprime la frecuencia y el DR de cada TX/RX |
| **Port** | el puerto del Paso 2.4 | |

> Si `LORAWAN_REGION` queda en otra región, **compila igual y no hace join nunca**. Revísalo primero
> cuando algo falle.

### Paso 5. Comprueba (o pon) las credenciales en `config_lorawan.h`

El device del curso, `itpa-meteor-lora`, **ya existe en ChirpStack** y sus credenciales **ya vienen en el
archivo**. Ábrelo en la pestaña `config_lorawan.h` y comprueba que dice:

```c
#define PICARO_DEV_EUI    { 0x62, 0x38, 0x51, 0xF5, 0x18, 0xC9, 0x21, 0x35 }
#define PICARO_JOIN_EUI   { 0x28, 0x80, 0x61, 0x53, 0xB3, 0xDD, 0xAD, 0xEB }
#define PICARO_APP_KEY    { 0x26, 0x32, 0xCF, 0xED, 0xC2, 0x22, 0x36, 0x8E, \
                            0x33, 0x01, 0x49, 0x32, 0x4E, 0xE7, 0xED, 0x82 }
```

Si algún día usas **otro device**, ChirpStack te muestra los valores así: `62 38 51 f5 18 c9 21 35`
(botón **MSB**). Se escriben **en el mismo orden**, cada par de caracteres con `0x` delante y separados
por comas. En la pila de Heltec el Join EUI se llama `appEui`: es el mismo dato.

Para verlos en la consola: <https://lns.pi-caro.org> → **Applications ▸ PiCARO ▸ itpa-meteor-lora** →
el **Device EUI** y el **Join EUI** están arriba; la **Application key** en la pestaña **OTAA keys**.

### Paso 6. Compila y sube

1. **Verify (✓)**: la primera compilación tarda 1–2 minutos.
   **Salida esperada:** `Sketch uses 80608 bytes (61%) of program storage space. Maximum is 131072 bytes.`
2. **Upload (→)**. El cargador de CubeCell muestra `Uploading ( 10 / 100 )` … `( 100 / 100 )`,
   `Checksum verifies OK.` y `Rebooting.` (unos 12 s). No hay que pulsar ningún botón.

### Paso 7. Mira el join en el Monitor Serie

**Tools ▸ Serial Monitor** → **115200 baud** → pulsa **RST** en la placa. Esto es lo que imprime
(captura real):

```
Copyright @2019-2020 Heltec Automation.All rights reserved.

##########################################################
#  EJERCICIO 13 - ESTACION METEO LoRaWAN (CubeCell AB01) #
##########################################################
[info] Sketch v1.0 | uplink cada 60 s en fPort 2 | confirmado cada 5 | ADR ON
[info] Sensores activos: DHT11=0 BMP280=0 BH1750=0 MH-RD=0
========== CREDENCIALES (deben ser IGUALES en ChirpStack) ==========
  DevEUI  (MSB): 62 38 51 F5 18 C9 21 35
  JoinEUI (MSB): 28 80 61 53 B3 DD AD EB
  AppKey  (MSB): 26 32 CF ED C2 22 36 8E 33 01 49 32 4E E7 ED 82
  Region: US915   Sub-banda: 2 (mascara 0xFF00 0x0000 0x0000 0x0000)   LoRaWAN 1.0.x   Clase A
====================================================================
[estado] INIT: arrancando la pila LoRaWAN de Heltec...

AT Rev 1.3
+AutoLPM=1
+LORAWAN=1
+KeepNet=0
+OTAA=1
+Class=A
+ADR=1
+IsTxConfirmed=0
+AppPort=2
+DutyCycle=60000
+ConfirmedNbTrials=4
+ChMask=00000000000000000000FF00
+DevEui=623851F518C92135(For OTAA Mode)
+AppEui=28806153B3DDADEB(For OTAA Mode)
+AppKey=2632CFEDC222368E330149324EE7ED82(For OTAA Mode)
+NwkSKey=00000000000000000000000000000000(For ABP Mode)
+AppSKey=00000000000000000000000000000000(For ABP Mode)
+DevAddr=00000000(For ABP Mode)

LoRaWAN US915 Class A start!

[estado] JOIN: enviando Join-Request (OTAA). Si falla, la pila reintenta a los 30 s.
         LED rojo = transmitiendo, azul/amarillo = RX1/RX2, morado = JOIN OK.
joining...TX on freq 904100000 Hz at DR 3 power 20 dBm
TX on freq 904100000 Hz at DR 3 power 20 dBm
RX on freq 923900000 Hz at DR 13
joined
```

Cómo leerlo:

- El bloque `+…` lo imprime la **pila de Heltec** con lo que va a usar. Comprueba `+OTAA=1`, `+ADR=1`,
  `+ChMask=…FF00` (sub-banda 2) y que `+DevEui`, `+AppEui` y `+AppKey` coinciden con ChirpStack.
- `LoRaWAN US915 Class A start!` confirma la región y la clase del menú Tools.
- `joining...` + `TX on freq 904100000 Hz at DR 3` es el **Join-Request** en un canal de la sub-banda 2
  (903.9–905.3 MHz). `RX on freq 923900000 Hz at DR 13` es la ventana donde escucha el **Join-Accept**.
- **`joined`** = join conseguido. El LED se pone **morado**. Tarda unos **13 s** desde el arranque.

> ℹ️ Con esta pila verás **dos `TX on freq…` seguidos** en el join, y en ChirpStack **dos `JoinRequest`
> con 5 s de diferencia**, ambos con su `JoinAccept`. Es su comportamiento normal: retransmite el
> Join-Request al cerrar la primera ventana. No es un error.

Después, cada 60 s (captura real):

```
=============== UPLINK #1 ===============
---- Lecturas ----
  Bateria : 3810 mV
  DHT11   : desactivado (SENSOR_DHT11=0)
  BMP280  : desactivado (SENSOR_BMP280=0)
  BH1750  : desactivado (SENSOR_BH1750=0)
  MH-RD   : desactivado (SENSOR_MHRD=0)
  Payload (hex, 11 bytes): 00 BE 7F FF 7F FF FF FF FF FF FF
unconfirmed uplink sending ...
TX on freq 903900000 Hz at DR 3 power 20 dBm
[estado] CYCLE: proximo uplink en 60503 ms (duerme tras RX2)TX on freq 903900000 Hz at DR 3 power 20 dBm
RX on freq 923300000 Hz at DR 13
received unconfirmed downlink: rssi = -66, snr = 9, datarate = 13

=============== UPLINK #2 ===============
...
TX on freq 904900000 Hz at DR 3 power 14 dBm
```

Cómo leerlo:

- `Payload (hex…)`: `00` = ningún sensor activo; `BE` = batería (0xBE × 20 mV = 3800 mV); el resto son
  los valores "no disponible" (`7F`, `FF`, `7FFF`, `FFFF`, `FFFF`, `FF`).
- `TX on freq … power 20 dBm` aparece **dos veces** por uplink (al iniciar y al terminar la transmisión).
- `received unconfirmed downlink: rssi = -66, snr = 9`: ChirpStack respondió con **comandos MAC**
  (`LinkADRReq`, `DevStatusReq`). Ese RSSI/SNR es el del **downlink**, es decir, lo bien que la placa oye al gateway.
- **El uplink #2 sale solo 7 s después del #1**: la pila envía enseguida la **respuesta** a esos comandos
  MAC, y el sketch aprovecha para mandar lecturas nuevas. A partir del #3 la cadencia es de 60 s.
- Mira cómo la **potencia baja** con los uplinks (20 → 16 → 14 → 10 dBm): es **ADR** en acción. La placa
  está al lado del gateway y la red le pide gastar menos.
- Cada 5 uplinks se pide uno **confirmado**: verás `confirmed uplink sending ...` y
  `[downlink] ACK del servidor: el uplink confirmado llego.`

### Paso 8. Verifica en ChirpStack y activa el codec

<https://lns.pi-caro.org> → **Applications ▸ PiCARO ▸ itpa-meteor-lora**:

- Arriba, **Last seen**: *a few seconds ago*.
- Pestaña **LoRaWAN frames**: `JoinRequest`, `JoinAccept` (dos pares seguidos, ver la nota del Paso 7)
  y después `UnconfirmedDataUp` cada 60 s con `UnconfirmedDataDown` de respuesta (los comandos MAC).
- Pestaña **Events**: cada evento `up` trae `data` en base64. **Para que aparezca `object` decodificado**,
  el device profile necesita el codec:

  **Device profiles ▸ CubeCellPSoC Lora ▸ pestaña Codec** → *Payload codec* = `JavaScript functions` →
  borra el ejemplo y pega **todo** el contenido de [`payload_decoder.js`](payload_decoder.js) → **Submit**.
  A partir del siguiente uplink, `object` aparece con `vbat_mv`, `battery_pct` y el bloque `sensores`.

  > Si tu usuario no puede editar el device profile, pide al instructor que pegue el codec.

- Pestaña **Device metrics**: RSSI y SNR con los que el gateway oye a la placa.

> ℹ️ El campo **Battery** que ChirpStack muestra en la cabecera del device lo reporta la pila de Heltec
> con el comando `DevStatusAns` y no está calibrado (puede decir 2 %). El voltaje **real** es el que va
> en el payload (`vbat_mv`).

### Paso 9. Estado de reposo y consumo

Tras RX2 la placa **duerme** (el LED se apaga y el Monitor Serie no imprime nada) hasta el siguiente
uplink. Es lo esperado: en reposo consume microamperios y una LiPo de 1000 mAh dura semanas. Si ves
que "se queda colgada", **no está colgada**: espera los 60 s.

### Paso 10. (Opcional) Escucha los uplinks por MQTT

El servidor publica cada uplink decodificado en un broker MQTT con TLS. Necesitas `mosquitto_sub`
(`mosquitto-clients` en Linux/WSL, `brew install mosquitto` en macOS), el usuario y contraseña **MQTT**
de tu equipo (distintos de los de la web) y el UUID de la aplicación (está en la URL de la consola:
`.../applications/<uuid>/...`).

```bash
# desde specs/exercises/13_cubecell-estacion-meteo/
export MQTT_USER=equipoNN MQTT_PASS='...' APP_ID='<uuid>'
./scripts/subscribe.sh
```

**Salida esperada** (una línea por uplink):
```
{"time":"2026-09-17T23:47:30Z","fCnt":4,"fPort":2,"dr":3,"rssi":-45,"snr":9.5,"gw":"2cf7f11153100079",
 "data":"ALl//3//////////","object":{"fport":2,"sensores":{"dht11":false,"bmp280":false,"bh1750":false,"mhrd":false},"vbat_mv":3700,"vbat_v":3.7,"battery_pct":44}}
```

---

## 🌦️ Etapas 1 a 5: los sensores y la energía

La estación crece **un sensor cada vez**. El método es siempre el mismo:

1. **Apaga** la placa (desconecta el USB) y cablea el sensor.
2. En `config_sensores.h` pon la macro del sensor en **1**.
3. **Upload** y abre el Monitor Serie: en `---- Lecturas ----` debe aparecer la medida.
4. En ChirpStack, `object` trae el campo nuevo y `sensores.<nombre>` pasa a `true`.

**Regla de alimentación:** todos los sensores van a **Vext** (no a 3V3) y a **GND**. El firmware enciende
Vext 1.2 s antes de medir y lo apaga después; así la estación dura con batería.

> ⚠️ **Etapas pendientes de verificar en hardware.** El código de los cuatro sensores está escrito y
> **compila** (probado con las cuatro macros en 1: 69 % de flash), pero al cierre de esta guía los
> módulos aún no estaban cableados. Las lecturas de ejemplo de abajo son "algo así"; se sustituirán por
> capturas reales al completar cada etapa.

### Etapa 1 — DHT11 (temperatura y humedad)

| Pin del DHT11 | Pin de la CubeCell |
|---|---|
| VCC (+) | **Vext** |
| DATA (S / out) | **GPIO5** |
| GND (−) | **GND** |

Los módulos de **3 pines** ya traen la resistencia de pull-up. Si tu DHT11 es el sensor **desnudo de 4
pines**, pon una resistencia de **10 kΩ entre DATA y VCC** y deja el pin 3 sin conectar.

`config_sensores.h`: `#define SENSOR_DHT11 1`. Monitor Serie esperado: `DHT11   : 24 C, 61 %HR`.
ChirpStack: `temp_dht_c`, `humedad_pct`. Si dice `SIN RESPUESTA`: revisa DATA→GPIO5, la pull-up y que
VCC vaya a Vext.

### Etapa 2 — BMP280 (presión y temperatura)

| Pin del BMP280 | Pin de la CubeCell |
|---|---|
| VCC / VIN | **Vext** |
| GND | **GND** |
| SDA | **SDA** |
| SCL | **SCL** |
| SDO (si lo tiene) | **GND** → dirección 0x76 (a VCC sería 0x77) |
| CSB (si lo tiene) | **Vext** (selecciona I2C) |

`config_sensores.h`: `#define SENSOR_BMP280 1` (y `BMP280_I2C_ADDR 0x77` si pusiste SDO a VCC).
Monitor Serie esperado: `BMP280  : 24.35 C, 1012.6 hPa`. ChirpStack: `temp_bmp_c`, `presion_hpa`.
Si dice `NO RESPONDE`: intercambia SDA/SCL o prueba la otra dirección.

### Etapa 3 — BH1750 (luz)

| Pin del BH1750 | Pin de la CubeCell |
|---|---|
| VCC | **Vext** |
| GND | **GND** |
| SDA | **SDA** (compartido con el BMP280) |
| SCL | **SCL** (compartido con el BMP280) |
| ADDR | sin conectar (o a GND) → dirección 0x23 |

`config_sensores.h`: `#define SENSOR_BH1750 1`. Monitor Serie esperado: `BH1750  : 312 lux`.
ChirpStack: `lux`.

### Etapa 4 — MH-RD (lluvia)

El MH-RD son dos piezas: la **placa sensora** (pistas expuestas) y el **módulo comparador** (LM393 con
potenciómetro). La placa sensora se conecta al comparador con sus dos cables; el comparador a la CubeCell:

| Pin del módulo MH-RD | Pin de la CubeCell |
|---|---|
| VCC | **Vext** |
| GND | **GND** |
| AO (analógico) | **ADC** |
| DO (digital) | **GPIO1** |

`config_sensores.h`: `#define SENSOR_MHRD 1`. Monitor Serie esperado: `MH-RD   : AO=3980 (raw) -> 3 % lluvia, DO=seco`.

**Calibración (una vez):** con la placa **seca**, anota el `AO=` que imprime y ponlo en `MHRD_DRY_RAW`;
moja la placa con unas gotas, anota el `AO=` y ponlo en `MHRD_WET_RAW`. Reflashea: `lluvia_pct` irá de
0 (seco) a 100 (empapado). El potenciómetro del módulo ajusta el umbral de `DO` (lluvia sí/no, bit
`lluvia` en ChirpStack).

### Etapa 5 — Energía: batería, módulo de carga USB-C y celda solar

Hasta aquí la estación vive del USB. Para dejarla en la intemperie necesita **batería** y algo que la
recargue. Hay dos formas de armarlo; la **A** usa lo que ya trae la placa y la **B** añade un módulo de
carga USB-C, que es más versátil (permite cargar desde cualquier cargador de móvil además del sol).

**Datos de la placa que mandan en las dos opciones** (documentación de Heltec):

| Dato | Valor |
|---|---|
| Conector de batería | **SH1.25-2** (2 pines, paso 1.25 mm), LiPo de **3.7 V** una celda |
| Cargador integrado | sí: carga la batería desde el **USB** y conmuta solo entre USB y batería |
| Entrada solar propia | conector de 2 pines / pin **VS**, panel de **5.5 a 7 V** |
| Consumo en reposo | **3.5 µA** en sueño profundo |
| Medida de batería | la lee el firmware (`Bateria : … mV` en el Monitor Serie y `vbat_mv` en ChirpStack) |

> ⚠️ **Antes de conectar nada:** mide con el multímetro la polaridad del cable de la batería y del
> panel. Los conectores SH1.25 de los vendedores **no siempre respetan** rojo = positivo. Un LiPo al
> revés destruye el cargador de la placa.

#### Opción A — la entrada solar de la propia CubeCell (la más simple)

```
   celda solar 6 V ──(+)──► conector SOLAR / pin VS de la CubeCell
                   ──(−)──► GND
   batería LiPo 3.7 V ─────► conector BAT (SH1.25-2) de la CubeCell
```

- Panel de **6 V nominal** (1–2 W): en sol directo da 6–7 V, dentro del rango 5.5–7 V que admite la
  placa. **No** uses un panel de 12 V ni uno de 5 V (con 5 V no llega a cargar).
- La batería va al conector **BAT**. El cargador integrado la carga desde el panel o desde el USB.
- No hay nada más que hacer. Es la opción recomendada si la placa va a estar en un sitio con sol.

#### Opción B — módulo de carga USB-C (TP4056) entre el panel y la batería

Con esta opción el **módulo** es el que carga la batería (desde el panel o desde su propio USB-C), y la
CubeCell solo **consume** de ella. El módulo de 6 pines lleva el cargador TP4056 y la protección
DW01 (corte por sobredescarga, sobrecarga y cortocircuito).

```
   celda solar 6 V ──(+)──► IN+  ┌───────────────┐  B+ ◄──(+)── batería LiPo 3.7 V
                   ──(−)──► IN−  │ módulo TP4056 │  B− ◄──(−)──
                                 │   USB-C       │
                                 │               │  OUT+ ──(+)──► CubeCell conector BAT (SH1.25-2)
                                 └───────────────┘  OUT− ──(−)──► CubeCell conector BAT (SH1.25-2)
```

| Pin del módulo | Se conecta a | Notas |
|---|---|---|
| **IN+ / IN−** | celda solar (+ / −) | o deja los pines libres y carga por el **USB-C** del módulo con un cargador de móvil |
| **B+ / B−** | batería LiPo (+ / −) | soldado o con el conector de la batería |
| **OUT+ / OUT−** | conector **BAT** de la CubeCell (+ / −) | por aquí sale la batería **protegida** hacia la placa |

Pasos:

1. **Corriente de carga.** El TP4056 viene ajustado a **1 A** (resistencia R3 de 1.2 kΩ). Para una
   batería de 1000 mAh y un panel pequeño es demasiado: cambia R3 por **5 kΩ (≈ 250 mA)** o
   **10 kΩ (≈ 130 mA)**. Si no quieres soldar, usa una batería de 2000 mAh o más, que admite 1 A.
2. **Panel.** 6 V nominal, 1–2 W. El TP4056 admite hasta 8 V en IN; un panel de 6 V en sol directo
   queda dentro. Un panel de 5 V carga solo con sol fuerte; uno de 12 V **lo quema**.
3. **Suelda o conecta** B+/B− a la batería y OUT+/OUT− a un cable con conector SH1.25-2 hacia el
   conector **BAT** de la CubeCell. Comprueba la polaridad con el multímetro **en el extremo del
   conector** antes de enchufarlo.
4. **Conecta el panel** a IN+/IN−. Con sol, el LED rojo del módulo indica *cargando* y el azul
   *batería llena*.
5. **No conectes el USB de la CubeCell mientras el módulo esté cargando.** El cargador integrado de la
   placa y el TP4056 estarían cargando la misma batería a la vez. Para programar o ver el Monitor Serie,
   desconecta antes el panel del módulo (o el USB-C del módulo).

> ℹ️ El conector **SOLAR / VS** de la CubeCell queda **sin usar** en la opción B: el panel entra por el
> módulo, no por la placa.

#### Verificación (las dos opciones)

- Con el USB desconectado, la placa sigue encendida y el Monitor Serie no está disponible: mira en
  ChirpStack que **Last seen** siga avanzando cada 60 s. Ese es el nodo funcionando **solo con batería**.
- En `object.vbat_mv` de cada uplink verás el voltaje. Con el panel al sol debe **subir** poco a poco
  (hasta ~4.2 V); de noche, **bajar** muy despacio.
- Autonomía orientativa: con el ciclo de 60 s y los cuatro sensores, una LiPo de 1000 mAh dura
  **varias semanas** sin sol. Para alargarla, sube `PICARO_UPLINK_INTERVAL_S` a 300 (5 min).

---

## ✅ Cómo saber que funcionó

- [ ] Monitor Serie: `joined` tras `joining...` y el LED en **morado**.
- [ ] Monitor Serie: `UPLINK #n` cada 60 s con `TX on freq 90xxxxxxx Hz` (sub-banda 2) y la potencia bajando por ADR.
- [ ] ChirpStack: *last seen* de hace segundos; **LoRaWAN frames** con `JoinRequest` / `JoinAccept` / `UnconfirmedDataUp`.
- [ ] ChirpStack: **Events ▸ up ▸ object** con `vbat_mv` (tras pegar el codec).
- [ ] En el uplink #5: `confirmed uplink sending ...` y `[downlink] ACK del servidor`.
- [ ] Por cada sensor activado: su lectura en el Monitor Serie y su campo en `object`.
- [ ] Etapa 5: con el USB desconectado, *last seen* sigue avanzando y `vbat_mv` sube con el panel al sol.

## 🛠️ Si algo falla

| Síntoma | Causa probable | Arreglo |
|---|---|---|
| *Tools ▸ Board* no lista `CubeCell` | El core no se instaló o la URL del 2.1 está mal | Repite 2.1 y 2.2; la URL termina en `package_CubeCell_index.json` |
| No aparece ningún puerto | Cable solo de carga o driver CP210x (Windows) | Cambia el cable; instala el driver (2.3) |
| Compila pero nunca hace join (`joining...` y a los 30 s `join failed`) | **`LORAWAN_REGION` no es `REGION_US915`**, o `LORAWAN_NETMODE` no es `OTAA`, o gateway offline | Menú Tools (Paso 4.3); consola → Gateways *Online* |
| `join failed, join again at 30s later` con región correcta | Credenciales distintas a las de ChirpStack, o `LORAWAN_DEVEUI` en *Generate By ChipID* | Compara el bloque `+DevEui/+AppEui/+AppKey` con la consola; pon `LORAWAN_DEVEUI = CUSTOM` |
| ChirpStack: *"MIC mismatch"* en el JoinRequest | AppKey mal copiada | Vuelve a copiarla byte a byte (MSB) |
| ChirpStack: *"DevNonce already used"* | Se borró y recreó el device y la placa repite nonces | Con `LORAWAN_Net_Reservation OFF` la pila usa nonces aleatorios; reinicia la placa. Si persiste, el instructor limpia los nonces del device en la consola (*Flush dev nonces*) |
| `payload length error ...` en el Monitor Serie | El payload supera lo que admite el DR actual (DR0 = 11 bytes) | El sketch envía 11 bytes justos; si lo amplías, espera a que ADR suba el DR o reduce campos |
| `TX on freq` en 902.x–903.x MHz (canales 0-7) | Máscara de canales en sub-banda 1 | `PICARO_SUBBAND 2` en `config_lorawan.h` |
| Uplinks OK pero sin `object` en ChirpStack | Falta el codec en el device profile | Paso 8 |
| El Monitor Serie no muestra nada tras RX2 | La placa **duerme**: es normal | Espera 60 s |
| El Monitor Serie corta una línea (`...RX2)TX on freq`) | La pila escribe desde su temporizador mientras el sketch imprime | Cosmético; no afecta |
| `DHT11 : SIN RESPUESTA` | Pull-up ausente, DATA en otro pin, o VCC no va a Vext | Etapa 1 |
| `BMP280 : NO RESPONDE` / `BH1750 : NO RESPONDE` | SDA/SCL cruzados, dirección I2C, o VCC no va a Vext | Etapas 2 y 3; prueba 0x77 en el BMP280 |
| `lluvia_pct` siempre 0 o 100 | `MHRD_DRY_RAW` / `MHRD_WET_RAW` sin calibrar | Etapa 4, calibración |
| Batería en ChirpStack "2 %" con LiPo cargada | Es el `DevStatusAns` de la pila, no calibrado | Usa `vbat_mv` del payload |
| La placa no enciende solo con batería | Polaridad del conector BAT invertida, o la protección del módulo cortó por batería vacía | Mide la polaridad; carga la batería por USB-C del módulo hasta que su LED cambie a azul |
| `vbat_mv` no sube con el panel al sol | Panel de 5 V (insuficiente), polaridad del panel, o corriente de carga muy alta para el panel | Usa panel de 6 V; revisa IN+/IN−; baja la corriente del TP4056 (R3) |

---

## 📤 Los datos (payload)

- **fPort:** 2
- **Formato (11 bytes, big-endian).** Cabe en **DR0** (el datarate más lento de US915, 11 bytes máx.),
  que es con el que arranca la pila antes de que ADR lo suba.

| Byte(s) | Campo | Tipo | Codificación | "no hay" |
|---|---|---|---|---|
| 0 | `status` | uint8 | bit0 DHT11 ok · bit1 BMP280 ok · bit2 BH1750 ok · bit3 lluvia (DO) · bit4 MH-RD ok | |
| 1 | `vbat` | uint8 | voltaje de batería / 20 mV | 0 |
| 2 | `temp_dht` | int8 | °C enteros | 0x7F |
| 3 | `hum_dht` | uint8 | % HR | 0xFF |
| 4..5 | `temp_bmp` | int16 | °C × 10 | 0x7FFF |
| 6..7 | `presion` | uint16 | hPa × 10 | 0xFFFF |
| 8..9 | `lux` | uint16 | lux (tope 65534) | 0xFFFF |
| 10 | `lluvia_pct` | uint8 | 0 seco … 100 empapado | 0xFF |

- **Decodificación:** [`payload_decoder.js`](payload_decoder.js) **omite** los campos "no hay", así el
  dashboard sabe qué sensores están conectados (`sensores.dht11: true/false`, etc.).
  Ejemplo real de hoy: `00 BE 7F FF 7F FF FF FF FF FF FF` →
  `{ fport: 2, sensores: {dht11: false, bmp280: false, bh1750: false, mhrd: false}, vbat_mv: 3800, vbat_v: 3.8, battery_pct: 55 }`.

## 📖 Nota didáctica — qué tiene de distinto esta placa

- **La pila LoRaWAN no es una librería, es parte del core.** Se configura con **variables globales de
  nombre fijo** (`devEui`, `appEui`, `appKey`, `userChannelsMask`…) y con **opciones del menú Tools**.
  Ventaja: cero dependencias. Desventaja: un menú mal puesto compila igual y falla en silencio.
- **Bajo consumo de verdad.** `LoRaWAN.sleep()` apaga el MCU entre uplinks. Por eso los sensores se
  alimentan de **Vext** solo durante la medida, y por eso el Monitor Serie "se calla" 60 s.
- **El payload cabe en DR0.** En US915 el DR0 (SF10) admite solo 11 bytes. Un payload mayor daría
  `payload length error` hasta que ADR subiera el datarate. Diseñar el payload **pensando en el peor
  DR** es una habilidad real de LoRaWAN.
- **ADR se ve en directo.** Con la placa junto al gateway, ChirpStack manda `LinkADRReq` en cada
  downlink y la potencia cae de 20 a 10 dBm en cuatro uplinks. Aleja la placa y verás el proceso inverso.

## 📎 Referencia

- **Credenciales y datos del device:** [`credentials.json`](credentials.json).
- **Archivos de esta carpeta:**
  - `sketches/cubecell_meteo/cubecell_meteo.ino` — la lógica (join, sensores, payload, Serie, downlinks).
  - `sketches/cubecell_meteo/config_lorawan.h` — credenciales y parámetros de red.
  - `sketches/cubecell_meteo/config_sensores.h` — qué sensores hay, pines y calibración.
  - `payload_decoder.js` — codec para el device profile.
  - `scripts/subscribe.sh` — escucha MQTT del servidor del curso.
- **Para saber más (no hace falta para este ejercicio):** [Wiki del proyecto](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki) · [Índice de ejercicios](../README.md) · [CubeCell en la documentación de Heltec](https://docs.heltec.org/en/node/asr650x/index.html)

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
> 🧩 **Código:** basado en los ejemplos LoRaWAN del CubeCell Development Framework de Heltec, bajo **licencia MIT**.
