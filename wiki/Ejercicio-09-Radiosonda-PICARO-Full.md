# Ejercicio 09 · Radiosonda PICARO Full (T-Beam Supreme + ESP-IDF)

El **ejercicio culminante**: una sola placa integrada (**LILYGO T-Beam Supreme**, ESP32-S3 + SX1262)
programada con **ESP-IDF** que hace **OTAA join** a ChirpStack, envía telemetría (GPS, temperatura,
presión, batería) y **guarda todo en la microSD**. Esta página cubre los **fundamentos** y la
**integración E2E con ChirpStack**; el paso a paso del código está en el
[`README.md` del ejercicio](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/main/specs/exercises/09_radiosonda_picaro_full).

## 🧠 ¿Qué lo hace distinto?
| | Ejercicios 05/06/08 (TTGO) | **Ejercicio 09 (T-Beam Supreme)** |
|---|---|---|
| MCU | ESP32 clásico | **ESP32-S3** (WiFi/BLE 5, PSRAM) |
| Radio | SX1276/SX1262 | **SX1262** (con TCXO) |
| Framework | Arduino IDE | **ESP-IDF v5.5** (profesional) |
| Stack LoRaWAN | RadioLib/LMIC (Arduino) | **RadioLib como componente oficial de ESP-IDF** |
| Sensores | 1–2 | GPS + BME280 + IMU + magnetómetro + RTC + PMU |
| Almacenamiento | — | **microSD** (log CSV completo) |

> No se reinventa nada: **RadioLib** es un [componente oficial del registro de Espressif](https://components.espressif.com/components/jgromes/radiolib)
> (`idf.py add-dependency "jgromes/radiolib"`) y trae su propio HAL para ESP-IDF. El ejercicio solo
> añade una capa fina para el ESP32-S3 y los drivers de los sensores.

## 🛠️ El flujo ESP-IDF (para investigar por tu cuenta)
ESP-IDF es el SDK oficial de Espressif. Aprende el flujo con la **documentación oficial** y aplícalo:

1. **Instalar** → [Get Started (v5.5)](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/get-started/index.html)
2. **Configurar target** → `idf.py set-target esp32s3`
3. **Compilar** → `idf.py build` · [Build System](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-guides/build-system.html)
4. **Flashear + monitor** → `idf.py -p COMx flash monitor` · [Tools/idf.py](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-guides/tools/idf-py.html)
5. **Menús de configuración** → `idf.py menuconfig` · [Kconfig](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/kconfig.html)

Periféricos usados (con su doc): [I²C](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/i2c.html) ·
[SPI master](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/spi_master.html) ·
[UART](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/uart.html) ·
[FAT/SD](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/storage/fatfs.html) ·
[NVS](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/storage/nvs_flash.html)

> ⚠️ **Flasheo por USB nativo (ESP32-S3):** la Supreme usa el **USB-Serial/JTAG** interno del chip.
> Si `idf.py flash` falla con *"No serial data received"*, entra a modo descarga a mano
> (**mantén BOOT, pulsa RST, suelta BOOT**) y reintenta. Con el USB nativo, **el COM puede cambiar
> tras cada reset**. Doc: [esptool troubleshooting](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/troubleshooting.html).

## 🔗 Integración E2E con ChirpStack (paso a paso)
El objetivo E2E: **placa → gateway → ChirpStack → `object` decodificado**. Necesitas ChirpStack
corriendo ([Ejercicio 00](Ejercicio-00-ChirpStack)) y un **gateway multicanal US915 (sub-banda 2)**.

### 1) Las tres credenciales OTAA
LoRaWAN 1.0.x usa: **DevEUI** (id del equipo), **JoinEUI** (id de app, aquí ceros) y **AppKey**
(secreta). Deben ser **idénticas** en la placa (`config_lorawan.h`) y en ChirpStack. Valores del ejercicio:

| | Valor |
|---|---|
| DevEUI | `70b3d57ed0090901` |
| JoinEUI | `0000000000000000` |
| AppKey | `50494341524f3039a5a5a5a5a5a5a5a5` |

### 2) Crear el Device Profile (con el codec)
*Device profiles ▸ Add*: **Region** `US915`, **MAC version** `LoRaWAN 1.0.4`, **Reg. params**
`RP002-1.0.3`, **OTAA** activado, **Class** `A`. En la pestaña **Codec** elige *JavaScript functions*
y pega **todo** [`chirpstack/decoder.js`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/main/specs/exercises/09_radiosonda_picaro_full/chirpstack/decoder.js).
El codec traduce los **19 bytes** del payload a campos legibles.

### 3) Crear el Device y su AppKey
En tu **Application** → *Add device*: pon el **DevEUI**, el **Join EUI** y elige el device profile
anterior. Guarda y ve a **OTAA keys** → **Application key** = la AppKey de arriba.
> En **1.0.x** solo se pide *Application key*. Por REST (`provision.sh`) esa misma clave va en `nwkKey`.

### 4) Encender y ver el join
Reinicia la placa. En el device:
- **LoRaWAN frames** → `JoinRequest` → `JoinAccept`, y luego **uplinks en fPort 10**.
- **Events → up** → el campo **`object`** con `temperature_c`, `pressure_hpa`, `battery_v`, `gps_fix`…

### 5) Consumir los datos
Los payloads llegan por **MQTT** en `application/<APP_ID>/device/<DevEUI>/event/up` (el REST no guarda
histórico de payloads). Ver [COMMON: ChirpStack API](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/main/specs/exercises/COMMON_CHIRPSTACK_API.md).
Atajo por API en vez de la web: `export TOKEN=...; ./provision.sh`.

## 📦 El payload y la relación payload ↔ datarate
Por defecto **19 bytes** (status, lat, lon, alt, sats, batería mV/%, temperatura, presión). En US915,
**DR0/SF10** solo admite ~11 bytes: con 19 necesitas **DR1+**. Juega en `sensores_config.h` activando o
quitando campos y observa cómo cambia lo que cabe en el aire. **La microSD guarda siempre la telemetría
completa** (todos los sensores), quepa o no en el uplink.

## 🧩 Sensores: activos vs. codificados
Activos por defecto: **SX1262, L76K (GPS), BME280, AXP2101 (batería), QMC6310 (magnetómetro), OLED,
microSD**. Codificados pero **apagados**: **PCF8563** (RTC) y demos **WiFi/BLE**. Cada uno tiene su
driver comentado en `firmware/main/`.

> ⚠️ **IMU QMI8658:** codificado pero **apagado** por defecto. Va en el bus **SPI3 compartido con la
> microSD** y en la unidad probada da lecturas inestables (`WHO_AM_I` intermitente 0x05/0x3E) — un
> problema de integridad de señal que necesita analizador lógico para depurar. El magnetómetro sí
> quedó activo y da rumbo real.

## 📟 GPS y estado en pantalla
El GPS se puede apagar por botón (ver [botones del T-Beam en Meshtastic](https://meshtastic.org/docs/hardware/devices/lilygo/tbeam/buttons/)).
El firmware detecta si el módulo **está enviando NMEA** (`gps_active`) y lo muestra en el OLED:
`GPS FIX`, `GPS ON s/fix` o `GPS: APAGADO`.

## 🗃️ Caja negra (microSD)
Registra **una fila por ciclo aunque NO haya join/enlace** (independiente de LoRaWAN), en
`/sdcard/PICARO.CSV`, con 25 columnas (incluida `gps_active`, que marca si el GPS estaba apagado).

---
- 🏠 [Índice de la Wiki](Home) · ⬅️ [Ejercicio 08](Ejercicio-08-Radiosonda-PICARO)
- 📂 [Código del ejercicio 09](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/main/specs/exercises/09_radiosonda_picaro_full)

_Docs © Omar Velazquez · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)_
