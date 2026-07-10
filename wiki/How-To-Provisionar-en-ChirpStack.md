# How-To · Provisionar y consumir datos en ChirpStack (API)

Registrar el device por la **API REST** y leer sus datos por **MQTT**. Reproducible con scripts (cada
ejercicio trae su `provision.sh` y su `consume.py`/`subscribe.sh`). Conceptos en
[ChirpStack en 5 minutos](ChirpStack-en-5-minutos).

## Paso 0 · Variables de entorno
Crea un **API key** en la web (*Tenant → API keys*) y expórtalo una vez por sesión:
```bash
export TOKEN="PEGA_TU_API_KEY"
export API="http://localhost:8090"
export TENANT="<tenant-id>"
export APP="<application-id>"
```
> El **token es secreto**: úsalo solo en la terminal, **nunca** lo escribas en ficheros del repo.

## Paso 1 · Crear el device y sus claves
```bash
# Crear device
curl -s -X POST "$API/api/devices" -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"device":{"devEui":"<DEVEUI>","name":"<nombre>","applicationId":"'"$APP"'","deviceProfileId":"<DP_ID>","joinEui":"<JOINEUI>"}}'

# Poner el AppKey — ⚠️ en LoRaWAN 1.0.x va en el campo nwkKey (NO appKey)
curl -s -X POST "$API/api/devices/<DEVEUI>/keys" -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"deviceKeys":{"devEui":"<DEVEUI>","nwkKey":"<APPKEY>"}}'
```
> El **DevEUI/JoinEUI/AppKey** aquí van en **MSB** (big-endian), tal cual el `credentials.json`. En
> el sketch TTGO el DevEUI/JoinEUI van **invertidos** (ver
> [How-To TTGO](How-To-Compilar-el-TTGO-en-Arduino)).

## Paso 2 · Verificar el join
Con el nodo encendido, mira el estado o los logs:
```bash
curl -s "$API/api/devices/<DEVEUI>" -H "Authorization: Bearer $TOKEN"     # lastSeenAt, deviceStatus
docker logs -f chirpstack-docker-chirpstack-1 | grep -i join               # trazas de join
```

## Paso 3 · Consumir payloads por MQTT (la fuente del dashboard)
ChirpStack publica cada uplink en `application/<APP_ID>/device/<DEVEUI>/event/up`. El mensaje trae el
payload crudo (`data`, base64) y, **si hay codec**, el objeto decodificado (`object`):
```bash
# Dentro del contenedor mosquitto (siempre disponible):
docker exec chirpstack-docker-mosquitto-1 \
  mosquitto_sub -h localhost -t "application/$APP/device/+/event/up" -v
```
En Python se usa `paho-mqtt` (ver el `consume.py` de cada ejercicio).

## Paso 4 · Subir el codec (payloads legibles)
Para que el `object` del MQTT venga con `temperature`, `pressure`… adjunta el **codec JavaScript** al
**device profile** (afecta a todos sus devices). Cada ejercicio trae su `payload_decoder.js` y un
`scripts/upload_codec.sh`.

## US915 · Alinear la sub-banda (device profile ↔ nodo)

En **US915** hay 8 sub-bandas de 8 canales. El **nodo** se fija a una (con `LMIC_selectSubBand(N)` en
el TTGO, o el plan de canales del LR1110) y el **device profile de ChirpStack** debe usar **la misma**,
o el join no cuaja. Estos ejercicios usan la **sub-banda índice 1** (canales **8–15**).

| Region config (ChirpStack) | Canales (`enabled_uplink_channels`) | Nodo (LMIC) |
|---|---|---|
| `us915_0` | `0–7, 64` | `LMIC_selectSubBand(0)` |
| **`us915_1`** | **`8–15, 65`** | **`LMIC_selectSubBand(1)`** ← estos ejercicios |

**Cómo cambiarlo** (el `chirpstack.toml` del ejercicio 00 ya trae **las dos** en `enabled_regions`, así
que **no hay que editar ningún `.toml`**):

1. **Device profiles → (tu perfil US915) → editar.**
2. Campo **"Region configuration ID"** → ponlo en **`us915_1`** (no `us915_0`). Guardar.
3. **Re-join** el nodo (RESET / borra la sesión del device) para que tome el nuevo plan de canales.

> Si en algún momento **sí** editas `chirpstack.toml`, reinicia el contenedor:
> `cd specs/exercises/00_chirpstack-docker && docker compose restart chirpstack`.
>
> **Numeración:** uso el índice **0-based** (igual que el sufijo `us915_N` de ChirpStack y que
> `LMIC_selectSubBand(N)`); en notación FCC "1-based" eso es la **sub-banda 2**. En **EU868 no aplica**
> (son 3 canales fijos: 868.1/868.3/868.5).

---

## Errores típicos (y su causa)
| Síntoma en logs | Causa | Arreglo |
|-----------------|-------|---------|
| `Unknown device` | el DevEUI no está registrado (o mal escrito) | registra el DevEUI correcto (MSB) |
| no responde / `invalid MIC` | AppKey mal o en campo equivocado | en 1.0.x va en **`nwkKey`** |
| `DevNonce has already been used` | nonce repetido tras reflashear | borra y recrea el device |
| join no llega al server (US915) | **sub-banda del nodo ≠ device profile** | nodo `LMIC_selectSubBand(1)` **y** device profile en `us915_1` (ver arriba) |

> El REST **no** guarda histórico de payloads: los datos llegan por **MQTT** (o una integración como
> InfluxDB). Referencia completa en `specs/exercises/COMMON_CHIRPSTACK_API.md` del repo.

> Volver: **[🏠 Inicio](Home)** · **[Cómo usar el proyecto](Cómo-usar-el-proyecto)**
