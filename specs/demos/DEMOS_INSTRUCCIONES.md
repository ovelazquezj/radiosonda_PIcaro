# Demos LR1110 — Guía paso a paso de flasheo y pruebas

Guía para flashear y probar los binarios del proyecto **Demos** en la placa
**ST Nucleo-L476RG + shield LR1110**, pensada para prácticas con alumnos.

Todos los binarios ya están compilados en [`artifacts/`](artifacts/). No necesitas compilar nada
para hacer las prácticas: **solo flashear y observar**.

| Demo | Binario | ¿Va a ChirpStack? | Qué enseña |
|------|---------|-------------------|------------|
| Periodical Uplink (US915) | `periodical-uplink_lr1110_us915.bin` | ✅ | Join OTAA + uplinks |
| Periodical Uplink (EU868) | `periodical-uplink_lr1110_eu868.bin` | ✅ | Join OTAA + uplinks |
| Hardware Modem | `hw-modem_lr1110_multiband.bin` | ✅ (avanzado) | Módem controlado por comandos |
| Wi-Fi Region Detection | `wifi-region-detection_lr1110_multiband.bin` | ❌ | Escaneo Wi-Fi del LR1110 |

---

## 0. Requisitos previos

**Hardware**
- Placa **Nucleo-L476RG** con el shield de radio **LR1110** montado.
- Cable **USB → micro-USB** (al conector ST-LINK de la Nucleo).
- Un **gateway LoRaWAN** de tu banda (US915 o EU868) dado de alta en tu ChirpStack, con cobertura RF.

**Software** (en tu PC)
- **Driver ST-LINK** instalado.
- Una herramienta de flasheo (elige una):
  - **STM32CubeProgrammer** (`STM32_Programmer_CLI`) — multiplataforma, recomendada.
  - **st-flash** (paquete `stlink-tools`) — Linux/Mac.
  - O el **arrastrar-y-soltar** al disco `NODE_L476RG` (la Nucleo se monta como USB).
- Un **terminal serie** para ver la traza: `tio`, `minicom`, `picocom`, PuTTY, o el monitor de Arduino IDE.
  Configuración: **115200 baudios, 8N1**.

---

## 1. Conectar la placa y localizar el puerto serie

1. Conecta la Nucleo por USB. Se encenderá el LED de alimentación.
2. Aparecerá:
   - Un **disco USB** llamado `NODE_L476RG` (para arrastrar-y-soltar).
   - Un **puerto serie virtual (VCP)**:
     - Linux: `/dev/ttyACM0`
     - macOS: `/dev/tty.usbmodemXXXX`
     - Windows: `COMx` (míralo en el Administrador de dispositivos).

---

## 2. Flashear un binario (los 3 métodos)

> El binario siempre se escribe en la dirección **`0x08000000`**. Flashea **un** `.bin` a la vez.
> Todos los comandos de esta guía se ejecutan desde la carpeta `artifacts/`.

**Método A — STM32CubeProgrammer (recomendado, multiplataforma)**
```bash
STM32_Programmer_CLI -c port=SWD mode=UR -w periodical-uplink_lr1110_us915.bin 0x08000000 -rst
```

**Método B — st-flash (Linux/macOS)**
```bash
st-flash --connect-under-reset write periodical-uplink_lr1110_us915.bin 0x08000000
```

**Método C — arrastrar y soltar**
- Copia el `.hex` (o `.bin`) correspondiente al disco `NODE_L476RG`. La placa se re-programa y reinicia sola.

Tras flashear, pulsa el botón **negro RESET (B2)** para reiniciar el firmware.

---

## 3. Abrir la traza serie (para todas las pruebas)

Abre el terminal serie **antes o justo después** de resetear, a **115200 8N1**:

```bash
# Linux/macOS (elige tu herramienta):
tio -b 115200 /dev/ttyACM0
#   picocom -b 115200 /dev/ttyACM0
#   minicom -D /dev/ttyACM0 -b 115200
```
En Windows: PuTTY → Serial → COMx, Speed 115200.

Verás el arranque del firmware (versión de LBM, versión de FW del LR1110, eventos, etc.).

---

## 4. Demo 1 — Periodical Uplink  *(el laboratorio principal)*

**Qué hace:** el dispositivo hace **join OTAA** a la red y luego envía **uplinks periódicos** y
**uno extra cada vez que pulsas el botón azul (B1)** de la Nucleo.

### 4.1 Registrar el device en ChirpStack (una vez por dispositivo)

> Usa los valores del binario que vas a flashear. **Entra los EUIs tal cual (MSB), sin invertir.**

| Banda | DevEUI | JoinEUI | AppKey |
|---|---|---|---|
| US915 | `AABBCCDD10915001` | `AABBCCDDEEFF0000` | `10151015101510151015101510151015` |
| EU868 | `AABBCCDD10868001` | `AABBCCDDEEFF0000` | `10681068106810681068106810681068` |

**Pasos (ChirpStack v4):**
1. **Device Profile** (Tenant → *Device profiles* → *Add device profile*):
   - Name: `Periodical US915 1.0.4` (o EU868)
   - Region: **US915** (o **EU868**)
   - MAC version: **LoRaWAN 1.0.4**
   - Regional parameters revision: **RP002-1.0.3**
   - Join: **OTAA**
2. **Add device** (Applications → tu app → *Add device*):
   - Name: `demo-periodical-us915`
   - **Device EUI:** el DevEUI de la tabla
   - **Join EUI:** `AABBCCDDEEFF0000`
   - Device profile: el del paso 1
3. **Keys (OTAA)** (device → pestaña *Keys (OTAA)*):
   - **Application key:** el AppKey de la tabla. (En 1.0.x solo se pide Application key; si pide Network key, pon el mismo.)

> ⚠️ La **región del Device Profile debe coincidir con la del binario** (US915 o EU868) o **no hará join**.

### 4.2 Flashear y probar

1. Flashea el `.bin` de la **misma banda** que registraste (§2).
2. Abre la traza serie (§3) y pulsa **RESET (B2)**.
3. **Observa en la traza:** el arranque, luego el intento de **Join** (`Joined` cuando lo logra).
4. **Observa en ChirpStack** (device → pestaña **LoRaWAN frames**):
   - Un **Join Request** seguido de **Join Accept** → activación correcta.
   - Después, **uplinks periódicos** (evento *Up* en la pestaña **Events**).
5. **Prueba el botón:** pulsa el **botón azul (B1)** → aparece un **uplink inmediato** en ChirpStack.

**Éxito =** ves Join + uplinks en ChirpStack y un uplink extra cada vez que pulsas B1.

---

## 5. Demo 2 — Hardware Modem  *(avanzado)*

**Qué hace:** convierte la placa en un **módem "todo integrado"** que **no actúa solo**: un
**host externo** (PC/MCU) lo controla enviando **comandos por UART** + 3 GPIO de handshake
(command / busy / event). Todas las funciones de la API de LBM se invocan por comando —
incluidas DevEUI, JoinEUI, AppKey, región y join (por eso este binario **no** trae credenciales).

### 5.1 Flashear
```bash
st-flash --connect-under-reset write hw-modem_lr1110_multiband.bin 0x08000000
```

### 5.2 Probar
- Este demo **necesita un host** que hable el protocolo del `hw_modem`. Sin ese host, la placa
  queda a la espera de comandos (no hace nada por sí sola).
- Opciones para el host:
  - La herramienta de host de módem de Semtech, o
  - Un script propio que implemente el protocolo definido en
    `lbm_examples/hw_modem/cmd_parser.h` (lista de comandos) y `hw_modem.h` (GPIO/handshake).
- Flujo típico del host: `set_deveui` → `set_joineui` → `set_nwkkey` → `set_region` →
  `join_network` → esperar evento *joined* → `request_uplink`.

> **Recomendación docente:** úsalo para **explicar la arquitectura** "host + módem por serie"
> (cómo un MCU de aplicación delega todo el LoRaWAN al LR1110). Para una práctica de conectividad
> "que se vea sola", usa el **Demo 1 (Periodical Uplink)**.

---

## 6. Demo 3 — Wi-Fi Region Detection  *(sin ChirpStack)*

**Qué hace:** usa el **escáner Wi-Fi pasivo del LR1110** para detectar puntos de acceso cercanos
y deducir la **región**. **No hace join ni envía nada a la red** — muestra los resultados **por UART**.

### 6.1 Flashear
```bash
st-flash --connect-under-reset write wifi-region-detection_lr1110_multiband.bin 0x08000000
```

### 6.2 Probar
1. Abre la traza serie (§3) a 115200 8N1.
2. Pulsa **RESET (B2)**.
3. **Observa en la traza:** el resultado de los **escaneos Wi-Fi** (MACs/RSSI de los AP detectados)
   y la **región** inferida. Se repite periódicamente.

> No aparece nada en ChirpStack (es intencional). Es un demo de la **capacidad Wi-Fi** del chip,
> útil para explicar la geolocalización por Wi-Fi de la familia LoRa Edge.

---

## 7. Solución de problemas

- **No hace Join (Demo 1):**
  - Región del **Device Profile** = región del **binario** (US915/EU868).
  - DevEUI/JoinEUI/AppKey idénticos en ChirpStack y en la tabla (MSB, sin invertir).
  - Gateway de esa banda dado de alta y con cobertura.
- **`st-flash` no encuentra la placa:** usa `--connect-under-reset`; revisa el driver ST-LINK;
  cierra STM32CubeIDE/Programmer si tienen la sonda ocupada.
- **No hay traza / caracteres raros:** confirma **115200 8N1** y el puerto correcto (VCP del ST-LINK).
- **EU868 transmite muy espaciado:** es el **duty cycle** regulatorio de EU868; es normal.
- **Reinstalar/olvidar el device:** en ChirpStack puedes borrar el device y volver a crearlo con
  las mismas claves si algo quedó mal configurado.

---

## Referencias del proyecto

- Manifiesto y credenciales: [`BUILD_MANIFEST.md`](BUILD_MANIFEST.md) · [`credentials.json`](credentials.json)
- Registrar un device en ChirpStack **por API** (paso a paso, ejercicio EU868): [`REGISTRO_API_CHIRPSTACK.md`](REGISTRO_API_CHIRPSTACK.md)
- Proyecto del sensor BMP280 (aparte): [`../bmp280-gnss-tracker/`](../bmp280-gnss-tracker/)
