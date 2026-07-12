#!/usr/bin/env bash
# =============================================================================
# register_gateway.sh — Alta de un gateway en ChirpStack v4 por API REST
# -----------------------------------------------------------------------------
# Aportación propia de radiosonda_PIcaro (fork educativo de Semtech SWL2001).
# Distribuido bajo la misma Clear BSD License que el resto del proyecto.
# NO forma parte del firmware de terceros ESP-1ch-Gateway (MIT, things4u).
# =============================================================================
#
# Registra (de forma idempotente) el gateway single-channel ESP-1ch-Gateway en
# ChirpStack, para que el Gateway Bridge (backend Semtech UDP, 1700/UDP) acepte
# sus paquetes. El Gateway EUI lo GENERA el firmware a partir de la MAC del ESP:
# léelo en el monitor serie (115200) o en la web del gateway, y pásalo aquí.
#
# Uso:
#   export TOKEN="tu_api_key"          # ChirpStack web -> Tenant -> API keys
#   ./register_gateway.sh <GATEWAY_EUI> [nombre]
#
# Ejemplo:
#   ./register_gateway.sh aabbccddeeff0011 "Gateway TTGO aula"
#
# Variables opcionales (con valores por defecto del stack local en Docker):
#   API=http://localhost:8090   TENANT=f8a271ec-591f-4e4c-956a-47d5d9ce9f87
# -----------------------------------------------------------------------------
set -euo pipefail

: "${TOKEN:?Exporta TOKEN con tu API key de ChirpStack (Tenant -> API keys)}"
API="${API:-http://localhost:8090}"
TENANT="${TENANT:-f8a271ec-591f-4e4c-956a-47d5d9ce9f87}"

GWID="${1:?Pasa el Gateway EUI (8 bytes hex, el que imprime el gateway por serie/web)}"
NAME="${2:-esp-1ch-gateway}"

# Normaliza el EUI: minúsculas y sin separadores (acepta 'AA:BB:..' o 'AABB..').
GWID="$(echo "$GWID" | tr 'A-F' 'a-f' | tr -d ' :-')"
if ! echo "$GWID" | grep -qE '^[0-9a-f]{16}$'; then
  echo "ERROR: el Gateway EUI debe ser 16 dígitos hex (8 bytes). Recibido: '$GWID'" >&2
  exit 1
fi

AUTH=(-H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json")

echo "== Gateway $GWID en tenant $TENANT =="
CODE=$(curl -s -o /dev/null -w "%{http_code}" "$API/api/gateways/$GWID" "${AUTH[@]}")
if [ "$CODE" = "200" ]; then
  echo "  ya existe (ok, idempotente)."
else
  echo "  creando gateway '$NAME'..."
  curl -s -X POST "$API/api/gateways" "${AUTH[@]}" -d "{\"gateway\":{
        \"gatewayId\":\"$GWID\",
        \"name\":\"$NAME\",
        \"description\":\"ESP-1ch-Gateway (single-channel) — radiosonda_PIcaro ej.07\",
        \"tenantId\":\"$TENANT\",
        \"statsInterval\":30}}" >/dev/null
  echo "  creado."
fi

echo "== LISTO =="
echo "   Gateway $GWID registrado en ChirpStack."
echo "   Comprueba en la web:  Gateways -> $GWID  (debe pasar a 'online' con tráfico)."
echo "   Si sigue offline: revisa que 1700/UDP esté abierto en el host y que el firmware"
echo "   apunte a este ChirpStack (ruta simple: _TTNSERVER = IP del host, _TTNPORT 1700)."
