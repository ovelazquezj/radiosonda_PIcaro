"""A1 · Enlace RF y gateways — Misiones Espaciales SatLink 2026.

Lee SOLO los CSV anonimizados de ../datos/ y escribe:
  - figuras  ../figuras/a1_*.png  (matplotlib, 150 dpi)
  - tablas   ../datos/derivados/a1_*.csv  (resúmenes derivados, también anónimos)
  - resumen  ../datos/derivados/a1_resumen.json (todas las cifras citadas en el borrador)

Ejecutar desde cualquier carpeta:   python -I a1_rf.py

Columnas derivadas que calcula este script (no están en datos/):
  s_dbm        potencia de señal estimada = rssi_dbm + min(snr_db, 0). Con SNR < 0 el RSSI
               del concentrador incluye el ruido; sumar el SNR aproxima la potencia de la señal.
  margen_snr   snr_db - límite de demodulación del SF (SF7 -7.5 dB, SF10 -15 dB, SF12 -20 dB).
  fspl_db      pérdida en espacio libre a 915 MHz = 91.67 + 20 log10(d_km).
  prx_presup   potencia recibida según el presupuesto de enlace previo al vuelo = PRESUP_DBM[gateway] - fspl_db
               (dBm). Supuestos: 22 dBm de TX, 5 dBi por extremo, 1 dB de cable por extremo y 3 dB de
               margen de patrón: 27 dBm. En picaro-lab-ags-001 se usa la ganancia real de su antena
               (dipolo RAIGEN de 5.8 dBi, datos/gateways.csv): 27.8 dBm.
  exceso_db    prx_presup - s_dbm: pérdida adicional respecto al presupuesto (positivo = peor).
  toa_ms       tiempo en el aire LoRa (fórmula de Semtech AN1200.13) con el payload real.
"""
import csv
import json
import math
import os
from collections import Counter, defaultdict
from datetime import datetime, timedelta, timezone

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

AQUI = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(AQUI)
DATOS = os.path.join(BASE, "datos")
FIG = os.path.join(BASE, "figuras")
DER = os.path.join(DATOS, "derivados")
os.makedirs(FIG, exist_ok=True)
os.makedirs(DER, exist_ok=True)

ORB = "Sonda Orbitium (Sci-Fi), ITPA"
TAES = "Sonda TAES (Instituto Tepeyac/UPA)"
SONDAS = [ORB, TAES]
CORTO = {ORB: "Orbitium (SF10)", TAES: "TAES (SF7)"}
GW00, GW01, LAB = "ITPA-IOT-GW00", "ITPA-IOT-GW01-kibts", "picaro-lab-ags-001"
GWS = [GW00, GW01, LAB]
GWC = {GW00: "GW00", GW01: "GW01-kibts", LAB: "lab-ags"}
SNR_LIM = {7: -7.5, 10: -15.0, 12: -20.0}
NF_DB = 6.0  # figura de ruido supuesta para la sensibilidad teórica
# Presupuesto de enlace previo al vuelo: 22 dBm TX + 5 dBi (sonda) + 5 dBi (gateway) - 2 x 1 dB de cable
# - 3 dB de margen de patrón = 27 dBm antes de la FSPL. La ganancia de 5 dBi del gateway es un supuesto,
# salvo en el laboratorio, cuya antena real es un dipolo RAIGEN de 5.8 dBi (+0.8 dB).
PRESUP_BASE_DBM = 22 + 5 + 5 - 1 - 1 - 3
G_ANT_GW_REAL_DBI = {LAB: 5.8}
PRESUP_DBM = {g: PRESUP_BASE_DBM + (G_ANT_GW_REAL_DBI[g] - 5.0 if g in G_ANT_GW_REAL_DBI else 0.0) for g in GWS}

# ---------------- estilo (skill dataviz: paleta de referencia, modo claro) ----------------
SURF = "#fcfcfb"
TXT1, TXT2, MUTED, GRID = "#0b0b0b", "#52514e", "#8a8984", "#e4e3df"
COL = {GW00: "#2a78d6", GW01: "#eb6834", LAB: "#1baf7a"}  # slots 1-3 (validan all-pairs)
SEQ = ["#86b6ef", "#2a78d6", "#104281"]  # rampa azul ordinal 250/450/650
plt.rcParams.update({
    "figure.facecolor": SURF, "axes.facecolor": SURF, "savefig.facecolor": SURF,
    "axes.edgecolor": MUTED, "axes.labelcolor": TXT2, "text.color": TXT1,
    "xtick.color": TXT2, "ytick.color": TXT2, "axes.grid": True, "grid.color": GRID,
    "grid.linewidth": 0.6, "axes.spines.top": False, "axes.spines.right": False,
    "font.size": 9, "axes.titlesize": 10, "axes.titleweight": "bold", "legend.frameon": False,
    "lines.linewidth": 2,
})


def guardar(fig, nombre):
    fig.savefig(os.path.join(FIG, nombre), dpi=150, bbox_inches="tight")
    plt.close(fig)


# ---------------- carga ----------------
def leer(nombre):
    with open(os.path.join(DATOS, nombre), encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def t(s):
    return datetime.fromisoformat(s.replace("Z", "+00:00"))


def fnum(s):
    return float(s) if s not in ("", None) else None


U = leer("uplinks.csv")
R = leer("recepciones.csv")
D = leer("downlinks.csv")
V = {v["sonda"]: v for v in leer("vuelos.csv")}
M = leer("gateway_metricas.csv")

for r in R:
    r["_t"] = t(r["t_utc"])
    r["_rssi"] = float(r["rssi_dbm"])
    r["_snr"] = float(r["snr_db"])
    r["_sf"] = int(r["sf"])
    r["_s"] = r["_rssi"] + min(r["_snr"], 0.0)
    r["_marg"] = r["_snr"] - SNR_LIM[r["_sf"]]
    r["_d"] = fnum(r["dist_slant_m"])
    r["_el"] = fnum(r["elev_deg"])
    r["_alt"] = fnum(r["alt_sonda_m"])
    r["_vuelo"] = r["en_ventana_vuelo"] == "1"
    r["_key"] = (r["sonda"], int(r["sesion"]), int(r["fcnt"]))
for u in U:
    u["_t"] = t(u["t_utc"])
    u["_key"] = (u["sonda"], int(u["sesion"]), int(u["fcnt"]))
    u["_vuelo"] = u["en_ventana_vuelo"] == "1"

T0 = {s: t(V[s]["t_lanzamiento_est_utc"]) for s in SONDAS}
TCORTE = t(V[ORB]["t_corte_inicio_utc"])
# la interpolación lineal entre los fixes que rodean la caída libre no es fiable
TFIX_POST = t(V[ORB]["t_ultimo_fix_utc"])
for r in R:
    # recepciones.csv marca con usar_geometria=0 (pos_origen=interpolada_caida) las posiciones
    # interpoladas entre los fixes que rodean el corte; además se excluye cualquier interpolada
    # entre el corte y el primer fix posterior
    r["_interp_caida"] = (r.get("usar_geometria") == "0" or
                          (r["sonda"] == ORB and r["pos_origen"] == "interpolada"
                           and TCORTE <= r["_t"] <= TFIX_POST))
    r["_geo_ok"] = r["_d"] is not None and not r["_interp_caida"]

rx_por_uplink = defaultdict(dict)  # key -> {gw: fila}
for r in R:
    rx_por_uplink[r["_key"]][r["gateway"]] = r

RES = {}


def pct(a, q):
    return float(np.percentile(np.array(a, dtype=float), q)) if a else None


def r1(x, n=1):
    return None if x is None else round(x, n)


def fspl(d_m):
    return 91.67 + 20 * math.log10(d_m / 1000.0)


def sens_teorica(sf, bw_khz=125):
    return -174 + 10 * math.log10(bw_khz * 1e3) + NF_DB + SNR_LIM[sf]


# =========== 1. Por gateway ===========
esperado = {}
for s in SONDAS:
    fc = defaultdict(list)
    for u in U:
        if u["sonda"] == s and u["_vuelo"]:
            fc[int(u["sesion"])].append(int(u["fcnt"]))
    esperado[s] = sum(max(v) - min(v) + 1 for v in fc.values())
    RES.setdefault("vuelo", {})[s] = {
        "sesiones": {k: [min(v), max(v)] for k, v in fc.items()},
        "fcnt_esperados": esperado[s],
        "uplinks_recibidos": sum(len(v) for v in fc.values()),
    }

filas_gw = []
for s in SONDAS:
    tot_s = sum(1 for u in U if u["sonda"] == s)
    tot_v = sum(1 for u in U if u["sonda"] == s and u["_vuelo"])
    for g in GWS:
        for fase in ("todo", "vuelo"):
            rr = [r for r in R if r["sonda"] == s and r["gateway"] == g and (fase == "todo" or r["_vuelo"])]
            if not rr:
                continue
            rssi = [r["_rssi"] for r in rr]
            snr = [r["_snr"] for r in rr]
            marg = [r["_marg"] for r in rr]
            geo = [r for r in rr if r["_geo_ok"]]
            geo_p = [r for r in rr if r["pos_origen"] == "propia"]
            dmax = max(geo, key=lambda r: r["_d"]) if geo else None
            dmaxp = max(geo_p, key=lambda r: r["_d"]) if geo_p else None
            filas_gw.append({
                "sonda": s, "gateway": g, "fase": fase, "n_rx": len(rr),
                "frac_uplinks": r1(100 * len(rr) / (tot_s if fase == "todo" else tot_v)),
                "frac_fcnt_esperado": r1(100 * len(rr) / esperado[s]) if fase == "vuelo" else "",
                "rssi_p10": r1(pct(rssi, 10)), "rssi_med": r1(pct(rssi, 50)), "rssi_p90": r1(pct(rssi, 90)),
                "rssi_min": min(rssi),
                "snr_p10": r1(pct(snr, 10)), "snr_med": r1(pct(snr, 50)), "snr_p90": r1(pct(snr, 90)),
                "snr_min": min(snr),
                "margen_snr_min": r1(min(marg)), "margen_snr_p10": r1(pct(marg, 10)),
                "margen_snr_med": r1(pct(marg, 50)),
                "d_slant_max_km": r1(dmax["_d"] / 1000, 2) if dmax else "",
                "d_slant_max_origen": dmax["pos_origen"] if dmax else "",
                "margen_snr_en_dmax": r1(dmax["_marg"]) if dmax else "",
                "alt_sonda_en_dmax": dmax["_alt"] if dmax else "",
                "d_slant_max_propia_km": r1(dmaxp["_d"] / 1000, 2) if dmaxp else "",
                "elev_min": r1(min(r["_el"] for r in geo), 2) if geo else "",
                "elev_max": r1(max(r["_el"] for r in geo), 2) if geo else "",
            })
with open(os.path.join(DER, "a1_por_gateway.csv"), "w", encoding="utf-8", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(filas_gw[0].keys()))
    w.writeheader()
    w.writerows(filas_gw)
RES["por_gateway"] = filas_gw

# =========== 2. Diversidad ===========
div = {}
for s in SONDAS:
    for fase in ("todo", "vuelo"):
        ups = [u for u in U if u["sonda"] == s and (fase == "todo" or u["_vuelo"])]
        n_gw = Counter(len(rx_por_uplink[u["_key"]]) for u in ups)
        solo = Counter(next(iter(rx_por_uplink[u["_key"]])) for u in ups if len(rx_por_uplink[u["_key"]]) == 1)
        comb = Counter(tuple(sorted(GWC[g] for g in rx_por_uplink[u["_key"]])) for u in ups)
        base = esperado[s] if fase == "vuelo" else len(ups)
        single = {GWC[g]: sum(1 for u in ups if g in rx_por_uplink[u["_key"]]) for g in GWS}
        campus = sum(1 for u in ups if (GW00 in rx_por_uplink[u["_key"]] or GW01 in rx_por_uplink[u["_key"]]))
        div[(s, fase)] = {
            "n_uplinks": len(ups), "base": base,
            "por_n_gateways": {k: n_gw.get(k, 0) for k in (1, 2, 3)},
            "salvados_solo_por": {GWC[g]: solo.get(g, 0) for g in GWS},
            "combinaciones": {"+".join(k): v for k, v in comb.items()},
            "por_gateway": single, "campus_gw00_o_gw01": campus,
            "entrega_mejor_gw_pct": r1(100 * max(single.values()) / base),
            "entrega_campus_pct": r1(100 * campus / base),
            "entrega_red_pct": r1(100 * len(ups) / base),
        }
RES["diversidad"] = {f"{s}|{f}": v for (s, f), v in div.items()}

# =========== 3. RSSI contra distancia y presupuesto ===========
geo_v = [r for r in R if r["_vuelo"] and r["_geo_ok"]]
for r in geo_v:
    r["_fspl"] = fspl(r["_d"])
    r["_prx"] = PRESUP_DBM[r["gateway"]] - r["_fspl"]
    r["_exc"] = r["_prx"] - r["_s"]
ajuste = {}
for s in SONDAS:
    for g in GWS:
        rr = [r for r in geo_v if r["sonda"] == s and r["gateway"] == g]
        if len(rr) < 5:
            continue
        x = np.log10(np.array([r["_d"] / 1000 for r in rr]))
        y = np.array([r["_s"] for r in rr])
        pend, ord0 = np.polyfit(x, y, 1)
        exc = [r["_exc"] for r in rr]
        ajuste[f"{s}|{g}"] = {
            "n": len(rr), "exceso_med_db": r1(pct(exc, 50)), "exceso_p10": r1(pct(exc, 10)),
            "exceso_p90": r1(pct(exc, 90)),
            "pendiente_db_por_decada": r1(pend), "n_exponente": r1(-pend / 10, 2),
            "d_min_km": r1(min(r["_d"] for r in rr) / 1000, 2), "d_max_km": r1(max(r["_d"] for r in rr) / 1000, 2),
        }
RES["presupuesto"] = {
    "ajuste": ajuste,
    "sens_teorica_dbm": {f"SF{sf}": r1(sens_teorica(sf)) for sf in (7, 10)},
    "sens_teorica_dl_sf12_500k": r1(sens_teorica(12, 500)),
    "prx_presup_30km": r1(PRESUP_BASE_DBM - fspl(30000)),
    "presup_dbm_por_gateway": {g: r1(PRESUP_DBM[g]) for g in GWS},
}
# margen real frente al previsto, en las distancias reales (la previsión es margen sobre sensibilidad)
cmp = []
for s in SONDAS:
    sf = 10 if s == ORB else 7
    sens_p = -124.0 if sf == 7 else sens_teorica(10)
    for g in GWS:
        rr = [r for r in geo_v if r["sonda"] == s and r["gateway"] == g]
        if not rr:
            continue
        prev = [r["_prx"] - sens_p for r in rr]
        real = [r["_marg"] for r in rr]
        cmp.append({"sonda": s, "gateway": g, "n": len(rr), "margen_previsto_med": r1(pct(prev, 50)),
                    "margen_previsto_min": r1(min(prev)), "margen_snr_real_med": r1(pct(real, 50)),
                    "margen_snr_real_min": r1(min(real)),
                    "margen_rssi_real_med": r1(pct([r["_s"] - sens_p for r in rr], 50)),
                    "margen_rssi_real_min": r1(min(r["_s"] - sens_p for r in rr))})
RES["margen_previsto_vs_real"] = cmp

# Nulo cenital: diferencia pareada GW01 - GW00 para el mismo uplink
par = []
for k, d in rx_por_uplink.items():
    if GW00 in d and GW01 in d and d[GW00]["_vuelo"] and d[GW00]["_geo_ok"]:
        a, b = d[GW00], d[GW01]
        par.append({"sonda": a["sonda"], "el": a["_el"], "d": a["_d"],
                    "ds": b["_s"] - a["_s"], "dsnr": b["_snr"] - a["_snr"],
                    "drssi": b["_rssi"] - a["_rssi"], "t": a["_t"]})
bins = [(0, 5), (5, 10), (10, 20), (20, 35), (35, 60)]
nulo = []
for lo, hi in bins:
    pp = [p for p in par if lo <= p["el"] < hi]
    ex0 = [r["_exc"] for r in geo_v if r["gateway"] == GW00 and lo <= r["_el"] < hi]
    ex1 = [r["_exc"] for r in geo_v if r["gateway"] == GW01 and lo <= r["_el"] < hi]
    nulo.append({"elev_deg": f"{lo}-{hi}", "n_pares": len(pp),
                 "ds_med": r1(pct([p["ds"] for p in pp], 50)) if pp else "",
                 "ds_p10": r1(pct([p["ds"] for p in pp], 10)) if pp else "",
                 "ds_p90": r1(pct([p["ds"] for p in pp], 90)) if pp else "",
                 "dsnr_med": r1(pct([p["dsnr"] for p in pp], 50)) if pp else "",
                 "exceso_gw00_med": r1(pct(ex0, 50)) if ex0 else "", "n_gw00": len(ex0),
                 "exceso_gw01_med": r1(pct(ex1, 50)) if ex1 else "", "n_gw01": len(ex1)})
with open(os.path.join(DER, "a1_nulo_cenital.csv"), "w", encoding="utf-8", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(nulo[0].keys()))
    w.writeheader()
    w.writerows(nulo)
xs = np.array([p["el"] for p in par])
ys = np.array([p["ds"] for p in par])
pend_el, ord_el = np.polyfit(xs, ys, 1)
RES["nulo_cenital"] = {"bins": nulo, "n_pares": len(par), "pendiente_db_por_grado": r1(pend_el, 3),
                       "ordenada_db": r1(ord_el), "corr": r1(float(np.corrcoef(xs, ys)[0, 1]), 2),
                       "elev_max": r1(float(xs.max()), 1)}


# =========== 4. SF10 contra SF7 ===========
def toa_ms(pl_bytes, sf, bw_khz=125, cr=1, preamb=8, crc=True, hdr_explicit=True):
    """Tiempo en el aire LoRa (Semtech AN1200.13). pl_bytes = PHYPayload completo."""
    ts = (2 ** sf) / (bw_khz * 1e3) * 1e3
    de = 1 if ts > 16 else 0
    h = 0 if hdr_explicit else 1
    num = 8 * pl_bytes - 4 * sf + 28 + (16 if crc else 0) - 20 * h
    npay = 8 + max(math.ceil(num / (4 * (sf - 2 * de))) * (cr + 4), 0)
    return (preamb + 4.25) * ts + npay * ts


LW_OVH = 13  # MHDR 1 + FHDR 7 (sin FOpts) + FPort 1 + MIC 4
toa = []
for s in SONDAS:
    tam = Counter(int(u["bytes_payload"]) for u in U if u["sonda"] == s)
    for b, n in sorted(tam.items()):
        sf = 10 if s == ORB else 7
        toa.append({"sonda": s, "sf": sf, "bytes_frmpayload": b, "bytes_phy": b + LW_OVH, "n_uplinks": n,
                    "toa_ms": r1(toa_ms(b + LW_OVH, sf), 1)})
# cadencia en vuelo y ocupación del canal
for s in SONDAS:
    tv = sorted(u["_t"] for u in U if u["sonda"] == s and u["_vuelo"])
    dt = np.diff([x.timestamp() for x in tv])
    tam_v = Counter(int(u["bytes_payload"]) for u in U if u["sonda"] == s and u["_vuelo"])
    sf = 10 if s == ORB else 7
    aire = sum(n * toa_ms(b + LW_OVH, sf) for b, n in tam_v.items()) / 1000
    dur = (tv[-1] - T0[s]).total_seconds()
    RES.setdefault("sf", {})[s] = {
        "cadencia_med_s": r1(float(np.median(dt))), "aire_total_vuelo_s": r1(aire),
        "duracion_vuelo_s": r1(dur, 0), "ocupacion_pct": r1(100 * aire / dur, 2),
        "entrega_red_pct": div[(s, "vuelo")]["entrega_red_pct"],
        "perdidos": esperado[s] - div[(s, "vuelo")]["n_uplinks"],
    }
RES["toa"] = toa
RES["sf"]["toa_igual_payload_11B"] = {f"SF{sf}": r1(toa_ms(11 + LW_OVH, sf), 1) for sf in (7, 8, 9, 10)}
RES["sf"]["toa_igual_payload_27B"] = {f"SF{sf}": r1(toa_ms(27 + LW_OVH, sf), 1) for sf in (7, 8, 9, 10)}
RES["sf"]["dl_toa_marco_sf12_500k"] = r1(toa_ms(LW_OVH + 4, 12, 500), 1)

# =========== 5. Laboratorio ===========
lab = {}
for s in SONDAS:
    ups = [u for u in U if u["sonda"] == s and u["_vuelo"]]
    filas = []
    for u in ups:
        d = rx_por_uplink[u["_key"]]
        ref = next((x for x in d.values() if x["_alt"] is not None and not x["_interp_caida"]), None)
        filas.append({"t_s": (u["_t"] - T0[s]).total_seconds(), "alt": ref["_alt"] if ref else None,
                      "lab": LAB in d, "post_corte": u["_t"] >= TCORTE and s == ORB})
    rec = [f for f in filas if f["lab"]]
    rec_alt = [f["alt"] for f in rec if f["alt"] is not None]
    no_alt = [f["alt"] for f in filas if not f["lab"] and f["alt"] is not None]
    lr = [r for r in R if r["sonda"] == s and r["gateway"] == LAB]
    lgeo = [r for r in lr if r["_geo_ok"]]
    asc = [f for f in filas if f["alt"] is not None and not f["post_corte"]]
    primera = next((f for f in sorted(filas, key=lambda f: f["t_s"]) if f["lab"]), None)
    lab[s] = {
        "n_rx": len(lr), "n_rx_fuera_vuelo": sum(1 for r in lr if not r["_vuelo"]),
        "n_uplinks_vuelo": len(ups),
        "pct_vuelo": r1(100 * len(rec) / len(ups)),
        "alt_min_recibido": min(rec_alt) if rec_alt else None,
        "alt_max_no_recibido": max(no_alt) if no_alt else None,
        "primera_rx_t_s": primera["t_s"] if primera else None,
        "primera_rx_alt": primera["alt"] if primera else None,
        "rssi_med": r1(pct([r["_rssi"] for r in lr], 50)), "rssi_min": min(r["_rssi"] for r in lr),
        "snr_med": r1(pct([r["_snr"] for r in lr], 50)), "snr_min": min(r["_snr"] for r in lr),
        "margen_snr_min": r1(min(r["_marg"] for r in lr)),
        "d_min_km": r1(min(r["_d"] for r in lgeo) / 1000, 1) if lgeo else None,
        "d_max_km": r1(max(r["_d"] for r in lgeo) / 1000, 1) if lgeo else None,
        "el_min": r1(min(r["_el"] for r in lgeo), 2) if lgeo else None,
        "el_max": r1(max(r["_el"] for r in lgeo), 2) if lgeo else None,
        "_filas": filas,
    }
    # fracción por bandas de altitud (solo ascenso; con altitud conocida)
    bandas = []
    for lo in range(1500, 9000, 1000):
        ff = [f for f in asc if lo <= f["alt"] < lo + 1000]
        if ff:
            bandas.append({"alt_m": f"{lo}-{lo + 1000}", "n": len(ff), "lab": sum(f["lab"] for f in ff)})
    lab[s]["bandas_ascenso"] = bandas
# horizonte de radio en Tierra lisa con k = 4/3 (d_km = 4.12 sqrt(h_m)), h sobre el nivel de la antena
RES["lab"] = {s: {k: v for k, v in d.items() if not k.startswith("_")} for s, d in lab.items()}
RES["lab"]["horizonte_km_por_h_sobre_antena"] = {h: r1(4.12 * math.sqrt(h), 0) for h in (100, 500, 1000, 3000, 6500)}

# =========== 6. Downlinks: margen en la sonda ===========
dl = []
for d in D:
    if not d["resp_fcnt"] or d["polo_snr_db"] == "":
        continue
    tr = t(d["t_respuesta_utc"])
    cand = [r for r in R if r["sonda"] == ORB and int(r["fcnt"]) == int(d["resp_fcnt"])
            and abs((r["_t"] - tr).total_seconds()) < 5]
    rgw = next((r for r in cand if r["gateway"] == d["gateway_tx"]), None)
    snr = float(d["polo_snr_db"])
    dl.append({"dl_n": d["dl_n"], "t_desde_lanz_s": d["t_desde_lanz_s"], "gateway_tx": d["gateway_tx"],
               "polo_rssi_dbm": d["polo_rssi_dbm"], "polo_snr_db": snr,
               "margen_dl_sf12_db": r1(snr - SNR_LIM[12]),
               "ul_snr_mismo_gw_db": rgw["_snr"] if rgw else "",
               "ul_margen_sf10_db": r1(rgw["_marg"]) if rgw else "",
               "d_slant_km": r1(rgw["_d"] / 1000, 1) if rgw and rgw["_d"] else "",
               "elev_deg": rgw["_el"] if rgw else "",
               "en_vuelo": d["t_desde_lanz_s"] != "" and float(d["t_desde_lanz_s"]) >= 0})
with open(os.path.join(DER, "a1_downlinks_margen.csv"), "w", encoding="utf-8", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(dl[0].keys()))
    w.writeheader()
    w.writerows(dl)
RES["downlinks"] = dl

# =========== 7. Línea base de gateways ===========
dias = sorted({m["t_utc"][:10] for m in M if m["agregacion"] == "DAY"})
base = {g: {} for g in GWS}
for m in M:
    if m["agregacion"] == "DAY" and m["etiqueta"] == "total":
        base[m["gateway"]].setdefault(m["t_utc"][:10], {})[m["metrica"]] = int(float(m["valor"]))
hour = defaultdict(dict)
for m in M:
    if m["agregacion"] == "HOUR" and m["metrica"] == "rx_paquetes" and m["etiqueta"] == "total":
        hour[m["gateway"]][m["t_utc"][:13]] = int(float(m["valor"]))
estados = Counter((m["gateway"], m["etiqueta"]) for m in M if m["metrica"] == "tx_por_estado")
RES["linea_base"] = {
    "dias": {g: {d: base[g].get(d, {}) for d in dias if "2026-10-27" <= d <= "2026-11-02"} for g in GWS},
    "hour_cobertura": {g: [min(v), max(v), len(v)] if v else None for g, v in hour.items()},
    "tx_estados": {f"{g}|{e}": sum(int(float(m["valor"])) for m in M if m["gateway"] == g and m["metrica"] == "tx_por_estado" and m["etiqueta"] == e and m["agregacion"] == "DAY") for g, e in estados},
}
# huecos de recepción durante la noche del evento: uplinks de las sondas perdidos por un gateway
# y recibidos por otro (28-oct 18:00 local = 29-oct 00:00 UTC, hasta el final del vuelo)
ini = datetime(2026, 10, 29, 0, 0, tzinfo=timezone.utc)
fin = max(t(V[s]["t_ultima_recepcion_utc"]) for s in SONDAS)
noche = sorted([u for u in U if ini <= u["_t"] <= fin], key=lambda u: u["_t"])
huecos = {}
for g in (GW00, GW01):
    falt = [u for u in noche if g not in rx_por_uplink[u["_key"]]]
    racha, mejor, ini_r = 0, (0, None, None), None
    prev = None
    # racha = uplinks consecutivos (de cualquiera de las dos sondas, en orden temporal) que el gateway no recibió
    for u in noche:
        if g not in rx_por_uplink[u["_key"]]:
            if racha == 0:
                ini_r = u["_t"]
            racha += 1
            if racha > mejor[0]:
                mejor = (racha, ini_r, u["_t"])
        else:
            racha = 0
    huecos[g] = {"n_uplinks_noche": len(noche), "no_recibidos": len(falt),
                 "racha_max": mejor[0],
                 "racha_ini_local": (mejor[1] - timedelta(hours=6)).strftime("%Y-%m-%d %H:%M:%S") if mejor[1] else None,
                 "racha_dur_s": r1((mejor[2] - mejor[1]).total_seconds(), 0) if mejor[1] else None,
                 "racha_ini_utc": mejor[1].isoformat() if mejor[1] else None}
RES["linea_base"]["huecos_noche"] = huecos


# =========== 3b. Comparaciones adicionales ===========
# (a) pérdida en exceso en tierra (antes del lanzamiento, sonda con fix propio, d < 1.5 km)
suelo = {}
for s in SONDAS:
    for g in (GW00, GW01):
        rr = [r for r in R if r["sonda"] == s and r["gateway"] == g and not r["_vuelo"]
              and r["pos_origen"] == "propia" and r["_d"] and r["_d"] < 1500]
        if rr:
            ex = [(PRESUP_DBM[g] - fspl(r["_d"])) - r["_s"] for r in rr]
            suelo[f"{s}|{g}"] = {"n": len(rr), "exceso_med_db": r1(pct(ex, 50)),
                                 "d_med_m": r1(pct([r["_d"] for r in rr], 50), 0),
                                 "rssi_med": r1(pct([r["_rssi"] for r in rr], 50))}
RES["exceso_suelo"] = suelo
# (a2) uso de canales US915 (todo el periodo) y entrega en vuelo por canal
RES["canales"] = {s: dict(sorted(Counter(u["canal_us915"] for u in U if u["sonda"] == s).items())) for s in SONDAS}
# (b) margen de SNR por banda de distancia oblicua (gateways del campus), SF10 contra SF7
bandas_d = [(0, 5), (5, 10), (10, 20), (20, 32)]
cmp_sf = []
for lo, hi in bandas_d:
    fila = {"d_km": f"{lo}-{hi}"}
    for s in SONDAS:
        rr = [r["_marg"] for r in geo_v if r["sonda"] == s and r["gateway"] in (GW00, GW01)
              and lo <= r["_d"] / 1000 < hi]
        fila[f"n_{CORTO[s]}"] = len(rr)
        fila[f"margen_med_{CORTO[s]}"] = r1(pct(rr, 50)) if rr else ""
        fila[f"snr_med_{CORTO[s]}"] = r1(pct([m + SNR_LIM[10 if s == ORB else 7] for m in rr], 50)) if rr else ""
    cmp_sf.append(fila)
RES["margen_por_distancia"] = cmp_sf
# (c) laboratorio a igual altitud (3500-5500 m, ascenso)
RES["lab_igual_altitud_3500_5500"] = {}
for s in SONDAS:
    ff = [f for f in lab[s]["_filas"] if f["alt"] is not None and not f["post_corte"] and 3500 <= f["alt"] < 5500]
    RES["lab_igual_altitud_3500_5500"][s] = {"n": len(ff), "lab": sum(f["lab"] for f in ff),
                                             "pct": r1(100 * sum(f["lab"] for f in ff) / len(ff)) if ff else None}

# ======================= FIGURAS =======================
MK = {"propia": "o", "interpolada": "^"}


def leyenda_gw(ax, extra=None, loc="best"):
    from matplotlib.lines import Line2D
    h = [Line2D([], [], ls="", marker="o", color=COL[g], label=g, ms=6) for g in GWS]
    if extra:
        h += extra
    ax.legend(handles=h, loc=loc, fontsize=8)


# --- Fig 1: RSSI (potencia de señal estimada) contra distancia oblicua ---
fig, axs = plt.subplots(1, 2, figsize=(11, 4.6), sharey=True)
dd = np.logspace(math.log10(1.5), math.log10(70), 100)
for ax, s in zip(axs, SONDAS):
    for g in GWS:
        for org in ("propia", "interpolada"):
            rr = [r for r in geo_v if r["sonda"] == s and r["gateway"] == g and r["pos_origen"] == org]
            if rr:
                ax.scatter([r["_d"] / 1000 for r in rr], [r["_s"] for r in rr], s=22, marker=MK[org],
                           color=COL[g], edgecolor=SURF, linewidth=0.6, alpha=0.9, zorder=3)
    ax.plot(dd, PRESUP_BASE_DBM - (91.67 + 20 * np.log10(dd)), color=TXT2, lw=1.5, ls="--", zorder=2)
    ax.text(dd[-1], PRESUP_BASE_DBM - fspl(70000) + 1.5,
            "presupuesto previo al vuelo\n(27 dBm − FSPL; laboratorio: 27.8 dBm)", ha="right",
            va="bottom", fontsize=7.5, color=TXT2)
    sf = 10 if s == ORB else 7
    sens = -124.0 if sf == 7 else sens_teorica(10)
    ax.axhline(sens, color=MUTED, lw=1, ls=":")
    ax.text(1.6, sens + 1, f"sensibilidad SF{sf} ≈ {sens:.0f} dBm", fontsize=7.5, color=TXT2)
    ax.set_xscale("log")
    ax.set_xticks([2, 5, 10, 20, 50])
    ax.set_xticklabels(["2", "5", "10", "20", "50"])
    ax.set_xlabel("distancia oblicua sonda–gateway (km, escala log)")
    ax.set_title(f"{s} · SF{sf}")
axs[0].set_ylabel("potencia de señal estimada, RSSI + mín(SNR, 0) (dBm)")
from matplotlib.lines import Line2D
leyenda_gw(axs[1], [Line2D([], [], ls="", marker="o", color=MUTED, label="posición propia"),
                    Line2D([], [], ls="", marker="^", color=MUTED, label="posición interpolada")], loc="lower left")
fig.suptitle("Señal recibida en vuelo contra distancia: medida vs. presupuesto de enlace (espacio libre, 915 MHz)",
             fontsize=10.5, fontweight="bold", x=0.01, ha="left")
fig.tight_layout()
guardar(fig, "a1_rssi_vs_distancia.png")

# --- Fig 2: margen de SNR contra distancia ---
fig, axs = plt.subplots(1, 2, figsize=(11, 4.4), sharey=True)
for ax, s in zip(axs, SONDAS):
    for g in GWS:
        for org in ("propia", "interpolada"):
            rr = [r for r in geo_v if r["sonda"] == s and r["gateway"] == g and r["pos_origen"] == org]
            if rr:
                ax.scatter([r["_d"] / 1000 for r in rr], [r["_marg"] for r in rr], s=22, marker=MK[org],
                           color=COL[g], edgecolor=SURF, linewidth=0.6, zorder=3)
    ax.axhline(0, color=TXT2, lw=1.2)
    sf = 10 if s == ORB else 7
    ax.text(0.02, 0.06, f"0 dB = límite de demodulación SF{sf} ({SNR_LIM[sf]:+.1f} dB de SNR)", fontsize=7.5,
            color=TXT2, transform=ax.transAxes)
    ax.set_xscale("log")
    ax.set_xticks([2, 5, 10, 20, 50])
    ax.set_xticklabels(["2", "5", "10", "20", "50"])
    ax.set_xlabel("distancia oblicua sonda–gateway (km, escala log)")
    ax.set_title(f"{s} · SF{sf}")
axs[0].set_ylabel("margen de SNR sobre el límite del SF (dB)")
leyenda_gw(axs[1], [Line2D([], [], ls="", marker="o", color=MUTED, label="posición propia"),
                    Line2D([], [], ls="", marker="^", color=MUTED, label="posición interpolada")], loc="upper right")
fig.suptitle("Margen de SNR en vuelo: cuánto le faltó a cada recepción para perderse",
             fontsize=10.5, fontweight="bold", x=0.01, ha="left")
fig.tight_layout()
guardar(fig, "a1_snr_margen.png")

# --- Fig 3: diversidad ---
fig, axs = plt.subplots(1, 2, figsize=(11, 3.8), gridspec_kw={"width_ratios": [1, 1.2]})
ax = axs[0]
y = np.arange(len(SONDAS))
for i, s in enumerate(SONDAS):
    dv = div[(s, "vuelo")]
    left = 0
    perd = dv["base"] - dv["n_uplinks"]
    segs = [(perd, "#d6d5d0", "perdidos")] + [(dv["por_n_gateways"][k], SEQ[k - 1], f"{k} gateway" + ("s" if k > 1 else ""))
                                               for k in (1, 2, 3)]
    for val, c, lab_ in segs:
        w_ = 100 * val / dv["base"]
        ax.barh(i, w_ - 0.4, left=left + 0.2, color=c, height=0.55, label=lab_ if i == 0 else None)
        if w_ >= 6:
            ax.text(left + w_ / 2, i, f"{val}", ha="center", va="center", fontsize=8,
                    color="white" if c in (SEQ[1], SEQ[2]) else TXT1)
        left += w_
ax.set_yticks(y)
ax.set_yticklabels([CORTO[s] for s in SONDAS])
ax.invert_yaxis()
ax.set_xlim(0, 100)
ax.set_xlabel("% de los fCnt esperados en vuelo")
ax.set_title("¿Cuántos gateways recibieron cada uplink en vuelo?")
ax.legend(ncol=4, loc="upper center", bbox_to_anchor=(0.5, -0.22), fontsize=8)
ax.grid(axis="y", visible=False)
ax = axs[1]
etq = ["GW00", "GW01-kibts", "lab-ags", "GW00 o GW01\n(campus)", "red completa\n(3 gateways)"]
wbar = 0.38
for i, s in enumerate(SONDAS):
    dv = div[(s, "vuelo")]
    vals = [100 * dv["por_gateway"][k] / dv["base"] for k in ("GW00", "GW01-kibts", "lab-ags")]
    vals += [100 * dv["campus_gw00_o_gw01"] / dv["base"], 100 * dv["n_uplinks"] / dv["base"]]
    xx = np.arange(len(etq)) + (i - 0.5) * wbar
    ax.bar(xx, vals, width=wbar - 0.03, color=["#6250d6", "#e87ba4"][i], label=CORTO[s])
    for xv, v in zip(xx, vals):
        ax.text(xv, v + 1.2, f"{v:.0f}", ha="center", fontsize=7.5, color=TXT2)
ax.set_xticks(np.arange(len(etq)))
ax.set_xticklabels(etq, fontsize=8)
ax.set_ylim(0, 120)
ax.set_ylabel("entrega en vuelo (% de fCnt esperados)")
ax.set_title("Tasa de entrega por gateway y combinada")
ax.legend(loc="upper center", ncol=2, fontsize=8)
ax.grid(axis="x", visible=False)
fig.tight_layout()
guardar(fig, "a1_diversidad.png")

# --- Fig 4: cobertura por gateway a lo largo del vuelo ---
fig, axs = plt.subplots(2, 2, figsize=(11, 5.6), sharex="col", gridspec_kw={"height_ratios": [2.2, 1]})
for j, s in enumerate(SONDAS):
    ax = axs[0, j]
    pts = sorted([(f["t_s"] / 60, f["alt"]) for f in lab[s]["_filas"] if f["alt"] is not None])
    if s == ORB:
        tc_ = (TCORTE - T0[s]).total_seconds() / 60
        a_ = [p for p in pts if p[0] <= tc_ + 0.01]
        b_ = [p for p in pts if p[0] >= a_[-1][0]]
        ax.plot([p[0] for p in a_], [p[1] / 1000 for p in a_], color=TXT2, lw=1.5, zorder=2)
        ax.plot([p[0] for p in b_], [p[1] / 1000 for p in b_], color=TXT2, lw=1.2, ls=":", zorder=2)
        ax.text(b_[-1][0], b_[-1][1] / 1000 + 0.25, "tras el corte: sin fixes\nintermedios (punteado)", fontsize=7.5,
                color=TXT2, ha="right")
    else:
        ax.plot([p[0] for p in pts], [p[1] / 1000 for p in pts], color=TXT2, lw=1.5, zorder=2)
    lp = [(f["t_s"] / 60, f["alt"]) for f in lab[s]["_filas"] if f["lab"] and f["alt"] is not None]
    ax.scatter([p[0] for p in lp], [p[1] / 1000 for p in lp], s=24, color=COL[LAB], edgecolor=SURF,
               linewidth=0.6, zorder=3, label=f"recibido también por {LAB}")
    if s == ORB:
        tc = (TCORTE - T0[s]).total_seconds() / 60
        for a in axs[:, j]:
            a.axvline(tc, color=MUTED, lw=1, ls="--")
        ax.text(tc + 0.5, 0.3, "corte", fontsize=7.5, color=TXT2, transform=ax.get_xaxis_transform())
    ax.set_title(f"{s}")
    ax.set_ylabel("altitud de la sonda (km s.n.m.)")
    ax.legend(loc="upper left", fontsize=8)
    ax = axs[1, j]
    ups = sorted([u for u in U if u["sonda"] == s and u["_vuelo"]], key=lambda u: u["_t"])
    for i, g in enumerate(GWS):
        si = [(u["_t"] - T0[s]).total_seconds() / 60 for u in ups if g in rx_por_uplink[u["_key"]]]
        no = [(u["_t"] - T0[s]).total_seconds() / 60 for u in ups if g not in rx_por_uplink[u["_key"]]]
        ax.scatter(si, [i] * len(si), marker="|", s=90, color=COL[g], linewidth=1.6)
        ax.scatter(no, [i] * len(no), marker="x", s=14, color=MUTED, linewidth=0.8)
    ax.set_yticks(range(3))
    ax.set_yticklabels([GWC[g] for g in GWS], fontsize=8)
    ax.set_ylim(2.6, -0.6)
    ax.set_xlabel("minutos desde el despegue estimado")
    ax.grid(axis="y", visible=False)
axs[1, 1].text(1.0, -0.42, "| recibido   × enviado (otro gateway lo recibió) y no recibido por este",
               transform=axs[1, 1].transAxes, ha="right", fontsize=7.5, color=TXT2)
fig.suptitle("Cobertura de cada gateway durante el vuelo (los fCnt perdidos por toda la red no aparecen)",
             fontsize=10.5, fontweight="bold", x=0.01, ha="left")
fig.tight_layout()
guardar(fig, "a1_cobertura_gateways.png")

# --- Fig 5: nulo cenital ---
fig, axs = plt.subplots(1, 2, figsize=(11, 4.2))
ax = axs[0]
for s, c in zip(SONDAS, ["#6250d6", "#e87ba4"]):
    pp = [p for p in par if p["sonda"] == s]
    ax.scatter([p["el"] for p in pp], [p["ds"] for p in pp], s=22, color=c, edgecolor=SURF, linewidth=0.6,
               label=CORTO[s], zorder=3)
xl = np.linspace(0, xs.max(), 10)
ax.plot(xl, ord_el + pend_el * xl, color=TXT2, lw=1.5, ls="--")
ax.axhline(0, color=MUTED, lw=1)
ax.set_xlabel("ángulo de elevación visto desde el campus (°)")
ax.set_ylabel("señal GW01-kibts − señal GW00, mismo uplink (dB)")
ax.set_title("Diferencia pareada entre los dos gateways del ITPA")
ax.legend(loc="lower left", fontsize=8)
ax = axs[1]
cent = [2.5, 7.5, 15, 27.5, 47.5]
for g in (GW00, GW01):
    k = "exceso_gw00_med" if g == GW00 else "exceso_gw01_med"
    nk = "n_gw00" if g == GW00 else "n_gw01"
    pts = [(c_, b[k], b[nk]) for c_, b in zip(cent, nulo) if b[k] != ""]
    ax.plot([p[0] for p in pts], [p[1] for p in pts], color=COL[g], marker="o", ms=6, label=g)
    for p in pts:
        ax.text(p[0], p[1] + 0.8, f"n={p[2]}", fontsize=7, color=TXT2, ha="center")
ax.set_xlabel("ángulo de elevación (°, centro de la banda)")
ax.set_ylabel("pérdida en exceso sobre el presupuesto, mediana (dB)")
ax.set_title("Pérdida respecto al presupuesto por banda de elevación")
ax.legend(loc="upper left", fontsize=8)
fig.tight_layout()
guardar(fig, "a1_nulo_cenital.png")

# --- Fig 6: línea base de los gateways ---
fig, axs = plt.subplots(1, 3, figsize=(11, 3.4))
dsel = [d for d in dias if "2026-10-27" <= d <= "2026-11-02"]
for ax, g in zip(axs, GWS):
    v = [base[g].get(d, {}).get("rx_paquetes", 0) for d in dsel]
    ax.bar(range(len(dsel)), v, color=COL[g], width=0.7)
    for i, x in enumerate(v):
        ax.text(i, x, f"{x}", ha="center", va="bottom", fontsize=7, color=TXT2)
    ax.set_xticks(range(len(dsel)))
    ax.set_xticklabels([d[8:10] + "-" + ("oct" if d[5:7] == "10" else "nov") for d in dsel], fontsize=7.5)
    ax.set_title(g)
    ax.set_xlabel("día (UTC)")
    ax.grid(axis="x", visible=False)
    i29 = dsel.index("2026-10-29")
    ax.axvspan(i29 - 0.45, i29 + 0.45, color=GRID, zorder=0)
axs[0].set_ylabel("paquetes recibidos por día (todo el tráfico)")
fig.suptitle("Línea base: paquetes LoRa recibidos por día según la API de métricas (el vuelo cae en el 29-oct UTC, sombreado)",
             fontsize=10, fontweight="bold", x=0.01, ha="left")
fig.tight_layout()
guardar(fig, "a1_linea_base.png")

# --- Fig 7: margen del downlink en la sonda ---
fig, ax = plt.subplots(figsize=(7.5, 3.8))
dv_ = [x for x in dl if x["en_vuelo"] and x["ul_margen_sf10_db"] != ""]
tt = [float(x["t_desde_lanz_s"]) / 60 for x in dv_]
ax.plot(tt, [x["margen_dl_sf12_db"] for x in dv_], color="#6250d6", marker="o", ms=6,
        label="downlink Marco (SF12/500 kHz) medido en la sonda")
ax.plot(tt, [x["ul_margen_sf10_db"] for x in dv_], color="#e87ba4", marker="s", ms=6,
        label="uplink Polo (SF10/125 kHz) en el gateway que transmitió")
ax.axhline(0, color=TXT2, lw=1.2)
ax.set_xlabel("minutos desde el despegue estimado")
ax.set_ylabel("margen de SNR sobre el límite (dB)")
ax.set_title("Sonda Orbitium (Sci-Fi), ITPA: margen del enlace de bajada contra el de subida")
ax.legend(loc="upper right", fontsize=8)
fig.tight_layout()
guardar(fig, "a1_downlink_margen.png")

with open(os.path.join(DER, "a1_resumen.json"), "w", encoding="utf-8") as f:
    json.dump(RES, f, ensure_ascii=False, indent=1, default=str)
print(json.dumps(RES, ensure_ascii=False, indent=1, default=str))
