# How-To · Compilar y flashear el TTGO ESP32 en Arduino (ejercicios 05–06)

Los nodos **TTGO ESP32 LoRa (SX1276)** no usan el toolchain ARM: se compilan y flashean con el
**Arduino IDE**. Requisitos previos (ESP32 + librerías) en
[Requisitos e instalación](How-To-Requisitos-e-instalación).

## Paso 1 · Abrir el sketch
- Ejercicio **05**: `specs/exercises/05_ttgo-lora32/sketches/TTGO_LoRaWAN_v3.ino`
- Ejercicio **06**: `specs/exercises/06_ttgo-bmp280/sketches/TTGO_BMP280_v1.ino` (añade el BMP280)

## Paso 2 · Configurar LMIC para tu banda
Edita `arduino_lmic_project_config.h` **de la librería LMIC**:
```c
#define CFG_us915 1        // (o #define CFG_eu868 1 para Europa)
#define CFG_sx1276_radio 1
```
Y en el sketch, para **US915** se selecciona la sub-banda del gateway:
```c
LMIC_selectSubBand(1);     // sub-banda 2 (canales 8-15), la que usa el gateway del lab
```
> `selectSubBand(1)` = sub-banda **2**. Si pones `(0)` (canales 0-7) el gateway no te oye y el join
> falla. Este es un fallo clásico. **Y en ChirpStack** el device profile debe usar la región
> `us915_1` (la misma sub-banda) → ver
> [Provisionar en ChirpStack § US915 sub-banda](How-To-Provisionar-en-ChirpStack).

> 🇪🇺 **EU868:** estos ejercicios (05/06) vienen para **US915**. Para **EU868** pon
> `#define CFG_eu868 1` y **elimina** la línea `LMIC_selectSubBand(...)`: EU868 **no** tiene
> sub-bandas (3 canales fijos 868.1/868.3/868.5). En ChirpStack usa un device profile **EU868**.

## Paso 3 · ⚠️ Orden de bytes de los EUIs (nota didáctica clave)
En **Arduino/LMIC** el **DevEUI** y el **JoinEUI/AppEUI** se escriben **al revés (LSB, little-endian)**
respecto a como se ven en **ChirpStack (MSB, big-endian)**. El **AppKey** va en **MSB** (igual en
ambos). **Confundir esto es la causa nº1 de fallos de join.**

| Valor | En ChirpStack (web/API) | En el sketch LMIC |
|-------|-------------------------|-------------------|
| DevEUI | `aabbccdd60915001` (MSB) | `{ 0x01,0x50,0x91,0x60,0xDD,0xCC,0xBB,0xAA }` (LSB, invertido) |
| JoinEUI | `aabbccddeeff0000` (MSB) | `{ 0x00,0x00,0xFF,0xEE,0xDD,0xCC,0xBB,0xAA }` (LSB, invertido) |
| AppKey | `60156015...60156015` (MSB) | `{ 0x60,0x15,... }` (MSB, **igual**) |

Regla práctica: **DevEUI y JoinEUI → invertidos** en el sketch; **AppKey → igual**.

## Paso 4 · Seleccionar placa y puerto
- *Tools → Board* → **"TTGO LoRa32-OLED"** (o "ESP32 Dev Module").
- *Tools → Port* → el `COMx` / `/dev/ttyUSB0` del TTGO.

## Paso 5 · Compilar y subir
Pulsa **Upload (→)**. Al terminar, abre el **Monitor Serie a 115200** para ver el `EV_JOINING` →
`EV_JOINED` y los envíos.

## Payloads que envía cada ejercicio
- **05**: 8 bytes en **fPort 1** → magic `0x99`, contador (u16 LE), timestamp (u32 LE), checksum.
- **06**: 5 bytes en **fPort 2** (big-endian) → versión, temperatura (int16 ×100), presión (u16 hPa).
  Ejemplo `01 09 29 03 F5` → `{version:1, temperature:23.45, pressure:1013}`.

El **codec** (`payload_decoder.js`) de cada ejercicio convierte esos bytes en campos legibles en
ChirpStack. Súbelo al device profile → [How-To Provisionar](How-To-Provisionar-en-ChirpStack).

## ¿El BMP280 necesita resistencias? (ejercicio 06)
No. Es I²C y las placas BMP280 traen sus **pull-ups** integrados. Conéctalo a los pines I²C del TTGO
(GPIO21=SDA, GPIO22=SCL, compartidos con la OLED), 3V3 y GND. El detalle de pines está en el README
del ejercicio 06.

> Siguiente: **[How-To Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)** →
