# Flasheo y alta en ChirpStack v4 — Tracker BMP280 + GNSS (LR1110)

Guía para compilar, flashear y registrar en ChirpStack v4 los dos firmwares del tracker
basados en la app de referencia **`lbm_applications/3_geolocation_on_lora_edge/`** de
LoRa Basics Modem (SWL2001).

- **Placa:** ST Nucleo-L476RG (MCU STM32L476RGT6).
- **Radio:** LR1110 (shield EVK / mb1lxks).
- **Crypto:** SOFT (por defecto). `USE_LR11XX_CREDENTIALS` está **desactivado**, por lo que
  las credenciales que edites a mano en `example_options.h` son las que realmente se usan.

## Dos builds

| Nombre del build | Región | DevEUI | JoinEUI | AppKey |
|---|---|---|---|---|
| `tracker-bmp280-gnss_lr1110_us915` | US_915 | `AA BB CC DD 00 91 50 01` | `AA BB CC DD EE FF 00 00` | `A9 15 A9 15 A9 15 A9 15 A9 15 A9 15 A9 15 A9 15` |
| `tracker-bmp280-gnss_lr1110_eu868` | EU_868 | `AA BB CC DD 00 86 80 01` | `AA BB CC DD EE FF 00 00` | `A8 68 A8 68 A8 68 A8 68 A8 68 A8 68 A8 68 A8 68` |

> **LAB — reemplazar antes de una red productiva.** Estas DevEUI/JoinEUI/AppKey son de
> laboratorio. No las uses en una red real; genera credenciales propias y protégelas.

---

## 1. Requisitos previos

- **Toolchain GCC ARM bare-metal:** `arm-none-eabi-gcc` 13.2.1 (`arm-none-eabi` en el `PATH`,
  o pásalo con `GCC_PATH=/ruta/gcc-arm/bin` a `make`).
- **Build tools:** `make`, y para la ruta CMake también `cmake` (>= 3.25) y `ninja`.
- **Driver / utilidad ST-Link:** driver ST-Link instalado y, para el flasheo por línea de
  comandos, `stlink-tools` (provee `st-flash`) o, alternativamente, `STM32_Programmer_CLI`.
- **Terminal serie:** para ver la traza UART (115200 8N1) — `minicom`, `picocom`, `tio`, PuTTY, etc.
- **Sensor BMP280 conectado al bus I²C** de la placa (VCC 3V3, GND, SCL, SDA).
  > Nota: es un **BMP280** (presión + temperatura). **No mide humedad**; a diferencia del
  > BME280, el canal de humedad no aplica y no debe esperarse ese dato en el uplink.

---

## 1.bis Conexión del BMP280 a la Nucleo-L476RG (diagrama de pines)

El firmware fija estos pines **en código** (no configurables por build):

| Parámetro | Valor (fijado en firmware) |
|-----------|----------------------------|
| Periférico | **I²C1** |
| SCL | **PB_8**  (Arduino **D15**, conector CN5) |
| SDA | **PB_9**  (Arduino **D14**, conector CN5) |
| Dirección I²C del sensor | **0x76** (`BMP280_I2C_ADDR_PRIMARY`) → por eso **SDO va a GND** |
| Velocidad | 100 kHz (Standard mode) |

> Los pines PB_8/PB_9 se eligieron por no colisionar con el SPI del radio LR1110
> (PA_5/6/7, PA_8, PB_3, PB_4) ni con la UART de depuración (PA_2/PA_3).

### Tabla de cableado (módulo típico GY-BMP280 de 6 pines)

| Pin del módulo BMP280 | Señal | Se conecta a (Nucleo, por rótulo de serigrafía) | Motivo |
|-----------------------|-------|--------------------------------------------------|--------|
| **VCC / VIN** | 3.3 V | pin **3V3** (conector de alimentación CN6) | El módulo GY-BMP280 lleva regulador y trabaja a 3.3 V |
| **GND** | Masa | pin **GND** (CN6) | Masa común |
| **SCL / SCK** | Reloj I²C | pin **D15** = PB_8 (CN5) | `I2C1_SCL` |
| **SDA / SDI** | Datos I²C | pin **D14** = PB_9 (CN5) | `I2C1_SDA` |
| **CSB / CS** | Selección de bus | **3V3** | Atar a VCC fuerza el **modo I²C** (si queda a GND el chip entra en modo SPI) |
| **SDO / SDD** | Bit de dirección | **GND** | GND → dirección **0x76** (la que usa el firmware). A VCC sería 0x77 (habría que cambiar a `BMP280_I2C_ADDR_SECONDARY`) |

### Diagrama

```
     BMP280 (GY-BMP280)                     STM32 Nucleo-L476RG
   ┌────────────────────┐               ┌──────────────────────────────┐
   │ VCC ───────────────┼──────────────►│ 3V3      (CN6, alimentación)  │
   │ GND ───────────────┼──────────────►│ GND      (CN6)                │
   │ SCL ───────────────┼──────────────►│ D15 / PB_8   (CN5)  I2C1_SCL  │
   │ SDA ───────────────┼──────────────►│ D14 / PB_9   (CN5)  I2C1_SDA  │
   │ CSB ───────────────┼───► 3V3       (fuerza modo I²C)               │
   │ SDO ───────────────┼───► GND       (dirección I²C = 0x76)          │
   └────────────────────┘               └──────────────────────────────┘
```

### Notas de conexión

- **Pull-ups:** la mayoría de módulos GY-BMP280 ya traen pull-ups (~4.7 kΩ) en SDA/SCL, y además
  el HAL habilita las pull-ups internas del STM32 → normalmente **no** hacen falta resistencias
  externas. Si el bus no arranca con cables largos, añade 4.7 kΩ de SDA y SCL a 3V3.
- **Cableado corto** (< 20 cm) para 100 kHz; no cruces SDA/SCL.
- **Verificación al arrancar:** en la traza UART (115200 8N1) debe aparecer
  `BMP280 detected (chip-id 0x58)`. Si ves `BMP280 not detected - environmental uplinks disabled`,
  revisa: alimentación 3V3, CSB→3V3, SDO→GND, y que SDA/SCL no estén invertidos.
- **Si tu módulo es un BME280** (chip-id 0x60) en lugar de BMP280 (0x58), el firmware **no lo
  detectará** — el driver valida explícitamente el chip-id 0x58. El GNSS seguirá funcionando igual.

### Pinout de los conectores Arduino de la Nucleo-L476RG

> No es posible incrustar una foto real en este documento. Para la imagen oficial consulta el
> manual **ST UM1724** ("STM32 Nucleo-64 boards"), sección *Arduino connectors*, o el rótulo de
> serigrafía impreso en la propia placa. Abajo tienes el mapa en texto con los **4 pines usados**.

**Ubicación de los conectores (vista superior, USB/ST-LINK arriba):**

```
                         ┌──────── USB ST-LINK ────────┐
        ┌────────────────┴─────────────────────────────┴───────────────┐
        │  CN7 (morpho izq.)                        CN10 (morpho der.)   │
        │                                                               │
        │  CN6 ▓ power  ┐                          ┌ CN5 ▓ digital alto │
        │               │   [ STM32L476RG ]        │                    │
        │  CN8   analog  ┘                          └ CN9   digital bajo │
        └───────────────────────────────────────────────────────────────┘
        ▓ = conectores donde están los pines que usamos (CN5 y CN6)
```

**CN5 — conector digital (lado derecho, parte alta). Aquí están SCL y SDA:**

| Pin CN5 | Función Arduino | MCU | ¿Se usa? |
|---------|-----------------|-----|----------|
| 10 | **D15 / SCL** | **PB_8** | ◄ **SÍ — SCL del BMP280** |
| 9  | **D14 / SDA** | **PB_9** | ◄ **SÍ — SDA del BMP280** |
| 8  | AVDD | — | no |
| 7  | **GND** | — | (GND alternativo, cómodo por estar junto a D14) |
| 6  | D13 / SCK | PA_5 | no (SPI radio) |
| 5  | D12 / MISO | PA_6 | no |
| 4  | D11 / MOSI | PA_7 | no |
| 3  | D10 / CS | PB_6 | no |
| 2  | D9 | PC_7 | no |
| 1  | D8 | PA_9 | no |

**CN6 — conector de alimentación (lado izquierdo, parte alta). Aquí están 3V3 y GND:**

| Pin CN6 | Rótulo | ¿Se usa? |
|---------|--------|----------|
| 1 | NC / VLCD | no |
| 2 | IOREF | no |
| 3 | NRST | no |
| 4 | **3V3** | ◄ **SÍ — VCC del BMP280 (y CSB)** |
| 5 | 5V | no (¡no usar para el BMP280!) |
| 6 | **GND** | ◄ **SÍ — GND del BMP280 (y SDO)** |
| 7 | GND | (GND alternativo) |
| 8 | VIN | no |

**Resumen de los 4 pines a conectar:**

| Señal BMP280 | Pin físico Nucleo | Rótulo serigrafía |
|--------------|-------------------|-------------------|
| VCC + CSB | CN6-4 | **3V3** |
| GND + SDO | CN6-6 (o CN5-7) | **GND** |
| SCL | CN5-10 | **D15** (PB_8) |
| SDA | CN5-9 | **D14** (PB_9) |

> Todos estos pines de la Nucleo están **también replicados** en los conectores morpho
> CN7/CN10 (los de doble fila que rodean la placa), por si prefieres cablear por ahí:
> PB_8 y PB_9 están en **CN10**, y hay pines **3V3/GND** tanto en CN7 como en CN10.

---

## 2. Compilación de cada build

Todos los comandos se ejecutan desde el directorio de la app:

```bash
cd lbm_applications/3_geolocation_on_lora_edge
```

### 2.1 Dónde se ponen las credenciales y la región (runtime)

Edita **`main_geolocation/example_options.h`**. Con crypto SOFT estos valores son los efectivos.

**Build US915** (`tracker-bmp280-gnss_lr1110_us915`):

```c
#define USER_LORAWAN_DEVICE_EUI \
    { 0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x91, 0x50, 0x01 }
#define USER_LORAWAN_JOIN_EUI \
    { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 }
#define USER_LORAWAN_APP_KEY \
    { 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, \
      0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15 }

#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_US_915
```

**Build EU868** (`tracker-bmp280-gnss_lr1110_eu868`):

```c
#define USER_LORAWAN_DEVICE_EUI \
    { 0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x86, 0x80, 0x01 }
#define USER_LORAWAN_JOIN_EUI \
    { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 }
#define USER_LORAWAN_APP_KEY \
    { 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, \
      0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68 }

#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_EU_868
```

> Hay dos capas de "región":
> - **`MODEM_EXAMPLE_REGION`** en `example_options.h` → región con la que el firmware
>   **arranca en runtime** (la que hace join).
> - **`REGION=` (make) / `-DAPP_REGION=` (cmake)** → qué región(es) se **compilan** en la
>   librería LBM. Deben ser coherentes con `MODEM_EXAMPLE_REGION`.

### 2.2 Compilación con `make`

`make full_lr1110` hace clean + build para radio LR1110. Añade `REGION=` para compilar solo
esa banda (más pequeño); si se omite, se compilan todas.

```bash
# US915  (tras editar example_options.h con las credenciales US y MODEM_EXAMPLE_REGION US_915)
make full_lr1110 REGION=US_915

# EU868  (tras editar example_options.h con las credenciales EU y MODEM_EXAMPLE_REGION EU_868)
make full_lr1110 REGION=EU_868
```

Opciones útiles: `LBM_TRACE=no` / `APP_TRACE=no` para desactivar trazas,
`GCC_PATH=/ruta/bin` si el compilador no está en el `PATH`.

**Salida (make):** directorio `build_lr1110_l4/`. Los artefactos son
`build_lr1110_l4/app_lr1110_<REGION>.{elf,hex,bin}`, p.ej.:

- `build_lr1110_l4/app_lr1110_US_915.bin` / `.elf` / `.hex`
- `build_lr1110_l4/app_lr1110_EU_868.bin` / `.elf` / `.hex`

> Nota: el directorio de salida NO incluye la región en su nombre (solo el nombre del binario
> sí). Compila y flashea una banda antes de cambiar a la otra, o renombra/copia el `.bin`
> para no confundirlos.

### 2.3 Compilación equivalente con `cmake` + `ninja`

```bash
# US915
cmake -B build -G Ninja -DLBM_RADIO=lr1110 -DCMAKE_BUILD_TYPE=MinSizeRel \
      -DLBM_CMAKE_CONFIG_AUTO=ON -DAPP=geolocation \
      -DAPP_REGION=SMTC_MODEM_REGION_US_915
ninja -C build

# EU868  (borra antes el dir build:  rm -rf build)
cmake -B build -G Ninja -DLBM_RADIO=lr1110 -DCMAKE_BUILD_TYPE=MinSizeRel \
      -DLBM_CMAKE_CONFIG_AUTO=ON -DAPP=geolocation \
      -DAPP_REGION=SMTC_MODEM_REGION_EU_868
ninja -C build
```

`-DAPP_REGION=` define `MODEM_EXAMPLE_REGION` en el firmware (equivale a fijarlo en
`example_options.h`). Las **credenciales** siguen tomándose de `example_options.h`, así que
edítalas igual que en 2.1 antes de compilar. Opcional: `-DLBM_MODEM_TRACE=no` para desactivar trazas.

**Salida (cmake):** `build/lbm_example.elf`, `build/lbm_example.hex`, `build/lbm_example.bin`.

---

## 3. Flasheo

La memoria flash del STM32L476 arranca en **`0x08000000`**.

### 3.1 Comando real soportado por el proyecto (recomendado)

La app define el target `flash` (ruta CMake/Ninja), que usa `st-flash` de `stlink-tools`:

```bash
ninja -C build flash
```

Internamente ejecuta:

```bash
st-flash --connect-under-reset write build/lbm_example.bin 0x8000000
```

Alternativa "por copia" (placa montada como disco USB `NODE_L476RG`):

```bash
ninja -C build flash_copy      # copia el .bin a /media/$USER/NODE_L476RG
```

### 3.2 Flasheo manual (si compilaste con `make`, o para usar el binario por región)

Con `st-flash` directamente (indicando el `.bin` de la banda deseada):

```bash
# US915
st-flash --connect-under-reset write build_lr1110_l4/app_lr1110_US_915.bin 0x08000000
# EU868
st-flash --connect-under-reset write build_lr1110_l4/app_lr1110_EU_868.bin 0x08000000
```

Con la herramienta oficial de ST (`STM32_Programmer_CLI`, multiplataforma):

```bash
STM32_Programmer_CLI -c port=SWD mode=UR -w build_lr1110_l4/app_lr1110_US_915.bin 0x08000000 -rst
```

También puedes arrastrar el `.bin` (o `.hex`) al disco `NODE_L476RG` que expone el ST-Link
integrado de la Nucleo.

### 3.3 Ver la traza UART (confirmar join y uplinks)

El ST-Link de la Nucleo expone un puerto serie virtual (VCP). Ábrelo a **115200 8N1**:

```bash
# Linux (ajusta el dispositivo: /dev/ttyACM0 suele ser el VCP de la Nucleo)
picocom -b 115200 /dev/ttyACM0
# o:  tio -b 115200 /dev/ttyACM0
# o:  minicom -D /dev/ttyACM0 -b 115200
```

En Windows usa PuTTY/TeraTerm sobre el COM del "STMicroelectronics STLink Virtual COM Port"
a 115200. Deberías ver el reset del modem, el intento de Join y, tras el `JOINED`, los
uplinks de sensor y de geolocalización (GNSS/Wi-Fi).

---

## 4. Registro de los devices en ChirpStack v4

Se asume que **ya tienes tenant y una aplicación creados**; aquí solo se **añaden los
devices**. Hazlo dos veces, uno por build/región.

### 4.1 Device Profile por región (uno para US915, otro para EU868)

`Device Profiles → Add device profile` (o selecciona uno existente que ya cumpla):

- **Name:** p.ej. `LR1110 Geoloc US915` / `LR1110 Geoloc EU868`.
- **Region:** `US915` para el build US, `EU868` para el build EU.
- **MAC version:** `LoRaWAN 1.0.4` (si no está disponible, `1.0.3`).
- **Regional Parameters revision:** la correspondiente a esa MAC/region (p.ej. `RP002-1.0.3`).
- **Activation / Join:** **OTAA** (Join). Deja desmarcado ABP.

> La **región del Device Profile DEBE coincidir con la del firmware**
> (`MODEM_EXAMPLE_REGION`). Un device US915 con perfil EU868 (o viceversa) **no hará join**.

### 4.2 Añadir el device

`Applications → (tu aplicación) → Add device`:

- **Device EUI (DevEUI):** el DevEUI de la tabla (introducir en orden **MSB**, tal cual,
  sin espacios). US915 → `AABBCCDD00915001`; EU868 → `AABBCCDD00868001`.
- **Name:** un nombre claro, p.ej. `tracker-us915` / `tracker-eu868`.
- **Device profile:** selecciona el perfil de **esa** región (4.1).
- **Join EUI (JoinEUI / AppEUI):** `AABBCCDDEEFF0000` (mismo para ambos). Guarda.

### 4.3 Claves OTAA (pestaña de keys del device)

`(device) → OTAA keys`:

- **Application key (AppKey):** pega el AppKey de la tabla (sin espacios).
  - US915 → `A915A915A915A915A915A915A915A915`
  - EU868 → `A868A868A868A868A868A868A868A868`
- En **LoRaWAN 1.0.x** solo se usa el **Application key**. Si el formulario pide también
  **Network key (NwkKey)**, ponlo **igual al AppKey** (`NwkKey = AppKey` en 1.0.x).

### 4.4 Tabla resumen de alta

| Device (name) | DevEUI (MSB) | JoinEUI | AppKey | Device profile / región | Application |
|---|---|---|---|---|---|
| `tracker-us915` | `AABBCCDD00915001` | `AABBCCDDEEFF0000` | `A915A915A915A915A915A915A915A915` | LR1110 Geoloc **US915** | tu app |
| `tracker-eu868` | `AABBCCDD00868001` | `AABBCCDDEEFF0000` | `A868A868A868A868A868A868A868A868` | LR1110 Geoloc **EU868** | tu app |

### 4.5 Verificar Join y uplinks

En el device abre las pestañas **`LoRaWAN frames`** y **`Events`**:

- Un **Join Request / Join Accept** confirma la activación OTAA.
- Luego llegan uplinks:
  - **fPort 10** → datos del sensor (BMP280: presión y temperatura).
  - **fPort del servicio GNSS** → paquete de geolocalización GNSS (además de los uplinks
    Wi-Fi). Estos payloads se resuelven en un servidor de aplicación / solver de geoloc.

### 4.6 Ficha lista para copiar (una por device)

Cada bloque es el recorrido completo para un device: crear/elegir su Device Profile, dar de
alta el device y pegar las claves. Los valores son los del binario correspondiente.

**A) Device US915 — binario `app_lr1110_US_915.bin`**

```text
1. Device Profile  (Tenant → Device profiles → Add device profile)
     General:
       Name .......................... Tracker BMP280 US915 1.0.4
       Region ........................ US915
       MAC version ................... LoRaWAN 1.0.4
       Regional parameters revision .. RP002-1.0.3
     Join (OTAA / ABP):
       Activation .................... OTAA (dejar ABP sin marcar)

2. Add device  (Applications → tu app → Add device → pestaña Configuration)
       Name .......................... tracker-us915
       Device EUI (EUI64) ............ AABBCCDD00915001      (MSB, sin invertir)
       Join EUI  (EUI64) ............. AABBCCDDEEFF0000
       Device profile ................ Tracker BMP280 US915 1.0.4

3. Keys (OTAA)  (device → pestaña Keys (OTAA))
       Application key ............... A915A915A915A915A915A915A915A915
       (LoRaWAN 1.0.x: solo Application key; si pide Network key, NwkKey = AppKey)
```

**B) Device EU868 — binario `app_lr1110_EU_868.bin`**

```text
1. Device Profile  (Tenant → Device profiles → Add device profile)
     General:
       Name .......................... Tracker BMP280 EU868 1.0.4
       Region ........................ EU868
       MAC version ................... LoRaWAN 1.0.4
       Regional parameters revision .. RP002-1.0.3
     Join (OTAA / ABP):
       Activation .................... OTAA (dejar ABP sin marcar)

2. Add device  (Applications → tu app → Add device → pestaña Configuration)
       Name .......................... tracker-eu868
       Device EUI (EUI64) ............ AABBCCDD00868001      (MSB, sin invertir)
       Join EUI  (EUI64) ............. AABBCCDDEEFF0000
       Device profile ................ Tracker BMP280 EU868 1.0.4

3. Keys (OTAA)  (device → pestaña Keys (OTAA))
       Application key ............... A868A868A868A868A868A868A868A868
       (LoRaWAN 1.0.x: solo Application key; si pide Network key, NwkKey = AppKey)
```

> **Orden de bytes:** la UI de ChirpStack usa **MSB** (tal como se muestra). El firmware LoRa
> Basics Modem también usa MSB en `example_options.h`, así que los EUIs se pegan **tal cual**,
> sin invertir (invertir a LSB solo aplica a firmwares tipo Arduino-LMIC, que **no** es el caso).

> **La región del Device Profile debe coincidir con la del firmware.** Un device US915 con
> perfil EU868 (o al revés) **no hará join**.

**Fuentes oficiales (ChirpStack v4):**
[Devices](https://www.chirpstack.io/docs/chirpstack/use/devices.html) ·
[Device profiles](https://www.chirpstack.io/docs/chirpstack/use/device-profiles.html) ·
[Connecting a device](https://www.chirpstack.io/docs/guides/connect-device.html)

---

## 5. Solución de problemas rápida

- **No hace Join:**
  - **Región:** que coincidan firmware (`MODEM_EXAMPLE_REGION`), `REGION=`/`APP_REGION`
    compilado y la **región del Device Profile** en ChirpStack.
  - **Claves:** DevEUI, JoinEUI y AppKey idénticos en firmware y en ChirpStack. Recuerda
    `NwkKey = AppKey` en 1.0.x.
  - **Cobertura de gateway:** que haya un gateway de esa banda dado de alta en el tenant y
    con cobertura RF del dispositivo.
- **DevEUI "no aparece" o join rechazado por EUI:** verifica el **orden de bytes**. En el
  firmware el array va MSB→LSB; en ChirpStack se introduce el mismo string MSB
  (`AABBCCDD00915001`). No lo inviertas.
- **EU868 transmite muy poco / uplinks espaciados:** es el **duty cycle** regulatorio de
  EU868 (límite de tiempo en aire). Es normal; espacia los envíos o revisa el periodo de
  scan. En US915 no aplica duty cycle (usa dwell time).
- **No hay traza UART:** confirma 115200 8N1 y el dispositivo/COM correcto (VCP del ST-Link);
  comprueba que no compilaste con `APP_TRACE=no`/`LBM_TRACE=no`.
- **`st-flash` no encuentra la placa:** usa `--connect-under-reset`, revisa el driver
  ST-Link y que ningún otro programa (STM32CubeIDE/Programmer) tenga la sonda ocupada.
