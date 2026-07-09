#!/usr/bin/env bash
# Suscripción a los uplinks de la app Demos-LR1110 SIN dependencias, usando el mosquitto_sub
# que ya vive dentro del contenedor de ChirpStack. Ctrl-C para salir.
#   Uso:  ./scripts/subscribe.sh                    (tracker US915 por defecto)
#         ./scripts/subscribe.sh aabbccdd00868001   (tracker EU868)
#         ./scripts/subscribe.sh +                  (todos los devices de la app)
APP="${APP:-5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7}"    # Demos-LR1110
DEV="${1:-aabbccdd00915001}"
TOPIC="application/$APP/device/$DEV/event/up"
echo "Suscrito a $TOPIC  (Ctrl-C para salir)"
docker exec chirpstack-docker-mosquitto-1 \
  mosquitto_sub -h localhost -t "$TOPIC" -v
