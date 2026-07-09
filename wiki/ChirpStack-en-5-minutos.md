# 4 · ChirpStack en 5 minutos

**ChirpStack** es un **Network Server LoRaWAN** de código abierto (v4). Es el "cerebro" que recibe
las tramas desde los gateways, valida la seguridad, gestiona el *join*, y entrega los datos a tu
aplicación. En este proyecto corre en **Docker** (ejercicio **00**).

## Jerarquía de objetos (de arriba a abajo)
- **Tenant** — el "espacio" de una organización.
- **Gateway** — se registra para que reciba tráfico LoRa.
- **Application** — agrupa dispositivos con un propósito (p.ej. "sensores-aula").
- **Device profile** — define capacidades comunes: región (US915/EU868), versión LoRaWAN, OTAA, y
  el **codec** de payload.
- **Device** — cada nodo, identificado por su **DevEUI**, con sus **claves (OTAA keys)**.

## Dos caras: provisión y datos
ChirpStack separa **configurar** de **recibir datos**:

- **Provisión (REST API):** crear application, device profile, device y claves. En este proyecto se
  hace con scripts `provision.sh` (curl) — reproducible.
- **Datos (MQTT):** cada uplink se publica en un *topic*:
  ```
  application/<app_id>/device/<devEui>/event/up
  ```
  El mensaje trae el payload **crudo** y, si hay **codec**, el objeto **decodificado**
  (`temperature`, `pressure`…) — la fuente directa para un **dashboard**.

> El **REST API no guarda un histórico de payloads**: sirve para provisionar y leer estado/métricas.
> Los **datos** de sensor llegan por **MQTT** (o una integración como InfluxDB).

## El codec (datos legibles)
Se adjunta un pequeño **JavaScript** al *device profile* que convierte los bytes en campos. Así el
MQTT entrega `object: { temperature: 23.45, pressure: 1013 }` en vez de bytes crudos. En el proyecto
se sube con `scripts/upload_codec.sh`.

## Errores típicos (y su causa)
| Síntoma en logs | Causa | Arreglo |
|-----------------|-------|---------|
| `Unknown device` | el DevEUI no está registrado (o al revés) | registrar el DevEUI correcto (MSB) |
| no responde / `invalid MIC` | AppKey mal o en el campo equivocado | en 1.0.x va en **`nwkKey`** |
| `DevNonce has already been used` | nonce repetido tras reflashear | borrar y recrear el device |

> Siguiente: **[Cómo usar el proyecto](Cómo-usar-el-proyecto)** →
