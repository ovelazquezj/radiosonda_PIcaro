# Ejercicio 04 — Wi-Fi Region Detection (capacidad Wi-Fi del LR1110)

Demuestra el **escaneo Wi-Fi pasivo** del LR1110: al reiniciar lanza `smtc_modem_wifi_scan()` y,
en el evento `WIFI_SCAN_DONE`, imprime **por UART** las MAC/RSSI de los puntos de acceso cercanos y
la **región inferida**. Importa porque es la base didáctica de la **geolocalización por Wi-Fi**
(familia LoRa Edge) — todo **sin unirse a la red**.

> **Carpeta del ejercicio:** [`specs/exercises/04_wifi-region-detection/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/04_wifi-region-detection) · **Plataforma:** radio **LR1110** sobre **Nucleo-L476RG**

| | |
|---|---|
| Qué demuestra | La **radio Wi-Fi** del LR1110 (escaneo pasivo 2.4 GHz): detecta APs e infiere región |
| Hardware | Nucleo-L476RG + shield LR1110 |
| ¿Join / ChirpStack? | ❌ **No.** No hace join, **no aparece en ChirpStack**, **sin credenciales** baked-in |
| Dato / observable | Lista de APs (**MAC + RSSI**) y **región inferida** — **solo por UART** |
| Binario / sketch | `wifi-region-detection_lr1110_multiband.{bin,hex,elf}` — **único multibanda** (US915+EU868), ya incluido en [`artifacts/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/04_wifi-region-detection/artifacts) |

## Ruta paso a paso

1. **Prepara el entorno** → [Requisitos e instalación](How-To-Requisitos-e-instalación).
   Para este ejercicio basta el **toolchain ARM** (y una terminal serie); **no** necesitas Docker/ChirpStack ni gateway, porque no hay tráfico LoRaWAN.
2. **(Opcional) Recompila** → [Compilar el firmware LR1110](How-To-Compilar-el-firmware).
   El binario **ya viene en `artifacts/`**; solo recompila si quieres tu variante. Objetivo del 04:
   `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_WIFI_REGION_DETECTION REGION=US_915,EU_868`. **No lleva credenciales compiladas** (no hace join).
3. **Flashea el binario** → [Flashear y ver la serie](How-To-Flashear-y-ver-la-serie).
   Usa `wifi-region-detection_lr1110_multiband` (`.hex` para arrastrar-y-soltar). La banda **da igual**: es multibanda porque el dispositivo **no se une** a ninguna red.
4. **Lee la traza serie** → [Flashear y ver la serie § Ver la traza serie](How-To-Flashear-y-ver-la-serie).
   Aquí termina el ejercicio: el resultado se observa en la **UART**. **No hay paso de provisionar ni de consumir** (ver *Credenciales y detalles*).

## Qué observar

- Abre una **terminal serie a 115200 8N1** sobre el VCP del ST-LINK (`/dev/ttyACM0`, `COMx`…).
- Pulsa **RESET (B2)**: cada pulsación repite el ciclo *escaneo → resultado*.
- Busca en la traza el evento **`WIFI_SCAN_DONE`**.
- Lee la lista de **APs detectados (MAC + RSSI)** y la **región inferida** a partir de ellos. Ese es el dato del ejercicio.
- En **ChirpStack**: *nada* — este demo no genera tráfico LoRaWAN (ni *last seen*, ni frames, ni MQTT).

## Credenciales y detalles

- **No hay credenciales** (DevEUI/JoinEUI/AppKey = `null`). El [`credentials.json`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/04_wifi-region-detection/credentials.json) marca explícitamente `goes_to_chirpstack: false`: no hay device que dar de alta ni provisión/consumo (secciones **N/A** frente a los ejercicios 01/05).
- El binario es **multibanda** (US915+EU868) precisamente porque, al no unirse a la red, la banda es irrelevante.
- **Nota — geolocalización Wi-Fi (LoRa Edge):** en un producto real, la lista de MACs/RSSI se enviaría por LoRaWAN a un servicio de resolución (p. ej. LoRa Cloud) que devuelve una posición estimada **sin GNSS**. Aquí nos quedamos en el **paso previo y observable**: demostrar que el LR1110 escanea Wi-Fi y produce esos datos.

## Ficheros del ejercicio

- [`README.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/04_wifi-region-detection/README.md) — guía completa del ejercicio (fuente de verdad).
- `artifacts/wifi-region-detection_lr1110_multiband.{bin,hex,elf}` — binario único multibanda, listo para flashear.
- `credentials.json` — marca "no va a ChirpStack" (sin credenciales).

> ◀ [Ejercicio 03 — Hardware Modem](Ejercicio-03-Hardware-Modem) · [Ejercicio 05 — TTGO LoRa32 ▶](Ejercicio-05-TTGO-LoRa32)
