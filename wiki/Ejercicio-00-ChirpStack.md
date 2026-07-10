# Ejercicio 00 — ChirpStack en Docker (el servidor de red)

Levanta el **Network Server LoRaWAN v4 (ChirpStack)** en Docker. No es un nodo: es la
**infraestructura** que recibe las tramas de los gateways, gestiona el *join* y entrega los datos.
Es el **prerequisito** de todos los ejercicios que se conectan a la red (01, 02, 03…).

> **Carpeta del ejercicio:** [`specs/exercises/00_chirpstack-docker/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/00_chirpstack-docker) · **Plataforma:** Docker en tu PC (sin radio ni placa)

| | |
|---|---|
| Qué demuestra | Arrancar un servidor de red LoRaWAN v4 completo (web + REST API + broker MQTT) en contenedores, listo para dar de alta devices y recibir uplinks |
| Hardware | Ninguno — solo **Docker** (no hay Nucleo, LR1110 ni TTGO) |
| ¿Join / ChirpStack? | **Es ChirpStack**: no hace join, lo **recibe**. Sin binario y sin credenciales de device |
| Dato / observable | Web `http://localhost:8080` (**admin / admin**), REST API `http://localhost:8090`, broker MQTT `localhost:1883` |
| Binario / sketch | Ninguno — se levanta con `docker compose up -d` |

## Ruta paso a paso

1. **Requisitos** → [Requisitos e instalación](How-To-Requisitos-e-instalación) · instala **Docker + docker compose** (sección **2** de la guía). No necesitas el toolchain ARM ni Arduino para este ejercicio.
2. **Levantar el stack** → sigue [Requisitos §2](How-To-Requisitos-e-instalación): desde la carpeta del ejercicio ejecuta `docker compose up -d`. Arranca ChirpStack, PostgreSQL, Redis, Mosquitto (MQTT) y los *gateway bridges*.
3. **Entrar a la web** → abre `http://localhost:8080` y entra con **admin / admin**. Aquí verás el *tenant*, las *applications*, los *device profiles* y los devices.
4. **Entender qué es cada objeto** → [ChirpStack en 5 minutos](ChirpStack-en-5-minutos) explica la jerarquía Tenant → Gateway → Application → Device profile → Device, y la separación provisión (REST) vs. datos (MQTT).
5. **Mandar el primer uplink** → con ChirpStack en marcha, continúa con [Ejercicio 01 — Periodical Uplink](Ejercicio-01-Periodical-Uplink), que flashea un nodo y lo da de alta aquí. Para que llegue tráfico necesitarás además un [gateway de tu banda](How-To-Montar-un-gateway-de-1-canal).

## Qué observar

- **Docker:** `docker compose ps` muestra los contenedores `chirpstack-docker-*` en estado *running* (chirpstack, postgres, redis, mosquitto, gateway-bridge).
- **Web (`:8080`):** login `admin/admin`; el *tenant* y las *applications* del laboratorio aparecen ya creados.
- **REST API (`:8090`):** la UI Swagger responde; es el endpoint que usan los `provision.sh` de cada ejercicio (header `Authorization: Bearer <API key>`).
- **MQTT (`:1883`):** el contenedor `chirpstack-docker-mosquitto-1` es donde se publican los uplinks (`application/<APP>/device/<DEVEUI>/event/up`) — la fuente de datos para los dashboards.

## Credenciales y detalles

- **Sin credenciales de device:** este ejercicio no hace join, así que **no** tiene DevEUI/JoinEUI/AppKey ni `credentials.json`. Las credenciales viven en los ejercicios de nodo (01, 02…).
- **Acceso web:** usuario `admin`, contraseña `admin`. El **API key** para los scripts se crea en la web → *Tenant → API keys* (ver [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)).
- **Regiones ya habilitadas:** el `configuration/chirpstack/chirpstack.toml` de este ejercicio ya trae **`us915_0`** y **`us915_1`** (además de `eu868`) en `enabled_regions` → **no hace falta editar ningún `.toml`**. Los nodos LR1110 de este proyecto usan la sub-banda **`us915_1`** (canales 8–15); asegúrate de poner esa misma en el *device profile* US915 (detalle en [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack), sección de sub-banda).

## Ficheros del ejercicio

- `docker-compose.yml` — define todos los servicios (ChirpStack, Postgres, Redis, Mosquitto, *gateway bridges*).
- `configuration/` — configuración de ChirpStack (incluye `chirpstack.toml` con las regiones habilitadas), del *gateway bridge*, de Mosquitto y los *scripts* de init de PostgreSQL.
- `Makefile` — atajo opcional `make import-device-profiles` para importar el repositorio de perfiles de dispositivo.
- Detalle completo en el [README de la carpeta](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/00_chirpstack-docker).

> ◀ [Cómo usar el proyecto](Cómo-usar-el-proyecto) · [Ejercicio 01 — Periodical Uplink ▶](Ejercicio-01-Periodical-Uplink)
