#!/usr/bin/env bash
# Provisiona (idempotente) el TTGO en ChirpStack: application + device profile + device + keys.
#   export TOKEN="tu_api_key";  ./provision.sh
set -euo pipefail
: "${TOKEN:?Exporta TOKEN con tu API key de ChirpStack}"
API="${API:-http://localhost:8090}"
TENANT="${TENANT:-f8a271ec-591f-4e4c-956a-47d5d9ce9f87}"
AUTH=(-H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json")

DEVEUI=02389205358e71db          # DevEUI del sketch en MSB (array LMIC reversado)
JOINEUI=505246f87143fd8a
APPKEY=8ac583dfeec76c81ffd19ccfe76b73bf
APP_NAME="TTGO-LoRa32"; DP_NAME="TTGO-US915"

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
  \"regParamsRevision\":\"A\",\"adrAlgorithmId\":\"default\",\"supportsOtaa\":true,\"uplinkInterval\":3600}}" | jid)
echo "   DP=$DP"

echo "== Device $DEVEUI =="
if [ "$(curl -s -o /dev/null -w '%{http_code}' "$API/api/devices/$DEVEUI" "${AUTH[@]}")" = "200" ]; then
  echo "   ya existe"
else
  curl -s -X POST "$API/api/devices" "${AUTH[@]}" -d "{\"device\":{\"devEui\":\"$DEVEUI\",
    \"name\":\"ttgo-lora32-us915\",\"applicationId\":\"$APP\",\"deviceProfileId\":\"$DP\",\"joinEui\":\"$JOINEUI\"}}" >/dev/null
  echo "   creado"
fi
KEYS="{\"deviceKeys\":{\"devEui\":\"$DEVEUI\",\"nwkKey\":\"$APPKEY\"}}"
curl -s -X POST "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
curl -s -X PUT  "$API/api/devices/$DEVEUI/keys" "${AUTH[@]}" -d "$KEYS" >/dev/null || true
echo "   AppKey fijada en nwkKey"

echo "== LISTO. Sube el codec para datos decodificados:  ./scripts/upload_codec.sh $DP =="
