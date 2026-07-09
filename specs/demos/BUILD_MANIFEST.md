# BUILD MANIFEST — Demos LR1110 (para alumnos)

Proyecto **Demos**: ejemplos para enseñar las capacidades del LR1110 y la conexión hacia ChirpStack.
Binarios ya compilados en [`artifacts/`](artifacts/). Instrucciones paso a paso:
[`DEMOS_INSTRUCCIONES.md`](DEMOS_INSTRUCCIONES.md).

> ⚠️ Credenciales de **LABORATORIO** deterministas (prefijo `AA:BB:CC:DD`).
> **Reemplazar antes de una red productiva.**
> El proyecto del sensor BMP280 vive aparte en [`../bmp280-gnss-tracker/`](../bmp280-gnss-tracker/).

## Esquema de credenciales

- **DevEUI:** `AA BB CC DD · <ID-ejemplo> · <tag-banda> · 01` — ID: periodical `10`, wifi `20`, hw-modem `30`; tag-banda US=`91 50`, EU=`86 80`.
- **JoinEUI (=AppEUI):** `AA BB CC DD EE FF 00 00` (común).
- **AppKey:** `[ID-ejemplo, byte-banda]`×8 — byte-banda US=`15`, EU=`68`.

## Builds

### 1. Periodical Uplink — conectividad LoRaWAN (SÍ va a ChirpStack)

OTAA join + uplinks periódicos y al pulsar el botón azul (B1). El demo base de conectividad.

| Build | Banda | DevEUI | JoinEUI | AppKey | Artefacto | md5 |
|---|---|---|---|---|---|---|
| `periodical-uplink_lr1110_us915` | US_915 | `AABBCCDD10915001` | `AABBCCDDEEFF0000` | `10151015101510151015101510151015` | `periodical-uplink_lr1110_us915.bin` | `780b66f02b3b39059ad037c814fb3b42` |
| `periodical-uplink_lr1110_eu868` | EU_868 | `AABBCCDD10868001` | `AABBCCDDEEFF0000` | `10681068106810681068106810681068` | `periodical-uplink_lr1110_eu868.bin` | `58dc5bd59a1424eb89187c94cc118082` |

Build: `make -C lbm_examples full_lr1110 MODEM_APP=PERIODICAL_UPLINK REGION=<banda>`

### 2. Hardware Modem — módem por comandos UART (SÍ va a ChirpStack, credenciales en runtime)

| Build | Bandas | Credenciales | Artefacto | md5 |
|---|---|---|---|---|
| `hw-modem_lr1110_multiband` | US_915 + EU_868 | **Ninguna baked-in** — DevEUI/JoinEUI/AppKey y **región** los fija el host por comando (`cmd_parser.c`) | `hw-modem_lr1110_multiband.bin` | `2fee1a2b4d30997dff976d536b22ecaa` |

Build: `make -C lbm_examples full_lr1110 MODEM_APP=HW_MODEM REGION=US_915,EU_868`

### 3. Wi-Fi Region Detection — capacidad Wi-Fi (NO va a ChirpStack)

| Build | Bandas | Credenciales | Artefacto | md5 |
|---|---|---|---|---|
| `wifi-region-detection_lr1110_multiband` | US_915 + EU_868 | **N/A** | `wifi-region-detection_lr1110_multiband.bin` | `4bcbfb669711544c2ccb90cc0c02ecba` |

Build: `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_WIFI_REGION_DETECTION REGION=US_915,EU_868`

> ⚠️ **No hace join ni uplink.** Sólo escanea Wi-Fi y muestra resultados **por UART**. No aparece en ChirpStack.

## Resumen ¿cuáles registrar en ChirpStack?

| Build | ¿Join/ChirpStack? | Credenciales para dar de alta |
|---|---|---|
| `periodical-uplink_lr1110_us915` | ✅ | DevEUI `AABBCCDD10915001`, AppKey `1015…` |
| `periodical-uplink_lr1110_eu868` | ✅ | DevEUI `AABBCCDD10868001`, AppKey `1068…` |
| `hw-modem_lr1110_multiband` | ✅ | Las que envíe el host por comando (runtime) |
| `wifi-region-detection_lr1110_multiband` | ❌ | — (no se une a la red) |
