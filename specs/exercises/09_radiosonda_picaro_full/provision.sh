#!/usr/bin/env bash
# ============================================================================
#  provision.sh  -  Da de alta la Radiosonda PICARO FULL en ChirpStack (REST).
#  Alternativa por API a hacerlo en la web (paso 6 del README). Idempotente.
#  Crea/reusa el device profile, el device, sus keys y sube el codec.
#
#  Uso:
#     export TOKEN="tu_api_key"            # web :8080 -> Tenant -> API keys
#     export APP="<tu_application_id>"     # (o se autodetecta el primero)
#     ./provision.sh
# ============================================================================
set -euo pipefail

: "${TOKEN:?Exporta TOKEN con tu API key de ChirpStack}"
API="${API:-http://localhost:8090}"
DEVEUI="70b3d57ed0090901"
JOINEUI="0000000000000000"
APPKEY="50494341524f3039a5a5a5a5a5a5a5a5"
DP_NAME="Radiosonda-PICARO-Full-US915"
DECODER="$(dirname "$0")/chirpstack/decoder.js"

AUTH=(-H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json")
jget() { python3 -c "import sys,json;d=json.load(sys.stdin);print($1)" 2>/dev/null || true; }

# --- Tenant (usa $TENANT o el primero) ---
TENANT="${TENANT:-}"
[ -z "$TENANT" ] && TENANT=$(curl -s "$API/api/tenants?limit=1" "${AUTH[@]}" | jget "next((t['id'] for t in d.get('result',[])),'')")
[ -n "$TENANT" ] || { echo "ERROR: sin tenant (¿levantaste ChirpStack y el TOKEN es valido?)"; exit 1; }
echo "== tenant: $TENANT =="

# --- Application (usa $APP o el primero del tenant) ---
APP="${APP:-}"
[ -z "$APP" ] && APP=$(curl -s "$API/api/applications?limit=100&tenantId=$TENANT" "${AUTH[@]}" | jget "next((a['id'] for a in d.get('result',[])),'')")
[ -n "$APP" ] || { echo "ERROR: sin application"; exit 1; }
echo "== application: $APP =="

# --- Device profile ---
DP=$(curl -s "$API/api/device-profiles?limit=100&tenantId=$TENANT" "${AUTH[@]}" \
     | jget "next((r['id'] for r in d.get('result',[]) if r['name']=='$DP_NAME'),'')")
if [ -z "$DP" ]; then
  echo "  creando device profile $DP_NAME..."
  DP=$(curl -s -X POST "$API/api/device-profiles" "${AUTH[@]}" -d "{\"deviceProfile\":{
        \"name\":\"$DP_NAME\",\"tenantId\":\"$TENANT\",\"region\":\"US915\",
        \"macVersion\":\"LORAWAN_1_0_4\",\"regParamsRevision\":\"RP002_1_0_3\",
        \"adrAlgorithmId\":\"default\",\"supportsOtaa\":true,\"uplinkInterval\":30}}" | jget "d.get('id','')")
fi
echo "== device profile: $DP =="

# --- Device ---
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$API/api/devices/$DEVEUI" "${AUTH[@]}")
if [ "$CODE" != "200" ]; then
  curl -s -X POST "$API/api/devices" "${AUTH[@]}" -d "{\"device\":{
        \"devEui\":\"$DEVEUI\",\"name\":\"radiosonda-picaro-full\",
        \"applicationId\":\"$APP\",\"deviceProfileId\":\"$DP\",\"joinEui\":\"$JOINEUI\"}}" >/dev/null
  echo "  device creado."
else echo "  device ya existe (ok)."; fi

# --- Keys (AppKey en nwkKey para 1.0.x) ---
KEYS="{\"deviceKeys\":{\"devEui\":\"$DEVEUI\",\"nwkKey\":\"$APPKEY\"}}"
curl -s -X POST "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
curl -s -X PUT  "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
echo "  AppKey fijada."

# --- Codec en el device profile ---
if [ -f "$DECODER" ]; then
  API="$API" TOKEN="$TOKEN" DP="$DP" DECODER="$DECODER" python3 - <<'PY'
import os, json, urllib.request
api, token, dp, path = os.environ["API"], os.environ["TOKEN"], os.environ["DP"], os.environ["DECODER"]
hdr = {"Authorization": "Bearer " + token, "Content-Type": "application/json"}
obj = json.load(urllib.request.urlopen(urllib.request.Request(f"{api}/api/device-profiles/{dp}", headers=hdr)))
obj["deviceProfile"]["payloadCodecRuntime"] = "JS"
obj["deviceProfile"]["payloadCodecScript"] = open(path, encoding="utf-8").read()
body = json.dumps({"deviceProfile": obj["deviceProfile"]}).encode()
urllib.request.urlopen(urllib.request.Request(f"{api}/api/device-profiles/{dp}", data=body, headers=hdr, method="PUT"))
print("  codec subido.")
PY
fi

echo "== LISTO: $DEVEUI provisionado (US915, app $APP, profile $DP). Exporta APP=$APP para consumir. =="
