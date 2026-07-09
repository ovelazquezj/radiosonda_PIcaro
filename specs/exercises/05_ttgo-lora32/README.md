# Ejercicio 05 — TTGO ESP32 LoRa v1 (nodo de terceros con LMIC)

**Qué demuestra:** integrar un **nodo LoRaWAN de terceros** (TTGO ESP32 LoRa v1 + SX1276 + OLED,
firmware Arduino/**MCCI LMIC**) contra el mismo ChirpStack. A diferencia de los ejercicios 01–04,
aquí **el binario no es nuestro**: es *bring-your-own-device*. El valor está en el **flujo de
depuración** de un join que fallaba y en cómo se provisiona y consume por API.

| | |
|---|---|
| Hardware | TTGO ESP32 LoRa v1 (SX1276) + OLED SSD1306 |
| Firmware | Arduino / MCCI LMIC — `sketches/TTGO_LoRaWAN_v3.ino` |
| Región | US915 (sub-banda 2, canales 8-15) |
| ¿Join/ChirpStack? | ✅ Sí (OTAA, LoRaWAN 1.0.3) |
| Payload | 8 bytes en fPort 1 → decodificado a `counter`/`timestamp` |
| Estado | **joined y enviando datos** |

## La historia (lo que enseñamos)

El join fallaba con **cero** frames llegando a ChirpStack. Diagnóstico por logs — **dos bugs
independientes**, y **no** era el orden de bytes:

### Bug A — sub-banda US915 mal seleccionada
```c
LMIC_selectSubBand(0);   // ❌ sub-banda 1 -> canales 0-7  (902.3-903.7 MHz)
LMIC_selectSubBand(1);   // ✅ sub-banda 2 -> canales 8-15 (903.9-905.3 MHz)  <- la del gateway
```
El gateway escucha **FSB2 (canales 8-15)**. Con `selectSubBand(0)` el TTGO transmitía en canales
0-7 → el gateway **no lo oía** → cero frames. El sketch `TTGO_LoRa_Join_Only.ino` tenía este bug
(el propio autor lo marcó *"AQUI esta el pain"*); el `TTGO_LoRaWAN_v3.ino` ya usa `selectSubBand(1)`.

### Bug B — credenciales del sketch ≠ credenciales registradas
Los arrays del sketch producían DevEUI `02389205358e71db`, pero en ChirpStack se había registrado
`3098f6bdb525853e` (otro device). Aunque el RF funcionara, ChirpStack diría *"Unknown device"*.
**Solución:** registrar las credenciales reales del sketch (abajo).

### 📌 Nota didáctica CLAVE — el orden de bytes de DevEUI y JoinEUI

**El mismo DevEUI/JoinEUI se escribe en DOS órdenes distintos según dónde:** en el **sketch (LMIC)
en LSB** (invertido) y en **ChirpStack en MSB** (tal como se lee). El **AppKey NO** se invierte.

| Campo | En el **sketch** (LMIC, arrays) | En **ChirpStack** (UI/API) |
|-------|---------------------------------|----------------------------|
| **DevEUI** | `DB 71 8E 35 05 92 38 02` ← **LSB (invertido)** | `02 38 92 05 35 8E 71 DB` ← **MSB** |
| **JoinEUI/AppEUI** | `8A FD 43 71 F8 46 52 50` ← **LSB (invertido)** | `50 52 46 F8 71 43 FD 8A` ← **MSB** |
| **AppKey** | `8A C5 83 DF … 73 BF` ← **MSB (igual)** | `8A C5 83 DF … 73 BF` ← **MSB (igual)** |

**¿Por qué?** MCCI/Arduino LMIC carga `DevEUI` y `AppEUI` en **little-endian** (así lo esperan
`os_getDevEui()`/`os_getArtEui()`), mientras que LoRaWAN/ChirpStack los representan en
**big-endian**. Por eso, para el **mismo** dispositivo, los EUIs se ven "al revés" entre el código y
ChirpStack. El `AppKey` va en el mismo orden (MSB) en ambos → **no** se invierte.

> **Error típico:** copiar el DevEUI de ChirpStack (MSB) al array del sketch sin invertir (o al
> revés) → el DevEUI registrado no coincide con el que transmite el nodo → `Unknown device` y nunca
> hace join. **Regla:** en el array C, DevEUI/AppEUI van **al revés** que en ChirpStack; el AppKey, igual.

### Posible Bug C (si el join se ve en logs pero el device sigue fallando)
En el TTGO v1, **DIO1 = GPIO33 suele necesitar un puente físico** (pad DIO1 del SX1276 → GPIO33).
Sin él, el join **sale** pero el device **no recibe el Join Accept** en la ventana RX.
(En este caso no hizo falta — señal RSSI −31, SNR 10.5.)

## Credenciales (MSB, para ChirpStack)

| Campo | Valor |
|-------|-------|
| DevEUI | `02389205358e71db` |
| JoinEUI/AppEUI | `505246f87143fd8a` |
| AppKey (→ `nwkKey` en 1.0.x) | `8ac583dfeec76c81ffd19ccfe76b73bf` |

## Provisión en ChirpStack (ya hecha por API)

| Recurso | Valor |
|---------|-------|
| Application | `TTGO-LoRa32` — `d19cf2cb-5a1b-452b-8cfe-638784e5fb80` |
| Device profile | `TTGO-US915` — `150ae510-0577-4352-9cd3-39c354889474` |
| Topic MQTT (datos) | `application/d19cf2cb-5a1b-452b-8cfe-638784e5fb80/device/02389205358e71db/event/up` |

Reproducible:
```bash
export TOKEN="tu_api_key"
./provision.sh                                   # app + profile + device + keys (idempotente)
./scripts/upload_codec.sh 150ae510-0577-4352-9cd3-39c354889474   # sube el decoder al profile
```

## Payload y decoder

8 bytes en **fPort 1** (ver [`payload_decoder.js`](payload_decoder.js)):

| Offset | Campo | Tipo | |
|--------|-------|------|--|
| 0 | magic | uint8 | fijo `0x99` |
| 1-2 | counter | uint16 **LE** | contador de paquetes |
| 3-6 | timestamp | uint32 **LE** | uptime del dispositivo |
| 7 | checksum | uint8 | `suma(bytes 0..6) & 0xFF` |

Vector real capturado: `99 05 00 12 01 00 00 B1` → `counter=5, timestamp=274, checksum_ok=true`.

## Consumir los datos (dashboard)

```bash
# Stream por MQTT (trae 'object' decodificado si subiste el codec):
./scripts/subscribe.sh

# Estado + métricas por REST (reutiliza el consume.py del ejercicio 01):
export APP="d19cf2cb-5a1b-452b-8cfe-638784e5fb80"
python3 ../01_periodical-uplink/consume.py --state 02389205358e71db
```
El campo **`object`** del MQTT (`{counter, timestamp, checksum_ok}`) es la fuente directa para un
dashboard.

## Archivos
```
sketches/TTGO_LoRaWAN_v3.ino   firmware del nodo (sub-banda correcta)
payload_decoder.js             codec ChirpStack del payload
provision.sh                   alta reproducible por API
scripts/upload_codec.sh        adjunta el codec al device profile
scripts/subscribe.sh           stream MQTT de uplinks
credentials.json               IDs y credenciales reales
```
