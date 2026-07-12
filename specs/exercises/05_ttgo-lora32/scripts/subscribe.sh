#!/usr/bin/env bash
# Suscripción a los uplinks del TTGO por MQTT usando el mosquitto_sub que ya vive dentro
# del contenedor de ChirpStack (sin dependencias de Python). Ctrl-C para salir.
#   ./scripts/subscribe.sh            (solo el device TTGO)
#   ./scripts/subscribe.sh +          (todos los devices de la app)
set -euo pipefail
APP="${APP:-d19cf2cb-5a1b-452b-8cfe-638784e5fb80}"
DEV="${1:-02389205358e71db}"
TOPIC="application/$APP/device/$DEV/event/up"

# Detecta el contenedor mosquitto sea cual sea el nombre de proyecto de Docker Compose
# (por defecto es 00_chirpstack-docker-mosquitto-1, según la carpeta del ejercicio 00).
MOSQ="$(docker ps --filter name=mosquitto --format '{{.Names}}' | head -n1)"
[ -n "$MOSQ" ] || { echo "ERROR: no encuentro el contenedor mosquitto. ¿Levantaste ChirpStack (ejercicio 00)?" >&2; exit 1; }

echo "Suscrito a $TOPIC  vía $MOSQ  (Ctrl-C para salir)"
docker exec "$MOSQ" mosquitto_sub -h localhost -t "$TOPIC" -v
