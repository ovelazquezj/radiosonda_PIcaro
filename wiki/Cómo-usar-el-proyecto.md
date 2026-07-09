# 5 · Cómo usar el proyecto (ruta didáctica)

Los ejercicios están en **[`specs/exercises/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises)**
y van de lo simple a lo avanzado. Cada uno es **autocontenido**: README, credenciales, provisión y
consumo.

## Requisitos
- **Toolchain ARM** (`arm-none-eabi-gcc` 13.2) + `make` — para los nodos LR1110.
- **Arduino IDE** (+ MCCI LMIC, U8g2, Adafruit BMP280) — para los TTGO.
- **ChirpStack** en Docker + un **gateway** de tu banda con cobertura (o **monta el tuyo** → ejercicio 07).
- **Hardware** según el ejercicio (ver [Hardware del proyecto](Hardware-del-proyecto)).

## La ruta recomendada

| Paso | Ejercicio | Qué aprendes |
|------|-----------|--------------|
| 0 | **00 · ChirpStack** | Levantar el Network Server en Docker |
| 1 | **01 · Periodical Uplink** | Tu primer **join OTAA** y ver uplinks en ChirpStack |
| 2 | **02 · BMP280 + GNSS** | Enviar **datos de sensor** + geolocalización, con codec |
| 3 | **03 · Hardware Modem** | Módem LoRaWAN controlado por un **host** (arquitectura) |
| 4 | **04 · Wi-Fi Region Detection** | La capacidad **Wi-Fi** del LR1110 (solo UART) |
| 5 | **05 · TTGO LoRa** | Integrar un **nodo de terceros** (ESP32/LMIC) |
| 6 | **06 · TTGO + BMP280** | Un **sensor real** en el TTGO enviando cada minuto |
| 7 | **07 · Gateway de 1 canal** | Montar **tu propio gateway** (TTGO) para recibir a los nodos |

> El **07** es distinto: no es un nodo, es **infraestructura** — montas el gateway que reciben los
> demás. Guía paso a paso: **[Montar un gateway de 1 canal](How-To-Montar-un-gateway-de-1-canal)**.

## El flujo de cada ejercicio (siempre igual)
1. **Compila** el binario (LR1110) o el sketch (TTGO) — ver `COMMON_BUILD.md`.
2. **Flashea** el nodo — ver `COMMON_FLASH.md`.
3. **Provisiona** el device en ChirpStack — `./provision.sh` (o por la web).
4. **Verifica** el join en los logs / la web de ChirpStack.
5. **Consume** los datos — `./scripts/subscribe.sh` (MQTT) → dashboard.

## Consejo para docentes
Empieza siempre por el **01** (es el "hola mundo": se ve el join y los uplinks al instante). El
**02** y el **06** son los mejores para hablar de **sensores y dashboards**. El **05** enseña que
LoRaWAN es **interoperable** entre fabricantes.

> Cierre: **[Glosario y recursos](Glosario-y-recursos)** →
