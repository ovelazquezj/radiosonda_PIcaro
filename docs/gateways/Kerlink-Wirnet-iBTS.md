# Manual: configurar un Kerlink Wirnet iBTS con el LNS de PiCARO

Este manual conecta un Kerlink Wirnet iBTS (banda **US915**) al LNS de PiCARO
(ChirpStack v4 en `lns.pi-caro.org`). Validado el 2026-10-08 con un iBTS del
laboratorio.

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
| `get_version: command not found` | Firmware antiguo | Saca el EUI con el paso 1.1 |
| `head: invalid option -- '6'` | El Kerlink usa BusyBox | Usa `head -n 60` |

## Comandos de referencia en el Kerlink

| Para qué | Comando |
|---|---|
| Ver el EUI | `cat /var/run/lora/gateway-id.toml` |
| Ver si corren el forwarder y la radio | `ps \| grep -i -E "lorafwd\|lorad" \| grep -v grep` |
| Ver la config del forwarder | `grep -v "^\s*#" /etc/lorafwd.toml \| grep -v "^\s*$"` |
| Reiniciar el forwarder | `/etc/init.d/lorafwd restart` |
| Ver el log del forwarder | `tail -f /var/log/messages \| grep -i lorafwd` (Ctrl+C para salir) |
