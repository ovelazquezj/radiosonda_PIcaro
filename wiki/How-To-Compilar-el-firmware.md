# How-To · Compilar el firmware LR1110 (ejercicios 01–04)

Los binarios **no** vienen en el repo: los **compilas tú** (así aprendes el flujo completo). Esta
guía asume que ya instalaste el toolchain ARM (ver
[Requisitos e instalación](How-To-Requisitos-e-instalación)). Los ejercicios TTGO (**05–06**) **no**
usan esto: ver [How-To Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).

> 🪟 **En Windows:** estos comandos `make` se ejecutan **dentro de WSL2 (Ubuntu)** — Git Bash y
> PowerShell **no** traen el toolchain. El repo se ve desde WSL en `/mnt/c/...`. Montaje y toolchain
> en [Requisitos e instalación](How-To-Requisitos-e-instalación).

## Paso 1 · Cambiar tu DevEUI / JoinEUI / AppKey y región
Las credenciales van **escritas en el firmware**, en el `example_options.h` de tu ejercicio (rutas
**relativas a la raíz del repo**, `C:\dev\radiosonda_PIcaro`):

| Ejercicio | Archivo a editar |
|-----------|------------------|
| 01, 03 | `lbm_examples/main_examples/example_options.h` |
| 02, 04 | `lbm_applications/3_geolocation_on_lora_edge/main_geolocation/example_options.h` |

Ábrelo y edita estos **tres** `#define` con los valores del `credentials.json` de tu ejercicio:

- `USER_LORAWAN_DEVICE_EUI` → tu **DevEUI** (8 bytes)
- `USER_LORAWAN_JOIN_EUI` → tu **JoinEUI** (8 bytes)
- `USER_LORAWAN_APP_KEY` → tu **AppKey** (16 bytes)

Cada byte es un `0xNN` separado por comas, en **orden MSB** (el mismo que muestra ChirpStack, leído de
izquierda a derecha). Las longitudes son fijas (8 / 8 / 16); no añadas ni quites elementos:

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

**Convertir un valor de ChirpStack:** parte el hex de dos en dos dígitos, prefija cada par con `0x` y
sepáralos por comas. DevEUI `70B3D57ED0060AF3` → `0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x0A, 0xF3`.

Ajusta también la región `MODEM_EXAMPLE_REGION` (y usa la misma banda en el `REGION=` del Paso 2):
- **US915** → `SMTC_MODEM_REGION_US_915`
- **EU868** → `SMTC_MODEM_REGION_EU_868`

> ⚠️ En el mismo archivo aparece `USER_LORAWAN_GEN_APP_KEY`: **no lo confundas con el AppKey** (es
> para otro modo). Para el join OTAA normal el que importa es `USER_LORAWAN_APP_KEY`; deja
> `GEN_APP_KEY` como está.
>
> ⚠️ El **DevEUI/JoinEUI/AppKey** deben **coincidir exactamente** con lo que registres en ChirpStack
> (mismos bytes, orden MSB). Es la causa nº1 de fallos de join. Ver
> [How-To Provisionar](How-To-Provisionar-en-ChirpStack).

## Paso 2 · Compilar (un comando por ejercicio)

| Ej. | Comando |
|-----|---------|
| 01 | `make -C lbm_examples full_lr1110 MODEM_APP=PERIODICAL_UPLINK REGION=US_915` |
| 02 | `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_GEOLOCATION REGION=US_915` |
| 03 | `make -C lbm_examples full_lr1110 MODEM_APP=HW_MODEM REGION=US_915,EU_868` |
| 04 | `make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 MODEM_APP=EXAMPLE_WIFI_REGION_DETECTION REGION=US_915,EU_868` |

- Cambia `REGION=EU_868` para la banda europea.
- `full_lr1110` **limpia y recompila desde cero** (borra el binario anterior).
- El **03 (Hardware Modem)** y el **04 (Wi-Fi)** no llevan credenciales compiladas: el 03 recibe la
  config por el host (UART) y el 04 solo escanea Wi-Fi (no hace join).

## Paso 3 · Dónde queda el binario
```
build_lr1110_l4/app_lr1110_<REGION>.bin      (+ .hex y .elf)
```
Por ejemplo, tras compilar el 01 en US915:
`lbm_examples/build_lr1110_l4/app_lr1110_US_915.bin`.

Cópialo a la carpeta `artifacts/` del ejercicio si quieres conservarlo, y **flashéalo** →
[How-To Flashear y ver la serie](How-To-Flashear-y-ver-la-serie).

## Errores de compilación típicos
| Síntoma | Causa | Arreglo |
|---------|-------|---------|
| `arm-none-eabi-gcc: command not found` | toolchain no está en el `PATH` | revisa el paso 1 de [Requisitos](How-To-Requisitos-e-instalación) |
| Compila pero el join falla | credenciales/region no coinciden con ChirpStack | vuelve al Paso 1; verifica MSB en ChirpStack |
| No aparece el `.bin` | `full_lr1110` limpió y otro error abortó antes | lee el log completo del `make` |

## 🛠️ Haz tus propios binarios
Cambia el **periodo de envío**, el **payload**, activa/desactiva **GNSS/Wi-Fi** o integra **tu
sensor**, y recompila para generar **tus** variantes. **Happy hacking 🚀**

> Siguiente: **[How-To Flashear y ver la serie](How-To-Flashear-y-ver-la-serie)** →
