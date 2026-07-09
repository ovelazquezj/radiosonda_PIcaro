# Ejercicios LR1110 — LoRa Basics Modem + ChirpStack

Cada carpeta es un **ejercicio autocontenido**: parte de que el **binario ya existe** y contiene
todo lo necesario para ejecutarlo de principio a fin — **flashear → provisionar en ChirpStack por
API → consumir los datos por API/MQTT** (la base para construir dashboards).

## Convención (molde de todo ejercicio)

```
NN_nombre/
  README.md          Qué demuestra, hardware, capacidad LR1110, credenciales, pasos
  artifacts/         El/los binario(s) del ejercicio (ya compilados)
  provision.sh       Alta en ChirpStack por API REST (cuando el ejemplo hace join)
  consume.py         Consumo de datos: REST (estado/métricas) + MQTT (payloads)
  scripts/           Helpers adicionales (fetch de estado, subir codec, etc.)
  credentials.json   DevEUI / JoinEUI / AppKey del/los device(s) del ejercicio
```
Para crear un ejercicio nuevo, copia [`_PLANTILLA/`](_PLANTILLA/) y rellena.

## Documentos comunes (compartidos por todos)

- [`COMMON_FLASH.md`](COMMON_FLASH.md) — cómo flashear un `.bin`/`.hex` en la **Nucleo-L476RG**
  (aplica a los ejercicios **01–04, que usan el LR1110**; los ejercicios TTGO **05–06** se compilan
  y flashean desde el **Arduino IDE**, no con este método).
- [`COMMON_CHIRPSTACK_API.md`](COMMON_CHIRPSTACK_API.md) — token, IDs del stack, endpoints REST,
  stream **MQTT** de uplinks y subida del **codec** al device profile.

## Índice de ejercicios

> **Dos plataformas de hardware — importante distinguirlas:**
> - **Ejercicios 01–04 → radio Semtech LR1110** ("LoRa Edge") sobre placa **Nucleo-L476RG**.
>   El binario ya viene **compilado** en cada carpeta `artifacts/`: **solo se flashea**.
> - **Ejercicios 05–06 → TTGO ESP32 LoRa** (radio **SX1276**), firmware **Arduino/LMIC**.
>   Traen el **sketch `.ino`** que **tú compilas** en el Arduino IDE.

| # | Ejercicio | Radio · Placa | Qué demuestra | ChirpStack | Dato para dashboard |
|---|-----------|---------------|---------------|:----------:|---------------------|
| [00](00_chirpstack-docker/) | ChirpStack (servidor) | — · Docker | Network Server LoRaWAN v4 (**prerequisito**) | — | — |
| [01](01_periodical-uplink/) | Periodical Uplink | **LR1110** · Nucleo-L476RG | Join OTAA + uplinks periódicos | ✅ | Contador/keep-alive por MQTT |
| [02](02_bme280-gnss-tracker/) | BMP280 + GNSS | **LR1110** · Nucleo-L476RG | Sensor I²C + GNSS/Wi-Fi | ✅ | Temperatura/presión decodificadas |
| [03](03_hw-modem/) | Hardware Modem | **LR1110** · Nucleo-L476RG | Módem controlado por un host | ✅ (claves del host) | Lo que envíe el host |
| [04](04_wifi-region-detection/) | Wi-Fi Region Detection | **LR1110** · Nucleo-L476RG | Escaneo Wi-Fi | ❌ | Solo por UART (no usa ChirpStack) |
| [05](05_ttgo-lora32/) | TTGO ESP32 LoRa | **SX1276** · TTGO ESP32 | Nodo de terceros (LMIC) | ✅ | `counter`/`timestamp` por MQTT |
| [06](06_ttgo-bmp280/) | TTGO ESP32 + BMP280 | **SX1276** · TTGO ESP32 | Sensor I²C, uplink cada 60 s | ✅ | `temperature`/`pressure` por MQTT |

## Arquitectura de datos (para dashboards)

ChirpStack separa **provisión** de **datos**:

```
                 ┌──────────── REST API (:8090) ────────────┐
   provisión ───►│  crear app / device / keys / codec        │   (estado, métricas de enlace,
                 │  leer estado del device, link metrics      │    frames recientes, downlinks)
                 └───────────────────────────────────────────┘
   uplinks  ───►  MQTT (:1883)  application/<appId>/device/<devEui>/event/up
                 └──►  payload (crudo + `object` DECODIFICADO por el codec)  ──►  tu dashboard
```

- **Provisionar y leer estado/métricas** → REST API.
- **Recibir los payloads** (la serie de datos del sensor) → **stream MQTT** de eventos `up`.
- Para que el payload llegue **ya decodificado** (campos `object.temperature`, etc.), se sube el
  **codec JS al device profile** (ver `COMMON_CHIRPSTACK_API.md` §Codec). Ese `object` es lo que
  consumirá el dashboard.
- Persistencia de series temporales (opcional): integración **InfluxDB/HTTP** de ChirpStack.

> Regla de aplicabilidad: los ejercicios que **hacen join** (01, 02, 03) usan provisión + consumo.
> El 04 (Wi-Fi) **no se une a la red**: su dato se observa por **UART**, no por ChirpStack.
