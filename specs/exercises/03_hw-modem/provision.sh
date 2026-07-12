#!/usr/bin/env bash
# Provisiona en ChirpStack el DevEUI que el HOST cargará en el hw-modem (idempotente).
# Como en este ejemplo las credenciales las fija el host en runtime, el script es PARAMETRIZABLE:
# acepta DEVEUI / JOINEUI / APPKEY por variable de entorno o como argumentos posicionales.
# TENANT, APP y el device profile US915 se AUTODETECTAN/crean si no los exportas
# (funciona en una instalación desde cero).
#
#   export TOKEN="tu_api_key"
#   DEVEUI=aabbccdd10930001 JOINEUI=aabbccddeeff0000 APPKEY=10301030103010301030103010301030 ./provision.sh
#   # ...o:
#   ./provision.sh <DEVEUI> <JOINEUI> <APPKEY>
set -euo pipefail

: "${TOKEN:?Exporta TOKEN con tu API key de ChirpStack (web :8080 -> Tenant -> API keys)}"
API="${API:-http://localhost:8090}"
APP_NAME="${APP_NAME:-Demos-LR1110}"
DP_NAME="${DP_NAME:-PIcaro-LR1110-US915}"
REGION=US915

# Credenciales: argumentos posicionales tienen prioridad; si no, variables de entorno.
DEVEUI="${1:-${DEVEUI:-}}"
JOINEUI="${2:-${JOINEUI:-aabbccddeeff0000}}"
APPKEY="${3:-${APPKEY:-}}"

: "${DEVEUI:?Falta DEVEUI (el que cargue el host con CMD_SET_DEV_EUI) -- pásalo por env o como 1er arg}"
: "${APPKEY:?Falta APPKEY (la que cargue el host con CMD_SET_NWKKEY) -- pásalo por env o como 3er arg}"

AUTH=(-H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json")
jget() { python3 -c "import sys,json;d=json.load(sys.stdin);print($1)" 2>/dev/null || true; }

# --- Tenant: usa $TENANT si lo exportaste; si no, toma el primero de tu ChirpStack ---
TENANT="${TENANT:-}"
if [ -z "$TENANT" ]; then
  TENANT=$(curl -s "$API/api/tenants?limit=1" "${AUTH[@]}" | jget "next((t['id'] for t in d.get('result',[])),'')")
fi
[ -n "$TENANT" ] || { echo "ERROR: no encuentro ningún tenant. ¿Levantaste ChirpStack (ej.00) y el TOKEN es válido?"; exit 1; }
echo "== tenant: $TENANT =="

# --- Application: usa $APP si lo exportaste; si no, busca/crea "$APP_NAME" ---
APP="${APP:-}"
if [ -z "$APP" ]; then
  APP=$(curl -s "$API/api/applications?limit=100&tenantId=$TENANT" "${AUTH[@]}" \
        | jget "next((a['id'] for a in d.get('result',[]) if a['name']=='$APP_NAME'),'')")
  if [ -z "$APP" ]; then
    echo "  creando application $APP_NAME..."
    APP=$(curl -s -X POST "$API/api/applications" "${AUTH[@]}" \
          -d "{\"application\":{\"name\":\"$APP_NAME\",\"tenantId\":\"$TENANT\"}}" | jget "d.get('id','')")
  fi
fi
[ -n "$APP" ] || { echo "ERROR: sin application"; exit 1; }
echo "== application: $APP =="

# --- Device profile US915: usa $DP si lo exportaste; si no, busca/crea "$DP_NAME" ---
DP="${DP:-}"
if [ -z "$DP" ]; then
  DP=$(curl -s "$API/api/device-profiles?limit=100&tenantId=$TENANT" "${AUTH[@]}" \
        | jget "next((r['id'] for r in d.get('result',[]) if r['name']=='$DP_NAME'),'')")
  if [ -z "$DP" ]; then
    echo "  creando device profile $DP_NAME ($REGION)..."
    DP=$(curl -s -X POST "$API/api/device-profiles" "${AUTH[@]}" -d "{\"deviceProfile\":{
          \"name\":\"$DP_NAME\",\"tenantId\":\"$TENANT\",\"region\":\"$REGION\",
          \"macVersion\":\"LORAWAN_1_0_4\",\"regParamsRevision\":\"RP002_1_0_3\",
          \"adrAlgorithmId\":\"default\",\"supportsOtaa\":true,\"uplinkInterval\":3600}}" \
          | jget "d.get('id','')")
  fi
fi
[ -n "$DP" ] || { echo "ERROR: sin device profile"; exit 1; }
echo "== device profile US915: $DP =="

echo "== hw-modem: device $DEVEUI (app $APP_NAME, profile US915 $DP) =="
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
echo "   Exporta tu app para consumir:   export APP=$APP"
echo "   Ahora el host debe cargar EXACTAMENTE estas credenciales en el módem:"
echo "     CMD_SET_DEV_EUI=$DEVEUI  CMD_SET_JOIN_EUI=$JOINEUI  CMD_SET_NWKKEY=$APPKEY"
echo "   ...luego CMD_SET_REGION (US915) -> CMD_JOIN_NETWORK -> esperar JOINED -> CMD_REQUEST_UPLINK."
