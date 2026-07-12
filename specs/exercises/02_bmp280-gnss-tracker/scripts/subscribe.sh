#!/usr/bin/env bash
# Suscripción a los uplinks de la app SIN dependencias de Python, usando el mosquitto_sub
# que ya vive dentro del contenedor de ChirpStack. Ctrl-C para salir.
#   Uso:  export APP=<tu-app-id>;  ./scripts/subscribe.sh                    (tracker US915 por defecto)
#         export APP=<tu-app-id>;  ./scripts/subscribe.sh aabbccdd00868001   (tracker EU868)
#         export APP=<tu-app-id>;  ./scripts/subscribe.sh +                  (todos los devices de la app)
set -euo pipefail
APP="${APP:?Exporta APP con el ID de tu aplicación (lo imprime ./provision.sh, o la web :8080)}"
DEV="${1:-aabbccdd00915001}"
TOPIC="application/$APP/device/$DEV/event/up"

# Detecta el contenedor mosquitto sea cual sea el nombre de proyecto de Docker Compose
# (por defecto es 00_chirpstack-docker-mosquitto-1, según la carpeta del ejercicio 00).
MOSQ="$(docker ps --filter name=mosquitto --format '{{.Names}}' | head -n1)"
[ -n "$MOSQ" ] || { echo "ERROR: no encuentro el contenedor mosquitto. ¿Levantaste ChirpStack (ejercicio 00)?" >&2; exit 1; }

echo "Suscrito a $TOPIC  vía $MOSQ  (Ctrl-C para salir)"
docker exec "$MOSQ" mosquitto_sub -h localhost -t "$TOPIC" -v
