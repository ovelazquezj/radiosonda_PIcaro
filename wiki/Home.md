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
5. **[Cómo usar el proyecto](Cómo-usar-el-proyecto)** — la ruta de ejercicios 00 → 06.
6. **[Glosario y recursos](Glosario-y-recursos)** — términos y enlaces oficiales.

## ¿Qué vas a lograr?
- Unir un dispositivo a una red LoRaWAN (**join OTAA**) y ver sus tramas.
- Enviar datos de un **sensor BMP280** (temperatura y presión) por radio.
- Hacer **geolocalización GNSS/Wi-Fi** con el LR1110.
- **Provisionar y leer datos** desde ChirpStack por API (REST/MQTT) para dashboards.
- Modificar el firmware y **compilar tus propios binarios**.

## Atribución
El proyecto deriva de **LoRa Basics Modem (SWL2001)** de **Semtech** — repo original:
<https://github.com/Lora-net/SWL2001>. Licencia **Clear BSD**. Este material es una capa
**educativa** encima de ese trabajo.

> ¿Listo? Empieza por **[¿Qué es LoRa?](Qué-es-LoRa)** 🚀
