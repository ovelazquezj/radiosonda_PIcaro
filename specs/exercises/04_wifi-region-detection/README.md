# Ejercicio 04 — Wi-Fi Region Detection (el LR1110 escanea Wi-Fi)

> **En una frase:** haces que el **LR1110 escanee las redes Wi-Fi** que le rodean y ves por el
> **terminal serie** la lista de puntos de acceso (MAC, RSSI, SSID) y una **región LoRaWAN estimada**
> a partir de ellos. Es una demo de la **radio Wi-Fi** del chip, no de LoRaWAN.
> **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG**. **Banda:** irrelevante
> (binario **multi-banda**; el nodo **no se une** a ninguna red). El binario **ya viene compilado**
> en `artifacts/` — aquí **no compilas nada** y **no usas ChirpStack**: todo el resultado sale por la **UART**.

## 🎯 Qué vas a conseguir
Al reiniciar la placa, el firmware lanza un **escaneo Wi-Fi pasivo** (`smtc_modem_wifi_scan()`),
escucha las balizas de los routers/APs cercanos y, en el evento `WIFI_SCAN_DONE`, imprime por el
terminal serie: los **APs detectados** (MAC, canal, RSSI, código de país, SSID), una **tabla de
puntuación por región** y una línea final con la **región estimada** y su confianza. Verás algo así
(los valores dependen de tu entorno):

```
INFO: Event received: WIFI_SCAN_DONE
SCAN_DONE info:
-- number of results: 5
-- power consumption: 20765 nah
-- scan duration: 7199 ms
[ 14 CC 20 CB 53 56 ]   Channel:8    Type:1  RSSI:-57  Origin:FIXED  Country code:..  SSID:TP-LINK_CB5356
[ 64 70 02 D9 94 55 ]   Channel:6    Type:1  RSSI:-74  Origin:FIXED  Country code:US  SSID:briere
[ 38 B5 C9 03 8A 60 ]   Channel:1    Type:1  RSSI:-94  Origin:FIXED  Country code:FR  SSID:Livebox-8A60

=== Region Scores (value, ratio%, margin%) ===
EU868 :  5,  83%,  80%
US915 :  1,  17%,  N/A
CN470 :  0,   0%,  N/A
...
===========================================

INFO: -----------------------
INFO: Estimated region: EU868 with 81% confidence
INFO: -----------------------
```

> ℹ️ La **región estimada** la calcula el **propio firmware** (módulo de ejemplo `wifi_region_finder`)
> con una heurística sencilla: puntúa cada región según el **código de país** de las balizas Wi-Fi y
> según **patrones de SSID** conocidos (p. ej. `xfinity` → US915, `Livebox`/`freebox` → EU868). No es
> la geolocalización real de LoRa Edge (posición a partir de las MAC): eso lo haría un servicio en la
> nube — ver [📖 Nota didáctica](#-nota-didáctica--esto-es-la-base-de-la-geolocalización-wi-fi).

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, no continúes):

- [ ] **Hardware:** Nucleo-L476RG + shield **LR1110**, con su **antena Wi-Fi/RF** conectada, y el cable USB del **ST-LINK**.
- [ ] **Un terminal serie** a **115200 8N1** → en **Windows**: PuTTY / Tera Term sobre `COMx`; en **Linux/WSL**: `tio` / `picocom` / `minicom` sobre `/dev/ttyACM0`.
- [ ] Una utilidad para **flashear** (arrastrar al disco `NODE_L476RG` o STM32CubeProgrammer) → [Wiki: Flashear y ver la serie](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Flashear-y-ver-la-serie).

> ✅ **Lo que NO necesitas:** ChirpStack, gateway, credenciales, provisión ni consumo. Este ejercicio
> **no hace join** ni envía uplinks (ver [*Por qué NO usa ChirpStack*](#-por-qué-no-usa-chirpstack)).
> ⚠️ **Nunca enciendas la radio sin antena** — además, sin antena el escaneo detectará pocos o ningún AP.

## 📟 Hardware y conexiones
Nada que cablear: el **LR1110** va montado como shield sobre la **Nucleo-L476RG**. Conéctala al PC por
el USB del **ST-LINK** (el mismo que usarás para flashear y para ver la traza serie). El escaneo usa la
**radio Wi-Fi de 2.4 GHz** del LR1110, así que asegúrate de que la **antena** esté puesta.

## 🪜 Paso a paso

### 1. Flashea el binario
Graba el `.hex` (o `.bin`) en la Nucleo. Detalle del método (arrastrar al disco `NODE_L476RG` o
STM32CubeProgrammer) en [`../COMMON_FLASH.md`](../COMMON_FLASH.md).
- **Binario (único, multi-banda):** `artifacts/wifi-region-detection_lr1110_multiband.hex`

La banda es **irrelevante** porque el dispositivo **no se une** a ninguna red: el mismo binario vale
para cualquier región.

**Salida esperada:** el LED del ST-LINK parpadea al copiar y la placa se reinicia sola al terminar.

### 2. Abre la traza serie
Abre tu terminal a **115200 8N1** sobre el VCP del ST-LINK:
```bash
# Linux / WSL:
tio -b 115200 /dev/ttyACM0        # o: picocom -b 115200 /dev/ttyACM0
```
```
:: Windows (PuTTY):  Connection type = Serial,  Serial line = COMx,  Speed = 115200
::                   (mira en el Administrador de dispositivos qué COM es el "STMicroelectronics STLink")
```

**Salida esperada** (nada más abrir, si la placa ya arrancó, verás el banner de inicio):
```
INFO: Region detection from WiFi example is starting
INFO: Event received: RESET
INFO: LR11XX FW: 0x0401, type: 0x01
INFO: -----------------------
WARN: Estimated region: UNKNOWN
INFO: -----------------------
```
> ℹ️ La primera estimación siempre sale **UNKNOWN**: aún no se ha hecho ningún escaneo. Los valores
> `0x0401`/`0x01` son la versión de firmware del LR1110 (pueden variar según tu chip).

### 3. Pulsa RESET (B2)
Pulsa el botón **RESET (B2)** de la Nucleo para reiniciar el firmware desde cero. Tras el arranque, el
LR1110 empieza a **escanear Wi-Fi en bucle**: cada escaneo dura unos segundos y, al terminar, lanza el
siguiente automáticamente (el LED de TX parpadea en cada ciclo).

**Salida esperada:** vuelves a ver el banner de arranque (paso 2) y, unos segundos después, el primer
resultado de escaneo (paso 4).

### 4. Observa el resultado del escaneo
En cada evento `WIFI_SCAN_DONE` verás el bloque completo: metadatos del escaneo, la **lista de APs** y
la **región estimada**.

**Salida esperada** (algo así — los APs y la región dependen de tu entorno):
```
INFO: Event received: WIFI_SCAN_DONE
SCAN_DONE info:
-- number of results: 5
-- power consumption: 20765 nah
-- scan duration: 7199 ms
[ 14 CC 20 CB 53 56 ]   Channel:8    Type:1  RSSI:-57  Origin:FIXED  Country code:..  SSID:TP-LINK_CB5356
[ 64 70 02 D9 94 55 ]   Channel:6    Type:1  RSSI:-74  Origin:FIXED  Country code:US  SSID:briere
[ F4 CA E5 8B 49 A0 ]   Channel:11   Type:1  RSSI:-91  Origin:FIXED  Country code:..  SSID:freebox_briere
[ 38 B5 C9 03 8A 60 ]   Channel:1    Type:1  RSSI:-94  Origin:FIXED  Country code:FR  SSID:Livebox-8A60

=== Region Scores (value, ratio%, margin%) ===
EU868 :  5,  83%,  80%
US915 :  1,  17%,  N/A
CN470 :  0,   0%,  N/A
AU915 :  0,   0%,  N/A
KR920 :  0,   0%,  N/A
IN865 :  0,   0%,  N/A
RU864 :  0,   0%,  N/A
AS923 :  0,   0%,  N/A
===========================================

INFO: -----------------------
INFO: Estimated region: EU868 with 81% confidence
INFO: -----------------------
```
- **MAC** entre corchetes, **canal**, **RSSI** (potencia recibida, más negativo = más lejos), **código de país** y **SSID** de cada AP.
- La **tabla de puntuación** suma puntos por región según el país/SSID de cada AP.
- La línea **`Estimated region: … with N% confidence`** es la región ganadora. Si ningún AP aporta
  pistas suficientes (país `..` y SSID desconocidos), verás **`Estimated region: UNKNOWN`** — es normal
  en muchos entornos y **no es un fallo**.

## ✅ Cómo saber que funcionó
- [ ] En la serie aparece **`Event received: WIFI_SCAN_DONE`** y una lista con al menos un AP (`number of results: ≥ 1`).
- [ ] Ves la **tabla `Region Scores`** y una línea **`Estimated region: …`** (una región concreta o `UNKNOWN`).
- [ ] El ciclo se **repite solo** cada pocos segundos (escaneo continuo), y el LED de TX parpadea en cada uno.
- [ ] En **ChirpStack no aparece nada** — correcto: este ejercicio no genera tráfico LoRaWAN.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| `number of results: 0` o siempre `UNKNOWN` | Pocos APs alrededor, antena floja, o los APs no anuncian país/SSID reconocibles | Acércate a una zona con Wi-Fi, revisa la **antena**; recuerda que `UNKNOWN` es un resultado válido, no un error |
| Caracteres basura en la serie | Baudios/formato mal | Ponlo a **115200 8N1** (8 bits, sin paridad, 1 stop) |
| No aparece **nada** en la serie | Puerto COM/tty equivocado o driver del ST-LINK ausente | Elige el **puerto correcto** (Administrador de dispositivos / `ls /dev/ttyACM*`); instala el **driver ST-LINK**; pulsa **RESET (B2)** |
| Secuencias raras tipo `[32m` alrededor del texto | Tu terminal no interpreta colores ANSI | Ignóralas o usa un terminal que soporte ANSI (`tio`, PuTTY); el texto útil sigue siendo legible |
| `Wrong LR1110 firmware version` / `not compatible` | El LR1110 tiene un firmware anterior al que exige el ejemplo (≥ 0x0401) | Actualiza el firmware del LR1110 (fuera del alcance de este ejercicio) |

## 🚫 Por qué NO usa ChirpStack
Es **intencional**. Este demo aísla la **capacidad de radio Wi-Fi** del LR1110, no la conectividad
LoRaWAN:

- **No hace join OTAA** ni envía uplinks → **no aparece en ChirpStack** (ni *last seen*, ni frames, ni MQTT).
- **No tiene credenciales baked-in** (DevEUI/JoinEUI/AppKey), por eso no hay device que dar de alta.
  El binario es multi-banda precisamente porque la banda no importa al no unirse a la red.
- **No hay provisión** (nada que registrar por API) ni **consumo** por REST/MQTT: el único canal de
  salida del dato es la **UART**. Por eso, a diferencia de los ejercicios 01/05, aquí no hay
  `provision.sh` ni `scripts/subscribe.sh`.

Para el flujo LoRaWAN completo (join + uplinks + consumo) usa el
**[Ejercicio 01 · Periodical Uplink](../01_periodical-uplink/)**.

## 📖 Nota didáctica — esto es la base de la geolocalización Wi-Fi
Este ejercicio es el **paso previo y observable** de la **geolocalización por Wi-Fi** de la familia
**LoRa Edge**. La diferencia clave:

- **Aquí (en el dispositivo):** el LR1110 escanea Wi-Fi y el firmware aplica una **heurística local**
  (código de país + patrones de SSID) para *estimar una región*. Es un ejemplo didáctico y aproximado.
- **En un producto real (en la nube):** la lista de **MAC/RSSI** de los APs se enviaría por LoRaWAN a
  un **servicio de resolución** (p. ej. LoRa Cloud), que la cruza con una base de datos mundial de APs
  y devuelve una **posición estimada** (lat/lon) sin GNSS. Esa parte **no** ocurre en este binario.

Es decir: el escaneo Wi-Fi que ves aquí es la **materia prima** de la geolocalización; el cálculo de
posición real vive fuera del dispositivo.

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 03 · Hardware Modem](../03_hw-modem/)
- ➡️ Siguiente: [Ejercicio 05 · TTGO ESP32 LoRa (LMIC)](../05_ttgo-lora32/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json) — marca explícitamente *"no va a ChirpStack"* (sin credenciales).
- **Archivos de esta carpeta:** `artifacts/wifi-region-detection_lr1110_multiband.{hex,bin,elf}` (binario único multi-banda) · `scripts/` (vacío: este ejercicio no consume por API).
- **Código fuente del ejemplo:** `lbm_applications/3_geolocation_on_lora_edge/main_wifi_region_detection/` (`main_wifi_region_detection.c` + módulo `wifi_region_finder`).
- **Guías comunes:** [Flashear](../COMMON_FLASH.md) · [Compilar (opcional)](../COMMON_BUILD.md).

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
