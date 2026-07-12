# COMMON — Compilar los binarios (ejercicios LR1110: 01–04)

Cada ejercicio LR1110 **ya trae su binario compilado** en `artifacts/` (listo para flashear). Esta
guía es para **recompilarlos tú** — cambiar credenciales, región, periodo o payload — y así aprender
el flujo completo. Los ejercicios TTGO (**05–06**) no usan esto: se compilan en el **Arduino IDE**.

## Requisitos
- **GNU Arm Embedded Toolchain** 13.2.rel1 (`arm-none-eabi-gcc`) en el `PATH`.
- **make** (y, opcional, `cmake` + `ninja`).

## Paso 1 — Cambiar DevEUI / JoinEUI / AppKey y región

Las credenciales van **escritas en el firmware**, en el archivo `example_options.h` de tu ejercicio
(rutas **relativas a la raíz del repo**, `C:\dev\radiosonda_PIcaro`):

| Ejercicio | Archivo a editar |
|-----------|------------------|
| 01, 03 | `lbm_examples/main_examples/example_options.h` |
| 02, 04 | `lbm_applications/3_geolocation_on_lora_edge/main_geolocation/example_options.h` |

Ábrelo y edita estos **tres** `#define` con los valores del `credentials.json` de tu ejercicio:

- `USER_LORAWAN_DEVICE_EUI` → tu **DevEUI** (8 bytes)
- `USER_LORAWAN_JOIN_EUI` → tu **JoinEUI** (8 bytes)
- `USER_LORAWAN_APP_KEY` → tu **AppKey** (16 bytes)

Cada byte es un `0xNN` separado por comas, y van en **orden MSB** — el mismo orden que muestra
ChirpStack, leído de izquierda a derecha. Las longitudes son fijas (8 / 8 / 16 bytes); no añadas ni
quites elementos. Así se ven en el archivo (valores de ejemplo que debes sustituir):

```c
#define USER_LORAWAN_DEVICE_EUI                        \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0x10, 0x86, 0x80, 0x01 \
    }
#define USER_LORAWAN_JOIN_EUI                          \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 \
    }
#define USER_LORAWAN_APP_KEY                                                                           \
    {                                                                                                  \
        0x10, 0x68, 0x10, 0x68, 0x10, 0x68, 0x10, 0x68, 0x10, 0x68, 0x10, 0x68, 0x10, 0x68, 0x10, 0x68 \
    }
```

**Cómo convertir un valor de ChirpStack:** parte el hex de dos en dos dígitos, prefija cada par con
`0x` y sepáralos por comas. Si tu DevEUI en ChirpStack es `70B3D57ED0060AF3`:

```c
#define USER_LORAWAN_DEVICE_EUI                        \
    {                                                  \
        0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x0A, 0xF3 \
    }
```

> ⚠️ En el mismo archivo verás también `USER_LORAWAN_GEN_APP_KEY`. **No lo confundas con el AppKey:**
> es para otro modo; para el join OTAA normal el que importa es `USER_LORAWAN_APP_KEY`. Puedes dejar
> `GEN_APP_KEY` como está.
>
> ⚠️ El DevEUI / JoinEUI / AppKey deben **coincidir exactamente** (mismos bytes, mismo orden MSB) con
> lo que registres en ChirpStack. Es la causa nº1 de fallos de join.

Ajusta también la **región** en el mismo archivo, y usa la misma banda en el `REGION=` del Paso 2:

```c
#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_US_915   // o SMTC_MODEM_REGION_EU_868
```

> El **03** y el **04** no llevan credenciales compiladas (el 03 las recibe por el host/UART; el 04
> solo escanea Wi-Fi y no hace join): en esos solo tocas la región.

## Paso 2 — Compilar (comando por ejercicio)

| Ej. | Comando de compilación |
|-----|------------------------|
| 01 | `make -C lbm_examples full_lr1110 MODEM_APP=PERIODICAL_UPLINK REGION=US_915` |
| 02 | `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_GEOLOCATION REGION=US_915` |
| 03 | `make -C lbm_examples full_lr1110 MODEM_APP=HW_MODEM REGION=US_915,EU_868` |
| 04 | `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_WIFI_REGION_DETECTION REGION=US_915,EU_868` |

> Cambia `REGION=EU_868` para la banda europea. `full_lr1110` limpia y recompila desde cero.

## Paso 3 — Dónde queda el binario
El `.bin` (junto con `.hex` y `.elf`) se genera en `build_lr1110_l4/`, **dentro de la carpeta que
compilaste** — la del `make -C`, no la raíz del repo:

| Ejercicio | Ruta del binario (relativa a la raíz del repo) |
|-----------|------------------------------------------------|
| 01, 03 | `lbm_examples/build_lr1110_l4/app_lr1110_<REGION>.bin` |
| 02, 04 | `lbm_applications/3_geolocation_on_lora_edge/build_lr1110_l4/app_lr1110_<REGION>.bin` |

Por ejemplo, el **01 en US915** →
`lbm_examples/build_lr1110_l4/app_lr1110_US_915.bin`. Cópialo a la carpeta `artifacts/` del ejercicio
si quieres conservarlo, y flashéalo (ver [`COMMON_FLASH.md`](COMMON_FLASH.md)).

## Modifica el firmware y crea tus propios binarios
Este proyecto es para **experimentar**: cambia el periodo de envío, el payload, los servicios
(GNSS/Wi-Fi), o integra tu propio sensor, y recompila para generar **tus** variantes.
**Happy hacking 🚀**
