# Manual: configurar un Kerlink Wirnet iBTS con el LNS de PiCARO

Este manual conecta un Kerlink Wirnet iBTS (banda **US915**) al LNS de PiCARO
(ChirpStack v4 en `lns.pi-caro.org`). Validado el 2026-10-08 con un iBTS del
laboratorio con **KerOS 5.11.0**.

```
radio (lorad) → lorafwd → UDP lns.pi-caro.org:1700 → ChirpStack
```

El Kerlink usa su packet forwarder de fábrica (`lorafwd`) y manda los
paquetes directo al servidor. **No se instala nada en el gateway.**

> **No uses la guía de ChirpStack para Kerlink**
> (<https://www.chirpstack.io/docs/gateway-bridge/install/kerlink.html>).
> Instala dentro del gateway el ChirpStack Gateway Bridge **3.14**, y
> ChirpStack v4 descarta sus paquetes con
> `tx_info of uplink event is empty, skipping`.

> **Seguridad.** El protocolo UDP de Semtech no cifra el tráfico ni autentica
> al gateway: el servidor lo identifica solo por el EUI que viaja en cada
> paquete. El contenido de los sensores sigue protegido por el cifrado propio
> de LoRaWAN, pero alguien que conozca el EUI podría suplantar al gateway y
> quedarse con sus downlinks. Es aceptable para uso escolar. Para uso real,
> conecta el gateway con TLS (ver el [Anexo](#anexo-migrar-a-basics-station-con-tls-mutuo)).
> **No publiques el EUI** de un gateway en repositorios ni documentos públicos.

## Requisitos

- Acceso SSH al Kerlink como `root`.
- Usuario con permisos de administración en la consola de ChirpStack.
- Salida a Internet desde el gateway hacia `lns.pi-caro.org`, puerto **UDP
  1700**.

---

## Paso 1. Obtener los datos del gateway

Entra al Kerlink:

```bash
ssh root@<IP-DEL-GATEWAY>
```

### 1.1 EUI

```bash
cat /var/run/lora/gateway-id.toml
```

```
gateway.id = 0x7076FFAABBCCDDEE
```

El EUI son los 16 caracteres después de `0x`, en minúsculas (en el ejemplo,
`7076ffaabbccddee`). Anótalo; lo necesitas en el paso 2.

> No supongas el EUI a partir de otro Kerlink: no todos empiezan por `7076FF`.
> Cópialo siempre del archivo.

### 1.2 Archivo de configuración del forwarder

```bash
ps | grep -i -E "lorafwd|lorad" | grep -v grep
```

```
lorafwd /var/run/lora/gateway-id.toml /etc/lorafwd.toml -vv ...
lorad-ibts -s 0xaabbccdd -g /dev/nmea1 -vv /etc/lorad/lorad.json ...
```

El archivo que va después de `gateway-id.toml` en la línea de `lorafwd` es el
que hay que editar en el paso 3. En el iBTS es **`/etc/lorafwd.toml`**.

### 1.3 Frecuencias de la radio

```bash
grep -i -E "freq|enable" /etc/lorad/lorad.json | head -n 60
```

Comprueba que haya canales habilitados (`"enable": true`) en
**903.9, 904.1, 904.3, 904.5, 904.7, 904.9, 905.1, 905.3 y 904.6 MHz**. Son
los de la subbanda 2 de US915, la que usa PiCARO. El iBTS de fábrica ya los
trae.

> El Kerlink usa BusyBox: escribe `head -n 60`, no `head -60`.

### 1.4 Versión de firmware

```bash
opkg list-installed | grep -i keros
```

```
keros - 5.11.0-0-g38de666d
```

Este manual está validado con **KerOS 5.11.0**.

---

## Paso 2. Registrar el gateway en ChirpStack

En la consola web de ChirpStack:

1. Entra a **Tenants → Proyecto PiCARO → Gateways → Add gateway**.
2. Llena el formulario:

   | Campo | Valor |
   |---|---|
   | Gateway ID | El EUI del paso 1.1 |
   | Name | Un nombre descriptivo, p. ej. `kerlink-ibts` |
   | Stats interval (secs) | `30` |

3. Guarda.

> Regístralo **siempre en el tenant Proyecto PiCARO**. Ese tenant comparte
> sus gateways con los dispositivos de todos los equipos; en otro tenant, solo
> lo usarían los dispositivos de ese tenant.

> Si sale `Max number of gateways exceeded for tenant`, sube el límite en
> **Tenants → Proyecto PiCARO → editar → Max. gateway count**, o borra un
> gateway que no se use.

---

## Paso 3. Configurar el forwarder

En el archivo del paso 1.2, la sección `[ gwmp ]` debe quedar así:

```
[ gwmp ]
node = "lns.pi-caro.org"
service.uplink = 1700
service.downlink = 1700
period.heartbeat = 60
period.statistics = 30
```

Para aplicar los cambios sin abrir un editor:

```bash
sed -i 's/^node = .*/node = "lns.pi-caro.org"/' /etc/lorafwd.toml
sed -i 's/^period.statistics = .*/period.statistics = 30/' /etc/lorafwd.toml
grep -E "^node|^service|^period" /etc/lorafwd.toml
```

| Parámetro | Por qué |
|---|---|
| `node` | Dirección del LNS. De fábrica viene `172.17.0.1`, que no lleva a ningún lado |
| `service.uplink` / `service.downlink` | **1700** es el puerto de US915 (el 1701 es de EU868). Si usas el puerto de la otra banda, ChirpStack descarta los paquetes |
| `period.statistics` | Debe coincidir con el *Stats interval* del paso 2 (30 s). De fábrica viene 300, y con eso el gateway aparece offline entre un reporte y otro |

Reinicia el forwarder:

```bash
/etc/init.d/lorafwd restart
```

---

## Paso 4. Verificar

1. **ChirpStack → Gateways:** en menos de un minuto el gateway debe aparecer
   **Online** y quedarse así.
2. **ChirpStack → el gateway → LoRaWAN frames:** con la pestaña abierta,
   espera a que transmita un sensor. La pestaña muestra los frames en vivo y
   **no guarda historial**.
3. **ChirpStack → un dispositivo cercano → Events → up:** en `rxInfo` debe
   aparecer el EUI del Kerlink con su `rssi` y `snr`.

**Antena:** ponla vertical y lo más alta posible, lejos de metal.

---

## Solución de problemas

| Síntoma | Causa | Solución |
|---|---|---|
| El gateway nunca aparece Online | `node` mal escrito, o el UDP 1700 está bloqueado en la red del sitio | Revisa el paso 3 y el log: `tail -f /var/log/messages \| grep -i lorafwd` |
| Aparece y desaparece (Online/Offline) | `period.statistics` distinto del *Stats interval* de ChirpStack | Que los dos valgan 30 (pasos 2 y 3) |
| Online, pero ningún dispositivo lo muestra en `rxInfo` | La radio no escucha la subbanda 2, o los sensores están lejos o la antena está mal | Revisa el paso 1.3 y la antena |
| En el log de ChirpStack: `tx_info of uplink event is empty, skipping` | Hay un ChirpStack Gateway Bridge 3.x instalado en el Kerlink y `lorafwd` le está mandando los datos | Apágalo con `monit stop chirpstack-gateway-bridge` y aplica el paso 3 |
| `get_version: command not found` | Ese comando no existe en KerOS 5.11 | Saca el EUI con el paso 1.1 y la versión con el paso 1.4 |
| `head: invalid option -- '6'` | El Kerlink usa BusyBox | Usa `head -n 60` |

## Comandos de referencia en el Kerlink

| Para qué | Comando |
|---|---|
| Ver el EUI | `cat /var/run/lora/gateway-id.toml` |
| Ver si corren el forwarder y la radio | `ps \| grep -i -E "lorafwd\|lorad" \| grep -v grep` |
| Ver la config del forwarder | `grep -v "^\s*#" /etc/lorafwd.toml \| grep -v "^\s*$"` |
| Reiniciar el forwarder | `/etc/init.d/lorafwd restart` |
| Ver el log del forwarder | `tail -f /var/log/messages \| grep -i lorafwd` (Ctrl+C para salir) |
| Ver la versión de KerOS | `opkg list-installed \| grep -i keros` |

---

## Anexo: migrar a Basics Station con TLS mutuo

> **Estado: pendiente de validar.** El KerOS 5.11.0 del iBTS no trae Basics
> Station ni lo ofrece por `opkg` (comprobado el 2026-10-08). Estos pasos
> siguen la documentación de Kerlink y el montaje que ya funciona con el
> SenseCAP M2 en el mismo LNS. Antes de usar cada herramienta de Kerlink,
> comprueba sus opciones con `--help` en el equipo.

Con Basics Station el gateway se conecta al LNS por **WebSocket con TLS
mutuo**:

| | UDP (este manual) | Basics Station |
|---|---|---|
| Tráfico cifrado | No | Sí (TLS) |
| El servidor comprueba la identidad del gateway | No, solo el EUI | Sí, con un certificado emitido para ese EUI |
| Reintentos si se pierde un paquete | No (UDP) | Sí (TCP) |

```
radio (lorad) → Basics Station → wss://lns.pi-caro.org:3001 (TLS mutuo) → ChirpStack
```

El registro en ChirpStack (paso 2) **no cambia**: mismo EUI, mismo tenant.

### A.1 Conseguir el paquete

Comprueba que el equipo no lo tenga ya:

```bash
opkg list 2>/dev/null | grep -i station
which klk_bs_config
```

Si no sale nada, pide el paquete **Kerlink Basic Station** a Kerlink (portal
de clientes o soporte), indicando el modelo (**Wirnet iBTS**) y la versión de
firmware del paso 1.4. Como referencia, Kerlink indica para KerOS 5 la
combinación Basic Station 3.4.1 con lorad 2.5.1.

### A.2 Instalar el paquete

Con el mecanismo estándar de KerOS para instalar paquetes:

```bash
cd /user/.updates
# copia aquí el .ipk que te dio Kerlink (por ejemplo, con scp desde tu PC)
sync
kerosd -u
reboot
```

Al volver a arrancar, comprueba que se instaló:

```bash
which klk_bs_config
ls /etc/station/
```

### A.3 Obtener los certificados

Pide al administrador del LNS los tres archivos **de este gateway**:

| Archivo | Qué es |
|---|---|
| `tc.trust` | Certificado de la CA del LNS de PiCARO. Con él, el gateway valida al servidor |
| `tc.crt` | Certificado del gateway, emitido con su EUI |
| `tc.key` | Clave privada del gateway |

> - Cada juego de certificados sirve **solo para un gateway**: el servidor
>   comprueba que el EUI del certificado coincida con el del gateway.
> - `tc.key` es un secreto. No lo subas a repositorios ni lo envíes por
>   canales públicos.

### A.4 Configurar Basics Station

Copia los certificados al gateway y protege la clave:

```bash
cp tc.trust tc.crt tc.key /etc/station/
chmod 600 /etc/station/tc.key
```

Configura la dirección del LNS con la herramienta de Kerlink:

```bash
klk_bs_config --enable --lns-uri "wss://lns.pi-caro.org:3001"
```

- **3001** es el puerto de US915; el 3002 es el de EU868.
- Usa `wss://` (con TLS), no `ws://`. El LNS de PiCARO no acepta conexiones
  sin certificado de cliente.

### A.5 Desactivar `lorafwd`

Basics Station sustituye a `lorafwd`. Los dos no deben quedar activos a la
vez; `lorad` sí debe seguir corriendo:

```bash
klk_apps_config --deactivate-lorafwd
ps | grep -i -E "lorafwd|lorad|station" | grep -v grep
```

En el `ps` deben aparecer `lorad` y el proceso de Basics Station, pero no
`lorafwd`.

### A.6 Verificar

Igual que en el paso 4: el gateway debe aparecer **Online** en ChirpStack, y
su EUI debe salir en el `rxInfo` de los dispositivos cercanos.

Log de Basics Station en el gateway:

```bash
tail -f /var/log/messages | grep -i station
```

| Síntoma | Causa probable |
|---|---|
| Errores de TLS o *handshake* en el log | Falta alguno de los tres certificados, o son de otro gateway |
| Conecta, pero el gateway no aparece Online | El EUI registrado en ChirpStack no coincide con el del certificado |

### A.7 Volver a UDP si algo falla

```bash
klk_bs_config --disable          # comprueba el nombre exacto con --help
klk_apps_config --activate-lorafwd
```

Y repite el paso 3 de este manual.
