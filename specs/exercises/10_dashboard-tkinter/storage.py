"""
storage.py  -  Capa de almacenamiento SQLite de la telemetria PICARO.

Guarda un registro por uplink (la "caja negra" del lado servidor) y ofrece
consultas para los paneles, las graficas de tendencia y el track del mapa.
Se accede SOLO desde el hilo principal de tkinter (el hilo de ingesta empuja a
una cola y el hilo principal inserta), asi que SQLite queda de un solo hilo.
"""
import sqlite3
import time

# Columnas del registro (deben coincidir con las que arma mqtt_ingest/replay).
FIELDS = [
    "ts", "iso", "dev_eui", "f_cnt", "f_port", "rssi", "snr", "gateway_id",
    "freq", "dr", "gps_active", "gps_fix", "latitude", "longitude",
    "altitude_m", "satellites", "battery_v", "battery_pct", "charging",
    "usb_powered", "temperature_c", "pressure_hpa", "raw_json",
]

_SCHEMA = """
CREATE TABLE IF NOT EXISTS telemetry (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    ts            REAL NOT NULL,      -- epoch unix (segundos)
    iso           TEXT,              -- marca de tiempo ISO-8601 UTC
    dev_eui       TEXT,
    f_cnt         INTEGER,
    f_port        INTEGER,
    rssi          INTEGER,
    snr           REAL,
    gateway_id    TEXT,
    freq          INTEGER,
    dr            INTEGER,
    gps_active    INTEGER,
    gps_fix       INTEGER,
    latitude      REAL,
    longitude     REAL,
    altitude_m    REAL,
    satellites    INTEGER,
    battery_v     REAL,
    battery_pct   INTEGER,
    charging      INTEGER,
    usb_powered   INTEGER,
    temperature_c REAL,
    pressure_hpa  REAL,
    raw_json      TEXT
);
CREATE INDEX IF NOT EXISTS idx_telemetry_ts ON telemetry(ts);
"""


class TelemetryStore:
    def __init__(self, path):
        self.path = path
        self._conn = sqlite3.connect(path, check_same_thread=False)
        self._conn.row_factory = sqlite3.Row
        self._conn.executescript(_SCHEMA)
        self._conn.commit()

    def insert(self, rec: dict) -> int:
        vals = [rec.get(c) for c in FIELDS]
        placeholders = ",".join("?" * len(FIELDS))
        cur = self._conn.execute(
            f"INSERT INTO telemetry ({','.join(FIELDS)}) VALUES ({placeholders})", vals)
        self._conn.commit()
        return cur.lastrowid

    def latest(self):
        row = self._conn.execute(
            "SELECT * FROM telemetry ORDER BY ts DESC LIMIT 1").fetchone()
        return dict(row) if row else None

    def recent(self, limit=200):
        rows = self._conn.execute(
            "SELECT * FROM telemetry ORDER BY ts DESC LIMIT ?", (limit,)).fetchall()
        return [dict(r) for r in rows]

    def count(self) -> int:
        return self._conn.execute("SELECT COUNT(*) FROM telemetry").fetchone()[0]

    def series(self, since_ts):
        """Serie temporal para las graficas (bateria/temp/presion)."""
        rows = self._conn.execute(
            "SELECT ts, battery_v, temperature_c, pressure_hpa "
            "FROM telemetry WHERE ts>=? ORDER BY ts ASC", (since_ts,)).fetchall()
        return [dict(r) for r in rows]

    def track(self, since_ts):
        """Puntos con fix para el mapa: [(ts, lat, lon), ...] en orden temporal."""
        rows = self._conn.execute(
            "SELECT ts, latitude, longitude FROM telemetry "
            "WHERE ts>=? AND gps_fix=1 AND latitude IS NOT NULL "
            "AND NOT (latitude=0 AND longitude=0) ORDER BY ts ASC",
            (since_ts,)).fetchall()
        return [(r["ts"], r["latitude"], r["longitude"]) for r in rows]

    def all_records_asc(self):
        rows = self._conn.execute(
            "SELECT * FROM telemetry ORDER BY ts ASC").fetchall()
        return [dict(r) for r in rows]

    def records_since(self, since_ts):
        """Todos los registros (todas las columnas) desde since_ts, para exportar."""
        rows = self._conn.execute(
            "SELECT * FROM telemetry WHERE ts>=? ORDER BY ts ASC", (since_ts,)).fetchall()
        return [dict(r) for r in rows]

    def close(self):
        try:
            self._conn.close()
        except Exception:
            pass


def decimate(points, max_n=500):
    """Submuestrea una lista para no saturar el mapa: conserva ~max_n puntos,
    incluido SIEMPRE el ultimo (posicion actual)."""
    n = len(points)
    if n <= max_n:
        return points
    step = n / float(max_n)
    out = [points[int(i * step)] for i in range(max_n)]
    if out[-1] is not points[-1]:
        out[-1] = points[-1]
    return out
