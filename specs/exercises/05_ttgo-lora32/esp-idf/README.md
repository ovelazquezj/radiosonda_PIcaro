# Ejercicio 05 · versión **ESP-IDF v5** (TTGO ESP32 LoRa, RadioLib)

> **En una frase:** el mismo nodo del [Ejercicio 05](../README.md) —una **TTGO ESP32 LoRa v1**
> (SX1276 + OLED)— pero con firmware **nativo ESP-IDF v5** y el stack **RadioLib** en lugar del
> sketch Arduino/LMIC. Hace **join OTAA** en **US915 / sub-banda 2 (FSB2)** y envía el **mismo
> payload de 8 bytes** cada 30 s, así que reutiliza **tal cual** el provisioning y el codec del
> ejercicio (mismo DevEUI/AppKey → mismo device en ChirpStack).

## 🆚 Arduino/LMIC vs ESP-IDF/RadioLib
Es el mismo hardware y el mismo resultado en ChirpStack; cambia el framework y el stack:

| | Sketch Arduino (`../sketches/`) | Este proyecto (ESP-IDF v5) |
|---|---|---|
| Framework | Arduino-ESP32 | **ESP-IDF v5** (nativo) |
| Stack LoRaWAN | MCCI **LMIC** | **RadioLib** |
| Build/flash | Arduino IDE (botón Upload) | **`idf.py build flash monitor`** |
| Región/sub-banda | `CFG_us915` + `LMIC_selectSubBand(1)` | `LoRaWANNode(&radio, &US915, 2)` |
| Orden de EUIs | **LSB** (invertidos en los arrays) | **MSB natural** (RadioLib no invierte) |
| OLED | U8g2 | componente `ssd1306` (nopnop2002) |

> 🔑 **La confusión de bytes desaparece:** RadioLib toma `DevEUI`/`JoinEUI` en su orden de lectura
> (MSB), igual que ChirpStack. Se ponen los valores de [`../credentials.json`](../credentials.json)
> tal cual, sin invertir. (El AppKey tampoco se invierte en ninguno de los dos.)

## 🧰 Antes de empezar
- [ ] Todo lo del [Ejercicio 05](../README.md#-antes-de-empezar): **ChirpStack**, tu **`TOKEN`**, un
      **gateway US915 en FSB2** *online*, la **placa TTGO v1** con **antena**, y el device
      provisionado (`../provision.sh` + `../scripts/upload_codec.sh`).
- [ ] **ESP-IDF v5.x** instalado y activado (`. $HOME/esp/esp-idf/export.sh`, o el *ESP-IDF PowerShell*
      en Windows). Comprueba con `idf.py --version` (debe decir `v5.x`).
- [ ] Acceso a Internet en el **primer build**: el *component manager* descarga **RadioLib** y el
      driver **SSD1306** a `managed_components/`.

## 📟 Hardware
Idéntico al ejercicio: placa integrada, nada que cablear. Los pines están en
[`main/config.h`](main/config.h) (SCK 5, MISO 19, MOSI 27, NSS 18, RST 14, DIO0 26, DIO1 33; OLED
SDA 21 / SCL 22). **⚠️ Nunca enciendas la radio sin antena.**

## 🗂️ Estructura
```
esp-idf/
├─ CMakeLists.txt          # proyecto ESP-IDF
├─ sdkconfig.defaults      # tick 1 kHz (ventanas RX), stack, baudios, OLED I2C
└─ main/
   ├─ CMakeLists.txt
   ├─ idf_component.yml     # deps: jgromes/radiolib + ssd1306 (nopnop2002)
   ├─ config.h             # pines + credenciales (MSB)
   ├─ EspHal.h             # HAL de RadioLib sobre ESP-IDF (GPIO/SPI/timing)
   └─ main.cpp             # join OTAA + uplink de 8 bytes cada 30 s
```

## 🪜 Paso a paso

### 1. Fija el target y compila
```bash
# desde specs/exercises/05_ttgo-lora32/esp-idf/
idf.py set-target esp32          # el TTGO v1 lleva un ESP32 clásico
idf.py build                     # descarga RadioLib + ssd1306 y compila
```

### 2. Flashea y abre el monitor
```bash
idf.py -p /dev/ttyUSB0 flash monitor      # Linux/WSL
# idf.py -p COM5 flash monitor            # Windows
```
> Sal del monitor con **Ctrl+]**.

### 3. Observa el join y los uplinks
**Salida esperada** (recortada):
```
I ttgo-lorawan: ========================================
I ttgo-lorawan:   TTGO T3 LoRaWAN — ESP-IDF v5 + RadioLib
I ttgo-lorawan:   Region:       US915
I ttgo-lorawan:   Sub-banda:    2 (FSB2, canales 8-15)
I ttgo-lorawan: Inicializando radio SX1276...
I ttgo-lorawan:   Radio OK (SX1276 detectado).
I ttgo-lorawan: Iniciando JOIN OTAA (esto puede tardar 10-30 s)...
I ttgo-lorawan: ========================================
I ttgo-lorawan:   CONECTADO A CHIRPSTACK (JOINED)
I ttgo-lorawan: ========================================
I ttgo-lorawan: TX #1 | fPort 1 | payload: 99 01 00 12 01 00 00 AD
I ttgo-lorawan:   Uplink OK — sin downlink
I ttgo-lorawan:   Proximo uplink en 30 s
```
El **OLED** muestra `Estado:JOINED` y el contador de **Paquetes** subiendo.

### 4. Provisiona / consume (idéntico al ejercicio)
Como el DevEUI y las claves son los mismos, **no hay que registrar nada nuevo**. Si aún no lo hiciste:
```bash
cd ..                             # a specs/exercises/05_ttgo-lora32/
export TOKEN="tu_api_key"
./provision.sh                    # alta del device (idempotente)
./scripts/upload_codec.sh <DP>    # codec para el 'object' decodificado
./scripts/subscribe.sh            # stream MQTT de uplinks
```

## ✅ Cómo saber que funcionó
- [ ] En el **monitor** ves `CONECTADO A CHIRPSTACK (JOINED)` y un `TX #n` cada 30 s.
- [ ] El **OLED** muestra `Estado:JOINED` y `Paquetes` subiendo.
- [ ] En **ChirpStack** el device tiene *last seen* reciente y frames en *LoRaWAN frames*.
- [ ] Por **MQTT** llega el JSON con `object: {magic, counter, timestamp, checksum_ok}`.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| `JOIN fallo` en bucle | No hay gateway US915 en **FSB2** online, o sub-banda mal | Enciende un gateway en FSB2 (ej.07); confirma `LoRaWANNode(&radio, &US915, 2)` |
| ChirpStack: *"Unknown device"* | El device no está dado de alta con esas credenciales | `../provision.sh` (registra el DevEUI del proyecto) |
| Error de compilación en llamadas a `node.*` | Tu versión de RadioLib tiene otra firma de la API LoRaWAN | Ver **Nota sobre la API de RadioLib** abajo |
| `radio.begin() fallo` | Cableado/antena o pin mal | Revisa los pines en `config.h` (deben coincidir con la TTGO v1) |
| Uplink llega pero `object` vacío | Falta el codec en el device profile | `../scripts/upload_codec.sh <DP>` |
| No quieres OLED / falla su dependencia | — | Pon `USE_OLED 0` en `config.h` y quita `ssd1306` de `idf_component.yml` |

## 📌 Nota sobre la API de RadioLib
Este código está escrito para **RadioLib ≥ 6.6 / 7.x**, con este flujo LoRaWAN:
```cpp
LoRaWANNode node(&radio, &US915, 2);          // sub-banda 2 (FSB2) en el constructor
radio.begin();
node.beginOTAA(joinEUI, devEUI, NULL, appKey); // 1.0.x -> nwkKey = NULL
node.activateOTAA();                           // == RADIOLIB_LORAWAN_NEW_SESSION al unir
node.sendReceive(payload, len, /*fPort*/ 1);
```
La API LoRaWAN de RadioLib ha cambiado entre versiones. Si el primer `idf.py build` falla en
`node.*`, mira el ejemplo de tu versión en `managed_components/jgromes__radiolib/examples/` y ajusta
las firmas. `EspHal.h` está adaptado del ejemplo oficial `NonArduino/ESP-IDF` (MIT); si tu versión de
RadioLib trae uno distinto, puedes sustituirlo por el de ahí.

> ⚠️ **Nonces / re-join:** este ejemplo mantiene la sesión en RAM (como el sketch). Tras un reinicio
> vuelve a hacer join; con ChirpStack 1.0.3 OTAA suele bastar. Para producción, persiste la sesión de
> RadioLib en NVS.

## ➡️ Navegación
- ⬆️ Volver al ejercicio (versión Arduino): [Ejercicio 05](../README.md)
- 🏠 [Índice de ejercicios](../../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
> `EspHal.h` deriva del ejemplo oficial de RadioLib (MIT).
