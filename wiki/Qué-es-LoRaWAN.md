# 2 · ¿Qué es LoRaWAN?

**LoRaWAN** es el **protocolo de red** (capa MAC) estandarizado por la **LoRa Alliance** que corre
**encima de la radio LoRa**. Define cómo los dispositivos se **identifican**, se **unen** a la red,
**cifran** sus datos y llegan hasta una aplicación.

## Arquitectura (topología "estrella de estrellas")

```
  [Dispositivos]  ...))))  [Gateways]  ──internet──  [Network Server]  ──  [Aplicación / Dashboard]
   (end-devices)    LoRa                  IP                 (ChirpStack)         (tu software)
```

- **End-device (nodo):** el sensor/actuador (aquí: LR1110 o TTGO+BMP280).
- **Gateway:** "antena" que oye las tramas LoRa y las reenvía por Internet. **No decide nada**, solo
  repite. Un mensaje puede ser oído por **varios** gateways.
- **Network Server (NS):** el cerebro (**ChirpStack** en este proyecto). Deduplica, valida
  seguridad, gestiona el join, elige ritmos (ADR) y enruta a la aplicación.
- **Application Server:** recibe los datos ya limpios (por MQTT/HTTP) para tu dashboard.

## Cómo se une un dispositivo: OTAA vs ABP
- **OTAA** (*Over-The-Air Activation*, el recomendado y el que usamos): el nodo hace un **Join
  Request** con tres credenciales y el servidor responde un **Join Accept**. Genera claves de sesión
  frescas cada vez.
  - **DevEUI** — identificador único del dispositivo (64 bits).
  - **JoinEUI** (antes *AppEUI*) — identifica el servidor de join.
  - **AppKey** — clave secreta raíz (nunca viaja por el aire).
- **ABP** (*Activation By Personalization*): claves de sesión fijas grabadas a mano. Más simple pero
  menos seguro; no lo usamos.

> 🔑 **Regla del proyecto:** en ChirpStack, para LoRaWAN **1.0.x** la *Application Key* se guarda en
> el campo **`nwkKey`**. Y en los nodos **Arduino/LMIC** (TTGO), el **DevEUI y el JoinEUI se escriben
> al revés (LSB)** que en ChirpStack (MSB). Estos dos detalles causan el 90% de los fallos de join.

## Clases de dispositivo
- **Clase A** (la de este proyecto): el nodo habla cuando **él** quiere (uplink) y solo escucha
  respuestas en dos ventanas cortas justo después. Mínimo consumo.
- **Clase B:** ventanas de escucha programadas (con *beacons*).
- **Clase C:** escucha casi todo el tiempo (más consumo, para dispositivos alimentados).

## Conceptos que verás
- **Uplink / Downlink:** del nodo al servidor / del servidor al nodo.
- **fPort:** "puerto" (1–223) que etiqueta el tipo de payload (p.ej. fPort 10 = datos del sensor).
- **Payload + codec:** los bytes crudos del sensor; un *codec* (JavaScript en ChirpStack) los
  convierte en campos legibles (`temperature`, `pressure`) para el dashboard.
- **Duty cycle / dwell time:** límites legales de tiempo en aire (EU868 tiene duty cycle ~1%;
  US915 usa límite de *dwell time*). Por eso no se envía "todo el tiempo".

> Siguiente: **[Hardware del proyecto](Hardware-del-proyecto)** →
