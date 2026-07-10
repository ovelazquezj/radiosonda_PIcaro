# How-To · Requisitos e instalación de prerequisitos

Esta guía instala **todo lo necesario** para compilar, flashear y probar los ejercicios. Hazla una
sola vez. Al final tendrás: toolchain ARM, `make`, Docker/ChirpStack, herramientas de flasheo/serie
y (para los TTGO) el Arduino IDE con sus librerías.

## ¿Qué necesito según el ejercicio?

| Ejercicio | Toolchain necesario |
|-----------|---------------------|
| 01–04 (Nucleo + LR1110) | **Arm GCC + make** (compilas un `.bin`) |
| 05–06 (TTGO ESP32) | **Arduino IDE + librerías** (compilas un sketch) |
| 07 (gateway TTGO) | **PlatformIO** o Arduino IDE (firmware de terceros ESP-1ch-Gateway) |
| Todos (join a la red) | **Docker + ChirpStack** y un **gateway** de tu banda |

> Si solo vas a probar el **TTGO**, sáltate el toolchain ARM. Si solo vas a probar la **Nucleo**,
> sáltate Arduino.

---

## 1 · Toolchain ARM (para los nodos LR1110, ej. 01–04)

Necesitas **GNU Arm Embedded Toolchain 13.2.rel1** (`arm-none-eabi-gcc`) y **make** en el `PATH`.

### Windows — compila DENTRO de WSL2 (Ubuntu)

> 🔑 **En Windows el firmware LR1110 se compila dentro de WSL2, no en Git Bash ni PowerShell**
> (que **no** traen `arm-none-eabi-gcc` ni `make`). Editas y flasheas desde Windows, pero **el `make`
> corre en WSL**. Así está montado este proyecto en esta máquina.

**1 · Instala el toolchain en Ubuntu (WSL).** Lo más simple es por `apt` (trae la 13.2):
```bash
sudo apt update
sudo apt install -y gcc-arm-none-eabi make git python3 python3-pip
```
> Alternativa (versión oficial exacta de Arm, si tu `apt` no tuviera la 13.2): descarga el tarball
> `arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi` de arm.com y añade su `bin/` al `PATH`.

**2 · Accede al repo desde WSL** (está en el disco de Windows, visible en `/mnt/c/...`):
```bash
cd /mnt/c/dev/PiCARO/SWL2001        # ajústalo a donde lo tengas clonado
```

**3 · Compila** (detalle en [Compilar el firmware](How-To-Compilar-el-firmware)):
```bash
make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 \
     MODEM_APP=EXAMPLE_GEOLOCATION REGION=US_915
```
El `.bin`/`.hex`/`.elf` queda en `build_lr1110_l4/` (visible también desde Windows en la carpeta del
repo). Luego flashea desde Windows → [How-To Flashear](How-To-Flashear-y-ver-la-serie).

### Linux (Ubuntu/Debian)
Igual que el bloque de WSL de arriba (WSL2 **es** Ubuntu).

### macOS
```bash
brew install --cask gcc-arm-embedded   # instala arm-none-eabi-gcc 13.x
brew install make
```

### Verifica
```bash
arm-none-eabi-gcc --version     # debe decir 13.2.x
make --version
```

---

## 2 · Docker + ChirpStack (para que el nodo se una a la red)

ChirpStack v4 corre en Docker (ejercicio **00**). Necesitas **Docker** y **docker compose**.

- **Windows:** instala **Docker Desktop** (con backend WSL2 activado).
- **Linux:** `sudo apt install -y docker.io docker-compose-plugin` y añade tu usuario al grupo
  `docker` (`sudo usermod -aG docker $USER`; reinicia sesión).
- **macOS:** **Docker Desktop**.

Verifica y levanta ChirpStack:
```bash
docker --version && docker compose version
cd specs/exercises/00_chirpstack-docker
docker compose up -d
# Web de ChirpStack:  http://localhost:8080   (admin / admin)
# API REST:           http://localhost:8090
```

> Detalles y objetos (tenant/app/device profile) en
> [ChirpStack en 5 minutos](ChirpStack-en-5-minutos).

---

## 3 · Herramientas de flasheo y de puerto serie

Para grabar el `.bin`/`.hex` en la Nucleo y ver la traza:
- **STM32CubeProgrammer** (`STM32_Programmer_CLI`) — la más robusta (Windows/Linux/macOS).
- o **st-link tools** (`st-flash`): `sudo apt install -y stlink-tools` (Linux).
- **Terminal serie:** `tio`/`picocom`/`minicom` (Linux/mac) o **PuTTY**/**Tera Term** (Windows), a
  **115200 8N1**.

> En **WSL2** el USB no se ve por defecto: lo más simple es flashear **arrastrando** el `.hex` al
> disco `NODE_L476RG` desde Windows. Ver [How-To Flashear](How-To-Flashear-y-ver-la-serie).

---

## 4 · Arduino IDE + librerías (para los TTGO, ej. 05–06)

1. Instala el **Arduino IDE 2.x**.
2. Añade el soporte **ESP32**: *Preferences → Additional Boards Manager URLs* →
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`,
   luego *Boards Manager → "esp32" → Install*.
3. Instala las librerías (*Library Manager*):
   - **MCCI LoRaWAN LMIC library**
   - **U8g2** (pantalla OLED)
   - **Adafruit BMP280 Library** (+ **Adafruit Unified Sensor**) — solo para el ejercicio 06
4. Configura LMIC para tu banda editando `arduino_lmic_project_config.h` de la librería:
   `#define CFG_us915 1` y `#define CFG_sx1276_radio 1` (para US915).

> Detalle completo del flujo TTGO en
> [How-To Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).

---

## 5 · PlatformIO (para el gateway del ejercicio 07)

El firmware del gateway (**ESP-1ch-Gateway**, ejercicio 07) se compila con **PlatformIO** (o Arduino
IDE). Instálalo de una de estas formas:
- **VS Code:** extensión *PlatformIO IDE* (busca "PlatformIO" en *Extensions*).
- **CLI:** `pip install platformio` (necesita Python 3) → habilita el comando `pio`.

El entorno concreto para la **TTGO por USB** y los pasos están en
[How-To Montar un gateway de 1 canal](How-To-Montar-un-gateway-de-1-canal) (y en `VERSIONS.md` del
ejercicio 07). **No** necesitas el toolchain ARM para el 07.

---

## Checklist final
- [ ] `arm-none-eabi-gcc --version` → 13.2.x (solo si usarás 01–04)
- [ ] `make --version` OK
- [ ] `docker compose version` OK y ChirpStack respondiendo en `:8080`
- [ ] Herramienta de flasheo + terminal serie a 115200 listas
- [ ] Arduino IDE con ESP32 + LMIC + U8g2 (+ BMP280 para el 06)
- [ ] PlatformIO (o Arduino IDE) instalado — solo si harás el ejercicio 07 (gateway)

> Siguiente: **[How-To Compilar el firmware](How-To-Compilar-el-firmware)** →
