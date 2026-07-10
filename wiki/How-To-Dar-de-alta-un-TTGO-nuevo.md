# How-To · Dar de alta un TTGO nuevo (tus propias DevEUI / JoinEUI / AppKey)

Guía **end-to-end** para poner en marcha un nodo **TTGO ESP32 (SX1276)** con **credenciales tuyas**:
**generarlas → ponerlas en el sketch → compilar → flashear → darlo de alta en ChirpStack → verificar
el join**. Es la versión TTGO de [Dar de alta un LR1110 nuevo](How-To-Dar-de-alta-un-LR1110-nuevo).

> Aplica a los nodos **TTGO/SX1276** con **Arduino/LMIC** (ejercicios 05–06). El flujo básico de
> Arduino está en [Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).

> 🔑 **La diferencia nº1 con el LR1110:** en el sketch LMIC el **DevEUI y el JoinEUI van INVERTIDOS
> (LSB)**, mientras que el **AppKey va en MSB (igual)**. Confundir esto es **la causa nº1 de fallos de
> join en TTGO.**

## Las tres credenciales (qué son y en qué orden van)

| Credencial | Tamaño | ¿Secreta? | En el **sketch** (LMIC) | En **ChirpStack** |
|-----------|--------|-----------|-------------------------|-------------------|
| **DevEUI** | 8 bytes | No | **LSB (invertido)** | **MSB** (original) |
| **JoinEUI** (AppEUI) | 8 bytes | No | **LSB (invertido)** | **MSB** (original) |
| **AppKey** | 16 bytes | **SÍ** | **MSB** (igual) | **MSB** (igual) |

> Es decir: eliges los valores en **MSB** (lo "normal"), los pones **tal cual** en ChirpStack, pero
> el **DevEUI y el JoinEUI los inviertes** al pegarlos en el sketch. El AppKey no se toca.

---

## Paso 1 · Genera tus credenciales (en MSB)

Igual que para el LR1110 — todo en hex:

- **AppKey (16 bytes, aleatorio, secreto):**
  ```bash
  openssl rand -hex 16
  ```
- **DevEUI (8 bytes, ÚNICO):** p.ej. `70 B3 D5 7E D0 06 0A 01` (prefijo + contador; que no se repita).
- **JoinEUI (8 bytes):** el común del proyecto u otro, p.ej. `AA BB CC DD EE FF 00 00`.

Anota los tres valores **en MSB**: son los que pondrás en ChirpStack (Paso 5). Para el sketch (Paso 2)
convertirás DevEUI y JoinEUI a LSB.

---

## Paso 2 · Cámbialas en el sketch (`.ino`)

Edita el sketch de tu ejercicio:

| Ejercicio | Archivo a editar |
|-----------|------------------|
| 05 | `specs/exercises/05_ttgo-lora32/sketches/TTGO_LoRaWAN_v3.ino` |
| 06 | `specs/exercises/06_ttgo-bmp280/sketches/TTGO_BMP280_v1.ino` |

Busca los tres arrays y sustitúyelos (LMIC los lee por los callbacks `os_get*`):

```c
// DevEUI  -> INVERTIDO (LSB).  MSB 70B3D57ED0060A01  ->  01,0A,06,D0,7E,D5,B3,70
static const u1_t PROGMEM DEVEUI[8]  = { 0x01, 0x0A, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };

// JoinEUI/AppEUI -> INVERTIDO (LSB). MSB AABBCCDDEEFF0000 -> 00,00,FF,EE,DD,CC,BB,AA
static const u1_t PROGMEM APPEUI[8]  = { 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };

// AppKey -> MSB (igual que en ChirpStack, NO se invierte)
static const u1_t PROGMEM APPKEY[16] = { 0x8A, 0xC5, /* ...tus 16 bytes... */ 0x73, 0xBF };

// (estas 3 funciones ya están en el sketch; no las cambies)
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }   // JoinEUI/AppEUI
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }   // DevEUI
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }  // AppKey
```

**Truco — invierte un DevEUI/JoinEUI de MSB a LSB** (para el sketch; el `tac` es el que invierte):
```bash
echo "70B3D57ED0060A01" | fold -w2 | tac | sed 's/^/0x/' | paste -sd, | sed 's/,/, /g'
# -> 0x01, 0x0A, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70
```
**Y el AppKey (MSB, SIN invertir → el mismo comando pero sin `tac`):**
```bash
echo "8AC583DFEEC76C81FFD19CCFE76B73BF" | fold -w2 | sed 's/^/0x/' | paste -sd, | sed 's/,/, /g'
# -> 0x8A, 0xC5, 0x83, 0xDF, 0xEE, 0xC7, 0x6C, 0x81, 0xFF, 0xD1, 0x9C, 0xCF, 0xE7, 0x6B, 0x73, 0xBF
```

> ⚠️ **No inviertas el AppKey.** Solo DevEUI y JoinEUI se invierten. Es el error más común.

---

## Paso 3 · Configura LMIC (banda y radio)

En el sketch se selecciona la **misma banda** que usarás en ChirpStack (detalle en
[Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino)):

- En `arduino_lmic_project_config.h` **de la librería LMIC**:
  ```c
  #define CFG_us915 1        // o  #define CFG_eu868 1
  #define CFG_sx1276_radio 1
  ```
- Para **US915**, en el sketch se fija la sub-banda del gateway del lab:
  ```c
  LMIC_selectSubBand(1);     // sub-banda 2 (canales 8-15). Con (0) el gateway no te oye.
  ```

---

## Paso 4 · Compila y flashea (Arduino IDE)

- *Tools → Board* → **"TTGO LoRa32-OLED"** (o "ESP32 Dev Module").
- *Tools → Port* → el `COMx` / `/dev/ttyUSB0` del TTGO.
- **Upload (→)**. Al terminar, abre el **Monitor Serie a 115200** (lo miras en el Paso 6).

---

## Paso 5 · Da de alta el device en ChirpStack

Igual que cualquier device (detalle en [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)):

1. **Device profile:** región = la de tu banda; **MAC version LoRaWAN 1.0.4** (o 1.0.3); **OTAA**.
2. **Add device:** pega el **DevEUI** y el **JoinEUI** en **MSB (los valores ORIGINALES del Paso 1,
   NO los invertidos del sketch)**.
3. **Keys (OTAA):** el **AppKey** ⚠️ **en el campo `nwkKey`** (LoRaWAN 1.0.x).

> 🔑 **Ojo con el orden de bytes aquí:** en ChirpStack se meten los EUIs en **MSB** (los del Paso 1).
> La inversión a LSB es **solo dentro del sketch**. Si pegas en ChirpStack los bytes invertidos del
> sketch, el DevEUI no coincidirá y el join fallará.

Por API es idéntico al de la [guía de provisión](How-To-Provisionar-en-ChirpStack) (`devEui`/`joinEui`
en MSB; AppKey en `nwkKey`).

---

## Paso 6 · Verifica el join

Con el device dado de alta, reinicia el TTGO y mira el **Monitor Serie (115200)**:

- Debe pasar de **`EV_JOINING`** a **`EV_JOINED`**, y luego los envíos.
- **ChirpStack → device → LoRaWAN frames:** Join Request → Join Accept y los uplinks.

---

## Errores típicos (y su causa)

| Síntoma | Causa | Arreglo |
|---------|-------|---------|
| No hace join (nunca sale de `EV_JOINING`) | **Orden de bytes** del DevEUI/JoinEUI | En el sketch van **LSB (invertidos)**; en ChirpStack **MSB**. AppKey siempre MSB |
| No hace join / no llega al server | **Sub-banda** equivocada (US915) | `LMIC_selectSubBand(1)` (sub-banda 2) |
| `invalid MIC` | **AppKey** mal o invertido | AppKey va **MSB** (no se invierte); revisa los 16 bytes |
| `Unknown device` en logs | DevEUI no registrado / mal | Registra el **DevEUI en MSB**, idéntico al del Paso 1 |
| No hace join | **Banda** distinta (firmware vs profile) | `CFG_us915`/`CFG_eu868` = región del device profile |
| `DevNonce has already been used` | Nonce repetido tras reflashear | Borra y recrea el device en ChirpStack |

> ⚠️ **No commitees AppKeys reales** en el repo. Trátalas como secretos.

> Relacionado: **[Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino)** ·
> **[Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)** ·
> **[Dar de alta un LR1110 nuevo](How-To-Dar-de-alta-un-LR1110-nuevo)** (la versión LR1110) ·
> Volver: **[🏠 Inicio](Home)**
