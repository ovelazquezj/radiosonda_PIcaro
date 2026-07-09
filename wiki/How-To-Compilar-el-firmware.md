# How-To · Compilar el firmware LR1110 (ejercicios 01–04)

Los binarios **no** vienen en el repo: los **compilas tú** (así aprendes el flujo completo). Esta
guía asume que ya instalaste el toolchain ARM (ver
[Requisitos e instalación](How-To-Requisitos-e-instalación)). Los ejercicios TTGO (**05–06**) **no**
usan esto: ver [How-To Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).

## Paso 1 · Poner tus credenciales y región
Antes de compilar, copia el **DevEUI / JoinEUI / AppKey** del `credentials.json` de tu ejercicio y la
**región** en el `example_options.h` correspondiente:

| Ejercicio | Archivo a editar |
|-----------|------------------|
| 01, 03 | `lbm_examples/main_examples/example_options.h` |
| 02, 04 | `lbm_applications/3_geolocation_on_lora_edge/main_geolocation/example_options.h` |

Ajusta también la región `MODEM_EXAMPLE_REGION`:
- **US915** → `SMTC_MODEM_REGION_US_915`
- **EU868** → `SMTC_MODEM_REGION_EU_868`

> ⚠️ El **DevEUI/JoinEUI/AppKey** deben **coincidir** con lo que registres en ChirpStack (en MSB).
> Es la causa nº1 de fallos de join. Ver [How-To Provisionar](How-To-Provisionar-en-ChirpStack).

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
