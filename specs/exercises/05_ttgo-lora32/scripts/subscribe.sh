#!/usr/bin/env bash
# Suscripción a los uplinks del TTGO por MQTT (sin dependencias). Ctrl-C para salir.
#   ./scripts/subscribe.sh            (el device TTGO)
#   ./scripts/subscribe.sh +          (todos los devices de la app)
APP="${APP:-d19cf2cb-5a1b-452b-8cfe-638784e5fb80}"
DEV="${1:-02389205358e71db}"
TOPIC="application/$APP/device/$DEV/event/up"
echo "Suscrito a $TOPIC  (Ctrl-C para salir)"
docker exec chirpstack-docker-mosquitto-1 mosquitto_sub -h localhost -t "$TOPIC" -v
