# VERSIONS.md — Firmware y dependencias fijadas (ejercicio 07)

Guía de **reproducibilidad** del gateway ESP-1ch-Gateway. El repositorio de terceros **no publica
releases ni tags** (`/tags` y `/releases/latest` están vacíos), así que la única forma fiable de
fijar una versión es **anclar el commit** y apoyarse en las librerías **vendorizadas** dentro del
propio repo.

## Firmware — commit exacto

| | |
|---|---|
| Repositorio | https://github.com/things4u/ESP-1ch-Gateway (Maarten Westenberg / things4u, **MIT**) |
| Commit fijado | **`3b023527bd23cf33657dc7ffdf5bedaf1b85cdcc`** |
| Fecha | 2021-10-18 · mensaje de commit *"Version 6.2.8: TTN3"* |
| Versión interna | `#define VERSION "V.6.2.8.EU868; PlatformIO 211015 a; distri GIT"` (en `src/configGway.h`) |
| Releases / tags | **Ninguno** — no existe un tag `v6.2.8`; se ancla por SHA |

```bash
git clone https://github.com/things4u/ESP-1ch-Gateway.git
cd ESP-1ch-Gateway
git checkout 3b023527bd23cf33657dc7ffdf5bedaf1b85cdcc
```

## Dependencias

La mayoría de librerías van **vendorizadas** en `lib/` del propio repo, así que **al fijar el commit
quedan congeladas** (no dependes de versiones externas). Contenido de `lib/`:

- `ArduinoJson`, `Streaming`, `Time`, `TinyGPSPlus-1.0.2b`, `WiFiManager-development`, `aes`,
  `gBase64`, `LoRaCode`, `esp32-http-update`, y los drivers OLED **SSD1306** (ESP8266/ESP32).

La **única** dependencia que se resuelve fuera es la declarada en `platformio.ini`:

```ini
lib_deps =
  sandeepmistry/LoRa @ ^0.8.0
```

> Para un pin **exacto** (no un rango caret) usa `sandeepmistry/LoRa @ 0.8.0`.

**No verificado:** las versiones internas de las librerías vendorizadas (salvo `TinyGPS++ 1.0.2b`,
que va en el nombre de la carpeta) no están etiquetadas; quedan definidas por el commit anclado.
El README del repo cita "ArduinoJson 6.10.0+", pero esa versión **no** está pinneada en el proyecto.

## PlatformIO — entorno para TTGO ESP32 por USB

El `platformio.ini` del repo trae **un único entorno activo** (`[env:Gateway_38]`) pensado para
**Heltec** y **subida OTA** (`upload_port = 192.168.2.38`). Para una **TTGO LoRa32 por USB**, añade
tu propio entorno (ajusta `board` a tu revisión de TTGO y `upload_port` a tu puerto serie):

```ini
[env:ttgo_usb]
platform = espressif32
board = ttgo-lora32-v1        ; o ttgo-lora32-v2 / ttgo-lora32-v21 segun tu placa
framework = arduino
lib_deps =
  sandeepmistry/LoRa @ 0.8.0
build_flags =
  -D _WIFIMANAGER=0
  -D _OLED=1
  -D _DUSB=1
  -D _STRICT_1CH=1
upload_protocol = esptool
upload_port = COM5            ; <-- tu puerto (Windows COMx; Linux /dev/ttyUSB0)
upload_speed = 921600
monitor_speed = 115200
```

```bash
pio run -e ttgo_usb -t upload      # compilar + flashear por USB
pio device monitor -b 115200       # ver el Gateway EUI y los uplinks
```

> El pin del radio (`_PIN_OUT 4` = ESP32/TTGO/Heltec) se fija en `configGway.h`
> (ver [`configGway.example.h`](configGway.example.h)), no en el `board` de PlatformIO.

## Arduino IDE (alternativa)

Si compilas con Arduino IDE en vez de PlatformIO: instala el core **ESP32**, la librería
**`LoRa` de sandeepmistry `0.8.0`**, y usa las librerías que trae el repo en `lib/` (cópialas a tu
carpeta de librerías de Arduino si el IDE no las encuentra). Selecciona la placa *TTGO LoRa32* y el
puerto COM correcto.

## Notas de reproducibilidad

- El entorno activo del repo usa `-D _STRICT_1CH=2` (modo estricto reforzado). El ejemplo de
  [`configGway.example.h`](configGway.example.h) usa `_STRICT_1CH 1`; ambos sirven para 1 canal
  (el `2` es más restrictivo con las ventanas de downlink).
- Firmware **sin mantenimiento desde 2021**: no lo migres a dependencias más nuevas sin probar,
  o dejará de compilar.
- El **Gateway EUI** se deriva de la MAC del ESP en runtime; **no** hay un `#define`. Léelo del
  serial/web y regístralo tal cual en ChirpStack.
