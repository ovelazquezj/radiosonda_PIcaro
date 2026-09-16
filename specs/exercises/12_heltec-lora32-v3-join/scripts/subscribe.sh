#!/usr/bin/env bash
# ============================================================================
#  subscribe.sh — escucha los uplinks del device por MQTT (servidor remoto)
# ----------------------------------------------------------------------------
#  El LNS del curso publica cada uplink ya decodificado en:
#      application/<APP_ID>/device/<DEV_EUI>/event/up
#  El broker es mqtt.pi-caro.org:8883 con TLS (certificado de Let's Encrypt)
#  y usuario/contrasena de tu equipo (ficha entregada por el instructor).
#
#  Uso (desde specs/exercises/12_heltec-lora32-v3-join/):
#      export MQTT_USER=equipoNN MQTT_PASS='...' APP_ID='<uuid>' DEV_EUI='<16 hex>'
#      ./scripts/subscribe.sh
#
#  Requiere mosquitto_sub (paquete mosquitto-clients) y, si quieres ver solo
#  el objeto decodificado, jq.
# ============================================================================
set -euo pipefail

: "${MQTT_USER:?Exporta MQTT_USER (usuario MQTT de tu equipo)}"
: "${MQTT_PASS:?Exporta MQTT_PASS (contrasena MQTT de tu equipo)}"
: "${APP_ID:?Exporta APP_ID (UUID de la aplicacion; esta en la URL de la consola)}"
: "${DEV_EUI:?Exporta DEV_EUI (16 hex, minusculas, tal como lo muestra ChirpStack)}"

HOST="${MQTT_HOST:-mqtt.pi-caro.org}"
PORT="${MQTT_PORT:-8883}"
DEV_EUI="$(echo "$DEV_EUI" | tr 'A-Z' 'a-z')"
TOPIC="application/${APP_ID}/device/${DEV_EUI}/event/up"

# Almacen de CAs del sistema (Let's Encrypt esta en todas las distros).
CAOPT=(--capath /etc/ssl/certs)
[[ -f /etc/ssl/certs/ca-certificates.crt ]] && CAOPT=(--cafile /etc/ssl/certs/ca-certificates.crt)

echo "== Escuchando ${TOPIC} en ${HOST}:${PORT} (Ctrl+C para salir) =="

if command -v jq >/dev/null 2>&1; then
  # Una linea por uplink: hora, fCnt, RSSI/SNR del mejor gateway y el objeto decodificado.
  mosquitto_sub -h "$HOST" -p "$PORT" "${CAOPT[@]}" -u "$MQTT_USER" -P "$MQTT_PASS" -t "$TOPIC" \
    | jq -c '{time: .time, fCnt: .fCnt, fPort: .fPort, dr: .dr,
              rssi: (.rxInfo[0].rssi), snr: (.rxInfo[0].snr), gw: (.rxInfo[0].gatewayId),
              data: .data, object: .object}'
else
  mosquitto_sub -h "$HOST" -p "$PORT" "${CAOPT[@]}" -u "$MQTT_USER" -P "$MQTT_PASS" -t "$TOPIC" -v
fi
