#!/usr/bin/env bash
# Provisiona (idempotente) el TTGO+BMP280 en ChirpStack: app + device profile + device + keys.
#   export TOKEN="tu_api_key";  ./provision.sh
set -euo pipefail
: "${TOKEN:?Exporta TOKEN con tu API key de ChirpStack}"
API="${API:-http://localhost:8090}"
TENANT="${TENANT:-f8a271ec-591f-4e4c-956a-47d5d9ce9f87}"
AUTH=(-H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json")

# Credenciales en MSB (las que ve ChirpStack). En el sketch LMIC van en LSB (DevEUI/AppEUI).
DEVEUI=aabbccdd60915001
JOINEUI=aabbccddeeff0000
APPKEY=60156015601560156015601560156015
APP_NAME="TTGO-BMP280"; DP_NAME="TTGO-BMP280-US915"

jid(){ python3 -c "import sys,json;print(json.load(sys.stdin).get('id',''))" 2>/dev/null; }
byname(){ curl -s "$1" "${AUTH[@]}" | python3 -c \
  "import sys,json;print(next((r['id'] for r in json.load(sys.stdin).get('result',[]) if r['name']==sys.argv[1]),''))" "$2" 2>/dev/null; }

echo "== Application $APP_NAME =="
APP=$(byname "$API/api/applications?limit=100&tenantId=$TENANT" "$APP_NAME")
[ -n "$APP" ] || APP=$(curl -s -X POST "$API/api/applications" "${AUTH[@]}" \
  -d "{\"application\":{\"name\":\"$APP_NAME\",\"tenantId\":\"$TENANT\"}}" | jid)
echo "   APP=$APP"

echo "== Device profile $DP_NAME (US915, 1.0.3, OTAA) =="
DP=$(byname "$API/api/device-profiles?limit=100&tenantId=$TENANT" "$DP_NAME")
[ -n "$DP" ] || DP=$(curl -s -X POST "$API/api/device-profiles" "${AUTH[@]}" -d "{\"deviceProfile\":{
  \"name\":\"$DP_NAME\",\"tenantId\":\"$TENANT\",\"region\":\"US915\",\"macVersion\":\"LORAWAN_1_0_3\",
  \"regParamsRevision\":\"A\",\"adrAlgorithmId\":\"default\",\"supportsOtaa\":true,\"uplinkInterval\":60}}" | jid)
echo "   DP=$DP"

echo "== Device $DEVEUI =="
if [ "$(curl -s -o /dev/null -w '%{http_code}' "$API/api/devices/$DEVEUI" "${AUTH[@]}")" = "200" ]; then
  echo "   ya existe"
else
  curl -s -X POST "$API/api/devices" "${AUTH[@]}" -d "{\"device\":{\"devEui\":\"$DEVEUI\",
    \"name\":\"ttgo-bmp280-us915\",\"applicationId\":\"$APP\",\"deviceProfileId\":\"$DP\",\"joinEui\":\"$JOINEUI\"}}" >/dev/null
  echo "   creado"
fi
KEYS="{\"deviceKeys\":{\"devEui\":\"$DEVEUI\",\"nwkKey\":\"$APPKEY\"}}"
curl -s -X POST "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
curl -s -X PUT  "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
echo "   AppKey fijada en nwkKey"
echo "== LISTO. Sube el codec:  ./scripts/upload_codec.sh $DP =="
