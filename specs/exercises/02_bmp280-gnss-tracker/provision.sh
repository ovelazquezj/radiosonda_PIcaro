#!/usr/bin/env bash
# Provisiona en ChirpStack el tracker BMP280+GNSS (LR1110) de una banda (idempotente).
# Crea/reutiliza un device profile PROPIO por banda (Tracker-BMP280-<banda>) para que el
# codec BMP280 no se mezcle con otros payloads de la aplicación.
# TENANT y APP se AUTODETECTAN/crean si no los exportas (funciona en una instalación desde cero).
# Uso:   export TOKEN="tu_api_key";  ./provision.sh us915   |   ./provision.sh eu868
set -euo pipefail

: "${TOKEN:?Exporta TOKEN con tu API key de ChirpStack (web :8080 -> Tenant -> API keys)}"
API="${API:-http://localhost:8090}"
APP_NAME="${APP_NAME:-Demos-LR1110}"
BAND="${1:-us915}"
JOINEUI="aabbccddeeff0000"

case "$BAND" in
  us915) REGION=US915; DEVEUI=aabbccdd00915001; APPKEY=A915A915A915A915A915A915A915A915
         DP_NAME="Tracker-BMP280-US915"; BIN="app_lr1110_US_915.bin";;
  eu868) REGION=EU868; DEVEUI=aabbccdd00868001; APPKEY=A868A868A868A868A868A868A868A868
         DP_NAME="Tracker-BMP280-EU868"; BIN="app_lr1110_EU_868.bin";;
  *) echo "Banda inválida: $BAND (usa us915|eu868)"; exit 1;;
esac

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

echo "== [$BAND] device profile $DP_NAME =="
DP=$(curl -s "$API/api/device-profiles?limit=100&tenantId=$TENANT" "${AUTH[@]}" \
      | jget "next((r['id'] for r in d.get('result',[]) if r['name']=='$DP_NAME'),'')")
if [ -z "$DP" ]; then
  echo "  creando device profile $DP_NAME ($REGION)..."
  DP=$(curl -s -X POST "$API/api/device-profiles" "${AUTH[@]}" -d "{\"deviceProfile\":{
        \"name\":\"$DP_NAME\",\"tenantId\":\"$TENANT\",\"region\":\"$REGION\",
        \"macVersion\":\"LORAWAN_1_0_4\",\"regParamsRevision\":\"RP002_1_0_3\",
        \"adrAlgorithmId\":\"default\",\"supportsOtaa\":true,\"uplinkInterval\":3600}}" \
        | jget "d.get('id','')")
else
  echo "  ya existe (ok, idempotente)."
fi
echo "  device profile id: $DP"
[ -n "$DP" ] || { echo "  ERROR: sin device profile"; exit 1; }

echo "== [$BAND] device $DEVEUI =="
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$API/api/devices/$DEVEUI" "${AUTH[@]}")
if [ "$CODE" = "200" ]; then
  echo "  ya existe (ok, idempotente)"
else
  echo "  creando device..."
  curl -s -X POST "$API/api/devices" "${AUTH[@]}" -d "{\"device\":{
        \"devEui\":\"$DEVEUI\",\"name\":\"tracker-bmp280-$BAND\",
        \"applicationId\":\"$APP\",\"deviceProfileId\":\"$DP\",\"joinEui\":\"$JOINEUI\"}}" >/dev/null
  echo "  creado."
fi

echo "== [$BAND] AppKey (nwkKey en 1.0.x) =="
# Idempotente: POST crea si no existen; PUT fija el valor si ya existían. El estado final = APPKEY.
KEYS="{\"deviceKeys\":{\"devEui\":\"$DEVEUI\",\"nwkKey\":\"$APPKEY\"}}"
curl -s -X POST "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
curl -s -X PUT  "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
echo "  AppKey fijada en nwkKey."

echo "== LISTO: $DEVEUI provisionado en $REGION (app $APP, profile $DP) =="
echo "   Ahora adjunta el codec BMP280 a este device profile:"
echo "       ./scripts/upload_codec.sh $DP"
echo "   Y flashea artifacts/$BIN y resetea la placa (B2)."
echo "   Exporta tu app para consumir:   export APP=$APP"
