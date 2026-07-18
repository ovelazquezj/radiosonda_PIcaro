# 🛰️ Ejercicio 09 — Radiosonda PICARO FULL (T-Beam Supreme + ESP-IDF → ChirpStack)

> **En una frase:** compilas con **ESP-IDF** un firmware para la **LILYGO T-Beam Supreme**
> (ESP32-S3 + **SX1262**) que **se une por OTAA** a tu red LoRaWAN, envía **telemetría** (GPS,
> temperatura, presión y batería) a **ChirpStack**, y **guarda la telemetría completa en la microSD**.
> **Plataforma:** radio **Semtech SX1262** · placa **T-Beam Supreme (ESP32-S3)** · stack **RadioLib**
> sobre **ESP-IDF v5.5**. **Banda:** US915 sub-banda 2 · **OTAA** · **Clase A** · **LoRaWAN 1.0.x**.

Este es el ejercicio **culminante** del curso: reúne todo (radio, GNSS, sensores, energía,
almacenamiento y LoRaWAN) en una sola placa integrada y sobre el framework profesional de Espressif.

---

## 🎯 Qué vas a conseguir

> 🏁 Al terminar verás en ChirpStack el campo **`object`** con tu telemetría ya decodificada, y en la
> **microSD** un `PICARO.CSV` con **todos** los sensores. Ejemplo real de un `up` por MQTT:

```json
{
  "gps_fix": false,
  "satellites": 0,
  "altitude_m": 0,
  "temperature_c": 26.12,
  "pressure_hpa": 815.5,
  "battery_v": 0.46,
  "battery_pct": null,
  "charging": false,
  "usb_powered": true
}
```

(Con el GPS a cielo abierto verás también `latitude`/`longitude`. La batería sale realista al
conectar una celda 18650; por USB sin celda se ve ~0.)

## 🧩 Qué demuestra (todas las capacidades de la placa)

| Bloque | Chip | En este firmware |
|---|---|---|
| MCU | ESP32-S3 (WiFi/BLE) | app ESP-IDF; WiFi/BLE **codificados y apagados** (demo opcional) |
| Radio LoRa | **SX1262** | LoRaWAN OTAA US915 (RadioLib) — **activo** |
| GNSS | **L76K** | lat/lon/alt/sats por UART — **activo** |
| Ambiental | **BME280** | temperatura + presión (+humedad apagada) — **activo** |
| Energía | **AXP2101** | rieles + batería mV/% — **activo** (va primero) |
| IMU | **QMI8658** | driver escrito — **apagado** (SPI inestable en el bus compartido con la SD en la unidad probada; ver nota abajo) |
| Magnetómetro | **QMC6310** | driver escrito — **ACTIVO** (rumbo real en la caja negra) |
| RTC | **PCF8563** | driver escrito — **apagado** |
| Display | **SH1106** | estado en OLED — **activo** |
| Almacenamiento | **microSD** | `PICARO.CSV` con telemetría COMPLETA — **activo** |

> 💡 **Idea del ejercicio:** *todo* está codificado, pero solo se **activa** lo que entra en la
> telemetría. Tú enciendes/apagas sensores y campos del payload en **`firmware/main/sensores_config.h`**
> y aprendes la relación **payload ↔ datarate**.

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir:

- [ ] **ChirpStack v4 corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Gateway multicanal US915** (sub-banda 2, canales 8–15) *online* y con cobertura. ⚠️ El gateway de **1 canal** del [Ejercicio 07](../07_esp-1ch-gateway/) **no basta** (con ADR el nodo salta entre 8 canales).
- [ ] **Hardware:** **LILYGO T-Beam Supreme** con **antena LoRa** ⚠️ *(nunca transmitas sin antena)*, **cable USB-C de datos**, y opcional **18650** + **antena GPS** + **microSD**.
- [ ] **ESP-IDF v5.5** instalado → **doc oficial**: <https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/get-started/index.html>

## 📟 Hardware y pinout
**Placa integrada — no cableas nada.** Pines fijados en `firmware/main/board_pins.h` (fuente: wiki y
`utilities.h` oficiales de LILYGO, perfil `T_BEAM_S3_SUPREME_SX1262`):

| Señal | GPIO | Señal | GPIO |
|---|:--:|---|:--:|
| LoRa SCK/MISO/MOSI | 12/13/11 | LoRa NSS/RST/BUSY/DIO1 | 10/5/4/1 |
| GPS RX(MCU)/TX(MCU) | 9/8 | GPS EN/PPS | 7/6 |
| I²C0 (sensores/OLED) SDA/SCL | 17/18 | I²C1 (PMU/RTC) SDA/SCL | 42/41 |
| SPI3 (IMU+SD) SCK/MISO/MOSI | 36/37/35 | IMU CS / SD CS | 34/47 |

> ⚠️ Detalle clave: el **SX1262 usa TCXO** (RadioLib lo configura con `setTCXO(1.8)`), y el **AXP2101
> debe encenderse PRIMERO** (el radio se alimenta del riel ALDO3). El firmware ya lo hace en ese orden.

## 🪜 Paso a paso

### 1. Instala ESP-IDF v5.5
Sigue la **guía oficial** (Windows/Linux/macOS): <https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/get-started/index.html>
Al terminar, abre el **"ESP-IDF Terminal"** (o ejecuta `export.ps1`/`export.sh`) para tener `idf.py` en el PATH.

### 2. Configura tus credenciales
Edita **`firmware/main/config_lorawan.h`** (es el **único** archivo que tocas):

```c
#define PICARO_DEV_EUI    0x70B3D57ED0090901ULL   // Device EUI (MSB)
#define PICARO_JOIN_EUI   0x0000000000000000ULL   // Join EUI (ceros para practicar)
#define PICARO_APP_KEY    { 0x50,0x49,0x43,0x41,0x52,0x4F,0x30,0x39, \
                            0xA5,0xA5,0xA5,0xA5,0xA5,0xA5,0xA5,0xA5 }  // AppKey (secreta)
```
> Puedes inventarlos y luego darlos de alta en ChirpStack (paso 6), o crear primero el device en
> ChirpStack y copiarlos aquí. Lo importante: **placa y ChirpStack idénticos**.

### 3. Compila (build)
```bash
# desde firmware/
idf.py set-target esp32s3
idf.py build
```
**Salida esperada (final):** `Project build complete.` y un `radiosonda_picaro_full.bin`.
La primera vez, el **IDF Component Manager** descarga **RadioLib** solito (no portas nada).
📚 Build system: <https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-guides/build-system.html>

### 4. Flashea a la placa
Averigua el puerto (Windows `COMx`, Linux `/dev/ttyACM0`) y:
```bash
idf.py -p COM17 flash monitor
```
> ⚠️ **La T-Beam Supreme usa el USB nativo del ESP32-S3 (USB-Serial/JTAG).** Si el flasheo falla con
> **`Failed to connect ... No serial data received`**, entra a **modo descarga a mano**:
> **mantén pulsado BOOT**, pulsa y suelta **RST**, **suelta BOOT**, y vuelve a lanzar el flash.
> Además, con el USB nativo **el número de COM puede cambiar tras cada reset** — vuelve a mirarlo.
> 📚 esptool/flashing: <https://docs.espressif.com/projects/esptool/en/latest/esp32s3/>

### 5. Observa el arranque (monitor serie)
`idf.py -p COMx monitor` (sal con `Ctrl+]`). Verás algo así:
```
#   RADIOSONDA PICARO FULL - arrancando...
pmu: AXP2101 detectado ... Rieles ON @3.3V: ALDO3=LoRa ALDO4=GPS ...
bme280: detectado chip-id 0x60 (BME280)
sdcard: microSD montada: 968 MB. CSV nuevo
gps: L76K en UART1 ... Esperando fix...
======= DAR DE ALTA EN CHIRPSTACK =======
  DevEUI  (MSB): 70 B3 D5 7E D0 09 09 01
lorawan: JOIN EXITOSO (sesion nueva). Ya estamos en la red.
---- ciclo #1 ----
  bat 388 mV(0%) USB=1 | 26.20C 815.5hPa | GPS -- sats=0 | payload=19B
  perifericos: SD=1 OLED=1 BME280=1 joined=1
lorawan: Uplink enviado (sin downlink).
```
> El GPS puede tardar **minutos** en el primer fix (colócalo junto a una ventana). El resto de la
> telemetría se envía igual.

### 6. Da de alta el dispositivo en ChirpStack (tú, por la web)
> Se asume el **gateway US915 ya reportando** y la región **US915** existente (ej. 00).

**6.1 Device Profile.** *Device profiles ▸ Add*:

| Campo | Valor |
|---|---|
| Name | `Radiosonda-PICARO-Full-US915` |
| Region | `US915` |
| MAC version | `LoRaWAN 1.0.4` (familia **1.0.x**) |
| Regional parameters | `RP002-1.0.3` |
| Join (OTAA) | activado |
| Class | `A` |

Luego, pestaña **Codec** → `JavaScript functions` → pega **todo** `chirpstack/decoder.js` y guarda.

**6.2 Application.** Usa tu app (o *Applications ▸ Add*).

**6.3 Device.** Dentro de la app → *Add device*:

| Campo | Valor |
|---|---|
| Device EUI | `70b3d57ed0090901` |
| Join EUI | `0000000000000000` |
| Device profile | `Radiosonda-PICARO-Full-US915` |

Guarda → pestaña **OTAA keys** → **Application key** = `50494341524f3039a5a5a5a5a5a5a5a5`.
> En **1.0.x** ChirpStack solo pide **Application key**. (Por REST/`provision.sh` esa misma clave va en el campo `nwkKey`.)

**6.4 Observa el join.** Reinicia la placa (RST). En el device → **LoRaWAN frames** verás
`JoinRequest → JoinAccept` y luego **uplinks en fPort 10**; en **Events → up**, el campo **`object`**
trae la telemetría decodificada.

## ✅ Cómo saber que funcionó
- [ ] En el monitor: **`JOIN NUEVO por aire`** (o `Sesion RESTAURADA`) y `perifericos: SD=1 OLED=1 BME280=1`.
- [ ] En ChirpStack: **JoinRequest → JoinAccept**, luego uplinks en **fPort 10**.
- [ ] En **Events → up**, el `object` trae `temperature_c`, `pressure_hpa`, `battery_v`, etc.
- [ ] En la **microSD**: existe `PICARO.CSV` con una fila por ciclo (todos los sensores).

## 🔒 ¿El join es REAL? (sin falsos positivos)
Un detalle crítico de LoRaWAN: **una "sesión" guardada NO prueba que haya gateway en rango**. El
firmware puede *restaurar* una sesión previa desde la NVS y seguir funcionando aunque estés lejos del
gateway. Por eso este ejercicio **distingue y muestra honestamente**:

| Lo que ves | Significado |
|---|---|
| `JOIN NUEVO OK` | Hubo **handshake OTAA real por aire** (JoinAccept del gateway). Enlace confirmado. |
| `SESION REST.` | Se reusó una sesión guardada en NVS. **Aún NO confirmado** que haya red. |
| `ENLACE OK` | Se recibió un **ACK/downlink real** (uplink confirmado). Estás **de verdad** conectado. |
| `SIN ENLACE` | Se pidió ACK y **no llegó** → no hay gateway en rango (aunque haya sesión). |

**Cómo lo logra:** tras el join, el firmware envía un **uplink confirmado** (`isConfirmed`) que exige
**ACK de la red**; solo si el ACK llega marca `ENLACE OK`. Así, lejos del gateway verás `SIN ENLACE`
(honesto), no un falso "conectado".

**Validación del lado servidor (la prueba definitiva):**
```bash
# El contador de uplinks DEBE subir en el tiempo si ChirpStack recibe de verdad:
curl -s "$API/api/devices/70b3d57ed0090901/activation" -H "Authorization: Bearer $TOKEN" \
  | python3 -c "import sys,json;a=json.load(sys.stdin)['deviceActivation'];print('DevAddr',a['devAddr'],'fCntUp',a['fCntUp'])"
# ...espera ~1 min y repite: si fCntUp NO sube, no está llegando nada (aunque el OLED tenga sesión).
# Y un uplink por MQTT trae el gateway/RSSI/SNR (prueba de RF real):
docker exec 00_chirpstack-docker-mosquitto-1 mosquitto_sub -h localhost \
  -t "application/$APP/device/+/event/up" -C 1
```
> **Por defecto `PICARO_FORCE_FRESH_JOIN 1`** (en `config_lorawan.h`): se **ignora la sesión guardada**
> y se exige un **JOIN OTAA real en cada arranque** — así `JOIN NUEVO OK` solo aparece si de verdad hubo
> JoinAccept (necesitas el gateway en rango al arrancar) y se evita arrastrar un datarate degradado por
> ADR. Ponlo en `0` si prefieres **reanudar** la sesión guardada entre reinicios (sin re-join).
> El datarate de uplink inicial se fija con `PICARO_UPLINK_DR` (DR3 por defecto, para que quepan los 19 bytes).

## 🛠️ Si algo falla
| Síntoma | Causa / Arreglo |
|---|---|
| Flash: `No serial data received` | USB nativo del S3. Entra a modo descarga: **hold BOOT + tap RST + suelta BOOT** y reintenta. El COM puede cambiar tras cada reset. |
| No aparece el puerto | Cable de solo carga → usa uno de datos. Prueba otro puerto USB. |
| `Radio begin=-2/-707` | Problema de TCXO/energía. Revisa que el AXP2101 arrancó (`pmu:` en la traza) y la antena. |
| `NO LLEGO EL JOIN-ACCEPT` / reintenta | (1) DevEUI/JoinEUI/AppKey no coinciden. (2) Perfil no es 1.0.x. (3) Gateway no cubre SB2 o es de 1 canal. (4) Antena. |
| `Uplink fallo (codigo -4)` = **PACKET_TOO_LONG** | El **ADR bajó el datarate** (tras muchos uplinks sin downlink lejos del gateway) a **DR0/SF10**, donde solo caben ~11 bytes y el payload de 19 no cabe. Arreglo: acércate al gateway y **reinicia** (hace join nuevo a `PICARO_UPLINK_DR`=DR3), o **quita campos** del payload en `sensores_config.h`. |
| `DevNonce already used` | Pon `#define PICARO_RESET_NONCES 1` en `config_lorawan.h`, flashea una vez, regrésalo a 0 y reflashea. |
| GPS siempre sin fix | Necesita cielo abierto y varios minutos en frío. Junto a una ventana. |
| `sdcard: no pude montar` | Tarjeta mal insertada o BLDO1 sin energía. El firmware la formatea a FAT si viene virgen. |
| Payload no cabe / uplink falla | El ADR bajó el datarate. Acerca la placa al gateway o **quita campos** del payload (ver abajo). |

## 📤 Los datos (payload) y la relación payload ↔ datarate
- **fPort:** **10**. **Tamaño por defecto: 19 bytes**, big-endian. Definido en `firmware/main/telemetry.c` y leído en `chirpstack/decoder.js` (**si cambias uno, cambia el otro**).

| Byte(s) | Campo | Tipo | Codificación |
|:--:|---|---|---|
| 0 | status | uint8 | bit0 fix · bit1 charging · bit2 usb · bit3 imu · bit4 mag |
| 1–4 · 5–8 | lat · lon | int32 | grados × 1e6 |
| 9–10 · 11 | alt · sats | int16 · uint8 | metros · nº |
| 12–13 · 14 | batería mV · % | uint16 · uint8 | mV · 0–100 (255=desconocido) |
| 15–16 | temperatura | int16 | °C × 100 |
| 17–18 | presión | uint16 | hPa × 10 |

**Ejemplo real** (USB, sin fix): `04 00000000 00000000 0000 00 01CC 00 0A34 1FDB`
→ status USB, lat/lon/alt/sats 0, batería `0x01CC`=460 mV, temp `0x0A34`=26.12 °C, presión `0x1FDB`=815.5 hPa.

> 🧠 **payload ↔ datarate:** en US915 el datarate mínimo (**DR0/SF10**) solo admite ~11 bytes de
> aplicación. Con 19 bytes necesitas **DR1 o mayor**. Si con mala señal el ADR baja a DR0 y el uplink
> no cabe, **desactiva campos** en `sensores_config.h` (p. ej. quita presión/humedad) o acerca la placa
> al gateway. **La microSD guarda SIEMPRE la telemetría completa**, quepa o no en el aire.

## 📦 La "caja negra" (microSD) — registra SIEMPRE, aunque no haya join
El firmware escribe una fila en **`/sdcard/PICARO.CSV`** en **cada ciclo**, **independientemente** de
si hay join o enlace LoRaWAN (es una caja negra). Registra incluso si el **GPS estaba apagado**
(`gps_active=0`). Columnas (25):

```
uptime_s, gps_active, gps_fix, lat, lon, alt_m, sats, hdop, speed_kmh,
batt_mv, batt_pct, usb, charging, temp_c, press_hpa, hum_pct,
imu_ok, ax, ay, az, gx, gy, gz, mag_ok, heading_deg
```
- **`gps_active`** = el módulo L76K está encendido y **enviando NMEA** (se detecta por recepción real;
  si lo apagas por botón → `0`). Distinto de **`gps_fix`** (tiene posición). En pantalla verás
  `GPS FIX`, `GPS ON s/fix` o `GPS: APAGADO`.
- **`mag_ok` / `heading_deg`**: magnetómetro **QMC6310 activo** (rumbo real; para brújula precisa hay que calibrar hard/soft-iron).
- **`imu_ok` / `ax..gz`**: IMU **QMI8658 apagado por defecto** → columnas a 0. En la unidad probada su SPI
  (bus compartido con la microSD) da lecturas inestables (`WHO_AM_I` intermitente 0x05/0x3E); se deja
  codificado y apagado, pendiente de depuración con analizador lógico. Actívalo en `sensores_config.h`.
- Ejemplo real (con fix): `34,1,1,21.933677,-102.298267,1872.4,7,4.7,1.9,810,0,1,0,26.26,816.6,54.1,0,0,0,0,0,0,0,1,328.3`

## 🔧 Cómo modificarlo (juega con esto)
Todo en **`firmware/main/sensores_config.h`**:
- **Magnetómetro:** ya viene **activo** (`USE_QMC6310 1`); su `heading_deg` va a la caja negra.
- **IMU:** `USE_QMI8658 1` lo enciende, pero en la unidad probada su SPI es inestable (ver caja negra).
  Si lo activas y ves `whoami=0x3E`, es ese problema de bus; con `TELEMETRY_IMU 1` además iría al payload
  (y habría que actualizar el decoder).
- **RTC:** `USE_PCF8563 1`.
- **Añade humedad al aire:** `TELEMETRY_HUMIDITY 1`.
- **Cambia el periodo:** `PICARO_UPLINK_INTERVAL_S` en `config_lorawan.h`.
- **WiFi/BLE:** `USE_WIFI_DEMO` / `USE_BLE_DEMO` (demos, sin tocar el timing LoRaWAN).

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 08 · Radiosonda PICARO (TTGO/Arduino)](../08_radiosonda_picaro/)
- 🏁 **Ejercicio culminante del curso** (T-Beam Supreme + ESP-IDF).
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/Ejercicio-09-Radiosonda-PICARO-Full)

## 📎 Referencia
- **Archivos de esta carpeta:**
  ```
  firmware/                proyecto ESP-IDF (target esp32s3)
    main/config_lorawan.h  ← ⭐ AQUÍ cambias DevEUI / JoinEUI / AppKey
    main/sensores_config.h ← ⭐ AQUÍ activas/desactivas sensores y campos del payload
    main/board_pins.h      pines del T-Beam Supreme
    main/*.c/.cpp          drivers (radio, PMU, GPS, BME280, IMU, mag, RTC, OLED, SD) + telemetría
  chirpstack/decoder.js    codec que "traduce" el payload en ChirpStack
  provision.sh             (opcional) da de alta el device por REST en vez de la web
  credentials.json         resumen de credenciales del ejercicio
  ```
- **Doc oficial ESP-IDF v5.5:** [get-started](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/get-started/index.html) · [build system](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-guides/build-system.html) · [I²C](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/i2c.html) · [SPI master](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/spi_master.html) · [UART](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/uart.html) · [FAT/SD](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/storage/fatfs.html) · [NVS](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/storage/nvs_flash.html)
- **RadioLib (stack de radio, componente IDF):** <https://components.espressif.com/components/jgromes/radiolib> · [LoRaWAN](https://jgromes.github.io/RadioLib/)
- **Guía común:** [ChirpStack API (REST · MQTT · codec)](../COMMON_CHIRPSTACK_API.md)

---
> 🧩 **Código:** stack **RadioLib** (MIT) sobre **ESP-IDF**; drivers de sensores propios de este
> ejercicio. Radio: SX1262 · Región: US915 SB2 · OTAA · LoRaWAN 1.0.x.
>
> 📄 Material educativo (esta guía) bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
