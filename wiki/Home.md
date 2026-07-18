# Bienvenido a radiosonda_PIcaro 📡

**radiosonda_PIcaro** es un proyecto **didáctico** para aprender **LoRa** y **LoRaWAN** de forma
práctica, con hardware real (Semtech **LR1110**, **TTGO ESP32**, sensor **BMP280**) y un servidor
**ChirpStack**. Esta Wiki te da los **fundamentos** para empezar de cero y te guía por los
ejercicios.

## Si empiezas de cero, lee en este orden
1. **[¿Qué es LoRa?](Qué-es-LoRa)** — la radio: alcance largo, bajo consumo.
2. **[¿Qué es LoRaWAN?](Qué-es-LoRaWAN)** — el protocolo de red encima de LoRa.
3. **[Hardware del proyecto](Hardware-del-proyecto)** — LR1110, TTGO ESP32, BMP280.
4. **[ChirpStack en 5 minutos](ChirpStack-en-5-minutos)** — el servidor de red LoRaWAN.
5. **[Cómo usar el proyecto](Cómo-usar-el-proyecto)** — la ruta de ejercicios 00 → 10.
6. **[Glosario y recursos](Glosario-y-recursos)** — términos y enlaces oficiales.

## 📟 Los ejercicios (00 → 10)
La ruta práctica, de lo simple a lo avanzado; cada uno es **autocontenido** (README, credenciales,
provisión y consumo). Recorrido guiado en **[Cómo usar el proyecto](Cómo-usar-el-proyecto)**.
- **[00 · ChirpStack](Ejercicio-00-ChirpStack)** — levantar el Network Server en Docker.
- **[01 · Periodical Uplink](Ejercicio-01-Periodical-Uplink)** — tu primer **join OTAA** y ver uplinks.
- **[02 · BMP280 + GNSS](Ejercicio-02-BMP280-GNSS)** — datos de **sensor** + geolocalización, con codec.
- **[03 · Hardware Modem](Ejercicio-03-Hardware-Modem)** — módem LoRaWAN controlado por un **host**.
- **[04 · Wi-Fi Region Detection](Ejercicio-04-Wi-Fi-Region-Detection)** — la capacidad **Wi-Fi** del LR1110 (solo UART).
- **[05 · TTGO LoRa32](Ejercicio-05-TTGO-LoRa32)** — integrar un **nodo de terceros** (ESP32/LMIC).
- **[06 · TTGO + BMP280](Ejercicio-06-TTGO-BMP280)** — un **sensor real** en el TTGO enviando cada minuto.
- **[07 · Gateway de 1 canal](Ejercicio-07-Gateway-1-canal)** — **monta tu propio gateway** (TTGO) para recibir a los nodos.
- **[08 · Radiosonda PICARO](Ejercicio-08-Radiosonda-PICARO)** — **LilyGo T-Beam** (GPS + batería) por RadioLib → ChirpStack.
- **[09 · Radiosonda PICARO Full](Ejercicio-09-Radiosonda-PICARO-Full)** — **LilyGo T-Beam Supreme** (ESP32-S3 + SX1262) con **ESP-IDF + RadioLib**: todos los sensores, "caja negra" en microSD y OTAA → ChirpStack.
- **[10 · Dashboard Mission Control](Ejercicio-10-Dashboard-Mission-Control)** — **dashboard** de escritorio (Python + tkinter) que consume la telemetría del ej.09 por MQTT: SQLite, gráficas, mapa real y CSV.

## 🔧 Guías prácticas (How-To)
¿Manos a la obra? Estas guías te llevan paso a paso:
1. **[Requisitos e instalación](How-To-Requisitos-e-instalación)** — instala toolchain, Docker, Arduino y herramientas.
2. **[Compilar el firmware LR1110](How-To-Compilar-el-firmware)** — builds de los ejercicios 01–04.
3. **[Flashear y ver la serie](How-To-Flashear-y-ver-la-serie)** — grabar la Nucleo y leer la traza.
4. **[Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino)** — ejercicios 05–06 (ESP32).
5. **[Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)** — registrar y consumir por API.
6. **[Dar de alta un LR1110 nuevo](How-To-Dar-de-alta-un-LR1110-nuevo)** — credenciales propias: código → build → ChirpStack.
7. **[Dar de alta un TTGO nuevo](How-To-Dar-de-alta-un-TTGO-nuevo)** — igual para TTGO (¡ojo al orden de bytes invertido!).
8. **[Montar un gateway de 1 canal](How-To-Montar-un-gateway-de-1-canal)** — tu propio gateway TTGO → ChirpStack (ejercicio 07).

## ¿Qué vas a lograr?
- Unir un dispositivo a una red LoRaWAN (**join OTAA**) y ver sus tramas.
- Enviar datos de un **sensor BMP280** (temperatura y presión) por radio.
- Hacer **geolocalización GNSS/Wi-Fi** con el LR1110.
- **Provisionar y leer datos** desde ChirpStack por API (REST/MQTT) para dashboards.
- **Montar tu propio gateway** de 1 canal (TTGO ESP32) para recibir a tus nodos en ChirpStack.
- Modificar el firmware y **compilar tus propios binarios**.

## Atribución
Este proyecto es un **fork / obra derivada no oficial** de **LoRa Basics Modem (SWL2001)** de
**Semtech Corporation** — repo original: <https://github.com/Lora-net/SWL2001>. Copyright © Semtech
Corporation; licencia **Clear BSD** (conservada intacta). Este material es una capa **educativa**
encima de ese trabajo.

> 📄 **Licencia del material educativo:** el contenido didáctico de esta Wiki y del repositorio
> (documentación) está licenciado bajo **[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)**
> © **Omar Velazquez**. El **código** permanece bajo **Clear BSD** (© Semtech). Detalle en
> [`LICENSE-CC-BY-4.0.md`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/LICENSE-CC-BY-4.0.md).

> **No es un producto de Semtech** y **no está afiliado a, patrocinado por ni respaldado por
> Semtech.** *LoRa®* y *LoRaWAN®* son marcas de Semtech / LoRa Alliance®, usadas aquí solo de forma
> nominativa con fines educativos. Detalles en el archivo
> [`NOTICE`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/NOTICE) del repositorio.

> ¿Listo? Empieza por **[¿Qué es LoRa?](Qué-es-LoRa)** 🚀
