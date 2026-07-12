#!/usr/bin/env bash
# Suscripción a los uplinks del TTGO+BMP280 por MQTT (sin dependencias de Python), usando el
# mosquitto_sub que ya vive dentro del contenedor de ChirpStack. Ctrl-C para salir.
#   ./scripts/subscribe.sh            (el device del ejercicio)
#   ./scripts/subscribe.sh +          (todos los devices de la app)
set -euo pipefail
APP="${APP:-8b11760e-6cfb-48e1-9ec2-f76da193e060}"
DEV="${1:-aabbccdd60915001}"
TOPIC="application/$APP/device/$DEV/event/up"

# Detecta el contenedor mosquitto sea cual sea el nombre de proyecto de Docker Compose
# (por defecto es 00_chirpstack-docker-mosquitto-1, según la carpeta del ejercicio 00).
MOSQ="$(docker ps --filter name=mosquitto --format '{{.Names}}' | head -n1)"
[ -n "$MOSQ" ] || { echo "ERROR: no encuentro el contenedor mosquitto. ¿Levantaste ChirpStack (ejercicio 00)?" >&2; exit 1; }

echo "Suscrito a $TOPIC  vía $MOSQ  (Ctrl-C para salir)"
docker exec "$MOSQ" mosquitto_sub -h localhost -t "$TOPIC" -v
