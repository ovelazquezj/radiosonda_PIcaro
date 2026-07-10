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
5. **[Cómo usar el proyecto](Cómo-usar-el-proyecto)** — la ruta de ejercicios 00 → 07.
6. **[Glosario y recursos](Glosario-y-recursos)** — términos y enlaces oficiales.

## 🔧 Guías prácticas (How-To)
¿Manos a la obra? Estas guías te llevan paso a paso:
1. **[Requisitos e instalación](How-To-Requisitos-e-instalación)** — instala toolchain, Docker, Arduino y herramientas.
2. **[Compilar el firmware LR1110](How-To-Compilar-el-firmware)** — builds de los ejercicios 01–04.
3. **[Flashear y ver la serie](How-To-Flashear-y-ver-la-serie)** — grabar la Nucleo y leer la traza.
4. **[Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino)** — ejercicios 05–06 (ESP32).
5. **[Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)** — registrar y consumir por API.
6. **[Dar de alta un LR1110 nuevo](How-To-Dar-de-alta-un-LR1110-nuevo)** — credenciales propias: código → build → ChirpStack.
7. **[Montar un gateway de 1 canal](How-To-Montar-un-gateway-de-1-canal)** — tu propio gateway TTGO → ChirpStack (ejercicio 07).

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

> **No es un producto de Semtech** y **no está afiliado a, patrocinado por ni respaldado por
> Semtech.** *LoRa®* y *LoRaWAN®* son marcas de Semtech / LoRa Alliance®, usadas aquí solo de forma
> nominativa con fines educativos. Detalles en el archivo
> [`NOTICE`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/NOTICE) del repositorio.

> ¿Listo? Empieza por **[¿Qué es LoRa?](Qué-es-LoRa)** 🚀
