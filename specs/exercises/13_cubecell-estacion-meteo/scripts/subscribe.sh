#!/usr/bin/env bash
# ============================================================================
#  subscribe.sh — escucha los uplinks de la estacion por MQTT (servidor remoto)
# ----------------------------------------------------------------------------
#  El LNS del curso publica cada uplink ya decodificado en:
#      application/<APP_ID>/device/<DEV_EUI>/event/up
#  Broker mqtt.pi-caro.org:8883 con TLS (Let's Encrypt) y usuario/contrasena
#  MQTT de tu equipo (ficha del instructor; NO son los de la web).
#
#  Uso (desde specs/exercises/13_cubecell-estacion-meteo/):
#      export MQTT_USER=equipoNN MQTT_PASS='...' APP_ID='<uuid>'
#      ./scripts/subscribe.sh
#
#  Requiere mosquitto_sub (paquete mosquitto-clients) y, opcionalmente, jq.
# ============================================================================
set -euo pipefail

: "${MQTT_USER:?Exporta MQTT_USER (usuario MQTT de tu equipo)}"
: "${MQTT_PASS:?Exporta MQTT_PASS (contrasena MQTT de tu equipo)}"
: "${APP_ID:?Exporta APP_ID (UUID de la aplicacion; esta en la URL de la consola)}"
DEV_EUI="${DEV_EUI:-623851f518c92135}"

HOST="${MQTT_HOST:-mqtt.pi-caro.org}"
PORT="${MQTT_PORT:-8883}"
DEV_EUI="$(echo "$DEV_EUI" | tr 'A-Z' 'a-z')"
TOPIC="application/${APP_ID}/device/${DEV_EUI}/event/up"

CAOPT=(--capath /etc/ssl/certs)
[[ -f /etc/ssl/certs/ca-certificates.crt ]] && CAOPT=(--cafile /etc/ssl/certs/ca-certificates.crt)

echo "== Escuchando ${TOPIC} en ${HOST}:${PORT} (Ctrl+C para salir) =="

if command -v jq >/dev/null 2>&1; then
  mosquitto_sub -h "$HOST" -p "$PORT" "${CAOPT[@]}" -u "$MQTT_USER" -P "$MQTT_PASS" -t "$TOPIC" \
    | jq -c '{time: .time, fCnt: .fCnt, fPort: .fPort, dr: .dr,
              rssi: (.rxInfo[0].rssi), snr: (.rxInfo[0].snr), gw: (.rxInfo[0].gatewayId),
              data: .data, object: .object}'
else
  mosquitto_sub -h "$HOST" -p "$PORT" "${CAOPT[@]}" -u "$MQTT_USER" -P "$MQTT_PASS" -t "$TOPIC" -v
fi
