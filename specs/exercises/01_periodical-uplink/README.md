# Ejercicio 01 — Periodical Uplink (tu primer LoRaWAN)

> **En una frase:** unes por primera vez un dispositivo a LoRaWAN (**join OTAA**) y ves sus **uplinks**
> llegar a ChirpStack. Es el "hola mundo" del curso.
> **Plataforma:** radio **Semtech LR1110** sobre placa **Nucleo-L476RG**. **Banda:** US915 (por
> defecto; hay binario EU868 alternativo). El binario **ya viene compilado** en `artifacts/` — aquí
> **no compilas nada**, solo flasheas.

## 🎯 Qué vas a conseguir
Un nodo que se **une** a tu red LoRaWAN y envía un pequeño mensaje cada cierto tiempo (más uno extra
al pulsar el botón azul **B1**). Al terminar, en ChirpStack verás el device con *last seen* reciente
y una fila por cada uplink; y por MQTT, un JSON como este:

```json
{ "deviceInfo": { "devEui": "aabbccdd10915001" },
  "fPort": 101,
  "data": "AA==" }
```

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir:

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Tu `TOKEN`** (API key de ChirpStack) exportado — lo creas en el [Ejercicio 00](../00_chirpstack-docker/): web `:8080` → *Tenant* → *API keys*.
- [ ] **Un gateway US915** con cobertura (sub-banda 2 / FSB2), encendido y *online* en ChirpStack → [Ejercicio 07](../07_esp-1ch-gateway/) o uno comercial. **Sin gateway el join nunca ocurre.**
- [ ] **Hardware:** Nucleo-L476RG + shield **LR1110**, y su cable USB.
- [ ] **Herramientas:** una utilidad para flashear + un **terminal serie** (115200 8N1) → [Wiki: Flashear y ver la serie](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Flashear-y-ver-la-serie). Para el consumo, **Python 3** (`pip install requests paho-mqtt`).

> ℹ️ Los `./*.sh` y `python3` corren en **bash** (Linux/macOS o **WSL** en Windows).
> **Credenciales de este ejercicio** (US915): DevEUI `aabbccdd10915001`, JoinEUI `aabbccddeeff0000`,
> AppKey `10151015…1015`. Completas en [`credentials.json`](credentials.json).

## 📟 Hardware y conexiones
Nada que cablear: el **LR1110** va montado como shield sobre la **Nucleo-L476RG**. Conecta la placa
al PC por el USB del **ST-LINK** (el mismo que usarás para flashear y para ver la traza serie).

## 🪜 Paso a paso

### 1. Flashea el binario
Graba el `.hex` (o `.bin`) de tu banda en la Nucleo. Detalle del método (arrastrar al disco
`NODE_L476RG` o STM32CubeProgrammer) en [`../COMMON_FLASH.md`](../COMMON_FLASH.md).
- **US915:** `artifacts/periodical-uplink_lr1110_us915.hex`
- **EU868:** `artifacts/periodical-uplink_lr1110_eu868.hex`

**Salida esperada:** el LED del ST-LINK parpadea al copiar y la placa se reinicia sola al terminar.

### 2. Provisiona el device en ChirpStack
Registra el device (idempotente: puedes repetirlo sin miedo). El script **autodetecta** tu tenant y
tu aplicación —los crea si hace falta—, así que solo necesitas el `TOKEN`:
```bash
# desde specs/exercises/01_periodical-uplink/
export TOKEN="tu_api_key"
./provision.sh us915          # o: ./provision.sh eu868
```
**Salida esperada:**
```
== tenant: 52f14cd4-... ==
== application: 5bc22cfa-... ==
== [us915] device profile PIcaro-LR1110-US915 ==
  device profile id: a1b2c3d4-...
== [us915] device aabbccdd10915001 ==
  creado.
== [us915] AppKey (nwkKey en 1.0.x) ==
  AppKey fijada en nwkKey.
== LISTO: aabbccdd10915001 provisionado en US915 (app 5bc22cfa-... , profile a1b2c3d4-...) ==
   Exporta tu app para consumir:   export APP=5bc22cfa-...
```
Copia esa línea `export APP=…` y ejecútala — la usarás en el paso 4.

### 3. Prueba el join
Abre la **traza serie** (115200 8N1) y pulsa el botón **RESET (B2)** de la Nucleo.
**Salida esperada** (algo así):
```
[INFO] LoRa Basics Modem - Periodical Uplink
[INFO] Joining...
[INFO] Joined
[INFO] TXDONE   fPort 101
```
En cuanto veas **`Joined`**, en ChirpStack (device → *LoRaWAN frames*) aparecerán el JoinRequest →
JoinAccept y los primeros uplinks. Pulsa **B1** para forzar un uplink extra (fPort 102).

### 4. Consume los datos (la base del dashboard)
```bash
# desde specs/exercises/01_periodical-uplink/
# a) Estado y métricas de enlace por REST:
python3 consume.py --state aabbccdd10915001

# b) Stream de payloads en tiempo real por MQTT (Ctrl-C para salir):
python3 consume.py --stream
#    (alternativa sin Python, usa el mosquitto del contenedor:)
export APP=5bc22cfa-...        # el que imprimió provision.sh
./scripts/subscribe.sh
```
**Salida esperada** (MQTT): un mensaje JSON por uplink en el topic
`application/<APP>/device/aabbccdd10915001/event/up`.

## ✅ Cómo saber que funcionó
- [ ] La traza serie muestra **`Joined`** y luego un **`TXDONE`** en cada uplink.
- [ ] En ChirpStack el device tiene *last seen* reciente y ves **JoinRequest→JoinAccept + uplinks** en *LoRaWAN frames*.
- [ ] Por **MQTT** llega un JSON por uplink; al pulsar **B1** aparece uno extra en **fPort 102**.

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| La serie se queda en `Joining…` para siempre | No hay gateway US915 con cobertura, o está *offline* | Enciende un gateway (ej.07) y verifica que está *online* en ChirpStack |
| `Joining…` eterno con gateway OK | Región del binario ≠ región del device profile, o claves mal | Usa el binario US915 con profile US915; reejecuta `./provision.sh` (fija DevEUI/JoinEUI/AppKey correctos) |
| Caracteres basura en la serie | Baudios mal | Ponlo a **115200 8N1** |
| `subscribe.sh`: *No such container* | ChirpStack no está levantado | Arranca el ej.00 (`docker compose up -d`); el script ya detecta el nombre del contenedor |
| No llega nada por MQTT | `APP` sin exportar o incorrecto | `export APP=<id>` (el que imprimió `provision.sh`) |

## 📤 Los datos (payload)
- **fPort 101:** uplink **periódico** (keep-alive, 1 byte).
- **fPort 102:** uplink **al pulsar B1**.
- Este ejercicio **no** lleva payload de sensor rico (es solo conectividad). Para datos decodificados
  de verdad (temperatura/presión), ve al **[Ejercicio 02](../02_bmp280-gnss-tracker/)**.

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio 00 · ChirpStack](../00_chirpstack-docker/)
- ➡️ Siguiente: [Ejercicio 02 · BMP280 + GNSS](../02_bmp280-gnss-tracker/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json) (US915 y EU868).
- **Archivos:** `provision.sh` (alta en ChirpStack) · `consume.py` (REST + MQTT) · `scripts/subscribe.sh` (MQTT sin Python) · `artifacts/` (binarios).
- **Guías comunes:** [Flashear](../COMMON_FLASH.md) · [ChirpStack API](../COMMON_CHIRPSTACK_API.md) · [Compilar (opcional)](../COMMON_BUILD.md).

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
