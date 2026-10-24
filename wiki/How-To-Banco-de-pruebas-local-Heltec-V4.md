# How-To · Banco de pruebas local: gateway de 1 canal Heltec V4 + ChirpStack en Docker

> **En una frase:** montas en tu mesa un **banco de pruebas LoRaWAN completo**: tu propio servidor
> ChirpStack corriendo en tu PC y un **gateway de 1 canal** hecho con una Heltec WiFi LoRa 32 **V4**,
> para probar tu radiosonda **sin depender de ningún servidor ni gateway externo**.
> **Plataforma:** Heltec WiFi LoRa 32 V4 (ESP32-S3 + SX1262) · firmware *One Channel Hub* de Semtech
> adaptado por PiCARO · ChirpStack v4 en Docker. **Banda:** US915, sub-banda 2, canal fijo 903.9 MHz, SF10.

Esta guía tiene **cinco partes**. Hazlas en orden la primera vez; después, para trabajar cada día, solo
necesitas la **Parte E**.

| Parte | Qué haces | Cuándo |
|---|---|---|
| [A](#parte-a--instalar-chirpstack-en-tu-pc-con-docker) | Instalar ChirpStack en tu PC con Docker | Una sola vez |
| [B](#parte-b--grabar-el-firmware-en-la-heltec-v4) | Grabar el firmware del gateway en la Heltec V4 | Una sola vez |
| [C](#parte-c--conectar-el-gateway-a-tu-wifi-y-a-tu-chirpstack) | Decirle al gateway tu WiFi y la IP de tu PC | Una vez (y cada vez que cambies de red) |
| [D](#parte-d--dar-de-alta-el-gateway-en-chirpstack) | Dar de alta el gateway en ChirpStack | Una sola vez |
| [E](#parte-e--usar-el-banco-de-pruebas-día-a-día) | **Usar el banco de pruebas** con tu radiosonda | Cada sesión de trabajo |

---

## 🎯 Qué vas a conseguir

```
   Tu radiosonda                 Heltec V4 (gateway)                    Tu PC
 ┌───────────────┐  LoRa US915  ┌──────────────────┐  WiFi de tu red  ┌──────────────────────────┐
 │ nodo LoRaWAN  │ ───────────▶ │ SX1262 escucha   │ ───────────────▶ │ ChirpStack en Docker     │
 │ (DR0 = SF10)  │  903.9 MHz   │ 903.9 MHz / SF10 │  UDP puerto 1700 │ web: http://localhost:8080│
 └───────────────┘ ◀─────────── └──────────────────┘ ◀─────────────── └──────────────────────────┘
                    downlinks                            órdenes de TX
```

Al terminar verás tu gateway **Online** en la web de ChirpStack, y en la pantalla de la Heltec su
identificador, su IP, el canal y el contador de paquetes recibidos (`up`) y enviados (`dn`):

```
LoRaHub - Receiving
AABBCCFFFE001122 - 192.168.1.60        <-- el tuyo será distinto
903.9000  SF10 BW125
up 3  dn 1
2601ABCD sf:10 rssi:-62 snr:9          <-- último paquete recibido
```

![Gateway en línea en ChirpStack](https://raw.githubusercontent.com/ovelazquezj/radiosonda_PIcaro/master/specs/exercises/14_heltec-v4-gateway-1canal/img/04-chirpstack-gateways-lista.png)

## ⚠️ Lo que este gateway puede y no puede hacer (léelo antes de empezar)

Un gateway comercial escucha **8 canales y todos los SF a la vez**. Este escucha **un solo canal y un
solo SF**. Es suficiente para aprender y para probar tu radiosonda en la mesa, pero tiene límites:

- ✅ Recibe los uplinks que tu radiosonda envíe **en 903.9 MHz con SF10** y entrega a la radiosonda los
  downlinks que mande ChirpStack (JoinAccept, Marco Polo, etc.).
- ⚠️ La radiosonda del curso **salta entre los 8 canales** de la sub-banda 2. El gateway oye solo el que
  cae en 903.9 MHz: **en promedio 1 de cada 8 uplinks** (~12 %). Es normal ver huecos.
- ⚠️ El **join** puede tardar varios intentos por el mismo motivo. Después la sesión queda guardada en la
  radiosonda y ya no hace falta repetirlo.
- ❌ No sirve para producción ni para la red pública (no cumple LoRaWAN). Es **solo para tu banco de pruebas**.

## 🧰 Antes de empezar

Marca esta lista **antes** de seguir. Si te falta algo, no continúes:

- [ ] **PC con Windows 10 u 11 de 64 bits**, con permisos de **administrador** y unos **5 GB libres** en disco.
  (En Linux/macOS también funciona; las diferencias se indican en notas.)
- [ ] El PC conectado a una **red WiFi o cableada de tu casa/laboratorio** (la "red local").
- [ ] Esa red tiene **WiFi de 2.4 GHz**. ⚠️ El ESP32 **no** se conecta a redes de 5 GHz.
- [ ] **Placa Heltec WiFi LoRa 32 V4** (en la serigrafía dice `WiFi LoRa 32(V4)` o `V4.2`/`V4.3`).
- [ ] **Antena de 915 MHz** para la Heltec. ⚠️ **Nunca la enciendas sin antena:** puedes dañar la radio.
- [ ] **Cable USB-C de datos** (no uno "solo de carga").
- [ ] Un **celular** (o laptop) con WiFi para configurar el gateway.
- [ ] **Conexión a internet** durante la instalación (se descargan programas e imágenes de Docker).

---

## Parte A — Instalar ChirpStack en tu PC con Docker

ChirpStack es el **servidor de red LoRaWAN** (LNS): recibe lo que captan los gateways, maneja los joins,
descifra los paquetes y te los muestra en una web. Lo vamos a correr dentro de **Docker**, que es un
programa para ejecutar "contenedores" (programas empaquetados con todo lo que necesitan).

### A1. Activa WSL2 (solo Windows)

Docker en Windows necesita **WSL2** (el subsistema de Linux de Windows).

1. Abre el menú Inicio, escribe **PowerShell**, haz clic derecho en *Windows PowerShell* →
   **Ejecutar como administrador**.
2. Escribe:
   ```powershell
   wsl --install
   ```
3. **Reinicia el PC** cuando termine.

**Salida esperada** (algo así): `Instalando: Subsistema de Windows para Linux ... La operación se
completó correctamente.` Si dice que **ya está instalado**, perfecto: sigue.

### A2. Instala Docker Desktop

1. Descarga **Docker Desktop para Windows** desde <https://www.docker.com/products/docker-desktop/>.
2. Ejecuta el instalador. Deja marcada la opción **Use WSL 2 instead of Hyper-V**.
3. Al terminar, **abre Docker Desktop** desde el menú Inicio y acepta los términos.
4. Espera a que el icono de la ballena 🐳 (abajo a la derecha, junto al reloj) **deje de moverse**: eso
   indica que Docker ya está listo.

> 🐧 **Linux:** instala *Docker Engine* y el plugin *docker compose* desde <https://docs.docker.com/engine/install/>.
> 🍎 **macOS:** instala Docker Desktop para Mac.

### A3. Comprueba que Docker funciona

Abre una **PowerShell normal** (no hace falta administrador) y escribe:

```powershell
docker --version
docker compose version
```

**Salida esperada** (los números pueden ser otros):
```
Docker version 27.3.1, build ce12230
Docker Compose version v2.29.7
```

Si sale `error during connect` o `no se reconoce como un comando`, **Docker Desktop no está abierto**:
ábrelo y espera a la ballena quieta.

### A4. Descarga el repositorio del curso

**Opción 1 — con Git** (recomendado). Si no tienes Git, instálalo desde <https://git-scm.com/download/win>
con las opciones por defecto. Luego, en PowerShell:

```powershell
cd $HOME\Documents
git clone https://github.com/ovelazquezj/radiosonda_PIcaro.git
```

**Opción 2 — sin Git:** en <https://github.com/ovelazquezj/radiosonda_PIcaro> pulsa **Code → Download ZIP**,
descomprímelo en `Documentos` y renombra la carpeta a `radiosonda_PIcaro`.

En los dos casos acabas con la carpeta `Documentos\radiosonda_PIcaro`.

### A5. Arranca ChirpStack

La configuración de ChirpStack **ya viene lista** en la carpeta del Ejercicio 00 (región US915 sub-banda 2
activada). Solo hay que arrancarla **desde esa carpeta**:

```powershell
cd $HOME\Documents\radiosonda_PIcaro\specs\exercises\00_chirpstack-docker
docker compose up -d
```

La **primera vez** tarda unos minutos porque descarga las imágenes. **Salida esperada** (al final):
```
 ✔ Container 00_chirpstack-docker-postgres-1                                Started
 ✔ Container 00_chirpstack-docker-redis-1                                   Started
 ✔ Container 00_chirpstack-docker-mosquitto-1                               Started
 ✔ Container 00_chirpstack-docker-chirpstack-gateway-bridge-1               Started
 ✔ Container 00_chirpstack-docker-chirpstack-gateway-bridge-basicstation-1  Started
 ✔ Container 00_chirpstack-docker-chirpstack-1                              Started
 ✔ Container 00_chirpstack-docker-chirpstack-rest-api-1                     Started
```

### A6. Verifica que los 7 contenedores están arriba

```powershell
docker compose ps
```

**Salida esperada:** 7 líneas, todas con `Up` en la columna *STATUS*. Las que importan para este
ejercicio:

| Servicio | Puerto | Para qué sirve |
|---|---|---|
| `chirpstack` | **8080** (TCP) | La web de ChirpStack |
| `chirpstack-gateway-bridge` | **1700/udp** | Aquí se conecta tu gateway Heltec (protocolo Semtech UDP) |
| `chirpstack-rest-api` | 8090 (TCP) | API REST (la usan otros ejercicios) |
| `mosquitto` | 1883 (TCP) | Broker MQTT (por aquí salen los datos de tus nodos) |

### A7. Entra a la web de ChirpStack

1. Abre el navegador en **<http://localhost:8080>**.
2. Usuario **`admin`**, contraseña **`admin`** → **Submit**.

![Pantalla de login de ChirpStack](https://raw.githubusercontent.com/ovelazquezj/radiosonda_PIcaro/master/specs/exercises/14_heltec-v4-gateway-1canal/img/01-chirpstack-login.png)

Verás el panel del tenant **ChirpStack** (el tenant que viene creado por defecto):

![Panel del tenant ChirpStack](https://raw.githubusercontent.com/ovelazquezj/radiosonda_PIcaro/master/specs/exercises/14_heltec-v4-gateway-1canal/img/02-chirpstack-dashboard.png)

> 🔐 `admin`/`admin` es la contraseña de fábrica. Para un banco de pruebas en tu casa está bien; si el PC
> está en una red compartida, cámbiala en **admin (arriba a la derecha) → Change password**.

### A8. Averigua la IP de tu PC en la red local

El gateway necesitará esta IP para mandarle los paquetes a ChirpStack. En PowerShell:

```powershell
ipconfig
```

Busca el bloque del adaptador con el que estás conectado a tu red (**Adaptador de LAN inalámbrica
Wi-Fi** o **Adaptador de Ethernet Ethernet**) y anota su **Dirección IPv4**:

```
Adaptador de LAN inalámbrica Wi-Fi:
   Dirección IPv4. . . . . . . . . . . . . . : 192.168.1.50      <-- ESTA (la tuya será distinta)
   Puerta de enlace predeterminada . . . . . : 192.168.1.1
```

⚠️ **No** uses las IPs de adaptadores `vEthernet (WSL...)`, `vEthernet (Default Switch)` ni
`192.168.56.x` (VirtualBox): son redes internas del PC, el gateway no llega a ellas.

📝 **Anota:** IP del PC = `__________________`

> 🐧/🍎 En Linux usa `ip -4 addr` y en macOS `ipconfig getifaddr en0`.

### A9. Abre el puerto UDP 1700 en el firewall de Windows

Windows bloquea por defecto las conexiones que **llegan** a tu PC, sobre todo si tu red está marcada como
**Pública**. Hay que permitir el UDP 1700 (y, si quieres ver ChirpStack desde el celular, el TCP 8080).

1. Abre **PowerShell como administrador** (como en A1).
2. Escribe:
   ```powershell
   New-NetFirewallRule -DisplayName "ChirpStack gateways UDP 1700" -Direction Inbound -Protocol UDP -LocalPort 1700 -Action Allow -Profile Any
   New-NetFirewallRule -DisplayName "ChirpStack web TCP 8080" -Direction Inbound -Protocol TCP -LocalPort 8080 -Action Allow -Profile Any
   ```

**Salida esperada:** un bloque por regla que incluye `Enabled : True` y `Action : Allow`.

✅ **Fin de la Parte A:** ChirpStack corre en tu PC, entras a la web y sabes la IP de tu PC.

---

## Parte B — Grabar el firmware en la Heltec V4

El firmware ya viene **compilado** en la carpeta [`artifacts/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/14_heltec-v4-gateway-1canal/artifacts) de este ejercicio:
`picaro-gw-heltec-v4_us915-fsb2.bin`. Solo hay que grabarlo con **esptool**, la herramienta oficial de
Espressif.

### B1. Instala Python

1. Descarga Python 3 desde <https://www.python.org/downloads/> e instálalo.
2. ⚠️ En la primera pantalla del instalador marca **"Add python.exe to PATH"**.
3. Comprueba en una PowerShell nueva:
   ```powershell
   py --version
   ```
   **Salida esperada:** `Python 3.12.x` (o cualquier 3.9 o superior).

### B2. Instala esptool

```powershell
py -m pip install esptool
py -m esptool version
```

**Salida esperada:** `esptool v5.x.x` (en la última línea, el número de versión).

> ℹ️ Esta guía usa la sintaxis de **esptool 5** (`erase-flash`, `write-flash`, con guion). Si tienes la
> versión 4, escribe los comandos con guion bajo: `erase_flash`, `write_flash`.

### B3. Enrosca la antena y conecta la placa

1. **Enrosca la antena de 915 MHz** en el conector de la Heltec. Hazlo siempre **antes** de conectarla.
2. Conecta la Heltec al PC con el cable **USB-C de datos**.
3. La V4 usa el **USB nativo del ESP32-S3**: **no necesita instalar ningún driver** en Windows 10/11.

### B4. Identifica el puerto COM de la Heltec

Abre el **Administrador de dispositivos** (clic derecho en Inicio → *Administrador de dispositivos*) y
despliega **Puertos (COM y LPT)**. La Heltec aparece como **Dispositivo serie USB (COMx)**.

O en PowerShell (muestra solo placas con ESP32-S3 por USB nativo, fabricante `303A`):

```powershell
Get-PnpDevice -PresentOnly -Class Ports | Where-Object InstanceId -match 'VID_303A' | Select-Object FriendlyName
```

**Salida esperada:**
```
FriendlyName
------------
Dispositivo serie USB (COM24)          <-- tu número será otro
```

⚠️ **Si tienes otra placa ESP32-S3 conectada** (por ejemplo, tu radiosonda T-Beam Supreme), saldrán
**dos** puertos. Para saber cuál es la Heltec: desconecta la Heltec, vuelve a listar, y el puerto que
**desaparece** es el suyo. **Nunca flashees la placa equivocada.**

📝 **Anota:** puerto de la Heltec = `COM____`

### B5. Borra la memoria de la placa

En los comandos siguientes **cambia `COM24` por tu puerto**. Primero ve a la carpeta del firmware:

```powershell
cd $HOME\Documents\radiosonda_PIcaro\specs\exercises\14_heltec-v4-gateway-1canal\artifacts
py -m esptool --chip esp32s3 --port COM24 erase-flash
```

Tarda unos 20–40 s. **Salida esperada** (algo así):
```
Connecting...
Connected to ESP32-S3 on COM24:
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz, Embedded PSRAM 2MB (AP_3v3)
MAC:                aa:bb:cc:00:11:22
...
Flash memory erased successfully in 25.3 seconds.
```

📝 **Anota la MAC** que aparece: la necesitarás en la Parte D. MAC = `__:__:__:__:__:__`

**Si sale `Failed to connect to ESP32-S3`:** pon la placa en modo de carga a mano:
1. **Mantén pulsado** el botón **PRG** (el de la izquierda, junto al USB).
2. Con PRG pulsado, **pulsa y suelta** el botón **RST**.
3. **Suelta PRG.**
4. Repite el comando (el puerto COM puede cambiar de número: vuelve a mirarlo como en B4).

### B6. Graba el firmware

```powershell
py -m esptool --chip esp32s3 --port COM24 -b 460800 write-flash 0x0 picaro-gw-heltec-v4_us915-fsb2.bin
```

**Salida esperada** (algo así):
```
Wrote 1247008 bytes (... compressed) at 0x00000000 in 9.1 seconds ...
Hash of data verified.
Hard resetting via RTS pin...
```

`Hash of data verified.` significa que se grabó bien. Si usaste el modo de carga manual (PRG + RST),
**pulsa RST una vez** para que arranque el firmware nuevo.

### B7. Calcula el Gateway EUI

Todo gateway LoRaWAN tiene un identificador de 8 bytes, el **Gateway EUI**. Este firmware lo forma con
la MAC de la placa, insertando **`FFFE`** en medio:

| | Bytes |
|---|---|
| MAC (6 bytes) | `aa:bb:cc` · `00:11:22` |
| Gateway EUI (8 bytes) | `aabbcc` · **`fffe`** · `001122` → **`aabbccfffe001122`** |

📝 **Anota:** Gateway EUI = `________________` (16 caracteres hexadecimales)

No te preocupes si te equivocas: en la Parte C la pantalla de la Heltec te mostrará el EUI correcto.

### B8. Qué debe mostrar la pantalla ahora

Como la placa todavía no conoce tu WiFi, al arrancar abre **su propia red de configuración**. En la
primera línea de la pantalla verás, desplazándose:

```
WiFi PiCARO-GW-001122 - http://192.168.4.1
```

Los últimos 6 caracteres del nombre (`001122` en el ejemplo) son el final de **tu** MAC.

✅ **Fin de la Parte B:** la Heltec tiene el firmware del gateway y está esperando que la configures.

---

## Parte C — Conectar el gateway a tu WiFi y a tu ChirpStack

El gateway necesita tres datos: **a qué WiFi conectarse**, **su contraseña** y **la IP de tu PC** (donde
corre ChirpStack). Se los das desde una página web que la propia placa publica en su red de configuración.
Los datos **se guardan en la placa** y se usan en todos los arranques siguientes.

### C1. Conéctate a la red de configuración del gateway

1. En tu **celular**, abre los ajustes de **WiFi**.
2. Elige la red **`PiCARO-GW-xxxxxx`** (la de tu pantalla).
3. Contraseña: **`picaro1234`**.
4. Si el celular avisa *"Esta red no tiene acceso a internet"*, elige **mantener la conexión**
   ("Usar esta red de todos modos" o similar). Es normal: es una red solo para configurar.

### C2. Abre la página de configuración

En la mayoría de los celulares la página **se abre sola** al conectarte. Si no, abre el navegador y entra a:

**<http://192.168.4.1>**


### C3. Llena el formulario

| Campo | Qué poner | Ejemplo |
|---|---|---|
| **Redes encontradas** | Elige tu red en la lista: rellena el campo de abajo. Si tu red no aparece, escríbela a mano | `MiCasa` |
| **Nombre de la red (SSID)** | El nombre **exacto** de tu WiFi de **2.4 GHz** (respeta mayúsculas y espacios) | `MiCasa` |
| **Contraseña** | La contraseña de esa WiFi | `********` |
| **IP o nombre del servidor** | La **IP de tu PC** que anotaste en **A8** | `192.168.1.50` |
| **Puerto UDP** | **`1700`** (no lo cambies) | `1700` |
| **Frecuencia (Hz)** | **`903900000`** (ya viene puesto) | `903900000` |
| **Spreading factor** | **`SF10`** (ya viene puesto; es el DR0 de la radiosonda) | `SF10` |

Pulsa **Guardar y reiniciar**.

### C4. Confirma que se guardó

El celular muestra **"Configuración guardada"** y, a los 2 segundos, la red `PiCARO-GW-xxxxxx`
**desaparece** (es normal: la placa se reinicia y se conecta a tu WiFi). Tu celular volverá solo a su red
de siempre.


### C5. Comprueba la pantalla de la Heltec

Tras unos 20 segundos la pantalla debe mostrar algo así:

```
LoRaHub - Receiving
AABBCCFFFE001122 - 192.168.1.60
903.9000  SF10 BW125
up 0  dn 0
```

| Línea | Qué significa |
|---|---|
| `LoRaHub - Receiving` | El gateway está escuchando la radio ✅ |
| `AABBCCFFFE001122` | **Tu Gateway EUI** (compáralo con el que calculaste en B7) |
| `192.168.1.60` | La IP que tu router le dio a la Heltec. 📝 Anótala: la usarás en E5 |
| `903.9000  SF10 BW125` | Canal que escucha: 903.9 MHz, SF10, 125 kHz |
| `up 0  dn 0` | Paquetes recibidos (uplinks) y enviados (downlinks) |

Si en lugar de eso vuelve a salir `WiFi PiCARO-GW-...`, la placa **no pudo entrar a tu WiFi** (contraseña
mal escrita o red de 5 GHz). Ve a la tabla de [problemas](#️-si-algo-falla).

✅ **Fin de la Parte C:** el gateway está en tu WiFi y ya envía datos a tu PC por UDP 1700.

---

## Parte D — Dar de alta el gateway en ChirpStack

ChirpStack **ignora** a los gateways que no conoce. Hay que registrarlo con su **Gateway EUI**.

### D1. Abre el formulario de alta

1. En <http://localhost:8080> (usuario `admin`), en el menú de la izquierda, dentro de **Tenant**, pulsa
   **Gateways**.
2. Pulsa el botón azul **Add gateway**.

### D2. Rellena los datos

| Campo | Qué poner |
|---|---|
| **Name** | Un nombre sin espacios, por ejemplo `heltec-v4-1canal` |
| **Description** | Opcional, por ejemplo `Heltec WiFi LoRa 32 V4 - One Channel Hub, US915 FSB2 903.9 MHz SF10` |
| **Gateway ID (EUI64)** | **Tu Gateway EUI** de la pantalla (16 caracteres). Mayúsculas o minúsculas da igual |
| **Stats interval (secs)** | **`30`** (el gateway manda sus estadísticas cada 30 s) |
| **Downlink priority** | Déjalo en `10` |
| **Location** | Opcional: arrastra el marcador a donde está tu gateway |

![Formulario de alta del gateway](https://raw.githubusercontent.com/ovelazquezj/radiosonda_PIcaro/master/specs/exercises/14_heltec-v4-gateway-1canal/img/03-chirpstack-alta-gateway.png)

Baja hasta el final de la página y pulsa **Submit**.

> ⚠️ El botón con flechas (🔄) junto al campo *Gateway ID* **genera un EUI al azar**. **No lo uses**: el
> EUI tiene que ser exactamente el de tu placa o ChirpStack no la reconocerá.

### D3. Comprueba que está en línea

Vuelve a **Gateways**. En menos de un minuto tu gateway debe aparecer con el punto verde **Online** y la
región **us915_1** (US915, sub-banda 2):

![Lista de gateways con el gateway Online](https://raw.githubusercontent.com/ovelazquezj/radiosonda_PIcaro/master/specs/exercises/14_heltec-v4-gateway-1canal/img/04-chirpstack-gateways-lista.png)

Haz clic en su nombre para ver el detalle (*Last seen* se actualiza cada 30 s):

![Detalle del gateway](https://raw.githubusercontent.com/ovelazquezj/radiosonda_PIcaro/master/specs/exercises/14_heltec-v4-gateway-1canal/img/05-chirpstack-gateway-detalle.png)

✅ **Fin de la Parte D:** tu banco de pruebas está completo.

---

## Parte E — Usar el banco de pruebas día a día

### E1. Encender el banco de pruebas (cada sesión)

1. **Abre Docker Desktop** y espera la ballena quieta.
2. ChirpStack arranca solo con Docker (sus contenedores están configurados con `restart: unless-stopped`).
   Compruébalo:
   ```powershell
   cd $HOME\Documents\radiosonda_PIcaro\specs\exercises\00_chirpstack-docker
   docker compose ps
   ```
   Si no salen los 7 servicios con `Up`, arráncalos con `docker compose up -d`.
3. **Conecta la Heltec** (con su antena) a cualquier USB o cargador. Ya no necesita el PC para nada más
   que la corriente.
4. Comprueba que la **IP de tu PC no cambió** (`ipconfig`, como en A8). Si cambió, ve a [E5](#e5-cambiar-la-configuración-del-gateway).
5. En <http://localhost:8080> → **Gateways**: tu gateway debe estar **Online**.

### E2. Preparar tu radiosonda para este banco

Para que el gateway oiga a tu radiosonda:

| Requisito de la radiosonda | Valor |
|---|---|
| Región | **US915**, **sub-banda 2** (canales 8–15) |
| Data rate de uplink | **DR0 = SF10 / BW125** |
| ADR | **Apagado** (con ADR la red cambiaría el SF y el gateway dejaría de oírla) |
| Dada de alta | En **tu** ChirpStack local: aplicación + *device profile* US915 + *device* con su DevEUI, JoinEUI y AppKey (ver [Ejercicio 12](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/12_heltec-lora32-v3-join/) para el procedimiento en la web) |

La radiosonda del curso ya cumple las tres primeras condiciones; solo tienes que darla de alta en tu
ChirpStack.

### E3. Probar la radiosonda

1. Coloca la radiosonda a **al menos 2–3 metros** del gateway. Si están pegados, la señal es tan fuerte que
   satura al receptor y los paquetes se pierden.
2. Enciende la radiosonda.
3. Mira la **pantalla de la Heltec**: el contador `up` sube cada vez que capta un paquete, y la última
   línea muestra el *DevAddr*, el SF, el RSSI y el SNR del último paquete.
4. En ChirpStack abre tu gateway → pestaña **LoRaWAN frames**: ahí aparecen **en directo** los paquetes
   que el gateway reenvía (*JoinRequest*, *UnconfirmedDataUp*, y los downlinks *JoinAccept*, etc.). Esta
   pestaña solo muestra lo que llega **mientras la tienes abierta**.
5. Para ver los **datos decodificados** de tu radiosonda, ve a **Applications → tu aplicación → tu
   device → Events** (o *LoRaWAN frames* del device).

**Cómo leer lo que ves:**

| Lo que observas | Qué significa |
|---|---|
| `JoinRequest` en el gateway, pero no `JoinAccept` | ChirpStack no conoce el device o las claves no coinciden. Revisa el alta del device |
| `JoinRequest` y `JoinAccept`, pero el device no se une | La radiosonda no recibió el downlink: aléjala un poco del gateway y deja que reintente |
| Llegan algunos uplinks y otros no | **Normal**: la radiosonda salta entre 8 canales y el gateway oye solo 1 (~1 de cada 8) |
| `up` sube en la pantalla pero no ves nada en ChirpStack | El gateway no llega al PC: revisa firewall (A9) e IP del PC (E5) |
| RSSI cerca de `-30` o mayor | Señal demasiado fuerte: separa más los equipos |

### E4. Apagar el banco de pruebas

- La Heltec se puede **desconectar en cualquier momento**: la configuración está guardada en su memoria.
- ChirpStack se puede dejar corriendo. Si quieres detenerlo (sin borrar nada):
  ```powershell
  cd $HOME\Documents\radiosonda_PIcaro\specs\exercises\00_chirpstack-docker
  docker compose stop
  ```
  Para volver a arrancarlo: `docker compose start` (o `docker compose up -d`).
- ⚠️ **No uses** `docker compose down -v`: la opción `-v` **borra la base de datos** (gateways, devices,
  todo lo registrado).

### E5. Cambiar la configuración del gateway

Hay **tres formas**, según lo que quieras cambiar:

**a) Cambiar la IP del servidor, el puerto o el canal (el gateway sigue en la misma WiFi).**
Desde cualquier equipo de tu red, abre en el navegador **`http://IP-DE-LA-HELTEC:8000`** (la IP de la
segunda línea de la pantalla). Cambia el campo, pulsa **configure** y después **reboot**:

![Página de configuración del gateway en la red local](https://raw.githubusercontent.com/ovelazquezj/radiosonda_PIcaro/master/specs/exercises/14_heltec-v4-gateway-1canal/img/08-gateway-pagina-8000.png)

**b) Cambiar de red WiFi (por ejemplo, llevaste el banco a otro laboratorio).**
1. Pulsa **RST** en la Heltec (o desconéctala y vuelve a conectarla).
2. Durante los **primeros 5 segundos** (mientras está encendido el LED de la placa), **pulsa una vez el botón
   PRG**.
3. La pantalla mostrará `WiFi PiCARO-GW-xxxxxx - http://192.168.4.1`: repite la **Parte C**. Los
   campos vienen con los valores actuales; si dejas la contraseña vacía y no cambias la red, se conserva
   la guardada.

**c) Automático: si la WiFi guardada no está disponible.**
Si el gateway no logra conectarse a su WiFi (10 intentos), **abre él solo la red de configuración durante
5 minutos**. Si nadie la usa, se reinicia y vuelve a intentar con la WiFi guardada. Así, si el router
estaba apagado, el gateway se recupera solo cuando vuelve.

---

## ✅ Cómo saber que funcionó

- [ ] `docker compose ps` muestra los **7 servicios `Up`** (A6).
- [ ] Entras a **<http://localhost:8080>** con `admin`/`admin` (A7).
- [ ] La pantalla de la Heltec muestra **`LoRaHub - Receiving`**, tu EUI y una IP de tu red (C5).
- [ ] En ChirpStack, **Gateways** muestra tu gateway **Online** en la región **us915_1** (D3).
- [ ] Con la radiosonda encendida, el contador **`up`** sube y ves paquetes en **LoRaWAN frames** (E3).

## 🛠️ Si algo falla

| Síntoma | Causa probable | Arreglo |
|---|---|---|
| `docker: error during connect` | Docker Desktop no está abierto | Ábrelo y espera la ballena quieta (A2) |
| `docker compose up` dice `port is already allocated` | Otro programa usa el 8080, 1700 o 1883 | Cierra ese programa o detén el otro ChirpStack (`docker ps` para verlo) |
| `no configuration file provided` | Lanzaste `docker compose` desde otra carpeta | Haz `cd ...\00_chirpstack-docker` primero (A5) |
| No aparece ningún puerto COM | Cable solo de carga | Usa un cable USB-C de datos; prueba otro puerto USB |
| `Failed to connect to ESP32-S3` | La placa no entró en modo de carga | PRG pulsado + RST + soltar PRG, y repite (B5) |
| Después de grabar, la pantalla queda en negro | El firmware no arrancó tras el modo manual | Pulsa **RST** una vez |
| La red `PiCARO-GW-...` no aparece en el celular | La placa ya tiene una WiFi guardada y entró en modo gateway | Ábrela a mano con **RST + PRG en 5 s** (E5-b) |
| La página `192.168.4.1` no abre | El celular salió de la red (no tiene internet) o usa datos móviles | Elige "mantener conexión"; apaga los datos móviles mientras configuras |
| Tu WiFi no aparece en **Redes encontradas** | Es de 5 GHz, está oculta o lejos | Usa una red de **2.4 GHz**; escribe el nombre a mano |
| Tras guardar, vuelve a salir `WiFi PiCARO-GW-...` | Contraseña o nombre de red mal escritos, o red de 5 GHz | Repite la Parte C con cuidado (mayúsculas, espacios) |
| El gateway sale **Never seen** u **Offline** en ChirpStack | Firewall bloquea UDP 1700, IP del PC equivocada, o EUI mal escrito | Revisa A9; compara IP (A8) con la del gateway (E5-a); compara EUI de la pantalla con el de ChirpStack |
| Estaba Online y pasó a **Offline** | Cambió la IP de tu PC (DHCP) o se apagó Docker | `ipconfig`; actualiza la IP en `http://IP-HELTEC:8000` (E5-a); `docker compose ps` |
| Gateway Online pero **no llega nada** de la radiosonda | SF distinto, radiosonda pegada al gateway, o simplemente cayó en otros canales | Confirma DR0/SF10 y ADR apagado (E2); sepáralos 2–3 m; espera varios envíos |
| `up` sube pero ChirpStack no muestra tramas | La pestaña *LoRaWAN frames* solo muestra lo que llega con ella abierta | Déjala abierta y espera al siguiente paquete |

## 📎 Referencia

- **Archivos de esta carpeta:**
  - [`artifacts/picaro-gw-heltec-v4_us915-fsb2.bin`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/14_heltec-v4-gateway-1canal/artifacts) — firmware listo para grabar en `0x0`
    (incluye bootloader y tabla de particiones). Suma SHA-256 en [`artifacts/SHA256SUMS.txt`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/14_heltec-v4-gateway-1canal/artifacts/SHA256SUMS.txt).
  - [`artifacts/LICENSE-one_channel_hub.txt`](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/master/specs/exercises/14_heltec-v4-gateway-1canal/artifacts/LICENSE-one_channel_hub.txt) — licencia (Clear BSD) del firmware de Semtech.
  - [`firmware/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/14_heltec-v4-gateway-1canal/firmware) — el parche sobre *One Channel Hub* v1.1.1 y la configuración de compilación,
    por si quieres recompilar (ver la memoria técnica, anexo A).
  - [`MEMORIA_TECNICA.md`](Memoria-tecnica-Gateway-Heltec-V4) — cómo está hecho el gateway, por qué y sus límites.
- **Valores por defecto del firmware:**

  | Parámetro | Valor |
  |---|---|
  | Red de configuración | `PiCARO-GW-` + 6 últimos dígitos de la MAC |
  | Contraseña de la red de configuración | `picaro1234` |
  | Página de configuración | `http://192.168.4.1` (red de configuración) · `http://IP-HELTEC:8000` (tu red) |
  | Servidor LNS / puerto | (el que pongas) / `1700` |
  | Canal | 903.9 MHz, SF10, BW 125 kHz |
  | Gateway EUI | MAC[0..2] + `FFFE` + MAC[3..5] |

- **Guías relacionadas:** [Ejercicio 00 · ChirpStack en Docker](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/00_chirpstack-docker/) ·
  [Ejercicio 07 · Gateway de 1 canal (TTGO)](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/07_esp-1ch-gateway/) ·
  [Ejercicio 12 · Join OTAA con Heltec](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/12_heltec-lora32-v3-join/)

> 📁 **Archivos del ejercicio:** [`specs/exercises/14_heltec-v4-gateway-1canal/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/14_heltec-v4-gateway-1canal) · 📐 [Memoria técnica](Memoria-tecnica-Gateway-Heltec-V4) · 📟 [Ejercicio 14](Ejercicio-14-Banco-de-pruebas-Heltec-V4)
> *El firmware del gateway se basa en One Channel Hub © Semtech (Clear BSD).*

---
_Docs © Omar Velazquez · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)_
