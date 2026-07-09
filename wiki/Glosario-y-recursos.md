# 6 · Glosario y recursos

## Glosario rápido
| Término | Significado |
|---------|-------------|
| **LoRa** | Modulación de radio (capa física) de largo alcance y bajo consumo. |
| **LoRaWAN** | Protocolo de red (capa MAC) que corre sobre LoRa. |
| **End-device / nodo** | El dispositivo final (sensor). |
| **Gateway** | Antena que reenvía tramas LoRa a Internet; no procesa. |
| **Network Server (NS)** | El cerebro de la red (aquí, ChirpStack). |
| **OTAA** | Activación por aire con DevEUI + JoinEUI + AppKey. |
| **ABP** | Activación con claves de sesión fijas (no usada aquí). |
| **DevEUI** | Identificador único del dispositivo (64 bits). |
| **JoinEUI / AppEUI** | Identifica el servidor de join (64 bits). |
| **AppKey** | Clave raíz secreta (en ChirpStack 1.0.x va en `nwkKey`). |
| **Clase A/B/C** | Modos de escucha del nodo (A = mínimo consumo). |
| **Uplink / Downlink** | Nodo→servidor / servidor→nodo. |
| **fPort** | Puerto (1–223) que etiqueta el tipo de payload. |
| **SF (Spreading Factor)** | SF7…SF12: compromiso alcance ↔ velocidad ↔ consumo. |
| **ADR** | Ajuste automático del data rate por el servidor. |
| **Codec** | Script que convierte bytes crudos en campos legibles. |
| **Duty cycle** | Límite legal de tiempo en aire (p.ej. EU868 ~1%). |
| **LR1110** | Chip "LoRa Edge": LoRa + GNSS + Wi-Fi + cripto. |
| **LMIC** | Librería LoRaWAN de Arduino (usada por el TTGO). |
| **LSB / MSB** | Orden de bytes; en LMIC los EUIs van en LSB, en ChirpStack en MSB. |

## Recursos oficiales
- **Semtech LoRa Basics Modem (SWL2001):** <https://github.com/Lora-net/SWL2001>
- **LoRa Alliance (especificaciones LoRaWAN):** <https://lora-alliance.org/>
- **ChirpStack (documentación):** <https://www.chirpstack.io/docs/>
- **ChirpStack Docker:** <https://github.com/chirpstack/chirpstack-docker>
- **Semtech LR1110:** <https://www.semtech.com/products/wireless-rf/lora-edge/lr1110>
- **MCCI Arduino LMIC:** <https://github.com/mcci-catena/arduino-lmic>

## Volver
- [🏠 Inicio de la Wiki](Home)
- [Repositorio radiosonda_PIcaro](https://github.com/ovelazquezj/radiosonda_PIcaro)
