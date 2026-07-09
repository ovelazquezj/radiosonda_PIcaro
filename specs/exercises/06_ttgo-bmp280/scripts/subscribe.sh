#!/usr/bin/env bash
# Suscripción a los uplinks del TTGO+BMP280 por MQTT (sin dependencias). Ctrl-C para salir.
#   ./scripts/subscribe.sh            (el device del ejercicio)
#   ./scripts/subscribe.sh +          (todos los devices de la app)
APP="${APP:-8b11760e-6cfb-48e1-9ec2-f76da193e060}"
DEV="${1:-aabbccdd60915001}"
TOPIC="application/$APP/device/$DEV/event/up"
echo "Suscrito a $TOPIC  (Ctrl-C para salir)"
docker exec chirpstack-docker-mosquitto-1 mosquitto_sub -h localhost -t "$TOPIC" -v
