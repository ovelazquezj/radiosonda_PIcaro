"""
mqtt_ingest.py  -  Ingesta de uplinks de ChirpStack por MQTT (paho-mqtt 2.x).

Se suscribe al topic de eventos 'up' de la aplicacion en ChirpStack y convierte
cada evento en un registro plano (parse_chirpstack_up). Corre el loop de paho en
un hilo aparte y entrega cada registro por callback; el hilo principal de la UI
lo mete a SQLite y refresca los paneles (tkinter NO es thread-safe).

Topic ChirpStack v4:  application/<APP_ID>/device/<DevEUI|+>/event/up
"""
import json
import time
from datetime import datetime, timezone

import paho.mqtt.client as mqtt


def _b(v):
    """bool/None -> 1/0/None (para columnas enteras de SQLite)."""
    return None if v is None else (1 if v else 0)


def parse_chirpstack_up(payload: dict) -> dict:
    """Convierte un evento 'up' de ChirpStack v4 en un registro plano."""
    obj = payload.get("object") or {}
    rx_list = payload.get("rxInfo") or []
    rx0 = rx_list[0] if rx_list else {}
    tx = payload.get("txInfo") or {}
    di = payload.get("deviceInfo") or {}

    t = payload.get("time")
    try:
        ts = datetime.fromisoformat(t.replace("Z", "+00:00")).timestamp() if t else time.time()
    except Exception:
        ts = time.time()

    return {
        "ts": ts,
        "iso": datetime.fromtimestamp(ts, timezone.utc).isoformat(timespec="seconds"),
        "dev_eui": di.get("devEui") or payload.get("devEUI") or payload.get("devEui"),
        "f_cnt": payload.get("fCnt"),
        "f_port": payload.get("fPort"),
        "rssi": rx0.get("rssi"),
        "snr": rx0.get("snr", rx0.get("loRaSNR")),
        "gateway_id": rx0.get("gatewayId") or rx0.get("gatewayID"),
        "freq": tx.get("frequency"),
        "dr": payload.get("dr"),
        "gps_active": _b(obj.get("gps_active")),
        "gps_fix": _b(obj.get("gps_fix")),
        "latitude": obj.get("latitude"),
        "longitude": obj.get("longitude"),
        "altitude_m": obj.get("altitude_m"),
        "satellites": obj.get("satellites"),
        "battery_v": obj.get("battery_v"),
        "battery_pct": obj.get("battery_pct"),
        "charging": _b(obj.get("charging")),
        "usb_powered": _b(obj.get("usb_powered")),
        "temperature_c": obj.get("temperature_c"),
        "pressure_hpa": obj.get("pressure_hpa"),
        "raw_json": json.dumps(payload, ensure_ascii=False),
    }


class MqttIngest:
    """Fuente de datos en vivo desde ChirpStack por MQTT."""

    def __init__(self, host, port, app_id, dev_eui, on_record, on_status):
        self.host = host
        self.port = int(port)
        self.on_record = on_record          # callback(rec)   (desde hilo MQTT)
        self.on_status = on_status          # callback(bool, str)
        dev = dev_eui if dev_eui else "+"
        self.topic = f"application/{app_id}/device/{dev}/event/up"
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

    def start(self):
        try:
            self.client.connect_async(self.host, self.port, keepalive=30)
            self.client.loop_start()
            self.on_status(False, f"conectando a {self.host}:{self.port} ...")
        except Exception as e:
            self.on_status(False, f"error MQTT: {e}")

    def stop(self):
        try:
            self.client.loop_stop()
            self.client.disconnect()
        except Exception:
            pass

    # --- callbacks paho 2.x ---
    def _on_connect(self, client, userdata, flags, reason_code, properties=None):
        if int(reason_code) == 0:
            client.subscribe(self.topic)
            self.on_status(True, f"MQTT OK · {self.topic}")
        else:
            self.on_status(False, f"MQTT rc={reason_code}")

    def _on_disconnect(self, client, userdata, *args):
        self.on_status(False, "MQTT desconectado")

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
            self.on_record(parse_chirpstack_up(payload))
        except Exception:
            pass   # ignora mensajes que no sean un uplink JSON valido
