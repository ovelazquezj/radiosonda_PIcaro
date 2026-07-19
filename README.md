# radiosonda_PIcaro — Laboratorios didácticos de LoRaWAN con LR1110 y ChirpStack

> ⚠️ **Fork educativo no oficial.** Este repositorio es un **derivado (fork)** de
> [**LoRa Basics Modem (SWL2001)**](https://github.com/Lora-net/SWL2001) de **Semtech Corporation**,
> con fines **exclusivamente didácticos**. **No es un producto de Semtech** y **no está afiliado a,
> patrocinado por, ni respaldado por Semtech Corporation.** El código original conserva su copyright
> y licencia **Clear BSD** originales (ver [`LICENSE.txt`](LICENSE.txt) y [`NOTICE`](NOTICE)).

> **Hands-on LoRaWAN labs** with the Semtech **LR1110** & **LoRa Basics Modem (SWL2001)**, an
> **ESP32-S3 + SX1262** radiosonde (LilyGo T-Beam Supreme, ESP-IDF + RadioLib) and **ChirpStack v4** —
> OTAA join, BMP280/BME280 sensor uplinks, GNSS/Wi-Fi geolocation, TTGO ESP32 nodes, REST/MQTT API
> provisioning and a **Python/tkinter mission-control dashboard** for IoT telemetry.
>
> *Unofficial educational fork of Semtech's SWL2001 — not affiliated with or endorsed by Semtech.*

**radiosonda_PIcaro** es un proyecto **educativo** para aprender **LoRa** y **LoRaWAN** de forma
práctica: desde el primer *join* OTAA hasta enviar datos de un sensor **BMP280**, hacer
**geolocalización GNSS/Wi-Fi** con el **Semtech LR1110**, integrar nodos **TTGO ESP32**, montar tu
**propio gateway** de 1 canal, programar una **radiosonda GPS ESP32-S3 (LilyGo T-Beam Supreme)** con
**ESP-IDF + RadioLib**, y **provisionar y consumir** todo desde **ChirpStack** por API — incluyendo un
**dashboard de escritorio estilo control de misión** (Python + tkinter).

> 📚 **¿Empiezas de cero?** La **[Wiki del proyecto](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)**
> explica LoRa y LoRaWAN desde los fundamentos y te guía por los **12 ejercicios (00 → 11)** paso a
> paso — es el mejor punto de partida. Aquí abajo tienes el arranque rápido.

---

## 🎓 Objetivo didáctico

Está pensado para **estudiantes y docentes**: cada ejercicio es **autocontenido**, va de lo simple a
lo avanzado, y explica el *porqué* de cada paso (incluyendo los errores típicos y cómo depurarlos).
No necesitas experiencia previa en LoRaWAN para empezar.

## 🙏 Basado en el trabajo original de Semtech (atribución)

Este repositorio es un **fork / obra derivada** de **LoRa Basics Modem (SWL2001)** de **Semtech
Corporation**:

- **Repo original (upstream):** **https://github.com/Lora-net/SWL2001**
- **Copyright © Semtech Corporation. All rights reserved.** Licencia **Clear BSD** — se conserva
  intacta en [`LICENSE.txt`](LICENSE.txt) y [`LICENSES.txt`](LICENSES.txt).
- El README original de Semtech se conserva íntegro en **[`README.SWL2001.md`](README.SWL2001.md)**.
- El detalle de la derivación y los cambios está en **[`NOTICE`](NOTICE)**.

Todo el crédito de la pila LoRa Basics Modem (`lbm_lib/`) es de **Semtech**. Este proyecto **solo
añade material didáctico y de integración** encima, bajo la misma licencia Clear BSD.

> **Descargo de responsabilidad y marcas.** Este es un proyecto independiente y **no oficial**;
> **Semtech no lo respalda ni está afiliado a él**. *LoRa®* y *LoRaWAN®* son marcas registradas de
> **Semtech Corporation** / **LoRa Alliance®**, y *ChirpStack* es una marca de sus respectivos
> titulares. Aquí se usan **solo de forma nominativa** para identificar las tecnologías con fines
> educativos, sin implicar patrocinio ni afiliación.

## ✨ Qué añade radiosonda_PIcaro sobre el original

- **Driver del sensor BMP280 + HAL I²C** para STM32L4 (no existían en el stack).
- **Provisión y consumo de ChirpStack por API** (REST + MQTT) con scripts reproducibles.
- **12 ejercicios guiados (00–11)** listos para clase, con credenciales, provisión y dashboards.
- **Integración de nodos de terceros:** **TTGO ESP32 (SX1276)** vía Arduino/LMIC, una **radiosonda GPS LilyGo T-Beam** vía RadioLib, y una **radiosonda ESP32-S3 + SX1262 (LilyGo T-Beam Supreme)** con **ESP-IDF + RadioLib** (drivers a nivel de registro para AXP2101, BME280, L76K GNSS, IMU… y "caja negra" en microSD).
- **Dashboard de escritorio "control de misión"** (Python + tkinter): consume la telemetría por MQTT, la guarda en SQLite, y muestra paneles, gráficas, mapa real con track y exportación a CSV.
- **Gateway LoRaWAN de 1 canal** (TTGO ESP32) para recibir a tus nodos sin infraestructura externa.
- **Especificaciones SDD (SRS)**, diagrama de **pinout** y guías comunes de **build/flash/ChirpStack**.
- **Wiki didáctica** con los fundamentos de LoRa/LoRaWAN y how-tos por tarea.

## 🗂️ Estructura del repositorio

| Carpeta | Contenido |
|---------|-----------|
| [`specs/exercises/`](specs/exercises/) | **Empieza aquí** — los 12 ejercicios didácticos (00 ChirpStack → 09 Radiosonda PICARO Full · 10 Dashboard Mission Control · 11 Dashboard contra servidor remoto) |
| [`specs/bmp280-gnss-tracker/`](specs/bmp280-gnss-tracker/) | Proyecto del tracker BMP280+GNSS: SRS, pinout, guía de flasheo/ChirpStack |
| [`specs/demos/`](specs/demos/) | Binarios de demo y guía de flasheo/registro por API |
| `lbm_lib/` | Pila **LoRa Basics Modem** de Semtech (upstream, sin modificar salvo lo indicado) |
| `lbm_examples/` · `lbm_applications/` | Ejemplos y aplicaciones (la de geolocalización, extendida con el BMP280) |

## 🔧 Requisitos

- **GNU Arm Embedded Toolchain** 13.2.rel1 (`arm-none-eabi-gcc`) + `make` — para los binarios LR1110 (ej. 01–04).
- **Arduino IDE** con las librerías MCCI LMIC, U8g2 y Adafruit BMP280 — para los nodos TTGO (ej. 05–06).
- **PlatformIO** (o Arduino IDE) — para el firmware del gateway de 1 canal (ej. 07).
- **Arduino IDE + RadioLib** — para la radiosonda **LilyGo T-Beam** (ej. 08).
- **ESP-IDF v5.x** (`idf.py`) — para la radiosonda **LilyGo T-Beam Supreme** ESP32-S3 (ej. 09).
- **Python 3.x** (`tkinter`, `matplotlib`, `paho-mqtt`, `tkintermapview`) — para el dashboard (ej. 10).
- **ChirpStack v4** (se incluye un `docker-compose` de referencia en el ejercicio 00).
- **Hardware:** Nucleo-L476RG + shield **LR1110** (01–04); **TTGO ESP32 LoRa** + **BMP280** (05–06); **TTGO ESP32** como gateway (07); **LilyGo T-Beam** (08); **LilyGo T-Beam Supreme** ESP32-S3 + SX1262 + microSD (09); solo un **PC** (Windows/Linux/macOS) para el dashboard (10).

> Instalación completa del toolchain paso a paso en la Wiki → **[Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación)**.

## 🚀 Primeros pasos

1. **Clona** el repo en `C:\dev\radiosonda_PIcaro` (Windows) o `~/dev/radiosonda_PIcaro` (Linux/mac) — detalle en la Wiki: [Requisitos e instalación · Paso 0](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación).
2. Levanta **ChirpStack** → [`specs/exercises/00_chirpstack-docker/`](specs/exercises/00_chirpstack-docker/).
3. Empieza por el **ejercicio 01** → [`specs/exercises/01_periodical-uplink/`](specs/exercises/01_periodical-uplink/).
4. **Compila** el binario de tu ejercicio → [`specs/exercises/COMMON_BUILD.md`](specs/exercises/COMMON_BUILD.md).
5. **Flashéalo** → [`specs/exercises/COMMON_FLASH.md`](specs/exercises/COMMON_FLASH.md).
6. **Provisiona y consume** en ChirpStack → [`specs/exercises/COMMON_CHIRPSTACK_API.md`](specs/exercises/COMMON_CHIRPSTACK_API.md).

👉 Índice completo de la ruta didáctica: **[`specs/exercises/README.md`](specs/exercises/README.md)**

## 📚 Wiki

La **[Wiki del repositorio](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)** es la guía
didáctica completa — **el mejor punto de partida si empiezas de cero**:

- **[Inicio](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)** — portada con la ruta de los 12 ejercicios (00 → 11).
- **[¿Qué es LoRa?](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/Qué-es-LoRa)** · **[¿Qué es LoRaWAN?](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/Qué-es-LoRaWAN)** — los fundamentos.
- **[Cómo usar el proyecto](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/Cómo-usar-el-proyecto)** — la ruta de ejercicios paso a paso.
- **[Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación)** · **[Compilar el firmware](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Compilar-el-firmware)** — how-tos por tarea.

## 📄 Licencia

Este proyecto usa **doble licencia**, según el tipo de contenido:

- **Código (software):** **The Clear BSD License** — la misma del proyecto original de Semtech.
  Cubre `lbm_lib/` y los ejemplos upstream (© **Semtech Corporation**) **y** el código propio de
  radiosonda_PIcaro (driver BMP280, HAL, scripts). Ver [`LICENSE.txt`](LICENSE.txt),
  [`LICENSES.txt`](LICENSES.txt) y [`NOTICE`](NOTICE).
- **Material educativo y documentación** (ejercicios, Wiki, READMEs, SRS, diagramas): **Creative
  Commons Attribution 4.0 International (CC BY 4.0)** © **Omar Velazquez
  \<ovelazquezj@gmail.com\>**. Ver [`LICENSE-CC-BY-4.0.md`](LICENSE-CC-BY-4.0.md).

> El copyright de Semtech sobre el código original se conserva **intacto**; CC BY 4.0 aplica **solo**
> al material educativo propio, **no** al código de Semtech.

---

## 🛠️ Modifica el firmware y haz tus propios binarios

Los ejercicios **01–04 ya traen su binario compilado** en `artifacts/` para que un clon esté **listo
para flashear**; aun así, la gracia del proyecto es que **generes los tuyos**. Cambia el periodo de
envío, el formato del payload, activa/desactiva servicios (GNSS/Wi-Fi), integra otro sensor… y
recompila para crear **tus propias variantes**. Todo el flujo (compilar → flashear → provisionar →
consumir) está documentado para que experimentes sin miedo.

**Happy hacking 🚀**
