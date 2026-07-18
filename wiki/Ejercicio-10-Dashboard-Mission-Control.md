# Ejercicio 10 · Dashboard Mission Control (tkinter)

Una **consola de escritorio** (Python + **tkinter**) estilo **control de misión** que **consume** la
telemetría del [Ejercicio 09](Ejercicio-09-Radiosonda-PICARO-Full) desde ChirpStack, la **guarda en
SQLite** y la muestra con paneles, gráficas de tendencia, un **mapa real con el track** y un log de
eventos. El paso a paso y la configuración están en el
[`README.md` del ejercicio](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/main/specs/exercises/10_dashboard-tkinter).

## 🔗 De dónde salen los datos
```
placa (ej.09) → gateway → ChirpStack ──MQTT :1883──► dashboard (tkinter)
     application/<app>/device/<devEui>/event/up  ─► object decodificado (codec del ej.09)
             + rxInfo (rssi/snr/gateway) + fCnt  ─► SQLite ─► paneles + gráficas + mapa
```
Usa el **mismo `object`** que produce el codec `decoder.js` del ej.09 (`gps_active, gps_fix, latitude,
longitude, altitude_m, satellites, battery_v, battery_pct, charging, usb_powered, temperature_c,
pressure_hpa`). Los payloads llegan por **MQTT** (el REST de ChirpStack no guarda histórico de payloads;
ver [ChirpStack: API/MQTT/codec](https://github.com/ovelazquezj/radiosonda_PIcaro/blob/main/specs/exercises/COMMON_CHIRPSTACK_API.md)).

## 🖥️ Tres modos (con o sin hardware)
- **`sim`** — telemetría sintética con **GPS en movimiento**: prueba el dashboard **sin gateway ni placa**.
- **`mqtt`** — en vivo desde ChirpStack.
- **`replay`** — reproduce lo ya guardado en la SQLite.

```
py -3 -m pip install -r requirements.txt
py -3 dashboard.py --mode sim
```

## 🗺️ Mapa + track + ventana de tiempo
El mapa (OpenStreetMap) une los puntos **con fix** con una polilínea (marcadores de inicio y posición
actual). El filtro **Ventana** (`15 min · 1 h · 6 h · 24 h · Todo`) controla cuánto historial se dibuja.
**Recomendado: 1 h** (~120 puntos a 30 s/uplink). Para no saturar, el track se **decima** a ~500 puntos
máx., conservando la posición actual.

## 🎨 Look & Feel legible
Alto contraste (fondo casi negro, texto casi blanco, acentos verde/cian/ámbar/rojo) y fuentes claras
(mono para valores). El estado **no depende solo del color** (siempre hay texto), pensado para leerse
con distinta luz y para daltonismo.

## 🗃️ Base de datos
Tabla `telemetry` en SQLite (una fila por uplink): metadatos de enlace + todos los campos del `object` +
`raw_json`. Se puede consultar con cualquier herramienta SQLite o reproducir con `--mode replay`.

---
- 🏠 [Índice de la Wiki](Home) · ⬅️ [Ejercicio 09](Ejercicio-09-Radiosonda-PICARO-Full)
- 📂 [Código del ejercicio 10](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/main/specs/exercises/10_dashboard-tkinter)

_Docs © Omar Velazquez · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)_
