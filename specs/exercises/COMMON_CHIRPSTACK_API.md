# COMMON — ChirpStack: API REST, MQTT y codec

Referencia compartida por los ejercicios que **hacen join** (01, 02, 03). Cubre: autenticación,
IDs de tu stack, provisión por REST, consumo por **MQTT**, y subida del **codec** al device profile.

## Entorno (stack local en Docker)

| Recurso | Valor |
|---------|-------|
| REST API | `http://localhost:8090` — header `Authorization: Bearer $TOKEN` |
| Broker MQTT | `localhost:1883` (contenedor `chirpstack-docker-mosquitto-1`, sin auth en lab) |
| Tenant `Hotizonte_I` | `f8a271ec-591f-4e4c-956a-47d5d9ce9f87` |
| Application `Demos-LR1110` | `5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7` |
| Device profile **US915** | `a177f6fe-a123-4684-8f9d-10084ac86af7` |
| Device profile **EU868** | *(créalo tú — ver `02`/`REGISTRO_API`)* |

Exporta una vez por sesión (usa tu propio API key de la web → Tenant → API keys):
```bash
export TOKEN="PEGA_TU_API_KEY"
export API="http://localhost:8090"
export TENANT="f8a271ec-591f-4e4c-956a-47d5d9ce9f87"
export APP="5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7"
```

## Provisión por REST (resumen)

Detalle completo y errores típicos: [`../demos/REGISTRO_API_CHIRPSTACK.md`](../demos/REGISTRO_API_CHIRPSTACK.md).

```bash
# Crear device
curl -s -X POST "$API/api/devices" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d '{"device":{"devEui":"<DEVEUI>","name":"<nombre>","applicationId":"'"$APP"'","deviceProfileId":"<DP>","joinEui":"<JOINEUI>"}}'

# Poner AppKey — OJO: en LoRaWAN 1.0.x va en el campo nwkKey (no appKey)
curl -s -X POST "$API/api/devices/<DEVEUI>/keys" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d '{"deviceKeys":{"devEui":"<DEVEUI>","nwkKey":"<APPKEY>"}}'
```

## Leer estado y métricas por REST (para dashboards de estado)

```bash
# Estado del device (lastSeenAt, deviceStatus, etc.)
curl -s "$API/api/devices/<DEVEUI>" -H "Authorization: Bearer $TOKEN"

# Métricas de enlace (RSSI, SNR, nº de paquetes) en una ventana de tiempo
curl -s "$API/api/devices/<DEVEUI>/link-metrics?start=<RFC3339>&end=<RFC3339>&aggregation=HOUR" \
  -H "Authorization: Bearer $TOKEN"
```

> El REST **no** guarda un histórico de payloads. Los **payloads llegan por MQTT** (abajo).

## Consumir payloads por MQTT (la fuente del dashboard)

ChirpStack publica cada uplink en:
```
application/<APP_ID>/device/<DEVEUI>/event/up
```
El mensaje es JSON e incluye `data` (payload crudo en base64) y, **si hay codec**, `object` (los
campos ya decodificados). Suscríbete a todos los uplinks de la app:

```bash
# CLI (dentro del contenedor mosquitto, que siempre está disponible):
docker exec chirpstack-docker-mosquitto-1 \
  mosquitto_sub -h localhost -t "application/$APP/device/+/event/up" -v

# o desde el host si tienes mosquitto-clients:  mosquitto_sub -h localhost -p 1883 -t "application/$APP/#" -v
```
En Python se usa `paho-mqtt` (ver el `consume.py` de cada ejercicio).

## Subir el codec al device profile (payloads decodificados)

Para que el `object` del MQTT venga con campos legibles (p.ej. `temperature`, `pressure`), se
adjunta el **codec JavaScript** al **device profile** (afecta a todos sus devices):

```bash
# El codec debe exponer decodeUplink(input) (formato ChirpStack/TTN).
SCRIPT=$(python3 -c "import json,sys;print(json.dumps(open(sys.argv[1]).read()))" payload_decoder.js)
curl -s "$API/api/device-profiles/<DP_ID>" -H "Authorization: Bearer $TOKEN" > /tmp/dp.json
# ...editar dp.json: payloadCodecRuntime="JS", payloadCodecScript=<contenido>, y hacer PUT:
curl -s -X PUT "$API/api/device-profiles/<DP_ID>" -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" -d @/tmp/dp.json
```
> Cada ejercicio con payload propio (p.ej. `02` BMP280) trae su `scripts/upload_codec.sh` que hace
> esto automáticamente con su `payload_decoder.js`.

## Errores típicos (resumen)

| Síntoma | Causa | Arreglo |
|---------|-------|---------|
| Log `Unknown device` | device no registrado / DevEUI mal | crear device (MSB, sin invertir) |
| No responde el join / `invalid MIC` | AppKey mal o puesta en `appKey` en vez de `nwkKey` | usar `nwkKey` en 1.0.x |
| `DevNonce has already been used` | nonce repetido tras reflashear | borrar y recrear el device |
| REST `code:16` | token ausente/incorrecto | revisar `Authorization: Bearer` |
| `object` vacío en MQTT | falta el codec en el device profile | subir el codec (§Codec) |
