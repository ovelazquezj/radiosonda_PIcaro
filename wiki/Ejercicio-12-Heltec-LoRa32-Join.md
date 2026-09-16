# Ejercicio 12 — 📡 Join OTAA con Heltec WiFi LoRa 32 (V3 / V4) y credenciales generadas en ChirpStack

Haces que una **Heltec WiFi LoRa 32** (ESP32-S3 + radio **SX1262** + OLED) se una por **OTAA** al
ChirpStack del curso usando un **DevEUI, JoinEUI y AppKey que genera ChirpStack** y que tú copias a la
placa. El join y cada paquete se ven con todo detalle en el **Monitor Serie** y resumidos en la **OLED**.
Lo único que editas es **`config_lorawan.h`**. Sirve para la **V3** y la **V4** de la placa.

> **Carpeta del ejercicio:** [`specs/exercises/12_heltec-lora32-v3-join/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/12_heltec-lora32-v3-join) · **Plataforma:** **Heltec WiFi LoRa 32 V3 / V4** (ESP32-S3 + **SX1262** + OLED SSD1306), firmware **Arduino IDE / RadioLib**

| | |
|---|---|
| Qué demuestra | **Join OTAA** con identidades creadas en el **servidor** (flujo de producción), uplinks periódicos y un uplink **confirmado** cada 5 para ver un ACK real |
| Hardware | **Heltec WiFi LoRa 32 V3 o V4** + antena de 915 MHz; batería opcional; un **gateway US915 sub-banda 2** online en el ChirpStack del curso |
| ¿Join / ChirpStack? | ✅ **Sí** (OTAA, LoRaWAN **1.0.x**, Clase A) contra `lns.pi-caro.org`; codec `payload_decoder.js` en el device profile |
| Dato / observable | **9 bytes** big-endian: `counter`, `uptime_s`, `vbat_mv` → `object` decodificado |
| Sketch | [`heltec_lora32_join.ino`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/12_heltec-lora32-v3-join/sketches/heltec_lora32_join/heltec_lora32_join.ino) (Arduino IDE + **RadioLib 7.x** + **U8g2**) |

**Parámetros de red (ya fijados en el código):** Región **US915** · **sub-banda 2** (canales 8–15) ·
**OTAA** · Clase **A** · LoRaWAN **1.0.x** (solo AppKey) · DR3 inicial con ADR.

> 📖 **La guía completa paso a paso está en el
> [README del ejercicio](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/12_heltec-lora32-v3-join)**
> (instalar Arduino IDE + ESP32, librerías, generar las credenciales en la consola, copiarlas al
> código, opciones de placa V3/V4, compilar, flashear y verificar). Esta página es el índice/resumen.

## Qué cambia respecto al 08

```
  ej.08   la placa trae credenciales  ──►  tú las das de alta en ChirpStack
  ej.12   ChirpStack genera las credenciales  ──►  tú las copias a la placa
```

Es el flujo real: el **servidor** es el dueño de las identidades. Un DevEUI generado no colisiona con el
de otro equipo, y la AppKey solo viaja de la consola a tu `config_lorawan.h`.

## Ruta paso a paso

1. **Arduino IDE + ESP32** — core `esp32` de Espressif (3.x) y librerías **RadioLib** (7.x) y **U8g2**.
2. **Device profile** en la consola del curso — `Heltec-Join-US915`: US915, LoRaWAN 1.0.3, RP A, OTAA, Clase A; pega `payload_decoder.js` en **Codec**.
3. **Device** — en tu aplicación: **Device EUI** con el icono de **generar**, **Join EUI** también generado, profile `Heltec-Join-US915`. En **OTAA keys**, **Application key** generada. Copia los tres valores en **MSB**.
4. **`config_lorawan.h`** — `HELTEC_BOARD_VERSION` (3 o 4) y las tres constantes: `0x` + 16 hex + `ULL` para los EUI; 16 bytes `0x..` para la AppKey. **Sin invertir nada.**
5. **Placa y opciones** — *"Heltec WiFi LoRa 32(V3)"* (sirve para la V4 si tu core no la lista), **Partition Scheme = Huge APP**; en la **V4**, **USB CDC On Boot = Enabled** (USB nativo).
6. **Flashear y Monitor Serie (115200)** — banner, pines, credenciales, `JOIN EXITOSO ... llego el Join-Accept`, `DevAddr`, y cada `UPLINK #n` con frecuencia, datarate, potencia, fCnt y tiempo en aire. Cada 5 uplinks, un `DOWNLINK en RX1 ... [ACK]`.
7. **OLED** — `JOIN OTAA intento n` → `CONECTADO (JOIN OK)` con DevAddr y DR → `TX #n fCnt …` con batería, RSSI/SNR del último downlink y resultado.
8. **ChirpStack** — device → **LoRaWAN frames** (`JoinRequest` / `JoinAccept` / `UnconfirmedDataUp`) y **Events** con el `object`. Opcional: `scripts/subscribe.sh` por MQTT con TLS.

## V3 y V4: qué cambia y qué resuelve el código

| | V3 | V4 (4.2 / 4.3) |
|---|---|---|
| USB | CP2102 (UART) | USB nativo del ESP32-S3 → *USB CDC On Boot* |
| Control del ADC de batería (GPIO37) | activo en **BAJO** | activo en **ALTO** |
| Amplificador de RF | no | sí (GC1109 o KCT8103L); el sketch lo alimenta y habilita |
| Potencia por defecto del SX1262 | 14 dBm | **2 dBm** (+~11 dB del amplificador) |

Radio (SX1262: NSS 8, SCK 9, MOSI 10, MISO 11, RST 12, BUSY 13, DIO1 14) y OLED (SDA 17, SCL 18,
RST 21, **Vext GPIO36 activo en bajo**) son iguales. Todo vive en `board_heltec.h`.

## Estructura del payload (9 bytes, big-endian, fPort 1)

| Byte(s) | Campo | Tipo | Codificación |
|:---:|---|---|---|
| 0 | `magic` | uint8 | `0x48` ('H'); el codec rechaza lo que no lo traiga |
| 1–2 | `counter` | uint16 | número de paquete |
| 3–6 | `uptime_s` | uint32 | segundos encendida |
| 7–8 | `vbat_mv` | uint16 | milivolts (0 = sin batería / USB) |

## Diagnóstico: leer el error correcto

| Mensaje del Monitor Serie | Qué sospechar |
|---|---|
| `NO LLEGO EL JOIN-ACCEPT` | gateway offline, sub-banda distinta, o DevEUI/JoinEUI que ChirpStack no conoce |
| `MIC INCORRECTO` | la AppKey de la placa no es la de ChirpStack (un byte, o copiada en LSB) |
| *DevNonce already used* (en la consola) | borraste y recreaste el device: `PICARO_RESET_NONCES 1` una vez |
| OLED negra | Vext (GPIO36) o `HELTEC_BOARD_VERSION` equivocada |
| Nada en el Monitor Serie (V4) | falta **USB CDC On Boot = Enabled** |

## Ficheros del ejercicio

- `sketches/heltec_lora32_join/heltec_lora32_join.ino` — la lógica (join, uplinks, Serie, OLED).
- `sketches/heltec_lora32_join/config_lorawan.h` — ⭐ versión de placa y credenciales — **lo único que editas**.
- `sketches/heltec_lora32_join/board_heltec.h` — pines de la V3 y la V4.
- `payload_decoder.js` — codec para el device profile.
- `scripts/subscribe.sh` — escucha MQTT (TLS) del servidor del curso.
- Guía completa en el [README de la carpeta](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/12_heltec-lora32-v3-join).

> ⚠️ Las salidas esperadas del README están escritas a partir del código, no de una captura real;
> los textos son exactos y los números orientativos.

> *Basado en los ejemplos LoRaWAN de RadioLib. Licencia MIT. Radio SX1262 · US915 · OTAA · LoRaWAN 1.0.x.*

> ◀ [Ejercicio 11 — Dashboard contra servidor remoto](Ejercicio-11-Dashboard-Servidor-Remoto)

---
_Docs © Omar Velazquez · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)_
