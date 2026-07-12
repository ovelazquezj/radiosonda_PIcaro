# Ejercicio 00 — ChirpStack en Docker (tu servidor LoRaWAN local)

> **En una frase:** levantas en tu propio PC el **servidor de red LoRaWAN** (ChirpStack v4) al que
> se conectarán **todos** los demás ejercicios: web de gestión en `:8080` y API REST en `:8090`.
> **Plataforma:** **Docker Compose** — aquí **no hay hardware ni radio todavía**, solo contenedores.
> Este ejercicio es el **prerequisito** de los ejercicios 01–08: sin él, nada tiene a dónde conectarse.

## 🎯 Qué vas a conseguir
Un **Network Server LoRaWAN v4** corriendo en local, con todo lo que los demás ejercicios dan por
hecho: la **web de gestión** (donde ves devices, gateways y tramas), la **API REST** (que usan
`provision.sh` y `consume.py`) y el **broker MQTT** (por donde salen los uplinks). Al terminar
tendrás además tu **API key** (el `TOKEN`) exportada, lista para el Ejercicio 01.

Así se ve el éxito: los 7 contenedores **arriba** y la web pidiéndote login.

```
$ docker compose ps
NAME                                             IMAGE                                    STATUS         PORTS
00_chirpstack-docker-chirpstack-1                chirpstack/chirpstack:4                  Up 40 seconds  0.0.0.0:8080->8080/tcp
00_chirpstack-docker-chirpstack-rest-api-1       chirpstack/chirpstack-rest-api:4         Up 40 seconds  0.0.0.0:8090->8090/tcp
00_chirpstack-docker-mosquitto-1                 eclipse-mosquitto:2                      Up 40 seconds  0.0.0.0:1883->1883/tcp
...
# y en el navegador: http://localhost:8080  →  pantalla de login (admin / admin)
```

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, no continúes):

- [ ] **Docker Desktop instalado y ABIERTO** (Windows/macOS) o **Docker Engine** corriendo (Linux) → [Wiki: Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación)
- [ ] En **Windows:** activa la **integración con WSL2** en Docker Desktop (*Settings → Resources → WSL integration*), y ejecuta los comandos desde una terminal WSL/Ubuntu.
- [ ] **Comprueba que Docker responde:** `docker --version` (ver salida abajo).
- [ ] **~2 GB de disco** libres y conexión a internet: la **primera vez** se descargan las imágenes.
- [ ] Un **navegador** y una terminal **bash** para los `curl` del bootstrap.

Comprobación rápida de que Docker está listo:
```bash
docker --version
```
**Salida esperada** (algo así; la versión exacta da igual):
```
Docker version 27.3.1, build ce12230
```
> ℹ️ Los comandos `./*.sh`, `curl` y `python3` corren en **bash** (Linux/macOS o **WSL** en Windows).
> Si `docker --version` no responde, es que **Docker Desktop no está arrancado** → ábrelo y espera a
> que el icono de la ballena deje de animarse.

## 🧩 Qué se levanta (los servicios)
No hay que cablear nada: `docker compose` arranca **7 contenedores** que juntos forman el servidor.
La configuración **ya viene fijada en este repo** (carpeta [`configuration/`](configuration/)) — no
tienes que tocar nada para los ejercicios del curso.

| Servicio (contenedor) | Imagen | Puerto expuesto | Para qué sirve |
|---|---|---|---|
| **chirpstack** | `chirpstack/chirpstack:4` | **8080** (web) | Network Server + **web de gestión** |
| **chirpstack-rest-api** | `chirpstack/chirpstack-rest-api:4` | **8090** | **API REST** (la usan `provision.sh` / `consume.py`) |
| **mosquitto** | `eclipse-mosquitto:2` | **1883** | **Broker MQTT** — por aquí salen los uplinks |
| chirpstack-gateway-bridge | `chirpstack/chirpstack-gateway-bridge:4` | 1700/udp | Recibe gateways **Semtech UDP** (ej. 07) |
| chirpstack-gateway-bridge-basicstation | `chirpstack/chirpstack-gateway-bridge:4` | 3001 | Recibe gateways **Basics Station** |
| postgres | `postgres:14-alpine` | (interno) | Base de datos |
| redis | `redis:7-alpine` | (interno) | Estado / caché |

> 🔎 **Ojo al nombre del contenedor MQTT.** Compose usa el nombre de **esta carpeta** como prefijo
> del proyecto, así que al lanzar el compose desde `00_chirpstack-docker/` el broker se llama
> **`00_chirpstack-docker-mosquitto-1`**. Los otros ejercicios lo buscan por ese nombre (sus scripts
> ya lo **autodetectan**), pero por eso conviene arrancar siempre el stack **desde esta carpeta**.

## 🪜 Paso a paso

### 1. Arranca el stack
```bash
# desde specs/exercises/00_chirpstack-docker/
cd specs/exercises/00_chirpstack-docker
docker compose up -d
```
La **primera vez** descargará las imágenes (tarda un poco). Cuando termine:

**Salida esperada** (algo así):
```
[+] Running 8/8
 ✔ Network 00_chirpstack-docker_default                                      Created
 ✔ Container 00_chirpstack-docker-mosquitto-1                                Started
 ✔ Container 00_chirpstack-docker-redis-1                                    Started
 ✔ Container 00_chirpstack-docker-postgres-1                                 Started
 ✔ Container 00_chirpstack-docker-chirpstack-1                               Started
 ✔ Container 00_chirpstack-docker-chirpstack-gateway-bridge-1               Started
 ✔ Container 00_chirpstack-docker-chirpstack-gateway-bridge-basicstation-1  Started
 ✔ Container 00_chirpstack-docker-chirpstack-rest-api-1                      Started
```
Comprueba que siguen **arriba**:
```bash
docker compose ps
```
Todos deben aparecer con estado **`Up`** (ver el bloque de la sección [🎯](#-qué-vas-a-conseguir)).
> 💡 `-d` los deja corriendo en segundo plano. Para ver los logs: `docker compose logs -f chirpstack`.
> Para pararlo todo (sin borrar datos): `docker compose down`.

### 2. Entra a la web
Abre en el navegador:
```
http://localhost:8080
```
**Salida esperada:** la pantalla de login de ChirpStack. Entra con:
- **Usuario:** `admin`
- **Contraseña:** `admin`

La API REST vive en paralelo en `http://localhost:8090` (interfaz de exploración de endpoints).

### 3. Bootstrap: tenant, aplicación, device profile y API key
Este es el paso que **los ejercicios 01–08 dan por hecho**. Lo haces **una sola vez**. La única
pieza **obligatoria** es la **API key** (el `TOKEN`); el resto (tenant, aplicación, device profile)
lo puede **crear automáticamente** `provision.sh`, pero aquí aprendes qué es cada cosa y **dónde ver
sus IDs**.

**3a. Tenant.** Un *tenant* es tu "organización" dentro de ChirpStack. La instalación ya trae uno
por defecto (suele llamarse **`ChirpStack`**); puedes usar ese o crear el tuyo (en el stack de
referencia se llama `Horizonte_1`). Entra al tenant desde la web; su **ID** aparece en la URL:
```
http://localhost:8080/#/tenants/f8a271ec-591f-4e4c-956a-47d5d9ce9f87/...
                                  └──────────── este UUID es tu TENANT ────────────┘
```
```bash
export TENANT="f8a271ec-591f-4e4c-956a-47d5d9ce9f87"   # ← el tuyo será distinto
```

**3b. Aplicación.** Dentro del tenant → *Applications* → *Add* → nombre **`Demos-LR1110`** (el que
usan las demos). Al abrirla, su **ID** también sale en la URL (`.../applications/<UUID>/...`):
```bash
export APP="5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7"      # ← el tuyo será distinto
```

**3c. Device profile US915.** *Device profiles* → *Add* con estos valores (los que espera el
binario del ejercicio 01):
- **Region:** `US915`
- **MAC version:** `LoRaWAN 1.0.4`
- **Regional parameters revision:** `RP002-1.0.3`
- **Activation:** OTAA (*Device supports OTAA*)

> Puedes saltarte 3b y 3c: `provision.sh` **busca o crea** la aplicación `Demos-LR1110` y un device
> profile `PIcaro-LR1110-US915` con exactamente estos parámetros si no existen. Lo importante es
> entender qué son.

**3d. API key (el `TOKEN`) — imprescindible.** Dentro del tenant → *API keys* → *Add API key* →
ponle un nombre → **copia el token** (se muestra **una sola vez**). Expórtalo:
```bash
export TOKEN="PEGA_AQUI_TU_API_KEY"
```
Verifica que el token funciona pidiendo tus tenants por REST:
```bash
curl -s "http://localhost:8090/api/tenants?limit=1" -H "Authorization: Bearer $TOKEN"
```
**Salida esperada** (algo así — verás tu tenant):
```json
{"totalCount":"1","result":[{"id":"f8a271ec-591f-4e4c-956a-47d5d9ce9f87","name":"Horizonte_1", ...}]}
```
> El detalle completo del alta por API (crear device, poner AppKey en `nwkKey`, errores típicos) está
> en [`COMMON_CHIRPSTACK_API.md`](../COMMON_CHIRPSTACK_API.md) y en el paso a paso de
> [`../../demos/REGISTRO_API_CHIRPSTACK.md`](../../demos/REGISTRO_API_CHIRPSTACK.md). Aquí solo
> necesitas dejar el **`TOKEN`** exportado.

## ✅ Cómo saber que funcionó
- [ ] `docker compose ps` muestra los **7 contenedores** con estado **`Up`**.
- [ ] `http://localhost:8080` **carga la web** y entras con **admin / admin**.
- [ ] Tienes una **API key** creada y el `curl` de verificación te devuelve tu tenant (no un error de auth).
- [ ] Has exportado tu **`TOKEN`** (y opcionalmente `TENANT` y `APP`) en la terminal.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| `Cannot connect to the Docker daemon` | Docker Desktop no está arrancado | Abre Docker Desktop y espera a que esté *Running*; repite `docker compose up -d` |
| `docker: command not found` (en Windows/WSL) | Falta la **integración WSL2** | Docker Desktop → *Settings → Resources → WSL integration* → activa tu distro |
| `docker compose` da *"is not a docker command"* | Tienes Compose **v1** | Usa `docker-compose up -d` (con guion), o actualiza a Docker con Compose **v2** |
| `Bind for 0.0.0.0:8080 failed: port is already allocated` | El puerto **8080/8090** ya está ocupado | Cierra el proceso que lo usa, o remapea el puerto en [`docker-compose.yml`](docker-compose.yml) (`"8081:8080"`) |
| La web no carga aunque el contenedor está `Up` | Aún inicializando la BD (primer arranque) | Espera ~30 s y recarga; mira `docker compose logs -f chirpstack` |
| `curl` devuelve `code:16` / *unauthenticated* | `TOKEN` mal copiado o no exportado | Repite el paso 3d; revisa el header `Authorization: Bearer $TOKEN` |
| Las imágenes no descargan / va lentísimo | Sin internet o Docker sin acceso a red | Verifica conexión; reintenta `docker compose pull` |

## ⚙️ Configuración avanzada (opcional)
Para el curso **no necesitas tocar nada**: el repo ya trae las regiones habilitadas (`eu868`,
`us915_0`, `us915_1` en `configuration/chirpstack/chirpstack.toml`) y el *gateway bridge* de Basics
Station preconfigurado para `us915_1`. Solo si quieres experimentar:

- **Importar el catálogo de device profiles** de la comunidad (opcional): `make import-device-profiles`
  (ver [`Makefile`](Makefile); requiere `make`).
- **Reconfigurar regiones / topic prefixes**, conectar un **ChirpStack Gateway Bridge** (UDP `:1700`)
  o **Basics Station** (`:3001`), y detalles de **persistencia de datos** (volúmenes de Postgres/Redis):
  documentación del proyecto **upstream** → [chirpstack-docker](https://github.com/chirpstack/chirpstack-docker).
- **Puesta a punto de ChirpStack en general:** [Wiki: ChirpStack en 5 minutos](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos).

## ➡️ Navegación
- ⬅️ Anterior: — (este es el **primer** ejercicio del curso)
- ➡️ Siguiente: [Ejercicio 01 · Periodical Uplink](../01_periodical-uplink/) — tu primer join OTAA
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Archivos de esta carpeta:** [`docker-compose.yml`](docker-compose.yml) (los 7 servicios) · [`Makefile`](Makefile) (`make import-device-profiles`) · [`configuration/`](configuration/) (config ya fijada de ChirpStack, gateway bridge y mosquitto) · [`LICENSE`](LICENSE).
- **Guías comunes:** [ChirpStack API (REST/MQTT/codec)](../COMMON_CHIRPSTACK_API.md) · [Alta de un device por REST paso a paso](../../demos/REGISTRO_API_CHIRPSTACK.md).
- **Upstream:** este esqueleto procede de [chirpstack/chirpstack-docker](https://github.com/chirpstack/chirpstack-docker) — el [`LICENSE`](LICENSE) de la carpeta es **MIT © Orne Brocaar**.

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
</content>
</invoke>
