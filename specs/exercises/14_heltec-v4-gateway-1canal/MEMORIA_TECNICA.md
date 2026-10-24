# Memoria técnica — Gateway LoRaWAN de 1 canal con Heltec WiFi LoRa 32 V4 para banco de pruebas local

| | |
|---|---|
| **Proyecto** | PiCARO — radiosonda LoRaWAN |
| **Componente** | Gateway de 1 canal + servidor ChirpStack local (banco de pruebas) |
| **Versión del firmware** | One Channel Hub v1.1.1 (Semtech) + parche PiCARO `heltec-v4-softap` |
| **Hardware** | Heltec WiFi LoRa 32 V4 (ESP32-S3R2 + SX1262 + amplificador de RF) |
| **Servidor de red** | ChirpStack v4 (probado con v4.19.1) en Docker, región `us915_1` |
| **Fecha** | 2026-10-23 |
| **Guía de uso** | [`README.md`](README.md) (instalación paso a paso y uso diario) |

---

## 1. Problema que resuelve

Para probar una radiosonda LoRaWAN, el alumno necesita tres cosas además de su nodo: un **gateway** que
capte sus paquetes, un **servidor de red (LNS)** que los procese y una **conexión entre ambos**. Hasta
ahora eso dependía de infraestructura compartida (el gateway del laboratorio o el servidor del curso en
internet), con estas consecuencias:

- El alumno **no puede trabajar en casa** ni fuera del horario del laboratorio.
- Varios equipos comparten el mismo servidor: los datos y errores de unos **se mezclan** con los de otros.
- Si el servidor remoto o el enlace a internet fallan, **nadie puede probar**.
- El alumno ve el sistema como una "caja negra": no instala ni configura el servidor.

**Solución:** un **banco de pruebas local, personal y de bajo costo**. Cada alumno o equipo corre
ChirpStack en su propio PC con Docker y usa una placa Heltec WiFi LoRa 32 V4 como gateway de 1 canal
conectada por WiFi a ese PC. El conjunto:

- funciona **sin internet** una vez instalado (la red local basta);
- cuesta lo que una placa de desarrollo, en lugar de un gateway comercial de 8 canales;
- es **reproducible**: firmware precompilado, configuración del servidor incluida en el repositorio y una
  guía paso a paso;
- obliga al alumno a entender **todas las piezas** de la cadena LoRaWAN: nodo → gateway → LNS → aplicación.

## 2. Alcance y limitaciones

| Capacidad | Estado | Comentario |
|---|---|---|
| Recibir uplinks LoRa en 1 canal y 1 SF | ✅ | 903.9 MHz, SF10, BW 125 kHz por defecto; configurable |
| Reenviar uplinks al LNS (Semtech UDP) | ✅ | Puerto 1700/UDP, compatible con `chirpstack-gateway-bridge` |
| Transmitir downlinks (JoinAccept, Clase A RX1/RX2, Clase C) | ✅ | El SX1262 transmite en la frecuencia y SF que ordene el LNS |
| Configurar WiFi y LNS sin recompilar | ✅ | Red WiFi propia con página de configuración; datos en NVS |
| Escuchar 8 canales / varios SF a la vez | ❌ | Limitación física del SX1262 (un solo demodulador) |
| Cumplimiento LoRaWAN / uso en red pública | ❌ | Un gateway de 1 canal no cumple la especificación; solo banco de pruebas |
| Recibir mientras transmite | ❌ | Semidúplex: durante un downlink no escucha |

**Consecuencia para la radiosonda del curso** (US915 sub-banda 2, DR0, ADR apagado):

| Aspecto | Compatibilidad | Explicación |
|---|---|---|
| Modulación de uplink | ✅ | Ambos usan SF10 / BW125 (DR0) |
| Frecuencia de uplink | ⚠️ ~1 de cada 8 | La radiosonda elige al azar uno de los 8 canales de 125 kHz (903.9–905.3 MHz) |
| Join | ⚠️ lento | JoinRequest al azar entre 8 canales de 125 kHz (DR0) y 1 de 500 kHz (DR4); solo se oyen los que caen en 903.9 MHz. La sesión queda guardada en la radiosonda |
| Downlinks (RX1 a 923.3 + 0.6·n MHz DR10; RX2/Clase C en 923.3 MHz DR8) | ✅ | El gateway transmite en BW500 sin limitación |

Se aceptó esta limitación de forma explícita: el objetivo del banco es **comprobar que la radiosonda se
une, transmite y decodifica correctamente**, no medir tasa de entrega. Para medir cobertura o tasa de
entrega se usa un gateway multicanal.

## 3. Arquitectura

```
┌──────────────┐  LoRa   ┌───────────────────────────────── Heltec WiFi LoRa 32 V4 ─────────────────────────────┐
│ Radiosonda   │ 903.9   │ Antena ─ FEM (GC1109 / KCT8103L) ─ SX1262 ──SPI── ESP32-S3                           │
│ (nodo US915) │ ──────▶ │                                                   ├─ HAL LoRaHub (RX/TX, temporización)│
└──────────────┘ ◀────── │                                                   ├─ Packet forwarder (Semtech UDP)   │
                         │                                                   ├─ Página de configuración (SoftAP) │
                         │                                                   └─ OLED SSD1306 (estado)            │
                         └──────────────────────────────────────────────┬───────────────────────────────────────┘
                                                                        │ WiFi 2.4 GHz de la red local
                                                                        │ UDP 1700: PUSH_DATA / PULL_DATA / PULL_RESP
┌───────────────────────────────── PC del alumno (Docker) ──────────────▼──────────────────────────────────────┐
│ chirpstack-gateway-bridge ──MQTT (us915_1/gateway/<EUI>/...)── mosquitto ── chirpstack ── postgres / redis  │
│                                                                              │                               │
│                                                              web :8080 · chirpstack-rest-api :8090 · MQTT :1883 │
└──────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

**Flujo de un uplink:**
1. La radiosonda transmite en 903.9 MHz con SF10.
2. El amplificador de la V4 (en modo recepción, con LNA) entrega la señal al SX1262, que la demodula y
   activa la interrupción DIO1.
3. El ESP32-S3 lee el paquete, le pone marca de tiempo (contador de µs), RSSI y SNR, y lo envía en JSON
   (`rxpk`) dentro de un `PUSH_DATA` UDP al puerto 1700 del PC.
4. `chirpstack-gateway-bridge` lo traduce a MQTT en el tópico de la región `us915_1`; ChirpStack lo
   procesa (MIC, deduplicación, sesión, decodificación) y lo publica en la aplicación.

**Flujo de un downlink:** ChirpStack decide el downlink → el bridge lo envía como `PULL_RESP` (`txpk`)
por el canal abierto con `PULL_DATA` (cada 10 s) → el firmware lo encola en su *just-in-time queue* →
en el instante exacto conmuta el amplificador a TX, configura el SX1262 y transmite → vuelve a RX.

## 4. Hardware

### 4.1 Placa

| Elemento | Detalle |
|---|---|
| MCU | ESP32-S3R2 (QFN56, rev. 0.2), 2 núcleos 240 MHz, **2 MB PSRAM** integrada |
| Flash | **16 MB** |
| Radio | Semtech **SX1262**, TCXO controlado por DIO3 a 1.8 V, DIO2 como conmutador de RF |
| Amplificador de RF (FEM) | **GC1109** (hardware V4.2) o **KCT8103L** (hardware V4.3): PA de hasta ~28 dBm y LNA |
| Pantalla | OLED SSD1306 128×64 por I²C, alimentada desde el riel **Vext** |
| USB | USB nativo del ESP32-S3 (USB-Serial/JTAG, VID:PID 303A:1001): no requiere driver |
| Botones | **PRG** (GPIO0) y **RST** |
| LED de usuario | GPIO35 |

La **V4-R2** se distingue de la V3 por software: la V3 no tiene PSRAM y trae 8 MB de flash; la V4-R2
trae 2 MB de PSRAM y 16 MB de flash (`esptool flash-id`).

### 4.2 Pines utilizados

| Función | GPIO | Notas |
|---|---|---|
| SX1262 NSS / SCK / MOSI / MISO | 8 / 9 / 10 / 11 | Igual que la V3 |
| SX1262 RESET / BUSY / DIO1 | 12 / 13 / 14 | Igual que la V3 |
| OLED SDA / SCL / RST | 17 / 18 / 21 | Igual que la V3 |
| **Vext** (OLED y amplificador) | **36** | Activo en **bajo**. Debe encenderse **antes** de iniciar la OLED |
| **VFEM_Ctrl** (alimentación del FEM) | **7** | Alto = encendido |
| **CSD / EN** (habilitación del FEM) | **2** | Alto = encendido. También sirve para **detectar** el modelo de FEM |
| **CPS** del GC1109 (V4.2) | **46** | Alto = PA (TX), bajo = bypass (RX) |
| **CTX** del KCT8103L (V4.3) | **5** | Alto = TX, bajo = RX por el LNA |

### 4.3 Detección del amplificador

Las dos revisiones de la V4 llevan amplificadores distintos con pines de control distintos. El firmware
los distingue **en el arranque**, sin configuración del usuario (mismo método que el firmware Meshtastic):

1. Enciende el LDO del FEM (GPIO7 = 1) y espera 1 ms.
2. Lee GPIO2 como entrada sin *pull*: el **KCT8103L** mantiene su línea CSD en **alto**; el **GC1109**
   deja su EN en **bajo**.
3. Configura GPIO2 como salida en alto (FEM habilitado) y deja el FEM en modo **RX**.

| Modo | GC1109 (V4.2) | KCT8103L (V4.3) |
|---|---|---|
| RX (por defecto) | EN=1, CPS=0 | CSD=1, CTX=0 (LNA activo) |
| TX | EN=1, CPS=1 | CSD=1, CTX=1 |

### 4.4 Potencia de transmisión

ChirpStack pide una potencia **en el conector de antena**. Como el FEM añade ganancia (≈11 dB con poca
excitación, ≈7 dB cerca de saturación), el firmware resta una **ganancia nominal de 10 dB** antes de
configurar el SX1262 y limita la salida a **27 dBm**:

```
potencia_SX1262 = potencia_pedida − 10 dB      (acotada a −9 … 17 dBm)
rango anunciado al packet forwarder: 1 … 27 dBm en antena
```

Ejemplo: un downlink pedido a 20 dBm se configura a 10 dBm en el SX1262.

## 5. Firmware

### 5.1 Base

Se parte de **One Channel Hub v1.1.1** de Semtech (`Lora-net/one_channel_hub`, licencia *Clear BSD*):
HAL de 1 canal sobre los drivers oficiales `sx126x_driver` v2.3.2, *packet forwarder* Semtech UDP,
pantalla con LVGL y página web de configuración (puerto 8000). Soporta oficialmente la Heltec **V3**, no
la V4. Se compila con **ESP-IDF 5.5.2** (Semtech lo valida con 5.3).

Se descartó **ESP-1ch-Gateway** (el firmware del [Ejercicio 07](../07_esp-1ch-gateway/)) porque solo
maneja radios SX127x y la V4 lleva SX1262.

### 5.2 Cambios del parche PiCARO

El parche [`firmware/one_channel_hub-v1.1.1-heltec-v4-softap.patch`](firmware/) (17 archivos) añade:

| Cambio | Archivos | Descripción |
|---|---|---|
| Placa Heltec V4 | `Kconfig.projbuild`, `hw_board_defs.h`, `ral_sx126x_bsp.c`, `lorahub_hal.c`, `smtc_ral/CMakeLists.txt` | Nueva opción `HELTEC_WIFI_LORA_32_V4`; reutiliza pinout, RF switch y regulador de la V3 |
| Definición de la radio V4 | `heltec_wifi_lora_32_v4.c/.h` (nuevos) | TCXO a 1.8 V, potencia compensada por el FEM, detección y conmutación del FEM |
| Conmutación TX/RX del FEM | `lorahub_hal_tx.c`, `lorahub_hal_rx.c` | Pasa el FEM a TX al configurar una transmisión y a RX al volver a recepción |
| Vext antes de la OLED | `main.c`, `hw_board_defs.h` | Sin esto la OLED no responde por I²C y el arranque aborta |
| Página de configuración | `wifi_portal.c/.h` (nuevos), `main.c`, `wifi.c` | SoftAP + DNS cautivo + formulario; sustituye al aprovisionamiento BLE de Semtech |
| Estado en la OLED | `display.c/.h` | Muestra el nombre de la red de configuración y `http://192.168.4.1` |
| Configuración de compilación | `sdkconfig.defaults.heltec_v4` (nuevo) | Placa V4, 16 MB, consola USB nativa, BT desactivado, US915 903.9 MHz SF10, portal |

### 5.3 Página de configuración (SoftAP)

**Lógica de arranque:**

```
                 ┌──────────────────────────┐
  arranque ────▶ │ ¿hay WiFi guardada (NVS)?│── no ──▶ PORTAL (sin límite de tiempo)
                 └────────────┬─────────────┘
                              sí
                 ┌────────────▼─────────────────────────────┐
                 │ ¿reinicio por fallo de WiFi anterior?     │── sí ─▶ PORTAL (5 min, luego reinicia)
                 │ (marca en RTC_NOINIT + reinicio software) │
                 └────────────┬─────────────────────────────┘
                              no
                 ┌────────────▼─────────────────────────┐
                 │ ventana de 5 s con LED encendido:     │── PRG pulsado ─▶ PORTAL (sin límite)
                 │ ¿se pulsa PRG?                        │
                 └────────────┬─────────────────────────┘
                              no
                 ┌────────────▼─────────────────────────┐
                 │ conectar a la WiFi (10 reintentos)    │── falla ─▶ marca RTC + reinicio ─▶ (PORTAL 5 min)
                 └────────────┬─────────────────────────┘
                              ok
                    SNTP → servidor web :8000 → packet forwarder
                    (si se pierde la WiFi en marcha y no vuelve tras 10 reintentos: reinicio)
```

**Portal:**
- Punto de acceso WPA2 `PiCARO-GW-XXXXXX` (los 3 últimos bytes de la MAC de estación, igual que el final
  del Gateway EUI), contraseña `picaro1234`, IP `192.168.4.1`, DHCP para los clientes.
- **DNS cautivo**: un servidor DNS mínimo (UDP 53) responde a toda consulta tipo A con `192.168.4.1`, y
  cualquier URL desconocida redirige con HTTP 302 a la página. Así los celulares abren la página solos.
- Antes de publicar la página se hace un **escaneo WiFi** (modo AP+STA) y se ofrece la lista de redes.
- El formulario valida: SSID de 1–32 caracteres; contraseña vacía (conserva la guardada si el SSID no
  cambia) o de 8–63 caracteres; LNS no vacío; puerto 1–65535; frecuencia 150–960 MHz; SF 7–12.
  Fija BW = 125 kHz.
- Al guardar escribe en NVS y reinicia a los 2 s.

**Datos en NVS** (espacio de nombres `storage`):

| Clave | Contenido | Origen |
|---|---|---|
| `wifi_ssid`, `wifi_pass` | Credenciales de la WiFi local | Portal (nuevo) |
| `lns_addr`, `lns_port` | IP/nombre y puerto del LNS | Portal o página :8000 |
| `chan_freq`, `chan_dr`, `chan_bw` | Canal de recepción | Portal o página :8000 |
| `chan_dr_2`, `sntp_addr` | SF secundario (solo LR1121), servidor SNTP | Página :8000 |

Las credenciales WiFi se guardan con `WIFI_STORAGE_RAM` para la pila WiFi (no se duplican en el NVS
interno de ESP-IDF) y **no** se exponen en la API `/api/v1/get_config`.

### 5.4 Valores por defecto (`sdkconfig.defaults.heltec_v4`)

| Parámetro Kconfig | Valor |
|---|---|
| `CONFIG_HELTEC_WIFI_LORA_32_V4` / `CONFIG_RADIO_TYPE_SX1262` | `y` |
| `CONFIG_ESPTOOLPY_FLASHSIZE_16MB` | `y` |
| `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` | `y` (logs por el USB nativo) |
| `CONFIG_BT_ENABLED` | `n` |
| `CONFIG_ESP_MAIN_TASK_STACK_SIZE` | `8192` |
| `CONFIG_WIFI_PROV_SOFTAP` | `y` |
| `CONFIG_WIFI_PORTAL_AP_PREFIX` / `_PASSWORD` / `_DELAY_S` | `PiCARO-GW-` / `picaro1234` / `5` |
| `CONFIG_NETWORK_SERVER_ADDRESS` / `_PORT` | vacío / `1700` |
| `CONFIG_CHANNEL_FREQ_HZ` | `903900000` |
| `CONFIG_CHANNEL_LORA_SPREADING_FACTOR_1` / `_2` | `10` / `0` |
| `CONFIG_CHANNEL_LORA_BANDWIDTH` | `125` |

### 5.5 Identidad del gateway

`Gateway EUI = MAC[0] MAC[1] MAC[2] FF FE MAC[3] MAC[4] MAC[5]` (MAC de estación WiFi). Ejemplo:
MAC `aa:bb:cc:00:11:22` → EUI `aabbccfffe001122`. Se muestra en la OLED (línea 2).

### 5.6 Pantalla OLED

| Línea | Contenido |
|---|---|
| 1 | Estado: `LoRaHub - Initializing` / `WiFi PiCARO-GW-xxxxxx - http://192.168.4.1` / `LoRaHub - Receiving` |
| 2 | `EUI - IP` del gateway |
| 3 | Canal: `903.9000  SF10 BW125` |
| 4 | Contadores: `up N  dn N` |
| 5 | Último paquete: `DevAddr sf:x rssi:x snr:x` |

## 6. Servidor de red (ChirpStack local)

Se reutiliza la composición del [Ejercicio 00](../00_chirpstack-docker/) sin cambios:

| Servicio | Imagen | Puerto | Función en el banco |
|---|---|---|---|
| `chirpstack` | `chirpstack/chirpstack:4` | 8080/TCP | LNS + web de gestión |
| `chirpstack-gateway-bridge` | `chirpstack/chirpstack-gateway-bridge:4` | **1700/UDP** | Recibe al gateway (Semtech UDP) y publica en MQTT con prefijo `us915_1` |
| `chirpstack-gateway-bridge-basicstation` | ídem | 3001/TCP | Gateways Basics Station (no usado aquí) |
| `chirpstack-rest-api` | `chirpstack/chirpstack-rest-api:4` | 8090/TCP | API REST |
| `mosquitto` | `eclipse-mosquitto:2` | 1883/TCP | Broker MQTT |
| `postgres`, `redis` | `postgres:14-alpine`, `redis:7-alpine` | internos | Persistencia y estado |

- Región: `us915_1` = **US915 sub-banda 2** (canales 8–15, 903.9–905.3 MHz; canal de 500 kHz 65 en
  904.6 MHz). Está en `enabled_regions` de `chirpstack.toml` y el bridge usa ese prefijo de tópico.
- El gateway se registra en el tenant por defecto **ChirpStack** con `statsInterval = 30 s` (coincide con
  el intervalo de estadísticas del *packet forwarder*).
- Requisito del sistema operativo: permitir **UDP 1700 entrante** en el firewall del PC.

## 7. Pruebas realizadas

| # | Prueba | Método | Resultado |
|---|---|---|---|
| 1 | Identificación de la placa | `esptool flash-id` | ESP32-S3 (QFN56) rev 0.2, PSRAM 2 MB, flash 16 MB → V4-R2 ✅ |
| 2 | Compilación desde cero | Clon limpio de v1.1.1 + parche + drivers + `idf.py build` (ESP-IDF 5.5.2) | Compila sin errores ni avisos tratados como error; app 1.13 MB (23 % libre en partición) ✅ |
| 3 | Grabación | `esptool write-flash` | `Hash of data verified` ✅ |
| 4 | Arranque sin configuración | Consola serie | OLED inicializada, red `PiCARO-GW-xxxxxx` activa, 6 redes encontradas en el escaneo ✅ |
| 5 | Portal desde celular | Configuración real desde un celular | Formulario completo, datos guardados y reinicio en modo gateway ✅ |
| 6 | Configuración persistida | `GET http://IP-HELTEC:8000/api/v1/get_config` | `lns_port 1700`, `chan_freq 903.9`, `chan_dr 10`, `chan_bw 125` y la IP del LNS configurada ✅ |
| 7 | Conexión al LNS | API de ChirpStack (`lastSeenAt`) y web | Gateway **Online** en `us915_1`, *last seen* actualizado cada 30 s ✅ |
| 8 | Recepción de uplinks de una radiosonda | Contador `up` y *LoRaWAN frames* | **Pendiente** de registrar en este documento |

**Problemas encontrados durante el desarrollo y su solución** (corregidos en el parche):

| Problema | Causa | Solución |
|---|---|---|
| Aborto en el arranque (`panel_ssd1306_init ... ESP_ERR_TIMEOUT`) | En la V4 la OLED se alimenta de Vext, que no estaba encendido | Encender Vext (GPIO36 = 0) al inicio de `app_main`, antes de crear la tarea de la pantalla |
| *Stack overflow* en la tarea `main` al abrir el portal | Lista de 20 `wifi_ap_record_t` en la pila (pila de 3.5 KB) | Lista en memoria dinámica y pila de `main` a 8 KB |
| La página del portal se cortaba antes del formulario | Un fragmento HTTP vacío (campo sin valor) **termina** una respuesta *chunked* | No enviar nunca fragmentos de longitud 0 |
| Lista de redes invisible en el navegador del portal cautivo | `<datalist>` mal soportado en los mini navegadores de los celulares | Desplegable `<select>` visible que rellena el campo SSID |

## 8. Seguridad

Este es un **banco de pruebas de red local**. Riesgos conocidos y su mitigación:

| Riesgo | Mitigación |
|---|---|
| La contraseña de la red de configuración (`picaro1234`) es la misma para todas las placas | La red solo existe mientras se configura; para un uso distinto, cambiar `CONFIG_WIFI_PORTAL_AP_PASSWORD` y recompilar |
| Semtech UDP no autentica al gateway: el **Gateway EUI** es su única identidad | No publicar EUIs reales (en esta documentación se usan valores de ejemplo); mantener el LNS en la red local |
| La página :8000 permite cambiar la configuración sin contraseña | Solo es accesible dentro de la red local |
| ChirpStack con `admin`/`admin` de fábrica | Cambiar la contraseña si el PC está en una red compartida |

## 9. Licencias

- Firmware base *One Channel Hub* y drivers de radio: © Semtech, **Clear BSD** — se incluye su licencia en
  [`artifacts/LICENSE-one_channel_hub.txt`](artifacts/LICENSE-one_channel_hub.txt), como exige para la
  redistribución de binarios. El parche conserva los avisos de copyright de los archivos que modifica.
- ChirpStack: © Orne Brocaar, **MIT**.
- Esta documentación: **CC BY 4.0** © Omar Velazquez.

## 10. Referencias

- Semtech, *One Channel Hub*: <https://github.com/Lora-net/one_channel_hub>
- Semtech, *sx126x_driver*: <https://github.com/Lora-net/sx126x_driver>
- Heltec, WiFi LoRa 32 V4: <https://heltec.org/project/wifi-lora-32-v4/> (esquemático V4.2 y hoja de datos V4.3.1)
- Meshtastic, variante `heltec_v4` (pines y control del FEM): <https://github.com/meshtastic/firmware/tree/master/variants/esp32s3/heltec_v4>
- ChirpStack v4: <https://www.chirpstack.io/docs/>
- Semtech, protocolo del *packet forwarder* (Semtech UDP): `lorahub/PROTOCOL.md` del repositorio One Channel Hub
- LoRa Alliance, *RP002-1.0.4 Regional Parameters* (plan US915)

---

## Anexo A — Recompilar el firmware

Solo es necesario si quieres cambiar algún valor por defecto (por ejemplo, la contraseña de la red de
configuración o el canal). Probado en Windows con ESP-IDF 5.5.2.

1. Instala **ESP-IDF 5.5.x** con el instalador oficial de Espressif (<https://dl.espressif.com/dl/esp-idf/>).
   ⚠️ En Windows usa una carpeta de trabajo con **ruta corta** (por ejemplo `C:\work`): con rutas largas
   la compilación falla por el límite de 260 caracteres.
2. Abre la consola **ESP-IDF 5.5 PowerShell** y descarga el código base y los drivers:
   ```powershell
   cd C:\work
   git clone https://github.com/Lora-net/one_channel_hub.git
   cd one_channel_hub
   git checkout 0d1023f        # v1.1.1
   cd components\radio_drivers
   git clone -b v2.3.2 https://github.com/Lora-net/sx126x_driver.git sx126x_driver
   git clone -b v2.3.2 https://github.com/Lora-net/llcc68_driver.git llcc68_driver
   git clone -b v2.4.1 https://github.com/Lora-net/SWDR001.git lr11xx_driver
   cd ..\..
   ```
3. Aplica el parche:
   ```powershell
   git apply <ruta>\specs\exercises\14_heltec-v4-gateway-1canal\firmware\one_channel_hub-v1.1.1-heltec-v4-softap.patch
   ```
4. (Opcional) Edita `lorahub\sdkconfig.defaults.heltec_v4` para cambiar los valores por defecto.
5. Compila:
   ```powershell
   cd lorahub
   idf.py -B build_heltec_v4 -D SDKCONFIG=sdkconfig.heltec_v4 -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.heltec_v4" set-target esp32s3 build
   ```
   **Salida esperada (final):** `Project build complete.`
6. Genera la imagen única para `0x0`:
   ```powershell
   cd build_heltec_v4
   esptool.py --chip esp32s3 merge_bin -o picaro-gw-heltec-v4_us915-fsb2.bin --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 lorahub.bin
   ```
7. Grábala como en la Parte B de la guía.

> Si cambias `sdkconfig.defaults.heltec_v4` después de una primera compilación, borra
> `lorahub\sdkconfig.heltec_v4` antes de recompilar para que se apliquen los valores nuevos.
