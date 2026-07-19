"""Prueba headless de la capa de datos (sin GUI)."""
import os, tempfile, time, json
from storage import TelemetryStore, decimate, FIELDS
from mqtt_ingest import parse_chirpstack_up
from replay import SimIngest

print("== 1) parser ChirpStack ==")
sample = {
    "deviceInfo": {"devEui": "70b3d57ed0090901"},
    "fCnt": 1042, "fPort": 10, "dr": 3,
    "time": "2026-07-17T19:52:03+00:00",
    "txInfo": {"frequency": 904300000},
    "rxInfo": [{"gatewayId": "abcd1234", "rssi": -47, "snr": 9.5}],
    "object": {"gps_active": True, "gps_fix": True, "latitude": 21.933677,
               "longitude": -102.298267, "altitude_m": 1872, "satellites": 7,
               "battery_v": 4.1, "battery_pct": 89, "charging": False,
               "usb_powered": True, "temperature_c": 26.12, "pressure_hpa": 815.5},
}
rec = parse_chirpstack_up(sample)
assert rec["dev_eui"] == "70b3d57ed0090901"
assert rec["f_cnt"] == 1042 and rec["f_port"] == 10
assert rec["rssi"] == -47 and rec["snr"] == 9.5
assert rec["temperature_c"] == 26.12 and rec["pressure_hpa"] == 815.5
assert rec["gps_fix"] == 1 and rec["gps_active"] == 1
assert abs(rec["latitude"] - 21.933677) < 1e-6
assert set(rec.keys()) == set(FIELDS)
print("   OK -> temp", rec["temperature_c"], "lat", rec["latitude"])

print("== 2) storage SQLite ==")
db = os.path.join(tempfile.gettempdir(), "picaro_selftest.db")
if os.path.exists(db):
    os.remove(db)
st = TelemetryStore(db)
st.insert(rec)
rec2 = dict(rec); rec2["ts"] += 30; rec2["f_cnt"] = 1043; rec2["temperature_c"] = 26.5
st.insert(rec2)
assert st.count() == 2
last = st.latest()
assert last["f_cnt"] == 1043
ser = st.series(0)
assert len(ser) == 2 and ser[0]["temperature_c"] == 26.12
trk = st.track(0)
assert len(trk) == 2 and abs(trk[0][1] - 21.933677) < 1e-6
print("   OK -> count", st.count(), "track pts", len(trk))

print("== 3) decimate ==")
pts = list(range(2000))
d = decimate(pts, max_n=500)
assert len(d) == 500 and d[-1] == 1999   # conserva el ultimo
assert decimate([1, 2, 3], 500) == [1, 2, 3]
print("   OK -> 2000 ->", len(d), "ultimo", d[-1])

print("== 4) simulador ==")
got = []
sim = SimIngest(on_record=lambda r: got.append(r),
                on_status=lambda ok, m: print("   status:", ok, m),
                period_s=0.05)
sim.start()
time.sleep(0.6)
sim.stop()
time.sleep(0.1)
assert len(got) >= 5, f"pocos registros: {len(got)}"
r0 = got[0]
assert set(r0.keys()) == set(FIELDS)
assert r0["f_port"] == 10 and r0["gps_active"] == 1
# el GPS se mueve
lats = [g["latitude"] for g in got if g["gps_fix"]]
assert len(set(lats)) > 1, "el GPS no se movio"
print("   OK ->", len(got), "registros, lat movida de", round(lats[0], 5), "a", round(lats[-1], 5))

st.close()
os.remove(db)
print("\nTODO OK: capa de datos verificada.")
