# 🎈 Radiosonda PICARO — LoRaWAN con LilyGo T-Beam → ChirpStack

> **En una frase:** armas una **"radiosonda"** que se **une por OTAA** a tu red LoRaWAN y envía su
> **posición GPS y batería** a ChirpStack, viéndolo todo por el **Monitor Serie** y la **OLED**.
> **Plataforma:** radio **Semtech SX1276** sobre placa **LilyGo T-Beam (ESP32)**, firmware
> **Arduino/RadioLib**. **Banda:** US915 (sub-banda 2). **Activación:** OTAA · **Clase A** · **LoRaWAN 1.0.x**.
> Está pensada para un estudiante **sin experiencia**: sigue los pasos en orden. Lo único que
> editarás es **`config_lorawan.h`**.

## 🎯 Qué vas a conseguir

> 🏁 **Al terminar verás en ChirpStack el campo `object`** con **lat/lon/altitud/batería** de tu
> radiosonda, ya traducido a números legibles.

Cada 30 s la placa lee GPS + batería, arma un paquete de 15 bytes y lo envía por radio → gateway →
ChirpStack. En el device → *Events* → evento **`up`**, el campo **`object`** se ve así:

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

🎉 Eso es la telemetría de tu radiosonda llegando a ChirpStack y ya decodificada a números legibles.

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, no continúes):

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) (así se levanta ChirpStack) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Un gateway multicanal US915**, encendido y *online* en ChirpStack, con cobertura donde esté la placa.
- [ ] **Hardware:** LilyGo T-Beam (ESP32 + **SX1276**) con **antena LoRa** ⚠️ *(nunca enciendas la radio sin antena)*, cable **USB de datos** (no solo carga), y *(opcional)* batería 18650 + OLED.
- [ ] **Software:** **Arduino IDE 2.x** + **core ESP32** + **4 librerías** (RadioLib, TinyGPSPlus, XPowersLib, U8g2) — los instalas en los **Pasos 1–2**, o con capturas en la [Wiki: Compilar en Arduino](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Compilar-el-TTGO-en-Arduino). Además, acceso web a tu **ChirpStack v4**.

> ⚠️ **AVISO DE BANDA (léelo):** este ejercicio es **US915** y necesita un **gateway multicanal
> US915**. El **gateway de 1 canal del [Ejercicio 07](../07_esp-1ch-gateway/) NO basta** para esta
> radiosonda: con ADR el nodo salta entre los 8 canales de la **sub-banda 2**, y un gateway de un
> solo canal apenas captará una fracción de los uplinks (el join tardará mucho o nunca ocurrirá) —
> **salvo** que fijes el nodo a ese único canal.

> 🧠 **Regla de oro:** lo único que editarás es **`config_lorawan.h`** (tus 3 credenciales). El resto
> es infraestructura que ya funciona.

**Parámetros de red de esta práctica** (ya fijados en el código):

| Parámetro | Valor |
|---|---|
| Región / banda | **US915** |
| Sub-banda | **2** (canales 8–15, el default de ChirpStack) |
| Versión LoRaWAN | **1.0.x** (solo se usa AppKey) |
| Activación / Clase | **OTAA** / **A** |
| fPort de uplink | **10** (`PICARO_UPLINK_FPORT`) |
| Intervalo de envío | **30 s** (`PICARO_UPLINK_INTERVAL_S`) |

## 📟 Hardware y conexiones
**Placa integrada — no hay que cablear nada.** La T-Beam ya trae radio, GPS y PMU (medidor de
batería). ⚠️ **Nunca enciendas la radio sin la antena LoRa conectada.**

Los pines ya están fijados en `utilities.h` con el perfil **`T_BEAM_SX1276`** (no los toques salvo
que cambies de chip de radio):

| Señal | Pin | Señal | Pin |
|---|:--:|---|:--:|
| Radio CS | **18** | Radio SCLK | 5 |
| Radio DIO0 | **26** | Radio MISO | 19 |
| Radio RST | **23** | Radio MOSI | 27 |
| Radio DIO1 | **33** | GPS RX / TX | 34 / 12 |
| I²C SDA / SCL | 21 / 22 | Botón / LED | 38 / 4 |

## 🪜 Paso a paso

> ⏩ **¿Ya compilas sketches ESP32 en Arduino?** Los pasos **1, 2, 5 y 6** son el flujo estándar de
> Arduino (instalar IDE/core/librerías, elegir placa, flashear). Puedes seguirlos con capturas en la
> [Wiki: Compilar en Arduino](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Compilar-el-TTGO-en-Arduino)
> *(la Wiki usa una TTGO, pero el flujo de la T-Beam es idéntico)* y concentrarte en lo propio de este
> ejercicio: los pasos **3, 4 y 7–9**.

### 1. Instala Arduino IDE + soporte ESP32

1. Descarga e instala **Arduino IDE** desde <https://www.arduino.cc/en/software>.
2. Abre Arduino IDE → **File ▸ Preferences**.
3. En **"Additional boards manager URLs"** pega:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Ve a **Tools ▸ Board ▸ Boards Manager…**, busca **`esp32`** (autor *Espressif Systems*) e instala la versión **3.x** (o 2.0.x).

### 2. Instala las librerías

En Arduino IDE ve a **Tools ▸ Manage Libraries…** (o el icono de libros 📚) e instala estas cuatro:

| Librería | Autor | Notas |
|---|---|---|
| **RadioLib** | Jan Gromes | Instala **7.6.0** (o la 7.x más nueva). El código está probado con 7.6.0. |
| **TinyGPSPlus** | Mikal Hart | Lee el GPS. |
| **XPowersLib** | lewisxhe (Lewis He) | Controla el PMU (energía y batería). |
| **U8g2** | oliver | Pantalla OLED. Necesaria aunque no tengas OLED (el código la incluye). |

> Las demás (`WiFi`, `SPI`, `Wire`, `FS`) ya vienen con el paquete ESP32; no instales nada extra.

### 3. Abre el proyecto y entiende el código

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

### 4. Cambia DevEUI, JoinEUI y AppKey

Abre la pestaña **`config_lorawan.h`**. Ahí están las 3 credenciales. Este es el orden recomendado:

> Puedes **inventar** estos valores y luego copiarlos a ChirpStack, **o** crear primero el dispositivo en ChirpStack y copiar de ahí. Lo importante es que **los valores en la placa y en ChirpStack sean idénticos**.

**4.1 DevEUI (identificador del dispositivo, 8 bytes)**
```c
#define PICARO_DEV_EUI   0x70B3D57ED0066A4C
```
- Se escribe como `0x` + **16 dígitos** hexadecimales, en orden **MSB primero**.
- Es el mismo texto que ves en ChirpStack cuando el botón de orden está en **MSB**.
  - ChirpStack muestra: `70 B3 D5 7E D0 06 6A 4C` → aquí escribes `0x70B3D57ED0066A4C`.

**4.2 JoinEUI (antes AppEUI, 8 bytes)**
```c
#define PICARO_JOIN_EUI  0x0000000000000000
```
- Para practicar se deja en **todo ceros** (lo más común). Solo asegúrate de poner **el mismo valor** en ChirpStack.

**4.3 AppKey (llave secreta, 16 bytes)**
```c
#define PICARO_APP_KEY   0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, \
                         0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
```
- Son **16 bytes** separados por comas, cada uno con `0x`, en orden **MSB primero**.
- ⚠️ Es **secreta**: no la subas a internet ni la compartas.

> No cambies `PICARO_REGION`, `PICARO_SUBBAND` ni los demás parámetros: ya están en **US915 / sub-banda 2**. El intervalo de envío (`PICARO_UPLINK_INTERVAL_S`, 30 s) sí lo puedes ajustar.

Guarda el archivo (**Ctrl+S**).

### 5. Selecciona la tarjeta y compila (build)

**5.1 Seleccionar la tarjeta correcta.** La T-Beam clásica usa el chip **ESP32**, así que la tarjeta a elegir se llama **"ESP32 Dev Module"**. Hay dos formas de seleccionarla:

**Forma A — desde el menú (Arduino IDE 1.8 y 2.x):**
1. Ve al menú **Tools ▸ Board ▸ esp32** (el grupo que instalaste en el Paso 1; puede aparecer como *"esp32"* o *"ESP32 Arduino"*).
2. En la lista larga que se despliega, busca y haz clic en **"ESP32 Dev Module"** (suele estar entre las primeras).

**Forma B — desde el selector de la barra superior (solo Arduino IDE 2.x):**
1. Arriba, al centro, hay un desplegable de tarjetas/puerto. Haz clic y elige **"Select other board and port…"**.
2. En la columna **"BOARDS"** escribe `ESP32 Dev Module`, haz clic en el resultado y confirma con **OK**.

> ⚠️ **No te confundas de tarjeta:** debe ser **"ESP32 Dev Module"** tal cual. **NO** elijas *"ESP32**S3** Dev Module"*, *"ESP32**C3**"*, *"TTGO T1"* ni un *"T-Beam"* de otro paquete. Para esta placa clásica, la opción probada es **"ESP32 Dev Module"**.

**5.2 Ajustar las opciones de la tarjeta.** Al seleccionar la tarjeta, Arduino IDE muestra más opciones abajo en el menú **Tools**. Ajústalas **exactamente** así (haz clic en cada una y elige el valor):

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

**5.3 Compilar.** Presiona el botón **✓ Verify** (la palomita, arriba a la izquierda). Si termina con **"Done compiling"**, ¡el build salió bien! Si hay errores rojos, mira la sección **🛠️ Si algo falla** (más abajo).

### 6. Conecta, elige el Puerto y flashea

**6.1 Conectar la placa.**
1. Conecta la T-Beam a la PC con un cable USB **que transmita datos** (no de solo carga).
2. Si es la primera vez en Windows, espera unos segundos a que termine de instalar el driver USB.

**6.2 Seleccionar el Puerto (COM).** El **"Puerto"** es el canal por el que la PC habla con la placa. Para saber cuál es el tuyo:

> 💡 **Truco infalible:** con la placa **desconectada**, abre **Tools ▸ Port** y fíjate qué puertos hay. **Conecta** la placa, vuelve a abrir **Tools ▸ Port**: el puerto **nuevo** que apareció es el de tu T-Beam.

- **Windows:** aparece como `COM3`, `COM4`, `COM5`, … Para confirmar cuál es, abre el **Administrador de dispositivos** (clic derecho en el botón *Inicio* ▸ *Administrador de dispositivos*) → **Puertos (COM y LPT)**. Verás algo como *"Silicon Labs CP210x USB to UART (COM5)"* o *"USB-Enhanced-SERIAL CH9102 (COM5)"*. Ese `COMx` es tu puerto.
- **Linux:** suele ser `/dev/ttyUSB0`. Si da error de permisos: `sudo usermod -aG dialout $USER` y vuelve a iniciar sesión.
- **macOS:** `/dev/cu.SLAB_USBtoUART` o `/dev/cu.wchusbserial…`.

Selecciónalo en **Tools ▸ Port ▸ (tu COMx)**, o desde el desplegable de la barra superior en Arduino IDE 2.x.

> **¿No aparece ningún puerto nuevo?** Casi siempre es una de estas tres causas:
> 1. **Falta el driver USB.** Instala **CP210x** (Silicon Labs) o **CH9102/CH340** (WCH), según el chip que viste en el Administrador de dispositivos. Reconecta la placa después de instalar.
> 2. **Cable de solo carga.** Cámbialo por uno que transmita datos.
> 3. **Puerto USB con problemas.** Prueba otro puerto USB de la PC, de preferencia directo (sin hub).

**6.3 Subir el programa (flashear).**
1. Verifica arriba que estén seleccionadas **la tarjeta** ("ESP32 Dev Module") **y el puerto** correctos.
2. Presiona el botón **→ Upload** (la flecha, junto a la palomita).
3. Espera a que aparezca **"Writing at 0x… (100 %)"** y luego **"Hard resetting via RTS pin…"**. Eso significa que se flasheó con éxito.

> La T-Beam normalmente entra sola en modo de programación (auto-boot). Si el upload se queda pegado en "Connecting……....", mantén presionado el botón **BOOT/IO0** de la placa mientras empieza a subir (suéltalo al ver "Writing"), o baja el **Upload Speed** a `115200`.

### 7. Abre el Monitor Serie

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
  Payload (hex): 040000000000000000000000100659
  Enviando uplink...
  Uplink enviado (sin downlink).
  RSSI: -47 dBm   SNR: 9.5 dB   Uplink #1
  Siguiente uplink en 30 s.
```

> 📌 **Copia el bloque "DATOS PARA DAR DE ALTA EN CHIRPSTACK"**: lo usarás en el siguiente paso.
> El GPS puede tardar **varios minutos** en obtener "fix" la primera vez (arranque en frío). Mientras
> tanto envía lat/lon en 0 — es normal. Colócalo cerca de una ventana o al aire libre.
> El uplink sale en **fPort 10**; el hex de arriba es solo un ejemplo — **el valor exacto depende de
> tu batería y GPS**.

### 8. Da de alta el dispositivo en ChirpStack

> Se asume que el **gateway multicanal US915 ya está agregado** y reportando, y que la región
> **US915** ya existe en tu ChirpStack. Si no, revisa el [Ejercicio 00](../00_chirpstack-docker/) o
> pídeselo al instructor.

**8.1 Crear un *Device Profile*.** Menú **Device profiles ▸ Add device profile**:

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

**8.2 Crear (o abrir) una *Application*.** Menú **Applications ▸ Add application** → ponle un nombre (ej. `curso-lora`) y guarda.

**8.3 Agregar el *Device*.** Dentro de tu aplicación → **Add device**:

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

> En LoRaWAN **1.0.x** ChirpStack solo pide **Application key** (que es tu AppKey). No hay que llenar "Network key". Si tu perfil quedó en 1.1 por error, aparecerá un campo extra y el join fallará: regrésalo a 1.0.x. *(Si en vez de la web usas la API REST / `provision.sh`, esa misma clave se envía en el campo `nwkKey` — ver la sección **📎 Referencia** al final.)*

**8.4 Encender y observar el join.**
- Reinicia la T-Beam (botón **RST**).
- En ChirpStack, entra al dispositivo → pestaña **LoRaWAN frames**: deberías ver un **JoinRequest** y un **JoinAccept**, y luego los **uplinks** (llegan en **fPort 10**).
- En la pestaña **Events** verás el dispositivo pasar a estado **activo** y llegar los eventos **`up`**.

### 9. Ve los datos decodificados

Con el `decoder.js` cargado (Paso 8), en el dispositivo → pestaña **Events** → abre un evento **`up`**.
En el campo **`object`** verás la telemetría ya traducida a números — **el mismo JSON que viste
arriba en 🎯 Qué vas a conseguir** (lat/lon/altitud/batería). 🎉

## ✅ Cómo saber que funcionó
- [ ] En el **Monitor Serie** aparece **`JOIN EXITOSO! Ya estamos en la red.`** y un `DevAddr asignado`.
- [ ] En ChirpStack (device → *LoRaWAN frames*) ves el **JoinRequest → JoinAccept** y, después, los **uplinks** (en **fPort 10**).
- [ ] En *Events* → evento **`up`**, el campo **`object`** trae la telemetría **decodificada** (lat/lon/altitud/batería).

## 🛠️ Si algo falla

| Síntoma | Causa probable / solución |
|---|---|
| **No aparece el puerto COM** | Falta el driver USB. Instala **CP210x** (Silicon Labs) o **CH9102/CH340** (WCH) según tu placa. Prueba otro cable (que sea de datos). |
| **`text section exceeds available space` al compilar** | La partición es muy chica. Pon **Partition Scheme = Huge APP** (Paso 5). |
| **Errores de compilación por librerías** | Faltó instalar RadioLib / TinyGPSPlus / XPowersLib / U8g2 (Paso 2), o RadioLib es una versión incompatible: usa **7.6.x**. |
| **El upload se queda en `Connecting…`** | Mantén **BOOT/IO0** presionado al iniciar la subida, o baja Upload Speed a `115200`. |
| **`NO LLEGO EL JOIN-ACCEPT`** | (1) DevEUI/JoinEUI/AppKey **no coinciden** con ChirpStack. (2) El **perfil no está en 1.0.x**. (3) El **gateway** no cubre la **sub-banda 2** de US915, está apagado, **o es de 1 canal (ej.07) y pierde la mayoría de los uplinks**. (4) Antena mal puesta. |
| **`AUN NO UNIDO` / union falla y reintenta** | Revisa que el gateway reporte a ChirpStack y que la **región US915** coincida en gateway, ChirpStack y placa. Acerca la placa al gateway. |
| **Error de `DevNonce`** (unión rechazada por número repetido) | Pon `#define PICARO_RESET_NONCES 1` en el `.ino`, sube una vez, regrésalo a `0` y vuelve a subir. Como alternativa, borra y vuelve a crear el dispositivo en ChirpStack. |
| **`PAYLOAD DEMASIADO LARGO`** | El ADR bajó el datarate por mala señal. Acerca la placa al gateway; con DR1 o mayor los 15 bytes caben de sobra. |
| **GPS siempre sin fix (lat/lon en 0)** | El GPS necesita cielo abierto y varios minutos en frío. Ponlo junto a una ventana. El resto de la telemetría (batería) sí se envía. |
| **Batería en 0 mV / -1 %** | Sin batería 18650 conectada solo hay lectura por USB; conecta una celda para ver el porcentaje. |

## 📤 Los datos (payload)
- **fPort:** **10** (definido en `config_lorawan.h` como `PICARO_UPLINK_FPORT`). ChirpStack muestra este puerto junto a cada frame; el codec lo usa para saber cómo interpretar los bytes.
- **Tamaño:** **15 bytes**, en orden **big-endian** (byte más significativo primero). Definidos en `radiosonda_picaro.ino` (`buildPayload`) y leídos en `chirpstack/decoder.js`. **Si cambias uno, cambia el otro.**

| Byte(s) | Campo | Tipo | Codificación |
|:---:|---|---|---|
| 0 | `status` | uint8 | bit0 = GPS con fix · bit1 = cargando · bit2 = USB conectado |
| 1–4 | `latitud` | int32 | grados × 1 000 000 |
| 5–8 | `longitud` | int32 | grados × 1 000 000 |
| 9–10 | `altitud` | int16 | metros |
| 11 | `satélites` | uint8 | número de satélites |
| 12–13 | `batería_mv` | uint16 | milivolts |
| 14 | `batería_pct` | uint8 | 0–100 (255 = desconocido) |

**Ejemplo** (sin fix de GPS, alimentado por USB): `040000000000000000000000100659`
- `04` → `status`: USB conectado, sin fix, sin carga.
- `00000000 00000000 0000 00` → latitud, longitud, altitud y satélites en 0 (aún sin fix).
- `1006` → batería `0x1006` = **4102 mV**.
- `59` → batería `0x59` = **89 %**.

> El firmware imprime el hex **sin espacios**. El valor exacto **depende de tu batería y tu GPS**:
> cuando el GPS tome fix, los bytes de lat/lon/altitud/satélites dejarán de ser 0.

## 🔧 Cómo modificarlo en el futuro

- **Cambiar cada cuánto envía:** ajusta `PICARO_UPLINK_INTERVAL_S` en `config_lorawan.h`.
- **Cambiar de región** (por ejemplo a EU868): cambia `PICARO_REGION` (y deja `PICARO_SUBBAND` como está; en EU868 se ignora). Recuerda igualar el gateway y el Device Profile de ChirpStack.
- **Otro chip de radio** (SX1262 / SX1278): edita `utilities.h` (arriba de todo) y deja activa solo la línea correcta; RadioLib crea el objeto correcto automáticamente.
- **Agregar un dato nuevo al mensaje** (ejemplo: velocidad del GPS):
  1. En `radiosonda_picaro.ino`, dentro de `buildPayload()`, agrega el/los byte(s) al final y aumenta `PICARO_PAYLOAD_SIZE`.
  2. En `chirpstack/decoder.js`, léelo en la misma posición y agrégalo a `data`.
  3. Vuelve a subir el firmware y vuelve a pegar el `decoder.js` en ChirpStack.
- **Agregar un sensor externo** (ej. BME280 de temperatura/humedad/presión): la T-Beam clásica **no** trae ese sensor de fábrica; se conecta por I²C (pines `I2C_SDA=21`, `I2C_SCL=22`) y se lee con su librería. Pídele al instructor la guía de ampliación.

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 07 · Gateway 1 canal](../07_esp-1ch-gateway/)
- 🏁 **Último ejercicio del curso.**
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/Ejercicio-08-Radiosonda-PICARO)

## 📎 Referencia
- **Archivos de esta carpeta:**
  ```
  radiosonda_picaro.ino   ← programa principal (la lógica). MUY comentado.
  config_lorawan.h        ← ⭐ AQUÍ cambias DevEUI / JoinEUI / AppKey.
  utilities.h             ← pines de la placa (perfil "T_BEAM_SX1276").
  LoRaBoards.h / .cpp     ← "motor" de la placa: enciende PMU, GPS y OLED.
  chirpstack/decoder.js   ← codec que "traduce" el payload en ChirpStack.
  ```
- **Guías comunes:** [ChirpStack API (REST · MQTT · codec)](../COMMON_CHIRPSTACK_API.md) · [Wiki: Compilar en Arduino](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Compilar-el-TTGO-en-Arduino).
- **UI vs REST — la misma clave, dos nombres:** en la **web** de ChirpStack, la AppKey va en el campo **"Application key"** (pestaña *OTAA keys*). Por **REST / `provision.sh`** ese mismo valor va en el campo **`nwkKey`**. Ambas son **correctas y equivalentes** en LoRaWAN 1.0.x — no te confundas si lees las dos.

---
> 🧩 **Código:** basado en los ejemplos oficiales de LilyGo (RadioLib LoRaWAN), bajo **licencia MIT**.
> Radio: SX1276 · Región: US915 · Activación: OTAA · LoRaWAN 1.0.x.
>
> 📄 Material educativo (esta guía) bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
