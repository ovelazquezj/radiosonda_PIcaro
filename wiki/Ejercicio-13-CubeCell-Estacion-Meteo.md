# Ejercicio 13 — 🌦️ Estación meteorológica LoRaWAN con Heltec CubeCell (HTCC-AB01)

Conviertes una **Heltec CubeCell HTCC-AB01** (ASR6501 + radio **SX1262**, pensada para batería y panel
solar) en una **estación meteorológica LoRaWAN**: hace **join OTAA** al ChirpStack del curso y envía
cada minuto un paquete de 11 bytes con batería, temperatura y humedad (**DHT11**), presión (**BMP280**),
luz (**BH1750**) y lluvia (**MH-RD**). Se construye **por etapas**: primero el join (sin sensores),
después cada sensor se activa con una constante cuando ya está cableado.

> **Carpeta del ejercicio:** [`specs/exercises/13_cubecell-estacion-meteo/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/13_cubecell-estacion-meteo) · **Plataforma:** **Heltec CubeCell HTCC-AB01 V1.2**, firmware **Arduino IDE + CubeCell Development Framework** (pila LoRaWAN de Heltec)

| | |
|---|---|
| Qué demuestra | Join OTAA con la **pila LoRaWAN de Heltec**, nodo de **bajo consumo** (duerme entre envíos), payload compacto que **cabe en DR0** y crece por etapas con sensores |
| Hardware | **CubeCell HTCC-AB01** + antena 915 MHz; batería LiPo opcional; sensores **DHT11, BMP280, BH1750, MH-RD**; un **gateway US915 sub-banda 2** online |
| ¿Join / ChirpStack? | ✅ **Sí** (OTAA, LoRaWAN **1.0.3**, Clase A) contra `lns.pi-caro.org`; codec `payload_decoder.js` en el device profile |
| Dato / observable | **11 bytes** big-endian: `status`, `vbat`, `temp_dht`, `hum`, `temp_bmp`, `presion`, `lux`, `lluvia_pct` → `object` decodificado |
| Sketch | [`cubecell_meteo.ino`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/13_cubecell-estacion-meteo/sketches/cubecell_meteo/cubecell_meteo.ino) |

**Parámetros de red (ya fijados):** Región **US915** · **sub-banda 2** (máscara `0xFF00`) · **OTAA** ·
Clase **A** · LoRaWAN **1.0.x** · ADR **ON** · uplink cada **60 s** en **fPort 2**, uno confirmado cada 5.

> ✅ **Verificado en hardware el 2026-09-17:** join a la primera, DevAddr asignado, uplinks cada 60 s,
> ADR bajando la potencia de 20 a 16 dBm. Las salidas del README son capturas reales.

> 📖 **La guía completa paso a paso está en el
> [README del ejercicio](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/13_cubecell-estacion-meteo)**
> (obtener el repo, paquete CubeCell en Arduino IDE, opciones de la placa, credenciales, compilar,
> flashear, verificar el join, y cablear y activar cada sensor). Esta página es el índice/resumen.

## Qué tiene de distinto esta placa

- **La pila LoRaWAN viene en el paquete de placas**, no en una librería: se configura con **variables
  globales** de nombre fijo (`devEui`, `appEui`, `appKey`, `userChannelsMask`…) y con **opciones del menú
  Tools** (región, clase, OTAA, ADR…). Si una opción del menú está mal, compila igual pero no hace join.
- **Duerme de verdad.** Entre uplinks el MCU entra en bajo consumo (microamperios). Por eso los
  sensores se alimentan de **Vext** (3.3 V conmutados) solo mientras se mide.
- **El payload cabe en DR0** (11 bytes). La pila arranca en el datarate más lento; un payload mayor
  fallaría con `payload length error` hasta que ADR subiera el DR.

## Ruta paso a paso

1. **Paquete CubeCell** en Arduino IDE (URL `https://resource.heltec.cn/download/package_CubeCell_index.json`, placa *CubeCell-Board (HTCC-AB01)*).
2. **Menú Tools:** `REGION_US915`, `CLASS_A`, `DEVEUI CUSTOM`, `OTAA`, `ADR ON`, `UNCONFIRMED`, `Net_Reserve OFF`, `AT_SUPPORT OFF`, `RGB ACTIVE`, `Debug Level Freq`.
3. **`config_lorawan.h`:** DevEUI, JoinEUI y AppKey como arreglos de bytes en MSB, tal como los muestra ChirpStack.
4. **Compilar, flashear (COM del CP2102) y Monitor Serie a 115200:** `joining...` → `TX on freq 904300000 Hz at DR 3` → `joined`, y cada minuto `UPLINK #n` con el payload en hex.
5. **ChirpStack:** *LoRaWAN frames* con `JoinRequest`/`JoinAccept`, *Events* con el `object`; pegar `payload_decoder.js` en el **Codec** del device profile.
6. **Sensores, uno a uno:** cablear a Vext/GND y sus pines, poner `SENSOR_xxx 1` en `config_sensores.h`, reflashear y ver la lectura en el Monitor Serie y en el `object`.
7. **Energía:** batería LiPo 3.7 V al conector BAT (SH1.25-2) y celda solar de 6 V, bien a la entrada solar de la placa (5.5–7 V), bien a través de un **módulo de carga USB-C TP4056** (panel → IN, batería → B, OUT → BAT de la CubeCell). Detalle y advertencias de polaridad en el README, Etapa 5.

## Cableado (todos a Vext y GND)

| Sensor | Pines de la CubeCell | Mide |
|---|---|---|
| DHT11 | DATA → **GPIO5** | temperatura (°C) y humedad (%) |
| BMP280 | SDA → **SDA**, SCL → **SCL** (0x76) | presión (hPa) y temperatura |
| BH1750 | SDA → **SDA**, SCL → **SCL** (0x23) | luz (lux) |
| MH-RD | AO → **ADC**, DO → **GPIO1** | lluvia (% y sí/no) |
| Batería + solar | LiPo → **BAT**; panel 6 V → **SOLAR/VS** o vía TP4056 → **BAT** | energía autónoma (`vbat_mv`) |

## Estructura del payload (11 bytes, big-endian, fPort 2)

| Byte(s) | Campo | Tipo | Codificación | "no hay" |
|:---:|---|---|---|:---:|
| 0 | `status` | uint8 | bit0 DHT11 · bit1 BMP280 · bit2 BH1750 · bit3 lluvia (DO) · bit4 MH-RD | |
| 1 | `vbat` | uint8 | voltaje / 20 mV | 0 |
| 2 | `temp_dht` | int8 | °C | 0x7F |
| 3 | `hum_dht` | uint8 | % | 0xFF |
| 4–5 | `temp_bmp` | int16 | °C × 10 | 0x7FFF |
| 6–7 | `presion` | uint16 | hPa × 10 | 0xFFFF |
| 8–9 | `lux` | uint16 | lux | 0xFFFF |
| 10 | `lluvia_pct` | uint8 | 0–100 | 0xFF |

El codec **omite** los campos "no hay", así el dashboard sabe qué sensores están conectados.

## Ficheros del ejercicio

- `sketches/cubecell_meteo/cubecell_meteo.ino` — la lógica (join, sensores, payload, Serie).
- `sketches/cubecell_meteo/config_lorawan.h` — ⭐ credenciales y parámetros de red.
- `sketches/cubecell_meteo/config_sensores.h` — ⭐ qué sensores están conectados y en qué pin.
- `payload_decoder.js` — codec para el device profile.
- `scripts/subscribe.sh` — escucha MQTT (TLS) del servidor del curso.
- Guía completa en el [README de la carpeta](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/13_cubecell-estacion-meteo).

> *Basado en los ejemplos LoRaWAN del CubeCell Development Framework de Heltec. Licencia MIT. Radio SX1262 · US915 · OTAA · LoRaWAN 1.0.x.*

> ◀ [Ejercicio 12 — Join OTAA con Heltec LoRa 32](Ejercicio-12-Heltec-LoRa32-Join)

---
_Docs © Omar Velazquez · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)_
