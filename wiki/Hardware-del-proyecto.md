# 3 · Hardware del proyecto

Este proyecto usa **dos plataformas de nodo** y un sensor.

## Semtech LR1110 (ejercicios 01–04)
El **LR1110** es un chip **"LoRa Edge"**: combina en un solo integrado:
- **Radio LoRaWAN** sub-GHz (transmisión/recepción).
- **Escáner GNSS** (GPS/BeiDou) para geolocalización de bajo consumo.
- **Escáner Wi-Fi pasivo** (detecta APs cercanos → posición/región).
- **Motor criptográfico** por hardware.

En este proyecto va sobre una placa **ST Nucleo-L476RG** (microcontrolador STM32L476) y corre la
pila **LoRa Basics Modem** de Semtech. Los binarios se **compilan** con el toolchain ARM y se
flashean a la Nucleo.

## TTGO ESP32 LoRa v1 (ejercicios 05–06)
Placa popular y económica: **ESP32** + radio **SX1276** (solo LoRa, sin GNSS/Wi-Fi de LoRa Edge) +
pantalla **OLED**. Se programa con **Arduino IDE** usando la librería **MCCI LMIC**. Es un ejemplo
de **nodo de terceros** para que el alumno vea que LoRaWAN es interoperable.

## Sensor BMP280 (ejercicios 02 y 06)
Sensor **Bosch** de **temperatura y presión** (no mide humedad; ese es el BME280). Se conecta por
**I²C** (dirección 0x76). El proyecto trae su **driver**, el **decoder** de payload y la
**documentación de pines** en cada ejercicio.

## ¿Qué usa cada ejercicio?

| Ejercicio | Nodo | Radio | Extra |
|-----------|------|-------|-------|
| 01 Periodical Uplink | Nucleo-L476 | LR1110 | — |
| 02 BMP280 + GNSS | Nucleo-L476 | LR1110 | BMP280 (I²C) + GNSS/Wi-Fi |
| 03 Hardware Modem | Nucleo-L476 | LR1110 | control por host (UART) |
| 04 Wi-Fi Region Detection | Nucleo-L476 | LR1110 | escaneo Wi-Fi |
| 05 TTGO LoRa | TTGO ESP32 | SX1276 | OLED |
| 06 TTGO + BMP280 | TTGO ESP32 | SX1276 | BMP280 (I²C) + OLED |

> Los detalles de **cableado del sensor** (pines) están en el README de los ejercicios 02 y 06 y en
> el diagrama `specs/bmp280-gnss-tracker/pinout_diagram.html`.

> Siguiente: **[ChirpStack en 5 minutos](ChirpStack-en-5-minutos)** →
