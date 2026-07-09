#!/usr/bin/env bash
# Adjunta payload_decoder.js al device profile del TTGO+BMP280 para que ChirpStack entregue
# el campo 'object' decodificado en el MQTT (base del dashboard).
#   export TOKEN="..."; ./scripts/upload_codec.sh <DEVICE_PROFILE_ID>
set -euo pipefail
: "${TOKEN:?Exporta TOKEN}"
API="${API:-http://localhost:8090}"
DP="${1:?Pasa el device-profile-id}"
DIR="$(cd "$(dirname "$0")/.." && pwd)"

python3 - "$API" "$TOKEN" "$DP" "$DIR/payload_decoder.js" <<'PY'
import sys, json, urllib.request
api, token, dp, codec_path = sys.argv[1:5]
def call(method, path, body=None):
    r = urllib.request.Request(api + path,
        data=(json.dumps(body).encode() if body is not None else None),
        headers={"Authorization": "Bearer " + token, "Content-Type": "application/json"},
        method=method)
    resp = urllib.request.urlopen(r)
    return json.load(resp) if method == "GET" else resp.read()
prof = call("GET", f"/api/device-profiles/{dp}")["deviceProfile"]
prof["payloadCodecRuntime"] = "JS"
prof["payloadCodecScript"]  = open(codec_path).read()
call("PUT", f"/api/device-profiles/{dp}", {"deviceProfile": prof})
print(f"Codec JS subido al device profile {dp}.")
PY
