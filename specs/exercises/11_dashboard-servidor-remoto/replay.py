"""
replay.py  -  Fuentes de datos sin gateway: REPLAY y SIMULADOR.

- ReplayIngest: reproduce los registros ya guardados en la SQLite, en orden y a
  velocidad acelerada. Util para "reproducir" una sesion pasada.
- SimIngest: genera telemetria sintetica creible (bateria que baja, temp/presion
  con variacion suave, y un GPS que se MUEVE) para desarrollar/mostrar el
  dashboard cuando la placa esta lejos del gateway.

Ambas imitan la misma interfaz que MqttIngest: start()/stop() y entregan cada
registro por el callback on_record desde un hilo aparte.
"""
import math
import random
import threading
import time
from datetime import datetime, timezone

from mqtt_ingest import _b


def _now_rec_base():
    ts = time.time()
    return ts, datetime.fromtimestamp(ts, timezone.utc).isoformat(timespec="seconds")


class ReplayIngest:
    def __init__(self, store, on_record, on_status, period_s=1.0):
        self.store = store
        self.on_record = on_record
        self.on_status = on_status
        self.period_s = period_s
        self._stop = threading.Event()
        self._thr = None

    def start(self):
        rows = self.store.all_records_asc()
        if not rows:
            self.on_status(False, "REPLAY: la base de datos esta vacia")
            return
        self._thr = threading.Thread(target=self._run, args=(rows,), daemon=True)
        self._thr.start()
        self.on_status(True, f"REPLAY: {len(rows)} registros")

    def _run(self, rows):
        for r in rows:
            if self._stop.is_set():
                return
            self.on_record(dict(r))
            time.sleep(self.period_s)
        self.on_status(True, "REPLAY: fin")

    def stop(self):
        self._stop.set()


class SimIngest:
    """Simulador: telemetria sintetica con GPS en movimiento."""

    def __init__(self, on_record, on_status, period_s=2.0,
                 base_lat=21.9337, base_lon=-102.2983):
        self.on_record = on_record
        self.on_status = on_status
        self.period_s = period_s
        self.base_lat = base_lat
        self.base_lon = base_lon
        self._stop = threading.Event()
        self._thr = None

    def start(self):
        self._thr = threading.Thread(target=self._run, daemon=True)
        self._thr.start()
        self.on_status(True, "SIMULADOR activo (datos sinteticos)")

    def stop(self):
        self._stop.set()

    def _run(self):
        fcnt = 0
        batt_pct = 92.0
        lat, lon = self.base_lat, self.base_lon
        heading = random.uniform(0, 2 * math.pi)
        while not self._stop.is_set():
            fcnt += 1
            # GPS: caminata con deriva suave (dibuja una trayectoria en el mapa)
            heading += random.uniform(-0.4, 0.4)
            step = 0.00025                      # ~25 m por paso
            lat += step * math.cos(heading)
            lon += step * math.sin(heading)
            fix = random.random() > 0.05        # 95% con fix
            batt_pct = max(0.0, batt_pct - random.uniform(0.0, 0.15))
            batt_v = 3.3 + (batt_pct / 100.0) * 0.9   # 3.3-4.2 V
            temp = 26.0 + 2.0 * math.sin(fcnt / 25.0) + random.uniform(-0.3, 0.3)
            press = 815.0 + 3.0 * math.sin(fcnt / 60.0) + random.uniform(-0.4, 0.4)
            hum = 52.0 + 5.0 * math.sin(fcnt / 40.0)

            ts, iso = _now_rec_base()
            rec = {
                "ts": ts, "iso": iso,
                "dev_eui": "70b3d57ed0090901",
                "f_cnt": fcnt, "f_port": 10,
                "rssi": int(-45 - random.uniform(0, 40)),
                "snr": round(random.uniform(-2, 11), 1),
                "gateway_id": "sim-gateway-0001",
                "freq": 904300000, "dr": random.choice([1, 2, 3]),
                "gps_active": 1,
                "gps_fix": _b(fix),
                "latitude": round(lat, 6) if fix else 0.0,
                "longitude": round(lon, 6) if fix else 0.0,
                "altitude_m": round(1870 + random.uniform(-5, 5)) if fix else 0,
                "satellites": random.randint(5, 11) if fix else 0,
                "battery_v": round(batt_v, 3),
                "battery_pct": int(batt_pct),
                "charging": 0,
                "usb_powered": 1,
                "temperature_c": round(temp, 2),
                "pressure_hpa": round(press, 1),
                "raw_json": "",
            }
            self.on_record(rec)
            time.sleep(self.period_s)
