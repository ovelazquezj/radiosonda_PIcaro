# Ejercicio 04 — Wi-Fi Region Detection (capacidad Wi-Fi del LR1110)

**Qué demuestra:** la capacidad de **escaneo Wi-Fi pasivo** del LR1110. Al reiniciar, el
firmware lanza `smtc_modem_wifi_scan()`, escucha las balizas de los puntos de acceso (APs)
cercanos y, en el evento `WIFI_SCAN_DONE`, **imprime por UART las MACs/RSSI de los APs
detectados y la región inferida** a partir de ellos. **No se une a la red ni envía uplinks**:
el resultado se observa únicamente en la traza serie.

| | |
|---|---|
| Hardware | Nucleo-L476RG + LR1110 |
| Capacidad LR1110 | **Radio Wi-Fi (escaneo pasivo 2.4 GHz)** |
| ¿Join/ChirpStack? | ❌ No (no hace join; no aparece en ChirpStack) |
| Binario | `artifacts/wifi-region-detection_lr1110_multiband.bin` (US915 + EU868) |
| Dato del ejercicio | APs (MAC/RSSI) + región inferida, **solo por UART** |
| Provisión / consumo | **N/A** — ver *"Por qué NO usa ChirpStack"* |

Credenciales (o su ausencia) en [`credentials.json`](credentials.json).

## Pasos del ejercicio

### 1. Flashear
Sigue [`../COMMON_FLASH.md`](../COMMON_FLASH.md) con
`wifi-region-detection_lr1110_multiband` (`.hex`/`.bin`). Es un **binario único multi-banda**;
la banda es irrelevante porque el dispositivo **no se une** a ninguna red.

### 2. Abrir la traza serie
Terminal serie a **115200 8N1** sobre el VCP del ST-LINK (`/dev/ttyACM0`, `COMx`…):
```bash
tio -b 115200 /dev/ttyACM0     # o picocom/minicom; en Windows: PuTTY/Tera Term
```

### 3. Pulsar RESET (B2)
Al reiniciar, el firmware ejecuta un escaneo Wi-Fi (`smtc_modem_wifi_scan()`). Cada pulsación
de **RESET (B2)** repite el ciclo escaneo → resultado.

### 4. Observar por UART
En el evento `WIFI_SCAN_DONE` verás la lista de **APs detectados (MAC + RSSI)** y la
**región inferida** a partir de ellos. Ese es el resultado del ejercicio.

## Qué observar
- **Traza UART:** inicio del escaneo, lista de APs (MAC/RSSI) y la región inferida.
- **ChirpStack:** *nada* — este ejemplo no genera tráfico LoRaWAN (ver siguiente sección).

## Por qué NO usa ChirpStack

Es **intencional**. Este demo aísla la **capacidad de radio Wi-Fi** del LR1110, no la
conectividad LoRaWAN:

- **No hace join OTAA** ni envía uplinks → **no aparece en ChirpStack** (ni *last seen*, ni
  frames, ni MQTT).
- **No tiene credenciales baked-in** (DevEUI/JoinEUI/AppKey), por eso no hay device que dar de
  alta. El binario es multi-banda solo porque no importa la banda al no unirse a la red.
- **No hay provisión** (nada que registrar por API) ni **consumo** por REST/MQTT: el único
  canal de salida del dato es la **UART**. Por eso esas secciones son **N/A** frente a los
  ejercicios 01/05.

Para el flujo LoRaWAN completo (join + uplinks + consumo) usa el **ejercicio 01
(Periodical Uplink)**.

## Nota — geolocalización por Wi-Fi (LoRa Edge)

Este ejercicio es la base didáctica para explicar la **geolocalización por Wi-Fi** de la
familia **LoRa Edge**: en un producto real, la lista de MACs/RSSI de los APs se enviaría (por
LoRaWAN) a un servicio de resolución (p. ej. LoRa Cloud), que devuelve una posición estimada
sin GNSS. Aquí nos quedamos en el **paso previo y observable**: demostrar que el LR1110
escanea Wi-Fi y produce esos datos, mostrados por UART.

## Archivos
```
artifacts/wifi-region-detection_lr1110_multiband.bin   binario único multi-banda (US915+EU868)
credentials.json                                       marca "no va a ChirpStack" (sin credenciales)
```
