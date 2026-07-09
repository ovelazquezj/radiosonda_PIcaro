#!/usr/bin/env python3
"""
Consumo de datos de ChirpStack para el ejercicio 01 (Periodical Uplink).

Dos modos (base para dashboards):
  --state <devEui>   Estado del device + métricas de enlace por REST API.
  --stream           Suscripción MQTT a los uplinks de la aplicación (payloads en vivo).

Config por variables de entorno (ver ../COMMON_CHIRPSTACK_API.md):
  TOKEN  API  APP        (REST)
  MQTT_HOST (def. localhost)  MQTT_PORT (def. 1883)

REST usa solo la librería estándar. El modo --stream requiere paho-mqtt:
  pip install paho-mqtt
"""
import argparse, json, os, sys, urllib.request, urllib.error
from datetime import datetime, timedelta, timezone

API   = os.environ.get("API", "http://localhost:8090")
TOKEN = os.environ.get("TOKEN", "")
APP   = os.environ.get("APP", "5bc22cfa-de6e-4c61-9567-3fc8cfe35ec7")


def _get(path):
    req = urllib.request.Request(API + path, headers={"Authorization": "Bearer " + TOKEN})
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            return json.load(r)
    except urllib.error.HTTPError as e:
        print(f"HTTP {e.code}: {e.read().decode(errors='ignore')}", file=sys.stderr)
        sys.exit(1)


def cmd_state(dev_eui):
    if not TOKEN:
        sys.exit("Exporta TOKEN con tu API key.")
    d = _get(f"/api/devices/{dev_eui}")
    dev = d.get("device", {})
    print(f"Device {dev_eui}")
    print(f"  nombre     : {dev.get('name')}")
    print(f"  last seen  : {d.get('lastSeenAt')}")
    st = d.get("deviceStatus") or {}
    print(f"  battery    : {st.get('batteryLevel')}   margin: {st.get('margin')}")
    # Métricas de enlace de las últimas 24 h (RFC3339 con sufijo Z; %2B evita que '+' pase a espacio)
    fmt = "%Y-%m-%dT%H:%M:%SZ"
    end = datetime.now(timezone.utc).strftime(fmt)
    start = (datetime.now(timezone.utc) - timedelta(days=1)).strftime(fmt)
    lm = _get(f"/api/devices/{dev_eui}/link-metrics"
              f"?start={start}&end={end}&aggregation=HOUR")
    print("  link-metrics (24h):")
    print("   ", json.dumps(lm, indent=2)[:800])


def cmd_stream():
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        sys.exit("Falta paho-mqtt.  pip install paho-mqtt   (o usa ./scripts/subscribe.sh)")
    host = os.environ.get("MQTT_HOST", "localhost")
    port = int(os.environ.get("MQTT_PORT", "1883"))
    topic = f"application/{APP}/device/+/event/up"

    def on_connect(c, u, flags, rc, *a):
        print(f"MQTT conectado ({host}:{port}) -> {topic}")
        c.subscribe(topic)

    def on_message(c, u, msg):
        try:
            p = json.loads(msg.payload.decode())
        except Exception:
            print(msg.topic, msg.payload[:120]); return
        dev = p.get("deviceInfo", {}).get("devEui", "?")
        fport = p.get("fPort")
        obj = p.get("object")          # decodificado por el codec (si lo hay)
        data = p.get("data")           # payload crudo base64
        fcnt = p.get("fCnt")
        print(f"[{dev}] fCnt={fcnt} fPort={fport} object={obj} raw={data}")

    cli = mqtt.Client()
    cli.on_connect = on_connect
    cli.on_message = on_message
    cli.connect(host, port, 30)
    print("Escuchando uplinks (Ctrl-C para salir)...")
    cli.loop_forever()


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Consumo ChirpStack (ejercicio 01)")
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--state", metavar="DEVEUI", help="estado + link-metrics por REST")
    g.add_argument("--stream", action="store_true", help="stream de uplinks por MQTT")
    a = ap.parse_args()
    if a.state:
        cmd_state(a.state)
    else:
        cmd_stream()
