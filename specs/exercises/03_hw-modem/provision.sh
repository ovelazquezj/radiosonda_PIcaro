#!/usr/bin/env bash
# Provisiona en ChirpStack el DevEUI que el HOST cargará en el hw-modem (idempotente).
# Como en este ejemplo las credenciales las fija el host en runtime, el script es PARAMETRIZABLE:
# acepta DEVEUI / JOINEUI / APPKEY por variable de entorno o como argumentos posicionales.
#
#   export TOKEN="tu_api_key"
#   DEVEUI=aabbccdd10930001 JOINEUI=aabbccddeeff0000 APPKEY=10301030103010301030103010301030 ./provision.sh
#   # ...o:
#   ./provision.sh <DEVEUI> <JOINEUI> <APPKEY>
set -euo pipefail

: "${TOKEN:?Exporta TOKEN con tu API key de ChirpStack (Tenant -> API keys)}"
API="${API:-http://localhost:8090}"
TENANT="${TENANT:-f8a271ec-591f-4e4c-956a-47d5d9ce9f87}"
APP="${APP:-5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7}"          # app Demos-LR1110
DP="${DP:-a177f6fe-a123-4684-8f9d-10084ac86af7}"            # device profile US915 existente

# Credenciales: argumentos posicionales tienen prioridad; si no, variables de entorno.
DEVEUI="${1:-${DEVEUI:-}}"
JOINEUI="${2:-${JOINEUI:-aabbccddeeff0000}}"
APPKEY="${3:-${APPKEY:-}}"

: "${DEVEUI:?Falta DEVEUI (el que cargue el host con CMD_SET_DEV_EUI) -- pásalo por env o como 1er arg}"
: "${APPKEY:?Falta APPKEY (la que cargue el host con CMD_SET_NWKKEY) -- pásalo por env o como 3er arg}"

AUTH=(-H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json")

echo "== hw-modem: device $DEVEUI (app Demos-LR1110, profile US915 $DP) =="
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$API/api/devices/$DEVEUI" "${AUTH[@]}")
if [ "$CODE" = "200" ]; then
  echo "  ya existe (ok, idempotente)"
else
  echo "  creando device..."
  curl -s -X POST "$API/api/devices" "${AUTH[@]}" -d "{\"device\":{
        \"devEui\":\"$DEVEUI\",\"name\":\"hw-modem-$DEVEUI\",
        \"applicationId\":\"$APP\",\"deviceProfileId\":\"$DP\",\"joinEui\":\"$JOINEUI\"}}" >/dev/null
  echo "  creado."
fi

echo "== AppKey (nwkKey en 1.0.x) =="
# Idempotente: POST crea si no existían; PUT fija el valor si ya existían. Estado final = APPKEY.
KEYS="{\"deviceKeys\":{\"devEui\":\"$DEVEUI\",\"nwkKey\":\"$APPKEY\"}}"
curl -s -X POST "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
curl -s -X PUT  "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
echo "  AppKey fijada en nwkKey."

echo "== LISTO: $DEVEUI provisionado (JoinEUI $JOINEUI) =="
echo "   Ahora el host debe cargar EXACTAMENTE estas credenciales en el módem:"
echo "     CMD_SET_DEV_EUI=$DEVEUI  CMD_SET_JOIN_EUI=$JOINEUI  CMD_SET_NWKKEY=$APPKEY"
echo "   ...luego CMD_SET_REGION (US915) -> CMD_JOIN_NETWORK -> esperar JOINED -> CMD_REQUEST_UPLINK."
