# Ejercicio 11 — Dashboard contra un ChirpStack remoto (MQTT sobre TLS)

> **En una frase:** conectas el dashboard del [Ejercicio 10](../10_dashboard-tkinter/) a un
> ChirpStack **en Internet** en lugar del que corre en tu portátil, usando **MQTT cifrado con TLS
> y autenticación**, y entiendes por qué un broker público no puede funcionar como el del laboratorio.
> **Plataforma:** Python 3.x en Windows/Linux/macOS. **Sin hardware:** trae **modo simulador**.

> ⚠️ **Los datos de servidor de esta guía son ficticios.** `example.org` es un dominio reservado
> ([RFC 2606](https://www.rfc-editor.org/rfc/rfc2606)) que **nadie puede registrar**: si copias y
> pegas sin cambiar nada, no acabarás conectándote al servidor de un desconocido. Tu instructor te
> dará los valores reales.

## 🎯 Qué vas a conseguir

El mismo dashboard del ej.10, pero alimentado desde un servidor remoto compartido. Al arrancar en
modo `mqtt` verás en la barra de estado:

```
MQTT OK · application/<TU_APP_ID>/device/+/event/up
```

Y en el log irán apareciendo los uplinks de **tus** dispositivos —solo los tuyos, aunque en ese
servidor haya otros quince equipos trabajando a la vez.

La diferencia con el ej.10 es invisible en la pantalla y enorme por debajo: el broker ya no está en
tu máquina, va cifrado, y exige que te identifiques.

## 🧰 Antes de empezar

Marca esta lista **antes** de seguir (si te falta algo, no continúes):

- [ ] **[Ejercicio 10](../10_dashboard-tkinter/) terminado y funcionando** al menos en modo `sim`.
      Este ejercicio da por hecho que ya sabes mover el dashboard.
- [ ] **Python 3.x** (en Windows, el lanzador `py -3`). Incluye `tkinter` y `sqlite3`.
- [ ] **Credenciales del servidor**, que te da tu instructor. Son **cuatro datos**:

| Dato | Ejemplo (ficticio) | Qué es |
|---|---|---|
| Host MQTT | `mqtt.example.org` | El broker del servidor remoto |
| Usuario | `equipo01` | **Tu** usuario, distinto al de los demás equipos |
| Contraseña | `s3cr3t0-de-tu-equipo` | La tuya |
| `app_id` | `3f2a...` (un UUID) | El ID de **tu** aplicación en ChirpStack |

- [ ] **Conexión a Internet** con el **puerto 8883 de salida abierto**. Algunas redes de campus o
      corporativas lo bloquean (§ Si algo falla).

> ℹ️ **No necesitas placa ni gateway propios.** El servidor es compartido: si otro equipo tiene su
> radiosonda emitiendo, sus datos van a *su* aplicación, no a la tuya. Para ver movimiento sin
> hardware, usa el modo `sim`.

## 📟 Hardware y conexiones

**Ninguno.** Este ejercicio es todo software: tu portátil contra un servidor en Internet.

```
   Tu portátil                    Internet                 Servidor remoto
  ┌───────────────┐                                      ┌──────────────────┐
  │  dashboard.py │═══ MQTT sobre TLS (8883) ═══════════▶│    Mosquitto     │
  │               │    usuario + contraseña              │        │         │
  │   SQLite      │                                      │        ▼         │
  └───────────────┘                                      │   ChirpStack     │
                                                         └──────────────────┘
                                                                  ▲
                                                       radiosondas │ vía gateway
```

## 🪜 Paso a paso

### 1. Instala las dependencias

Este ejercicio trae su **propia copia completa** del dashboard, para que no tengas que ir saltando
entre carpetas. Es independiente del ej.10.

```bash
# desde specs/exercises/11_dashboard-servidor-remoto/
py -3 -m pip install -r requirements.txt
```

**Salida esperada:**
```
Successfully installed paho-mqtt-2.x.x matplotlib-3.x.x tkintermapview-1.x.x
```
(o `Requirement already satisfied` si ya las tenías del ej.10).

### 2. Crea tu `config.json`

```bash
# desde specs/exercises/11_dashboard-servidor-remoto/
cp config.example.json config.json
```

> En Windows sin bash: `copy config.example.json config.json`

`config.json` está en el `.gitignore`: **tus credenciales nunca se suben al repositorio.** Esa es la
razón de que exista `config.example.json` como plantilla.

### 3. Rellena tus cuatro datos

Abre `config.json` y sustituye **solo** estos campos con lo que te dio tu instructor:

```json
{
  "mode": "mqtt",
  "mqtt": {
    "host": "mqtt.example.org",          ← el host que te dieron
    "port": 8883,
    "tls": true,
    "username": "equipo01",              ← TU usuario
    "password": "PON_AQUI_TU_CONTRASENA" ← TU contraseña
  },
  "app_id": "00000000-0000-0000-0000-000000000000",   ← TU app_id
  ...
}
```

⚠️ **No cambies `"port": 8883` ni `"tls": true`.** El puerto 1883 (MQTT en claro) **no está abierto**
en un servidor de este tipo, y con razón: por él viajarían tu usuario y tu contraseña en texto plano.

### 4. Arranca en modo `mqtt`

```bash
# desde specs/exercises/11_dashboard-servidor-remoto/
py -3 dashboard.py --mode mqtt
```

**Salida esperada** (en la barra de estado del dashboard, abajo):
```
MQTT OK · application/3f2a.../device/+/event/up
```

Si en su lugar ves `MQTT rc=...`, ve a **§ Si algo falla**: el mensaje dice exactamente qué falló.

### 5. Comprueba que estás aislado del resto

Con el dashboard conectado, fíjate en que **solo llegan uplinks de tus dispositivos**. No es
casualidad ni suerte: el servidor tiene una **ACL** (lista de control de acceso) que ata tu usuario
a tu aplicación.

Puedes comprobarlo tú mismo. Cambia temporalmente en `config.json` el `app_id` por el de otro equipo
(pídeselo, no pasa nada) y reinicia:

**Salida esperada:** el dashboard conecta, pero **no llega ni un solo mensaje**. El broker acepta tu
suscripción y simplemente no te entrega nada de esa aplicación.

> Devuelve tu `app_id` a su valor antes de seguir.

Esto es lo que hace que quince equipos puedan compartir un servidor sin pisarse: cada uno tiene
credenciales propias y **solo lee lo suyo**.

## ✅ Cómo saber que funcionó

- [ ] La barra de estado muestra `MQTT OK · application/<tu app_id>/device/+/event/up`
- [ ] Con el `app_id` de otro equipo, conecta pero **no llega nada** (paso 5)
- [ ] Si hay una radiosonda emitiendo a tu aplicación, los paneles se actualizan y el log avanza
- [ ] Se crea `picaro_telemetry.db` y `--mode replay` reproduce lo capturado
- [ ] `git status` **no** muestra tu `config.json` como archivo nuevo

## 🛠️ Si algo falla

| Síntoma | Causa probable | Arreglo |
|---|---|---|
| `MQTT rc=... not authorised` | Usuario o contraseña mal escritos | Revísalos; ojo con espacios al copiar y pegar |
| Conecta, pero **no llega nada** | `app_id` equivocado, o no hay uplinks ahora mismo | Confirma tu `app_id`. Si nadie está emitiendo, es lo normal: prueba `--mode sim` |
| `ssl.SSLCertVerificationError` | Reloj del sistema desfasado, o inspección TLS de la red | Pon el reloj en hora. En redes corporativas con proxy que inspecciona TLS, prueba desde otra red |
| Se queda en `conectando a ...` | Puerto **8883 de salida bloqueado** por el firewall de tu red | Prueba desde otra red (p. ej. datos del móvil) para confirmarlo |
| `ConnectionRefusedError` | Host mal escrito, o el servidor caído | Revisa `host`. Si es correcto, avisa al instructor |
| `ModuleNotFoundError` | Faltan dependencias | `py -3 -m pip install -r requirements.txt` |
| `object` vacío en los datos | Falta el **codec** en el device profile del servidor | Es cosa del servidor: avisa al instructor |

> 🔎 **Cómo leer el error.** `not authorised` es el broker diciendo *"sé quién dices ser, pero no te
> dejo"*: es de **credenciales**. Un error de `SSLCertVerification` ocurre **antes**, al negociar el
> cifrado: es de **TLS**. Distinguirlos te ahorra media hora.

## 📤 Los datos (payload)

Idénticos al ej.10 — no cambia el formato, solo por dónde llega. El dashboard consume el campo
`object` que produce el codec del device profile:

```
gps_active, gps_fix, latitude, longitude, altitude_m, satellites,
battery_v, battery_pct, charging, usb_powered, temperature_c, pressure_hpa
```

Topic MQTT de ChirpStack v4:
```
application/<APP_ID>/device/<DevEUI|+>/event/up
```

## 🧩 Apéndice — Qué debe tener el servidor

*Para instructores, o si te pica la curiosidad. **No hace falta para completar el ejercicio.***

Este ejercicio no cubre montar el servidor, pero sí conviene saber qué hay al otro lado. Un
ChirpStack expuesto a Internet necesita, como mínimo:

| Requisito | Por qué |
|---|---|
| **Dominio propio y certificado TLS** | Sin nombre de dominio no hay certificado válido, y sin certificado el TLS no verifica |
| **MQTT solo en 8883, con TLS** | El 1883 en claro no debe exponerse: las credenciales viajarían legibles |
| **Un usuario MQTT por equipo** | Credenciales compartidas son imposibles de revocar individualmente |
| **ACL por aplicación** | Es lo que impide que un equipo lea —o escriba— en los datos de otro |
| **Un tenant por equipo** | Aísla también la consola web: nadie borra por error el trabajo ajeno |
| **Puertos de gateway abiertos** | `1700/udp` (packet forwarder) o WebSocket para Basics Station |

Dos detalles que sorprenden al montarlo:

- **La consola web y el broker MQTT son cosas distintas** y llevan credenciales distintas. Tu usuario
  de la web no sirve para el MQTT.
- **El servidor no guarda un histórico de payloads.** ChirpStack publica cada uplink por MQTT y se
  olvida. Por eso el dashboard tiene su propia SQLite: **el histórico lo construyes tú**, en el lado
  del consumidor. Si tu dashboard no está corriendo, esos datos no los recupera nadie.

## ➡️ Navegación

- ⬅️ Anterior: [Ejercicio 10 · Dashboard Mission Control](../10_dashboard-tkinter/)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia

- **Configuración:** [`config.example.json`](config.example.json) — cópialo a `config.json` (ignorado por git)
- **Archivos de esta carpeta:**
```
dashboard.py     app principal (UI tkinter + loops)
mqtt_ingest.py   ingesta MQTT con TLS + autenticación, y parser de eventos ChirpStack v4
storage.py       capa SQLite (esquema + consultas)
theme.py         paleta, fuentes y estilos "control de misión"
replay.py        modo replay (desde SQLite) y simulador
selftest.py      prueba headless de la capa de datos
config.example.json  plantilla SIN credenciales
```
- **Guía común:** [ChirpStack API (REST · MQTT · codec)](../COMMON_CHIRPSTACK_API.md)

---
> 💻 **Código** de este dashboard bajo **licencia MIT** — ver [`LICENSE`](LICENSE).
> 📄 Material educativo (esta guía) bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
