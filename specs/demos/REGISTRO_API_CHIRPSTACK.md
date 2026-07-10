# Registrar un device en ChirpStack v4 por API REST — paso a paso

Guía para dar de alta un dispositivo LoRaWAN **desde la API REST de ChirpStack** (sin la web),
como ejercicio. El **ejemplo trabajado** es el device **EU868** de la demo Periodical Uplink;
los mismos pasos sirven para cualquier device.

> Contexto: es exactamente lo que se hizo para el device US915 (`aabbccdd10915001`).
> El fallo que resuelve es el clásico `Unknown device` en los logs → ChirpStack recibe el
> JoinRequest pero, al no existir el device, lo descarta sin responder → `JOINFAIL` en la placa.

---

## 0. Datos y entorno

| Dato | Valor |
|------|-------|
| API REST | `http://localhost:8090` |
| Header de auth | `Authorization: Bearer <API_KEY>` |
| Tenant (Horizonte_1) | `f8a271ec-591f-4e4c-956a-47d5d9ce9f87` |
| Application `Demos-LR1110` (reutilizable) | `5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7` |
| Device profile **US915** existente | `a177f6fe-a123-4684-8f9d-10084ac86af7` |
| Device profile **EU868** | ❌ no existe → lo creamos en el paso 3 |

**Credenciales del device EU868 (ejemplo de este ejercicio):**

| Campo | Valor |
|-------|-------|
| DevEUI | `aabbccdd10868001` |
| JoinEUI | `aabbccddeeff0000` |
| AppKey | `10681068106810681068106810681068` |

Exporta el token y la base en tu terminal (usa tu API key; sirve la de `claude-registro`):

```bash
export TOKEN="PEGA_AQUI_TU_API_KEY"
export API="http://localhost:8090"
export TENANT="f8a271ec-591f-4e4c-956a-47d5d9ce9f87"
```

---

## 1. Conseguir el API key

Web de ChirpStack → dentro de tu **Tenant** → **API keys** → **Add API key** → copia el token
(se muestra una sola vez). Es *tenant-scoped*: puede crear apps, device-profiles y devices
**dentro de ese tenant**.

> **Obtener el `tenant_id`:** aparece en la URL de la web al entrar al tenant
> (`.../tenants/<UUID>/...`). Si el key es tenant-scoped, el JWT **no** lo lleva dentro; también
> puedes sacarlo de la BD:
> ```bash
> docker exec chirpstack-docker-postgres-1 psql -U chirpstack -d chirpstack -tA \
>   -c "select tenant_id from api_key where id='<sub-del-JWT>';"
> ```

---

## 2. Explorar lo que ya existe (apps y device-profiles)

```bash
# Aplicaciones del tenant
curl -s "$API/api/applications?limit=100&tenantId=$TENANT" -H "Authorization: Bearer $TOKEN"

# Device profiles del tenant
curl -s "$API/api/device-profiles?limit=100&tenantId=$TENANT" -H "Authorization: Bearer $TOKEN"
```

- Si ya existe la app **Demos-LR1110**, reutiliza su `id` (arriba) y salta el paso "crear app".
- Verás que hay device profile **US915** pero **no EU868** → hay que crear el de EU868.

*(Crear una app nueva, si la necesitas:)*
```bash
curl -s -X POST "$API/api/applications" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d "{\"application\":{\"name\":\"Demos-LR1110\",\"tenantId\":\"$TENANT\"}}"
# → {"id":"<APP_ID>"}
```

---

## 3. Crear el device profile EU868 (OTAA, 1.0.x)

```bash
curl -s -X POST "$API/api/device-profiles" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d "{\"deviceProfile\":{
        \"name\":\"Demos-LR1110-EU868\",
        \"tenantId\":\"$TENANT\",
        \"region\":\"EU868\",
        \"macVersion\":\"LORAWAN_1_0_4\",
        \"regParamsRevision\":\"RP002_1_0_3\",
        \"adrAlgorithmId\":\"default\",
        \"supportsOtaa\":true,
        \"uplinkInterval\":3600,
        \"flushQueueOnActivate\":true
      }}"
# → {"id":"<DP_EU868_ID>"}    ← guarda este id
```

Guárdalo:
```bash
export DP="<DP_EU868_ID>"
export APP="5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7"   # o el APP_ID que creaste
```

> **Enums útiles**
> - `region`: `EU868`, `US915`, `AU915`, `AS923`, `IN865`, …
> - `macVersion`: `LORAWAN_1_0_2` / `LORAWAN_1_0_3` / `LORAWAN_1_0_4` / `LORAWAN_1_1_0`
> - `regParamsRevision`: `A`, `B`, `RP002_1_0_1` … `RP002_1_0_4`
> - `1.0.3` y `1.0.4` son compatibles para el join OTAA de LoRa Basics Modem.

---

## 4. Crear el device

```bash
curl -s -X POST "$API/api/devices" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d "{\"device\":{
        \"devEui\":\"aabbccdd10868001\",
        \"name\":\"periodical-uplink-eu868\",
        \"description\":\"Demo Periodical Uplink EU868\",
        \"applicationId\":\"$APP\",
        \"deviceProfileId\":\"$DP\",
        \"joinEui\":\"aabbccddeeff0000\",
        \"isDisabled\":false
      }}"
# → {}   (vacío = OK)
```

---

## 5. Poner la AppKey  ⚠️ (el detalle clave de 1.0.x)

En ChirpStack, para un device **LoRaWAN 1.0.x** la **Application Key va en el campo `nwkKey`**
(el `appKey` es solo para 1.1). Si la pones en el campo equivocado → **MIC inválido** y no hace join.

```bash
curl -s -X POST "$API/api/devices/aabbccdd10868001/keys" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d "{\"deviceKeys\":{
        \"devEui\":\"aabbccdd10868001\",
        \"nwkKey\":\"10681068106810681068106810681068\"
      }}"
# → {}   (vacío = OK)
```

---

## 6. Verificar el alta

```bash
# El device existe y apunta al profile/app correctos
curl -s "$API/api/devices/aabbccdd10868001" -H "Authorization: Bearer $TOKEN"

# La key quedó en nwkKey (appKey en ceros es lo esperado en 1.0.x)
curl -s "$API/api/devices/aabbccdd10868001/keys" -H "Authorization: Bearer $TOKEN"
```

Luego **resetea la placa (B2)** (o espera ~42 s a que reintente) y confirma el join desde los logs:

```bash
docker logs --since 3m chirpstack-docker-chirpstack-1 2>&1 | grep -iE "aabbccdd10868001|join|Unknown|mic"
```

- Antes del alta verías: `... join: Unknown device dev_eui=aabbccdd10868001`.
- Tras el alta y un join correcto: desaparece el `Unknown device`, el device muestra **last seen**
  y la placa imprime **`Joined`** en la traza UART.

---

## 7. Errores típicos y qué significan

| Síntoma | Causa | Arreglo |
|---------|-------|---------|
| Log `Unknown device dev_eui=…` | El device no está registrado (o DevEUI mal / en otro tenant/app) | Crear el device (paso 4) con el DevEUI **en MSB, sin invertir** |
| Log `invalid MIC` / no responde | AppKey mal, o puesta en `appKey` en vez de `nwkKey` | Repetir paso 5 con la clave en `nwkKey` |
| Log `DevNonce has already been used` | Nonce repetido tras reflashear | Borrar y recrear el device (limpia el historial de DevNonce) |
| HTTP/gRPC `code:16` (unauthenticated) | Token ausente/incorrecto | Revisar el header `Authorization: Bearer $TOKEN` |
| gRPC `code:7` (permission denied) | Operación fuera del alcance del key (p.ej. listar todos los tenants con un key de tenant) | Usar un key global admin, o pasar `tenantId` en la query |
| `invalid length: expected 32` | Falta el `tenantId` en la query | Exportar `TENANT` y añadirlo (`?tenantId=$TENANT`) |

---

## Apéndice — plantilla rápida (cualquier device nuevo)

```bash
export TOKEN="..."; export API="http://localhost:8090"
export TENANT="f8a271ec-591f-4e4c-956a-47d5d9ce9f87"
export APP="5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7"
export DP="<device-profile-id de la región correcta>"
DEVEUI="aabbccdd10868001"; JOINEUI="aabbccddeeff0000"; APPKEY="10681068106810681068106810681068"

curl -s -X POST "$API/api/devices" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d "{\"device\":{\"devEui\":\"$DEVEUI\",\"name\":\"$DEVEUI\",\"applicationId\":\"$APP\",\"deviceProfileId\":\"$DP\",\"joinEui\":\"$JOINEUI\"}}"

curl -s -X POST "$API/api/devices/$DEVEUI/keys" -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" \
  -d "{\"deviceKeys\":{\"devEui\":\"$DEVEUI\",\"nwkKey\":\"$APPKEY\"}}"
```

> La región del **device profile** debe coincidir con la del **binario** (EU868 con EU868).
> Recuerda flashear `periodical-uplink_lr1110_eu868.bin` en la placa que uses para EU868.
