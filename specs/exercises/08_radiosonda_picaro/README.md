# 🎈 Radiosonda PICARO — LoRaWAN con LilyGo T-Beam → ChirpStack

Práctica didáctica para armar una **"radiosonda"** con una **LilyGo T-Beam (ESP32, radio SX1276)** que:

1. Se **une (join OTAA)** a una red **LoRaWAN** montada con **ChirpStack**.
2. Lee sus **sensores integrados** (GPS y batería) y **envía la telemetría** por radio a través de un **gateway** hacia ChirpStack.
3. Imprime todo el proceso por el **Monitor Serie** y en la **pantalla OLED** (si la placa la trae).

> Está pensada para un estudiante **sin experiencia**: sigue los pasos en orden. Lo único que vas a editar es el archivo **`config_lorawan.h`**.

---

## 📋 Índice

1. [Qué necesitas](#1-qué-necesitas)
2. [Qué hace cada archivo](#2-qué-hace-cada-archivo-de-esta-carpeta)
3. [Paso 1 — Instalar Arduino IDE + ESP32](#3-paso-1--instalar-arduino-ide--soporte-esp32)
4. [Paso 2 — Instalar las librerías](#4-paso-2--instalar-las-librerías)
5. [Paso 3 — Abrir el proyecto y entender el código](#5-paso-3--abrir-el-proyecto-y-entender-el-código)
6. [Paso 4 — Cambiar DevEUI, JoinEUI y AppKey](#6-paso-4--cambiar-deveui-joineui-y-appkey)
7. [Paso 5 — Seleccionar la tarjeta y compilar (build)](#7-paso-5--seleccionar-la-tarjeta-y-compilar-build)
8. [Paso 6 — Conectar, seleccionar el Puerto y flashear](#8-paso-6--conectar-seleccionar-el-puerto-y-flashear)
9. [Paso 7 — Ver el Monitor Serie](#9-paso-7--ver-el-monitor-serie)
10. [Paso 8 — Dar de alta el dispositivo en ChirpStack](#10-paso-8--dar-de-alta-el-dispositivo-en-chirpstack)
11. [Paso 9 — Ver los datos decodificados](#11-paso-9--ver-los-datos-decodificados)
12. [Estructura del payload](#12-estructura-del-payload-los-bytes-que-se-envían)
13. [Solución de problemas](#13-solución-de-problemas-troubleshooting)
14. [Cómo modificar el proyecto en el futuro](#14-cómo-modificar-el-proyecto-en-el-futuro)

---

## 1. Qué necesitas

### Hardware
- 1 × **LilyGo T-Beam** (ESP32 clásico) con radio **SX1276** y antena LoRa conectada ⚠️ *(nunca enciendas el radio sin antena)*.
- 1 × cable **USB-C o micro-USB** (según tu versión de placa) que **transmita datos** (no solo carga).
- 1 × **gateway LoRaWAN** ya funcionando y reportando a tu **ChirpStack** (dado por el instructor).
- *(Opcional)* batería 18650 y pantalla OLED, si tu T-Beam las trae.

### Software
- **Arduino IDE 2.x** (o 1.8.19).
- Acceso web a tu servidor **ChirpStack v4**.

### Parámetros de red de esta práctica (ya configurados en el código)
| Parámetro | Valor |
|---|---|
| Región / banda | **US915** |
| Sub-banda | **2** (canales 8–15, el default de ChirpStack) |
| Versión LoRaWAN | **1.0.x** (solo se usa AppKey) |
| Activación | **OTAA** |
| Clase | **A** |

---

## 2. Qué hace cada archivo de esta carpeta

```
radiosonda_picaro/
├── radiosonda_picaro.ino   ← programa principal (la lógica). MUY comentado.
├── config_lorawan.h        ← ⭐ AQUÍ cambias DevEUI / JoinEUI / AppKey.
├── utilities.h             ← pines de la placa. Ya viene en "T-Beam SX1276".
├── LoRaBoards.h  / .cpp     ← "motor" de la placa: enciende PMU, GPS y OLED.
│                             No necesitas editarlo para la práctica.
├── chirpstack/
│   └── decoder.js          ← se pega en ChirpStack para "traducir" el payload.
└── README.md               ← esta guía.
```

> 🧠 **Regla de oro:** como estudiante, el 99 % de las veces **solo tocas `config_lorawan.h`**. El resto es infraestructura que ya funciona.

---

## 3. Paso 1 — Instalar Arduino IDE + soporte ESP32

1. Descarga e instala **Arduino IDE** desde <https://www.arduino.cc/en/software>.
2. Abre Arduino IDE → **File ▸ Preferences**.
3. En **"Additional boards manager URLs"** pega:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Ve a **Tools ▸ Board ▸ Boards Manager…**, busca **`esp32`** (autor *Espressif Systems*) e instala la versión **3.x** (o 2.0.x).

---

## 4. Paso 2 — Instalar las librerías

En Arduino IDE ve a **Tools ▸ Manage Libraries…** (o el icono de libros 📚) e instala estas cuatro:

| Librería | Autor | Notas |
|---|---|---|
| **RadioLib** | Jan Gromes | Instala **7.6.0** (o la 7.x más nueva). El código está probado con 7.6.0. |
| **TinyGPSPlus** | Mikal Hart | Lee el GPS. |
| **XPowersLib** | lewisxhe (Lewis He) | Controla el PMU (energía y batería). |
| **U8g2** | oliver | Pantalla OLED. Necesaria aunque no tengas OLED (el código la incluye). |

> Las demás (`WiFi`, `SPI`, `Wire`, `FS`) ya vienen con el paquete ESP32; no instales nada extra.

---

## 5. Paso 3 — Abrir el proyecto y entender el código

1. Abre **`radiosonda_picaro.ino`** con Arduino IDE (doble clic). Arduino abrirá automáticamente **todos** los archivos de la carpeta como pestañas.

2. Lee el `.ino` de arriba hacia abajo. Está dividido en 6 secciones numeradas:

   | Sección | Qué hace |
   |---|---|
   | 1) Objetos principales | crea el `radio`, el `node` LoRaWAN y el `gps`. |
   | 2) Estructura del payload | define los 15 bytes que se transmiten. |
   | 3) Funciones de ayuda | traducir errores, imprimir la identidad, leer GPS. |
   | 4) `buildPayload()` | **lee los sensores** y arma el paquete de bytes. |
   | 5) `setup()` | enciende la placa, inicia el radio y **hace el join**. |
   | 6) `loop()` | **envía** telemetría cada `PICARO_UPLINK_INTERVAL_S` segundos. |

3. 🔎 **Para el futuro (corregir o ampliar):** si quieres agregar un dato al mensaje, se toca en **dos lugares que deben coincidir**:
   - `buildPayload()` en el `.ino` (donde se escriben los bytes), y
   - `chirpstack/decoder.js` (donde se leen los bytes).

---

## 6. Paso 4 — Cambiar DevEUI, JoinEUI y AppKey

Abre la pestaña **`config_lorawan.h`**. Ahí están las 3 credenciales. Este es el orden recomendado:

> Puedes **inventar** estos valores y luego copiarlos a ChirpStack, **o** crear primero el dispositivo en ChirpStack y copiar de ahí. Lo importante es que **los valores en la placa y en ChirpStack sean idénticos**.

### 6.1 DevEUI (identificador del dispositivo, 8 bytes)
```c
#define PICARO_DEV_EUI   0x70B3D57ED0066A4C
```
- Se escribe como `0x` + **16 dígitos** hexadecimales, en orden **MSB primero**.
- Es el mismo texto que ves en ChirpStack cuando el botón de orden está en **MSB**.
  - ChirpStack muestra: `70 B3 D5 7E D0 06 6A 4C` → aquí escribes `0x70B3D57ED0066A4C`.

### 6.2 JoinEUI (antes AppEUI, 8 bytes)
```c
#define PICARO_JOIN_EUI  0x0000000000000000
```
- Para practicar se deja en **todo ceros** (lo más común). Solo asegúrate de poner **el mismo valor** en ChirpStack.

### 6.3 AppKey (llave secreta, 16 bytes)
```c
#define PICARO_APP_KEY   0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, \
                         0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
```
- Son **16 bytes** separados por comas, cada uno con `0x`, en orden **MSB primero**.
- ⚠️ Es **secreta**: no la subas a internet ni la compartas.

> No cambies `PICARO_REGION`, `PICARO_SUBBAND` ni los demás parámetros: ya están en **US915 / sub-banda 2**. El intervalo de envío (`PICARO_UPLINK_INTERVAL_S`, 30 s) sí lo puedes ajustar.

Guarda el archivo (**Ctrl+S**).

---

## 7. Paso 5 — Seleccionar la tarjeta y compilar (build)

### 7.1 Seleccionar la tarjeta correcta

La T-Beam clásica usa el chip **ESP32**, así que la tarjeta a elegir se llama **"ESP32 Dev Module"**. Hay dos formas de seleccionarla:

**Forma A — desde el menú (Arduino IDE 1.8 y 2.x):**
1. Ve al menú **Tools ▸ Board ▸ esp32** (el grupo que instalaste en el Paso 1; puede aparecer como *"esp32"* o *"ESP32 Arduino"*).
2. En la lista larga que se despliega, busca y haz clic en **"ESP32 Dev Module"** (suele estar entre las primeras).

**Forma B — desde el selector de la barra superior (solo Arduino IDE 2.x):**
1. Arriba, al centro, hay un desplegable de tarjetas/puerto. Haz clic y elige **"Select other board and port…"**.
2. En la columna **"BOARDS"** escribe `ESP32 Dev Module`, haz clic en el resultado y confirma con **OK**.

> ⚠️ **No te confundas de tarjeta:** debe ser **"ESP32 Dev Module"** tal cual. **NO** elijas *"ESP32**S3** Dev Module"*, *"ESP32**C3**"*, *"TTGO T1"* ni un *"T-Beam"* de otro paquete. Para esta placa clásica, la opción probada es **"ESP32 Dev Module"**.

### 7.2 Ajustar las opciones de la tarjeta

Al seleccionar la tarjeta, Arduino IDE muestra más opciones abajo en el menú **Tools**. Ajústalas **exactamente** así (haz clic en cada una y elige el valor):

| Opción (en el menú Tools) | Valor a elegir |
|---|---|
| **Board** | `ESP32 Dev Module` |
| **Upload Speed** | `921600` |
| **CPU Frequency** | `240MHz (WiFi/BT)` |
| **Flash Frequency** | `80MHz` |
| **Flash Mode** | `QIO` |
| **Flash Size** | `4MB (32Mb)` |
| **Partition Scheme** | `Huge APP (3MB No OTA/1MB SPIFFS)` ← **importante**, el programa es grande |
| **PSRAM** | `Disabled` |
| **Port** | *(lo eliges en el Paso 6, cuando conectes la placa)* |

> **¿Por qué "Huge APP"?** RadioLib + LoRaWAN + las librerías ocupan bastante. Con la partición normal aparece el error *"text section exceeds available space"*. Con "Huge APP" hay espacio de sobra (el programa usa ~33 %).

### 7.3 Compilar

Presiona el botón **✓ Verify** (la palomita, arriba a la izquierda). Si termina con **"Done compiling"**, ¡el build salió bien! Si hay errores rojos, ve a [Solución de problemas](#13-solución-de-problemas-troubleshooting).

---

## 8. Paso 6 — Conectar, seleccionar el Puerto y flashear

### 8.1 Conectar la placa
1. Conecta la T-Beam a la PC con un cable USB **que transmita datos** (no de solo carga).
2. Si es la primera vez en Windows, espera unos segundos a que termine de instalar el driver USB.

### 8.2 Seleccionar el Puerto (COM)

El **"Puerto"** es el canal por el que la PC habla con la placa. Para saber cuál es el tuyo:

> 💡 **Truco infalible:** con la placa **desconectada**, abre **Tools ▸ Port** y fíjate qué puertos hay. **Conecta** la placa, vuelve a abrir **Tools ▸ Port**: el puerto **nuevo** que apareció es el de tu T-Beam.

- **Windows:** aparece como `COM3`, `COM4`, `COM5`, … Para confirmar cuál es, abre el **Administrador de dispositivos** (clic derecho en el botón *Inicio* ▸ *Administrador de dispositivos*) → **Puertos (COM y LPT)**. Verás algo como *"Silicon Labs CP210x USB to UART (COM5)"* o *"USB-Enhanced-SERIAL CH9102 (COM5)"*. Ese `COMx` es tu puerto.
- **Linux:** suele ser `/dev/ttyUSB0`. Si da error de permisos: `sudo usermod -aG dialout $USER` y vuelve a iniciar sesión.
- **macOS:** `/dev/cu.SLAB_USBtoUART` o `/dev/cu.wchusbserial…`.

Selecciónalo en **Tools ▸ Port ▸ (tu COMx)**, o desde el desplegable de la barra superior en Arduino IDE 2.x.

> **¿No aparece ningún puerto nuevo?** Casi siempre es una de estas tres causas:
> 1. **Falta el driver USB.** Instala **CP210x** (Silicon Labs) o **CH9102/CH340** (WCH), según el chip que viste en el Administrador de dispositivos. Reconecta la placa después de instalar.
> 2. **Cable de solo carga.** Cámbialo por uno que transmita datos.
> 3. **Puerto USB con problemas.** Prueba otro puerto USB de la PC, de preferencia directo (sin hub).

### 8.3 Subir el programa (flashear)
1. Verifica arriba que estén seleccionadas **la tarjeta** ("ESP32 Dev Module") **y el puerto** correctos.
2. Presiona el botón **→ Upload** (la flecha, junto a la palomita).
3. Espera a que aparezca **"Writing at 0x… (100 %)"** y luego **"Hard resetting via RTS pin…"**. Eso significa que se flasheó con éxito.

> La T-Beam normalmente entra sola en modo de programación (auto-boot). Si el upload se queda pegado en "Connecting……....", mantén presionado el botón **BOOT/IO0** de la placa mientras empieza a subir (suéltalo al ver "Writing"), o baja el **Upload Speed** a `115200`.

---

## 9. Paso 7 — Ver el Monitor Serie

1. Abre **Tools ▸ Serial Monitor** (o la lupa 🔍 arriba a la derecha).
2. Abajo a la derecha, pon la velocidad en **`115200 baud`**.
3. Pulsa el botón **RST** de la placa para reiniciarla y ver el arranque desde el inicio.

Verás algo así:

```
#############################################
#     RADIOSONDA PICARO - arrancando...     #
#############################################
setupBoards
AXP2101 PMU init succeeded, using AXP2101 PMU
[1/3] Inicializando radio SX1276...
      Radio OK.

========== DATOS PARA DAR DE ALTA EN CHIRPSTACK ==========
  DevEUI  (MSB): 70 B3 D5 7E D0 06 6A 4C
  JoinEUI (MSB): 00 00 00 00 00 00 00 00
  AppKey  (MSB): 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
  Region: US915   Sub-banda: 2   Version LoRaWAN: 1.0.x
=========================================================

[2/3] Uniendose a la red LoRaWAN (OTAA)...
      Intento de union #1...
      JOIN EXITOSO! Ya estamos en la red.
      DevAddr asignado: 1FC204A3
[3/3] Listo. Comenzando a enviar telemetria.

---- Lectura de sensores ----
  GPS: SIN FIX todavia... Ponlo cerca de una ventana.
  Bateria: 4102 mV  (89%)  [USB]
  Payload (hex): 0400000000000000000000001006 59
  Enviando uplink...
  Uplink enviado (sin downlink).
  RSSI: -47 dBm   SNR: 9.5 dB   Uplink #1
  Siguiente uplink en 30 s.
```

> 📌 **Copia el bloque "DATOS PARA DAR DE ALTA EN CHIRPSTACK"**: lo usarás en el siguiente paso.
> El GPS puede tardar **varios minutos** en obtener "fix" la primera vez (arranque en frío). Mientras tanto envía lat/lon en 0 — es normal. Colócalo cerca de una ventana o al aire libre.

---

## 10. Paso 8 — Dar de alta el dispositivo en ChirpStack

> Se asume que el **gateway ya está agregado** y reportando, y que la región **US915** ya existe en tu ChirpStack. Si no, pídeselo al instructor.

### 10.1 Crear un *Device Profile* (perfil del dispositivo)
Menú **Device profiles ▸ Add device profile**:

| Campo | Valor |
|---|---|
| Name | `Radiosonda PICARO` |
| Region | `US915` |
| MAC version | `LoRaWAN 1.0.3` (o `1.0.4`) — es la familia **1.0.x** |
| Regional parameters revision | `A` |
| Join (OTAA/ABP) | **OTAA** (activado, es el default) |
| Class | `A` |

Luego, en la pestaña **Codec** del mismo perfil:
- **Payload codec** → `JavaScript functions`.
- Pega **todo** el contenido de **`chirpstack/decoder.js`** y **guarda**.

### 10.2 Crear (o abrir) una *Application*
Menú **Applications ▸ Add application** → ponle un nombre (ej. `curso-lora`) y guarda.

### 10.3 Agregar el *Device*
Dentro de tu aplicación → **Add device**:

| Campo | Valor |
|---|---|
| Name | `radiosonda-01` |
| Device EUI (DevEUI) | el que imprimió el Monitor Serie, ej. `70b3d57ed0066a4c` |
| Join EUI | el mismo de `config_lorawan.h` (ej. `0000000000000000`) |
| Device profile | `Radiosonda PICARO` (el que creaste) |

Guarda. Después ve a la pestaña **OTAA keys** del dispositivo:

| Campo | Valor |
|---|---|
| **Application key** | tu **AppKey** (ej. `00112233445566778899aabbccddeeff`) |

> En LoRaWAN **1.0.x** ChirpStack solo pide **Application key** (que es tu AppKey). No hay que llenar "Network key". Si tu perfil quedó en 1.1 por error, aparecerá un campo extra y el join fallará: regrésalo a 1.0.x.

### 10.4 Encender y observar el join
- Reinicia la T-Beam (botón **RST**).
- En ChirpStack, entra al dispositivo → pestaña **LoRaWAN frames**: deberías ver un **JoinRequest** y un **JoinAccept**.
- En la pestaña **Events** verás el dispositivo pasar a estado **activo** y llegar los **uplinks**.

---

## 11. Paso 9 — Ver los datos decodificados

Con el `decoder.js` cargado, en el dispositivo → pestaña **Events** → abre un evento **`up`**. En el campo **`object`** verás algo así:

```json
{
  "gps_fix": true,
  "latitude": 19.432608,
  "longitude": -99.133209,
  "altitude_m": 2240,
  "satellites": 9,
  "battery_v": 4.102,
  "battery_pct": 89,
  "charging": false,
  "usb_powered": true
}
```

🎉 ¡Eso es la telemetría de tu radiosonda llegando a ChirpStack y ya traducida a números legibles!

---

## 12. Estructura del payload (los bytes que se envían)

Son **15 bytes**, en orden **big-endian** (byte más significativo primero). Definidos en `radiosonda_picaro.ino` (función `buildPayload`) y leídos en `chirpstack/decoder.js`. **Si cambias uno, cambia el otro.**

| Byte(s) | Campo | Tipo | Codificación |
|:---:|---|---|---|
| 0 | `status` | uint8 | bit0 = GPS con fix · bit1 = cargando · bit2 = USB conectado |
| 1–4 | `latitud` | int32 | grados × 1 000 000 |
| 5–8 | `longitud` | int32 | grados × 1 000 000 |
| 9–10 | `altitud` | int16 | metros |
| 11 | `satélites` | uint8 | número de satélites |
| 12–13 | `batería_mv` | uint16 | milivolts |
| 14 | `batería_pct` | uint8 | 0–100 (255 = desconocido) |

---

## 13. Solución de problemas (troubleshooting)

| Síntoma | Causa probable / solución |
|---|---|
| **No aparece el puerto COM** | Falta el driver USB. Instala **CP210x** (Silicon Labs) o **CH9102/CH340** (WCH) según tu placa. Prueba otro cable (que sea de datos). |
| **`text section exceeds available space` al compilar** | La partición es muy chica. Pon **Partition Scheme = Huge APP** (Paso 5). |
| **Errores de compilación por librerías** | Faltó instalar RadioLib / TinyGPSPlus / XPowersLib / U8g2 (Paso 2), o RadioLib es una versión incompatible: usa **7.6.x**. |
| **El upload se queda en `Connecting…`** | Mantén **BOOT/IO0** presionado al iniciar la subida, o baja Upload Speed a `115200`. |
| **`NO LLEGO EL JOIN-ACCEPT`** | (1) DevEUI/JoinEUI/AppKey **no coinciden** con ChirpStack. (2) El **perfil no está en 1.0.x**. (3) El **gateway** no cubre la **sub-banda 2** de US915 o está apagado. (4) Antena mal puesta. |
| **`AUN NO UNIDO` / union falla y reintenta** | Revisa que el gateway reporte a ChirpStack y que la **región US915** coincida en gateway, ChirpStack y placa. Acerca la placa al gateway. |
| **Error de `DevNonce`** (unión rechazada por número repetido) | Pon `#define PICARO_RESET_NONCES 1` en el `.ino`, sube una vez, regrésalo a `0` y vuelve a subir. Como alternativa, borra y vuelve a crear el dispositivo en ChirpStack. |
| **`PAYLOAD DEMASIADO LARGO`** | El ADR bajó el datarate por mala señal. Acerca la placa al gateway; con DR1 o mayor los 15 bytes caben de sobra. |
| **GPS siempre sin fix (lat/lon en 0)** | El GPS necesita cielo abierto y varios minutos en frío. Ponlo junto a una ventana. El resto de la telemetría (batería) sí se envía. |
| **Batería en 0 mV / -1 %** | Sin batería 18650 conectada solo hay lectura por USB; conecta una celda para ver el porcentaje. |

---

## 14. Cómo modificar el proyecto en el futuro

- **Cambiar cada cuánto envía:** ajusta `PICARO_UPLINK_INTERVAL_S` en `config_lorawan.h`.
- **Cambiar de región** (por ejemplo a EU868): cambia `PICARO_REGION` (y deja `PICARO_SUBBAND` como está; en EU868 se ignora). Recuerda igualar el gateway y el Device Profile de ChirpStack.
- **Otro chip de radio** (SX1262 / SX1278): edita `utilities.h` (arriba de todo) y deja activa solo la línea correcta; RadioLib crea el objeto correcto automáticamente.
- **Agregar un dato nuevo al mensaje** (ejemplo: velocidad del GPS):
  1. En `radiosonda_picaro.ino`, dentro de `buildPayload()`, agrega el/los byte(s) al final y aumenta `PICARO_PAYLOAD_SIZE`.
  2. En `chirpstack/decoder.js`, léelo en la misma posición y agrégalo a `data`.
  3. Vuelve a subir el firmware y vuelve a pegar el `decoder.js` en ChirpStack.
- **Agregar un sensor externo** (ej. BME280 de temperatura/humedad/presión): la T-Beam clásica **no** trae ese sensor de fábrica; se conecta por I²C (pines `I2C_SDA=21`, `I2C_SCL=22`) y se lee con su librería. Pídele al instructor la guía de ampliación.

---

*Basado en los ejemplos oficiales de LilyGo (RadioLib LoRaWAN). Licencia MIT.*
*Radio: SX1276 · Región: US915 · Activación: OTAA · LoRaWAN 1.0.x.*
