# COMMON — Compilar los binarios (ejercicios LR1110: 01–04)

Los binarios **no** vienen en el repositorio: los **compilas tú** con el toolchain (así aprendes el
flujo completo). Los ejercicios TTGO (**05–06**) no usan esto: se compilan en el **Arduino IDE**.

## Requisitos
- **GNU Arm Embedded Toolchain** 13.2.rel1 (`arm-none-eabi-gcc`) en el `PATH`.
- **make** (y, opcional, `cmake` + `ninja`).

## Paso 1 — Poner credenciales y región
Antes de compilar, copia las credenciales de tu device (DevEUI/JoinEUI/AppKey del
`credentials.json` del ejercicio) y la región en el `example_options.h` correspondiente:

| Ejercicio | Archivo de credenciales/región a editar |
|-----------|-----------------------------------------|
| 01, 03 | `lbm_examples/main_examples/example_options.h` |
| 02, 04 | `lbm_applications/3_geolocation_on_lora_edge/main_geolocation/example_options.h` |

Ajusta también `MODEM_EXAMPLE_REGION` (`SMTC_MODEM_REGION_US_915` o `SMTC_MODEM_REGION_EU_868`) a
tu banda.

## Paso 2 — Compilar (comando por ejercicio)

| Ej. | Comando de compilación |
|-----|------------------------|
| 01 | `make -C lbm_examples full_lr1110 MODEM_APP=PERIODICAL_UPLINK REGION=US_915` |
| 02 | `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_GEOLOCATION REGION=US_915` |
| 03 | `make -C lbm_examples full_lr1110 MODEM_APP=HW_MODEM REGION=US_915,EU_868` |
| 04 | `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_WIFI_REGION_DETECTION REGION=US_915,EU_868` |

> Cambia `REGION=EU_868` para la banda europea. `full_lr1110` limpia y recompila desde cero.

## Paso 3 — Binario resultante
Se genera en la carpeta del proyecto compilado:
```
build_lr1110_l4/app_lr1110_<REGION>.bin   (+ .hex y .elf)
```
Cópialo a la carpeta `artifacts/` del ejercicio si quieres, y flashéalo (ver
[`COMMON_FLASH.md`](COMMON_FLASH.md)).

## Modifica el firmware y crea tus propios binarios
Este proyecto es para **experimentar**: cambia el periodo de envío, el payload, los servicios
(GNSS/Wi-Fi), o integra tu propio sensor, y recompila para generar **tus** variantes.
**Happy hacking 🚀**
