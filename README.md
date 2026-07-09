# radiosonda_PIcaro — Laboratorios didácticos de LoRaWAN con LR1110 y ChirpStack

> ⚠️ **Fork educativo no oficial.** Este repositorio es un **derivado (fork)** de
> [**LoRa Basics Modem (SWL2001)**](https://github.com/Lora-net/SWL2001) de **Semtech Corporation**,
> con fines **exclusivamente didácticos**. **No es un producto de Semtech** y **no está afiliado a,
> patrocinado por, ni respaldado por Semtech Corporation.** El código original conserva su copyright
> y licencia **Clear BSD** originales (ver [`LICENSE.txt`](LICENSE.txt) y [`NOTICE`](NOTICE)).

> **Hands-on LoRaWAN labs** with the Semtech **LR1110** & **LoRa Basics Modem (SWL2001)** and
> **ChirpStack v4** — OTAA join, BMP280 sensor uplinks, GNSS/Wi-Fi geolocation, TTGO ESP32 nodes,
> and REST/MQTT API provisioning for IoT dashboards.
>
> *Unofficial educational fork of Semtech's SWL2001 — not affiliated with or endorsed by Semtech.*

**radiosonda_PIcaro** es un proyecto **educativo** para aprender **LoRa** y **LoRaWAN** de forma
práctica: desde el primer *join* OTAA hasta enviar datos de un sensor **BMP280**, hacer
**geolocalización GNSS/Wi-Fi** con el **Semtech LR1110**, integrar nodos **TTGO ESP32**, y
**provisionar y consumir** todo desde **ChirpStack** por API para construir dashboards.

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
- **6 ejercicios guiados (00–06)** listos para clase, con credenciales, provisión y dashboards.
- **Especificaciones SDD (SRS)**, diagrama de **pinout** y guías de flasheo/ChirpStack.
- Integración de un **nodo TTGO ESP32 (SX1276)** de terceros vía Arduino/LMIC.

## 🗂️ Estructura del repositorio

| Carpeta | Contenido |
|---------|-----------|
| [`specs/exercises/`](specs/exercises/) | **Empieza aquí** — los 6 ejercicios didácticos (00 ChirpStack → 06 TTGO+BMP280) |
| [`specs/bme280-gnss-tracker/`](specs/bme280-gnss-tracker/) | Proyecto del tracker BMP280+GNSS: SRS, pinout, guía de flasheo/ChirpStack |
| [`specs/demos/`](specs/demos/) | Binarios de demo y guía de flasheo/registro por API |
| `lbm_lib/` | Pila **LoRa Basics Modem** de Semtech (upstream, sin modificar salvo lo indicado) |
| `lbm_examples/` · `lbm_applications/` | Ejemplos y aplicaciones (la de geolocalización, extendida con el BMP280) |

## 🔧 Requisitos

- **GNU Arm Embedded Toolchain** 13.2.rel1 (`arm-none-eabi-gcc`) + `make` — para los binarios LR1110.
- **Arduino IDE** con las librerías MCCI LMIC, U8g2 y Adafruit BMP280 — para los nodos TTGO.
- **ChirpStack v4** (se incluye un `docker-compose` de referencia en el ejercicio 00).
- **Hardware:** Nucleo-L476RG + shield **LR1110** (ej. 01–04); **TTGO ESP32 LoRa** + **BMP280** (ej. 05–06).

## 🚀 Primeros pasos

1. **Clona** el repositorio.
2. Levanta **ChirpStack** → [`specs/exercises/00_chirpstack-docker/`](specs/exercises/00_chirpstack-docker/).
3. Empieza por el **ejercicio 01** → [`specs/exercises/01_periodical-uplink/`](specs/exercises/01_periodical-uplink/).
4. **Compila** el binario de tu ejercicio → [`specs/exercises/COMMON_BUILD.md`](specs/exercises/COMMON_BUILD.md).
5. **Flaséalo** → [`specs/exercises/COMMON_FLASH.md`](specs/exercises/COMMON_FLASH.md).
6. **Provisiona y consume** en ChirpStack → [`specs/exercises/COMMON_CHIRPSTACK_API.md`](specs/exercises/COMMON_CHIRPSTACK_API.md).

👉 Índice completo de la ruta didáctica: **[`specs/exercises/README.md`](specs/exercises/README.md)**

## 📚 Wiki

Los **fundamentos de LoRa, LoRaWAN y cómo usar este proyecto** están en la **[Wiki del repositorio](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)** — ideal si empiezas de cero.

## 📄 Licencia

Publicado bajo la **Clear BSD License** (la misma del proyecto original de Semtech). Consulta
[`LICENSE.txt`](LICENSE.txt), [`LICENSES.txt`](LICENSES.txt) y el archivo [`NOTICE`](NOTICE) (que
documenta la derivación y los cambios). Las adiciones didácticas de radiosonda_PIcaro se distribuyen
bajo la misma licencia Clear BSD, conservando el copyright de Semtech en el código original.

---

## 🛠️ Modifica el firmware y haz tus propios binarios

Ninguno de los binarios viene precompilado a propósito: **la idea es que los generes tú**. Cambia el
periodo de envío, el formato del payload, activa/desactiva servicios (GNSS/Wi-Fi), integra otro
sensor… y recompila para crear **tus propias variantes**. Todo el flujo (compilar → flashear →
provisionar → consumir) está documentado para que experimentes sin miedo.

**Happy hacking 🚀**
