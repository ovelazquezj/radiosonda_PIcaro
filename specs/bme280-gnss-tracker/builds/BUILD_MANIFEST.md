# BUILD MANIFEST — Tracker BMP280 + GNSS (LR1110)

Proyecto **BME**: tracker ambiental (BMP280) + geolocalización GNSS/Wi-Fi sobre LR1110.
Registro trazable de cada binario y sus credenciales LoRaWAN.

> ⚠️ Credenciales de **LABORATORIO** deterministas (prefijo `AA:BB:CC:DD`).
> **Reemplazar antes de una red productiva.** Fuente de verdad por banda:
> [`creds_us915.h`](creds_us915.h) · [`creds_eu868.h`](creds_eu868.h).
>
> Los demos didácticos (Periodical Uplink, HW Modem, Wi-Fi) se movieron al proyecto
> **`specs/demos/`** — ver [`../../demos/BUILD_MANIFEST.md`](../../demos/BUILD_MANIFEST.md).

## Convención de nombre

`tracker-bme280-gnss_<radio>_<region>`. El sensor físico es **BMP280** (temp+presión, sin humedad).

## Estado global

| Item | Resultado |
|------|-----------|
| Sensor | BMP280 — I²C1 SCL=PB_8 / SDA=PB_9, addr 0x76 |
| Payload sensor | fport **10**, 5 bytes big-endian: `version(0x02) · temp×100 (int16) · presión hPa (uint16)` |
| Toolchain | arm-none-eabi-gcc 13.2.1 · ambos builds SUCCESS, sin warnings nuevos |
| Símbolos verificados (`nm`) | `bmp280_init`, `bmp280_read`, `bmp280_payload_encode`, `smtc_hal_i2c_*` |

## Builds

| Build | Región | DevEUI | JoinEUI | AppKey | Artefacto (.bin) | md5 |
|---|---|---|---|---|---|---|
| `tracker-bme280-gnss_lr1110_us915` | US_915 | `AABBCCDD00915001` | `AABBCCDDEEFF0000` | `A915A915A915A915A915A915A915A915` | `app_lr1110_US_915.bin` (135 720 B) | `1c32bcab3a65f5df2cf88fdf6ff65810` |
| `tracker-bme280-gnss_lr1110_eu868` | EU_868 | `AABBCCDD00868001` | `AABBCCDDEEFF0000` | `A868A868A868A868A868A868A868A868` | `app_lr1110_EU_868.bin` (135 600 B) | `09c54117e14483135908babc523b0714` |

Build: `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_GEOLOCATION REGION=<banda>`

## Notas

- `full_lr1110` hace `clean` y ambos builds generan `app_lr1110_<REGION>.bin`; por eso se copian a
  `builds/artifacts/` con su región en el nombre.
- JoinEUI compartido; DevEUI y AppKey únicos por banda. En 1.0.x el AppKey se usa como NwkKey.
- Flasheo y alta en ChirpStack: [`../FLASH_AND_CHIRPSTACK.md`](../FLASH_AND_CHIRPSTACK.md).
- Diagrama de pines del sensor: [`../pinout_diagram.html`](../pinout_diagram.html).
