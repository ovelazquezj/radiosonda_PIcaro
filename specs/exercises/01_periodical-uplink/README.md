# Ejercicio 01 — Periodical Uplink (conectividad LoRaWAN)

**Qué demuestra:** el flujo básico de LoRaWAN con el LR1110 — **join OTAA** y **uplinks
periódicos** (más uno extra al pulsar el botón azul **B1**). Es el laboratorio base de
conectividad y el primer contacto con ChirpStack.

> **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG**. El binario ya está
> **compilado** en `artifacts/` — solo hay que **flashearlo** (no necesitas compilar nada).

| | |
|---|---|
| Hardware | Nucleo-L476RG + LR1110 |
| Capacidad LR1110 | Radio LoRaWAN (sub-GHz) |
| ¿Join/ChirpStack? | ✅ Sí |
| Binario US915 | `artifacts/periodical-uplink_lr1110_us915.bin` (y su `.hex`/`.elf`) |
| Binario EU868 | `artifacts/periodical-uplink_lr1110_eu868.bin` (y su `.hex`/`.elf`) |
| Dato para dashboard | payload keep-alive (fPort 2) vía MQTT |

Credenciales en [`credentials.json`](credentials.json). JoinEUI común `aabbccddeeff0000`.

## Pasos del ejercicio

### 1. Flashear
Sigue [`../COMMON_FLASH.md`](../COMMON_FLASH.md) con el archivo de tu banda:
- **US915:** `artifacts/periodical-uplink_lr1110_us915.hex` (o `.bin`)
- **EU868:** `artifacts/periodical-uplink_lr1110_eu868.hex` (o `.bin`)

### 2. Provisionar en ChirpStack (por API)
Setup del token en [`../COMMON_CHIRPSTACK_API.md`](../COMMON_CHIRPSTACK_API.md). Luego:
```bash
export TOKEN="tu_api_key"
./provision.sh us915      # o: ./provision.sh eu868
```
El script crea (idempotente) el device profile de la banda, el device y la AppKey (en `nwkKey`).

### 3. Probar el join
Abre la traza serie (115200 8N1) y pulsa **RESET (B2)**. Debe aparecer **`Joined`**.
En ChirpStack el device mostrará *last seen* y verás Join + uplinks. Pulsa **B1** → uplink extra.

### 4. Consumir los datos (base del dashboard)
```bash
# Estado y métricas de enlace por REST:
python3 consume.py --state aabbccdd10915001

# Stream de payloads en tiempo real por MQTT (Ctrl-C para salir):
python3 consume.py --stream
#   (o sin dependencias:  ./scripts/subscribe.sh )
```

## Qué observar
- **Traza UART:** `Joined`, luego `TXDONE` en cada uplink.
- **ChirpStack (LoRaWAN frames):** JoinRequest→JoinAccept y uplinks periódicos.
- **MQTT:** un mensaje JSON por uplink en `application/<APP>/device/aabbccdd10915001/event/up`.

## Notas
- Región del **device profile** = región del **binario** (US915 con US915).
- Este ejemplo no tiene payload de sensor rico; para datos decodificados de verdad, ver el
  **ejercicio 02 (BMP280+GNSS)**.
