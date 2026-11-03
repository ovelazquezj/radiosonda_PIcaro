# -*- coding: utf-8 -*-
"""
A3 · Operaciones de misión y comandos — SatLink 2026.

Lee SOLO los CSV anonimizados de ../datos/ y produce:
  - ../figuras/a3_linea_tiempo_noche.png   línea de tiempo de la noche del 28-oct (hora local)
  - ../figuras/a3_fase2_comandos.png       Fase 2: Marcos/Polos frente a las ventanas T+5/15/30
  - ../figuras/a3_gateways_diario.png      paquetes recibidos por día y gateway (métricas DAY)
  - ../datos/derivados/a3_*.csv            tablas derivadas que cita el borrador A3
y escribe en stdout todas las cifras que usa el borrador.

Ejecutar:  python -I a3_operaciones.py   (desde cualquier carpeta)

Columnas derivadas (todas calculadas aquí):
  - t_rel_s: segundos desde el despegue estimado (vuelos.csv, t_lanzamiento_est_utc). Es el único
    T0 del reporte; en la Orbitium sale de la detección de despegue del firmware y está acotado entre
    t_lanzamiento_min_utc (armado) y t_lanzamiento_max_utc (fase 'vuelo').
  - ida_vuelta_s: t_respuesta_utc - t_encolado_utc (t_encolado tiene resolución de 1 s).
  - intervalo nominal: mediana de los intervalos entre uplinks consecutivos del periodo
    (Orbitium: 60 s del fPort 32; TAES: por formato de payload, 26 B o 27 B). Un hueco es
    un intervalo > 2 x nominal.
  - disponibilidad nocturna de gateway: bins de 5 min entre 17:00 y la última recepción
    del 28-oct en los que alguna sonda fue oída por algún gateway; se cuenta en cuántos
    de ellos cada gateway oyó al menos un uplink.
  - tiempos objetivo T+5/T+15/T+30: para cada uno se da el Marco respondido más cercano y su
    desfase (con el rango que deja la incertidumbre del T0). Es una descripción de tiempos:
    este análisis no evalúa si un comando cumplió o no una ventana.
"""
import csv
import os
import statistics
from collections import Counter, defaultdict
from datetime import datetime, timedelta, timezone

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.lines import Line2D

AQUI = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(AQUI)
DATOS = os.path.join(BASE, "datos")
FIG = os.path.join(BASE, "figuras")
DER = os.path.join(DATOS, "derivados")
os.makedirs(FIG, exist_ok=True)
os.makedirs(DER, exist_ok=True)

LOCAL = timezone(timedelta(hours=-6))
ORB = "Sonda Orbitium (Sci-Fi), ITPA"
TAES = "Sonda TAES (Instituto Tepeyac/UPA)"
GWS = ["ITPA-IOT-GW00", "ITPA-IOT-GW01-kibts", "picaro-lab-ags-001"]

# --- estilo (paleta de referencia del skill dataviz, modo claro) ---
SUP = "#fcfcfb"; TXT = "#0b0b0b"; TXT2 = "#52514e"; GRID = "#e4e3df"
C_ORB = "#2a78d6"   # slot 1 azul  -> Sonda Orbitium
C_TAES = "#eb6834"   # slot 2 naranja -> Sonda TAES
C_GW = {"ITPA-IOT-GW00": "#1baf7a", "ITPA-IOT-GW01-kibts": "#6250d6",
        "picaro-lab-ags-001": "#52514e"}
C_FALLO = "#e34948"
plt.rcParams.update({
    "figure.facecolor": SUP, "axes.facecolor": SUP, "savefig.facecolor": SUP,
    "axes.edgecolor": TXT2, "axes.labelcolor": TXT, "xtick.color": TXT2,
    "ytick.color": TXT2, "text.color": TXT, "axes.grid": True, "grid.color": GRID,
    "grid.linewidth": 0.6, "axes.spines.top": False, "axes.spines.right": False,
    "font.size": 9, "axes.titlesize": 10, "axes.titleweight": "bold",
    "legend.frameon": False, "lines.linewidth": 2,
})


def leer(nombre):
    with open(os.path.join(DATOS, nombre), encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def t(s):
    return datetime.fromisoformat(s.replace("Z", "+00:00")) if s else None


def loc(dt):
    return dt.astimezone(LOCAL)


def hl(dt):
    return loc(dt).strftime("%d-%b %H:%M:%S") if dt else ""


def fnum(s):
    return float(s) if s not in ("", None) else None


def escribir(nombre, filas, campos):
    with open(os.path.join(DER, nombre), "w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=campos)
        w.writeheader()
        for r in filas:
            w.writerow(r)


up = leer("uplinks.csv")
rec = leer("recepciones.csv")
dl = leer("downlinks.csv")
jn = leer("joins.csv")
ev = leer("eventos_red.csv")
vu = {r["sonda"]: r for r in leer("vuelos.csv")}
gm = leer("gateway_metricas.csv")

for r in up:
    r["_t"] = t(r["t_utc"])
for r in rec:
    r["_t"] = t(r["t_utc"])
for r in jn:
    r["_t"] = t(r["t_utc"])
for r in ev:
    r["_t"] = t(r["t_utc"])

T0_EST = t(vu[ORB]["t_lanzamiento_est_utc"])
T0_FW = t(vu[ORB]["t_fase_vuelo_firmware_utc"])
T_ARM_FW = t(vu[ORB]["t_armado_firmware_utc"])
T0_TAES = t(vu[TAES]["t_lanzamiento_est_utc"])
T0_MIN = t(vu[ORB]["t_lanzamiento_min_utc"]) if vu[ORB].get("t_lanzamiento_min_utc") else T0_EST
T0_MAX = t(vu[ORB]["t_lanzamiento_max_utc"]) if vu[ORB].get("t_lanzamiento_max_utc") else T0_EST
T_CORTE = t(vu[ORB]["t_corte_inicio_utc"])
NOCHE_INI = datetime(2026, 10, 28, 17, 0, tzinfo=LOCAL)
NOCHE_FIN = datetime(2026, 10, 29, 0, 0, tzinfo=LOCAL)

print("=" * 72)
print("A3 · OPERACIONES DE MISIÓN — cifras para el borrador")
print("=" * 72)

# ---------------------------------------------------------------- 1. cronología
print("\n[1] CRONOLOGÍA DE LA NOCHE (hora local, 28-oct)")
crono = []


def hito(dt, sonda, que, fuente):
    crono.append({"t_local": loc(dt).isoformat(timespec="seconds"), "sonda": sonda,
                  "hito": que, "fuente": fuente})


for s in (ORB, TAES):
    u = sorted([r for r in up if r["sonda"] == s], key=lambda r: r["_t"])
    noche = [r for r in u if r["_t"] >= NOCHE_INI]
    j_s = sorted([r for r in jn if r["sonda"] == s], key=lambda r: r["_t"])
    ult_join = j_s[-1]
    # sesión activa al inicio de la noche y sesión de vuelo
    ses_vuelo = Counter(r["sesion"] for r in u if r["en_ventana_vuelo"] == "1")
    print(f"  {s}")
    print(f"    primer uplink del dataset: {hl(u[0]['_t'])}; primer uplink desde 17:00: "
          f"{hl(noche[0]['_t'])} (sesión {noche[0]['sesion']}, fCnt {noche[0]['fcnt']})")
    print(f"    último join: {hl(ult_join['_t'])} (sesión {ult_join['sesion']}, clase {ult_join['clase']}); "
          f"sesiones en la ventana de vuelo: {dict(ses_vuelo)}")
    hito(noche[0]["_t"], s, "primer uplink desde las 17:00", "uplinks.csv")
    hito(ult_join["_t"], s, f"último join (sesión {ult_join['sesion']}); ya no hubo más", "joins.csv")
    hito(t(vu[s]["t_ultimo_suelo_utc"]), s, "último fix en tierra", "vuelos.csv")
    hito(t(vu[s]["t_lanzamiento_est_utc"]), s, "despegue estimado", "vuelos.csv")
    hito(t(vu[s]["t_primer_aire_utc"]), s, "primer fix en el aire", "vuelos.csv")
    hito(t(vu[s]["t_alt_max_utc"]), s, f"altitud máxima {vu[s]['alt_max_m']} m", "vuelos.csv")
    hito(t(vu[s]["t_ultima_recepcion_utc"]), s, "última recepción", "vuelos.csv")
    if s == ORB:
        hito(T_ARM_FW, s, "temporizador armado (firmware)", "vuelos.csv")
        hito(T0_FW, s, "fase 'vuelo' del firmware (detección de despegue)", "vuelos.csv")
        hito(t(vu[s]["t_corte_inicio_utc"]), s, "inicio de corte por downlink", "vuelos.csv")
        hito(t(vu[s]["t_corte_confirmado_utc"]), s, "corte confirmado", "vuelos.csv")
    if s == TAES:
        errs = [r for r in ev if r["sonda"] == s and r["nivel"] == "ERROR"]
        hito(min(r["_t"] for r in errs), s, "primer join-request rechazado", "eventos_red.csv")
        print(f"    joins rechazados antes del primero aceptado: {len(errs)} "
              f"({Counter(r['codigo'] for r in errs)}) entre {hl(min(r['_t'] for r in errs))} y "
              f"{hl(max(r['_t'] for r in errs))}")
        fm = sorted([r for r in u if r["bytes_payload"] == "27"], key=lambda r: r["_t"])
        hito(fm[0]["_t"], s, "primer paquete en formato de 27 B (cambio de firmware)", "uplinks.csv")
        print(f"    primer paquete de 27 B: {hl(fm[0]['_t'])}")
for d in dl:
    if d["sonda"] == ORB and t(d["t_encolado_utc"]) >= NOCHE_INI:
        hito(t(d["t_encolado_utc"]), ORB, f"downlink {d['tipo']} seq {d['seq']} "
             f"(respuesta: {d['respuesta']})", "downlinks.csv")
crono.sort(key=lambda r: r["t_local"])
escribir("a3_cronologia_noche.csv", crono, ["t_local", "sonda", "hito", "fuente"])
for c in crono:
    print(f"    {c['t_local'][11:19]}  {c['sonda'][:18]:18s}  {c['hito']}")
print(f"  despegue estimado de la Orbitium: {hl(T0_EST)} (acotado {hl(T0_MIN)} - {hl(T0_MAX)})")
print(f"  separación entre despegues estimados: {(T0_TAES - T0_EST).total_seconds() / 60:.1f} min")
print(f"  retraso del 1.er despegue vs 22:00: "
      f"{(T0_EST - datetime(2026, 10, 28, 22, 0, tzinfo=LOCAL)).total_seconds() / 60:.1f} min")

# ---------------------------------------------------------------- 2. downlinks
print("\n[2] DOWNLINKS DE LA ORB-FI")
tabla_dl = []
for d in dl:
    te = t(d["t_encolado_utc"]); tr = t(d["t_respuesta_utc"]); ta = t(d["t_txack_utc"])
    fila = {
        "dl_n": d["dl_n"], "tipo": d["tipo"], "fport": d["fport"], "seq": d["seq"],
        "t_encolado_local": hl(te), "txack_s": round((ta - te).total_seconds(), 1) if ta else "",
        "gateway_tx": d["gateway_tx"], "respuesta": d["respuesta"],
        "latencia_desde_txack_s": d["latencia_resp_s"],
        "ida_vuelta_s": round((tr - te).total_seconds(), 1) if tr else "",
        "rssi_dl_en_sonda_dbm": d["polo_rssi_dbm"], "snr_dl_en_sonda_db": d["polo_snr_db"],
        "t_rel_s": round((te - T0_EST).total_seconds()) if te >= T0_EST - timedelta(hours=3) else "",
        "dia_local": loc(te).strftime("%d-oct"),
    }
    tabla_dl.append(fila)
escribir("a3_downlinks.csv", tabla_dl, list(tabla_dl[0].keys()))
print("  por tipo y respuesta:", Counter((f["tipo"], f["respuesta"]) for f in tabla_dl))
print("  por día local:", Counter(f["dia_local"] for f in tabla_dl))
print("  gateway TX:", Counter(f["gateway_tx"] for f in tabla_dl))
print("  TX params:", Counter((d["frec_tx_mhz"], d["sf_tx"], d["bw_tx_khz"], d["potencia_tx_dbm"]) for d in dl),
      " txack:", Counter(d["resultado_txack"] for d in dl), " confirmados:", Counter(d["confirmado"] for d in dl))
lat = [float(f["latencia_desde_txack_s"]) for f in tabla_dl if f["latencia_desde_txack_s"]]
iv = [f["ida_vuelta_s"] for f in tabla_dl if f["ida_vuelta_s"] != ""]
tx = [f["txack_s"] for f in tabla_dl if f["txack_s"] != ""]
print(f"  latencia txack->respuesta: n={len(lat)} min {min(lat)} med {statistics.median(lat)} máx {max(lat)} s")
print(f"  encolado->txack: min {min(tx)} med {statistics.median(tx)} máx {max(tx)} s (encolado con resolución 1 s)")
print(f"  ida y vuelta encolado->respuesta: min {min(iv)} med {statistics.median(iv)} máx {max(iv)} s")
for g in GWS[:2]:
    l = [float(f["latencia_desde_txack_s"]) for f in tabla_dl if f["gateway_tx"] == g and f["latencia_desde_txack_s"]]
    print(f"    latencia mediana vía {g}: {statistics.median(l)} s (n={len(l)})")
for tp in ("marco", "corte", "prueba_corte", "armar_temporizador"):
    l = [float(f["latencia_desde_txack_s"]) for f in tabla_dl if f["tipo"] == tp and f["latencia_desde_txack_s"]]
    if l:
        print(f"    latencia mediana {tp}: {statistics.median(l)} s (n={len(l)})")

print("\n  En vuelo (desde el despegue estimado):")
envuelo = [f for f in tabla_dl if f["t_rel_s"] != "" and f["t_rel_s"] >= 0]
for f in envuelo:
    print(f"    {f['t_encolado_local'][7:]}  {f['tipo']:18s} seq {f['seq']:>3}  T+{f['t_rel_s']:>5} s  "
          f"{f['gateway_tx']:20s} resp={f['respuesta']}  "
          f"lat={f['latencia_desde_txack_s']}  RSSI/SNR dl={f['rssi_dl_en_sonda_dbm']}/{f['snr_dl_en_sonda_db']}")
marcos_v = [f for f in envuelo if f["tipo"] == "marco"]
ok_v = [f for f in marcos_v if f["respuesta"] == "si"]
print(f"  Marcos en vuelo: {len(marcos_v)} enviados, {len(ok_v)} con Polo; antes del corte: "
      f"{sum(1 for f in marcos_v if f['t_rel_s'] < (T_CORTE - T0_EST).total_seconds())} enviados")
print(f"  Gateway TX en vuelo: {Counter(f['gateway_tx'] for f in envuelo)}")

# tiempos objetivo T+5/15/30: descripción, sin veredicto
vent = []
for nombre, obj in (("T+5", 300), ("T+15", 900), ("T+30", 1800)):
    mejor = min(ok_v, key=lambda f: abs(f["t_rel_s"] - obj))
    te = t(next(x for x in dl if x["dl_n"] == mejor["dl_n"])["t_encolado_utc"])
    d = mejor["t_rel_s"] - obj
    d_lo = round((te - T0_MAX).total_seconds()) - obj
    d_hi = round((te - T0_MIN).total_seconds()) - obj
    vent.append({"objetivo": nombre, "objetivo_s": obj, "seq_marco_mas_cercano": mejor["seq"],
                 "t_encolado_local": mejor["t_encolado_local"][7:], "t_rel_s": mejor["t_rel_s"],
                 "desfase_s": d, "desfase_min_s": d_lo, "desfase_max_s": d_hi,
                 "latencia_s": mejor["latencia_desde_txack_s"],
                 "rssi_dl_dbm": mejor["rssi_dl_en_sonda_dbm"], "snr_dl_db": mejor["snr_dl_en_sonda_db"]})
escribir("a3_ventanas_comando.csv", vent, list(vent[0].keys()))
print("\n  Tiempos objetivo (Marco con Polo más cercano; desfase con el rango que deja el T0):")
for v in vent:
    print(f"    {v['objetivo']:5s} seq {v['seq_marco_mas_cercano']} a T+{v['t_rel_s']} s "
          f"(desfase {v['desfase_s']:+} s; entre {v['desfase_min_s']:+} y {v['desfase_max_s']:+} s)")
tm = sorted(f["t_rel_s"] for f in marcos_v)
hueco = max(zip(tm, tm[1:]), key=lambda p: p[1] - p[0])
print(f"  mayor intervalo sin Marcos en vuelo: T+{hueco[0] / 60:.1f} a T+{hueco[1] / 60:.1f} min")
for f in marcos_v:
    print(f"    Marco seq {f['seq']}: T+{f['t_rel_s'] / 60:.1f} min resp={f['respuesta']} SNR dl={f['snr_dl_en_sonda_db']}")

# Marcos sin respuesta: ¿qué pasó con los uplinks cercanos?
print("\n  Marcos sin Polo:")
for f in tabla_dl:
    if f["respuesta"] == "no":
        d = next(x for x in dl if x["dl_n"] == f["dl_n"])
        te = t(d["t_encolado_utc"])
        cerca = sorted([r for r in up if r["sonda"] == ORB and abs((r["_t"] - te).total_seconds()) <= 150],
                       key=lambda r: r["_t"])
        print(f"    seq {f['seq']} {f['t_encolado_local']} vía {f['gateway_tx']}; uplinks ±150 s: "
              + ", ".join(f"{hl(r['_t'])[7:]} fC{r['fcnt']}/p{r['fport']}" for r in cerca))
print(f"  última recepción Orbitium: {hl(t(vu[ORB]['t_ultima_recepcion_utc']))}")

# ---------------------------------------------------------------- 3. joins y huecos
print("\n[3] JOINS Y HUECOS")
for s in (ORB, TAES):
    j_s = sorted([r for r in jn if r["sonda"] == s], key=lambda r: r["_t"])
    porhora = Counter(loc(r["_t"]).strftime("%d-%H") for r in j_s)
    print(f"  {s}: {len(j_s)} joins; por día-hora local: {dict(porhora)}")
    t0 = t(vu[s]["t_lanzamiento_est_utc"]); t1 = t(vu[s]["t_ultima_recepcion_utc"])
    print(f"    joins en la ventana de vuelo: {sum(1 for r in j_s if t0 <= r['_t'] <= t1)}; "
          f"joins desde las 17:00: {sum(1 for r in j_s if r['_t'] >= NOCHE_INI)}")
    print(f"    sesión de vuelo abierta {((t0 - j_s[-1]['_t']).total_seconds() / 3600):.2f} h antes del lanzamiento")
print("  Logs por sonda y código:", Counter((r["sonda"][:12], r["codigo"]) for r in ev if r["tipo"] == "log"))
st = [r for r in ev if r["tipo"] == "status"]
print(f"  DevStatus TAES: {len(st)} respuestas, margen {min(fnum(r['margen_db']) for r in st)}-"
      f"{max(fnum(r['margen_db']) for r in st)} dB, todas antes de {hl(max(r['_t'] for r in st))}")
fcr = [r for r in ev if r["codigo"] in ("UPLINK_F_CNT_RETRANSMISSION", "UPLINK_F_CNT_RESET")]
print(f"  avisos de fCnt: {Counter((r['sonda'][:12], r['codigo']) for r in fcr)}")

huecos = []
for s in (ORB, TAES):
    u = sorted([r for r in up if r["sonda"] == s and r["_t"] >= NOCHE_INI], key=lambda r: r["_t"])
    grupos = defaultdict(list)
    for a, b in zip(u, u[1:]):
        if a["sesion"] != b["sesion"]:
            clave = "cambio de sesión (re-join)"
        else:
            clave = "fPort32" if s == ORB else f"sesión {b['sesion']} ({b['bytes_payload']} B)"
        grupos[clave].append((a, b, (b["_t"] - a["_t"]).total_seconds()))
    for clave, lista in grupos.items():
        if clave.startswith("cambio"):
            nom = None
        else:
            nom = 60.0 if s == ORB else statistics.median(x[2] for x in lista)
        print(f"  {s[:14]} [{clave}] intervalo nominal {nom if nom is None else round(nom)} s (n={len(lista)})")
        for a, b, g in lista:
            if nom is None:
                # en un re-join el hueco se compara con el nominal de la sesión siguiente
                sig = [y[2] for y in grupos.get(f"sesión {b['sesion']} ({b['bytes_payload']} B)", [])]
                nom_s = statistics.median(sig) if sig else 30.0
            else:
                nom_s = nom
            if g > 2 * nom_s:
                huecos.append({"sonda": s, "periodo": clave, "nominal_s": round(nom_s), "desde_local": hl(a["_t"]),
                               "hasta_local": hl(b["_t"]), "hueco_s": round(g),
                               "fcnt_antes": a["fcnt"], "fcnt_despues": b["fcnt"],
                               "sesion_antes": a["sesion"], "sesion_despues": b["sesion"],
                               "rejoin": "si" if a["sesion"] != b["sesion"] else "no",
                               "en_vuelo": b["en_ventana_vuelo"]})
escribir("a3_huecos_noche.csv", huecos, list(huecos[0].keys()))
for h in huecos:
    print(f"    {h['sonda'][:14]} {h['desde_local'][7:]} -> {h['hasta_local'][7:]}  {h['hueco_s']} s "
          f"(nom {h['nominal_s']}) fCnt {h['fcnt_antes']}->{h['fcnt_despues']} rejoin={h['rejoin']} vuelo={h['en_vuelo']}")
# llegada fuera de orden (Orbitium, misma sesión)
fo = []
u = sorted([r for r in up if r["sonda"] == ORB and r["_t"] >= NOCHE_INI], key=lambda r: r["_t"])
mx = {}
for r in u:
    k = r["sesion"]; f = int(r["fcnt"])
    if k in mx and f < mx[k]:
        fo.append(r)
    mx[k] = max(mx.get(k, -1), f)
print(f"  Orbitium: uplinks que llegaron con fCnt menor que uno ya recibido (fuera de orden) desde 17:00: {len(fo)}, "
      f"entre {hl(fo[0]['_t'])[7:]} y {hl(fo[-1]['_t'])[7:]}")
# ráfagas: uplinks separados < 1 s
raf = [b for a, b in zip(u, u[1:]) if (b["_t"] - a["_t"]).total_seconds() < 1.0]
print(f"  Orbitium: uplinks llegados < 1 s después del anterior: {len(raf)}; horas: "
      f"{sorted(set(hl(r['_t'])[7:12] for r in raf))}")
gw_raf = Counter(x["gateway"] for x in rec for r in raf
                 if x["sonda"] == ORB and x["sesion"] == r["sesion"] and x["fcnt"] == r["fcnt"])
print(f"    gateways que oyeron esas ráfagas: {dict(gw_raf)}")

# ---------------------------------------------------------------- 4. infraestructura
print("\n[4] INFRAESTRUCTURA")
fin_noche = max(r["_t"] for r in up)
bins = defaultdict(set)
for x in rec:
    if NOCHE_INI <= x["_t"] <= fin_noche:
        b = int((x["_t"] - NOCHE_INI).total_seconds() // 300)
        bins[b].add(x["gateway"])
nb = len(bins)
disp = []
for g in GWS:
    n = sum(1 for b in bins.values() if g in b)
    faltan = sorted(k for k, v in bins.items() if g not in v)
    disp.append({"gateway": g, "bins_5min_con_trafico": nb, "bins_con_recepcion": n, "pct": round(100 * n / nb, 1)})
    print(f"  {g}: oyó algo en {n}/{nb} bins de 5 min con tráfico de las sondas (17:00-{hl(fin_noche)[7:12]})")
    if g != "picaro-lab-ags-001":
        print("    bins sin recepción (inicio local):",
              [(NOCHE_INI + timedelta(minutes=5 * k)).strftime("%H:%M") for k in faltan])
for s, t0 in ((ORB, T0_EST), (TAES, T0_TAES)):
    t1 = t(vu[s]["t_ultima_recepcion_utc"])
    nup = sum(1 for r in up if r["sonda"] == s and r["en_ventana_vuelo"] == "1")
    for g in GWS:
        n = sum(1 for x in rec if x["sonda"] == s and x["gateway"] == g and x["en_ventana_vuelo"] == "1")
        print(f"    vuelo {s[:14]}: {g} {n}/{nup} uplinks")
    sin_itpa = [r for r in up if r["sonda"] == s and r["en_ventana_vuelo"] == "1" and not any(
        x["sonda"] == s and x["sesion"] == r["sesion"] and x["fcnt"] == r["fcnt"] and x["gateway"] != "picaro-lab-ags-001"
        for x in rec)]
    print(f"    vuelo {s[:14]}: uplinks oídos SOLO por el laboratorio: {len(sin_itpa)}")
escribir("a3_disponibilidad_noche.csv", disp, list(disp[0].keys()))

day = defaultdict(dict)
for r in gm:
    if r["agregacion"] == "DAY" and r["etiqueta"] == "total":
        day[(r["gateway"], r["metrica"])][r["t_utc"][:10]] = int(float(r["valor"]))
dias = sorted({d for v in day.values() for d in v})
print("  rx/tx DAY por gateway (día UTC):")
filas_day = []
for g in GWS:
    for d in dias:
        filas_day.append({"gateway": g, "dia_utc": d, "rx": day[(g, "rx_paquetes")].get(d, 0),
                          "tx": day[(g, "tx_paquetes")].get(d, 0)})
    print(f"    {g}: " + " ".join(f"{d[5:]}:{day[(g, 'rx_paquetes')].get(d, 0)}/{day[(g, 'tx_paquetes')].get(d, 0)}"
                                  for d in dias))
    cero = [d for d in dias if d >= "2026-10-27" and day[(g, "rx_paquetes")].get(d, 0) == 0]
    print(f"      días (27-oct a 2-nov) con rx = 0: {cero}")
escribir("a3_gateways_diario.csv", filas_day, ["gateway", "dia_utc", "rx", "tx"])
est = Counter((r["gateway"], r["etiqueta"]) for r in gm if r["metrica"] == "tx_por_estado")
print("  tx_por_estado (filas DAY):", {k: sum(int(float(r['valor'])) for r in gm if r['metrica'] == 'tx_por_estado'
                                         and (r['gateway'], r['etiqueta']) == k) for k in est})
print("  tx_por_estado distinto de OK:", [(r["gateway"], r["t_utc"][:10], r["etiqueta"], r["valor"]) for r in gm
                                       if r["metrica"] == "tx_por_estado" and r["etiqueta"] != "OK"])
for g in GWS:
    h = sorted(r["t_utc"] for r in gm if r["gateway"] == g and r["agregacion"] == "HOUR"
               and r["metrica"] == "rx_paquetes")
    if h:
        ini = t(h[0]); fin = t(h[-1]); tot = int((fin - ini).total_seconds() // 3600) + 1
        print(f"    HOUR {g}: {len(h)} horas con rx>0 de {tot} entre {h[0][:13]} y {h[-1][:13]} UTC")

# ---------------------------------------------------------------- FIGURA 1: línea de tiempo de la noche
CARR = {"TAES · comandos": 0.0, "TAES · uplinks": 1.0, "TAES · joins": 2.0,
        "Orbitium · comandos": 3.3, "Orbitium · uplinks": 4.3, "Orbitium · joins": 5.3}
mk = {"marco": "o", "prueba_corte": "s", "corte": "v", "armar_temporizador": "^"}
HORARIO = [(datetime(2026, 10, 28, 17, 30, tzinfo=LOCAL), datetime(2026, 10, 28, 19, 45, tzinfo=LOCAL), "Trabajo libre"),
          (datetime(2026, 10, 28, 19, 45, tzinfo=LOCAL), datetime(2026, 10, 28, 21, 15, tzinfo=LOCAL), "Fase 1 (tierra)"),
          (datetime(2026, 10, 28, 22, 0, tzinfo=LOCAL), datetime(2026, 10, 29, 0, 0, tzinfo=LOCAL), "Lanzamientos")]


def carriles(ax, x0, x1, zoom):
    for (a, b, n), c in zip(HORARIO, ["#f1f0ec", "#e9e8e3", "#f1f0ec"]):
        if b < x0 or a > x1:
            continue
        ax.axvspan(a, b, color=c, zorder=0, lw=0)
        if not zoom:
            ax.text(max(a, x0) + (min(b, x1) - max(a, x0)) / 2, 5.85, n, ha="center", va="bottom", color=TXT2, fontsize=8)
    for hh, mm in ((22, 0), (22, 30), (23, 0)):
        x = datetime(2026, 10, 28, hh, mm, tzinfo=LOCAL)
        if x0 <= x <= x1:
            ax.axvline(x, color=TXT2, lw=0.8, ls=(0, (2, 2)), zorder=1)
            if zoom:
                ax.text(x, 5.95, f"{hh}:{mm:02d}", fontsize=7, color=TXT2, ha="center", va="bottom")
    for s, y, c in ((ORB, 4.3, C_ORB), (TAES, 1.0, C_TAES)):
        a = loc(t(vu[s]["t_lanzamiento_est_utc"])); b = loc(t(vu[s]["t_ultima_recepcion_utc"]))
        ax.barh(y, (b - a), left=a, height=0.62, color=c, alpha=0.18, zorder=1, lw=0)
        xs = [loc(r["_t"]) for r in up if r["sonda"] == s and x0 <= loc(r["_t"]) <= x1]
        ax.scatter(xs, [y] * len(xs), marker="|", s=60, color=c, lw=0.8, zorder=3)
    for s, y, c in ((ORB, 5.3, C_ORB), (TAES, 2.0, C_TAES)):
        xs = [loc(r["_t"]) for r in jn if r["sonda"] == s and x0 <= loc(r["_t"]) <= x1]
        ax.scatter(xs, [y] * len(xs), marker="D", s=34, color=c, edgecolor=SUP, lw=1, zorder=4)
        xe = [loc(r["_t"]) for r in ev if r["sonda"] == s and r["nivel"] == "ERROR" and x0 <= loc(r["_t"]) <= x1]
        ax.scatter(xe, [y] * len(xe), marker="x", s=30, color=C_FALLO, lw=1.2, zorder=4)
    for d in dl:
        te = loc(t(d["t_encolado_utc"]))
        if not (x0 <= te <= x1):
            continue
        if d["respuesta"] == "si":
            ax.scatter([te], [3.3], marker=mk[d["tipo"]], s=48, color=C_ORB, edgecolor=SUP, lw=1, zorder=5)
        else:
            ax.scatter([te], [3.3], marker=mk[d["tipo"]], s=48, facecolor=SUP, edgecolor=C_FALLO, lw=1.4, zorder=5)
    ax.set_yticks(list(CARR.values()))
    ax.set_yticklabels(list(CARR.keys()))
    ax.set_ylim(-0.6, 6.5)
    ax.set_xlim(x0, x1)
    ax.grid(axis="y", visible=False)


fig, (axA, axB) = plt.subplots(2, 1, figsize=(11, 8.4), gridspec_kw={"height_ratios": [1, 1.1]})
xA0, xA1 = NOCHE_INI, NOCHE_FIN
carriles(axA, xA0, xA1, False)
axA.xaxis.set_major_locator(mdates.HourLocator(tz=LOCAL))
axA.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M", tz=LOCAL))
axA.text(xA0 + timedelta(minutes=3), 5.55, "sin joins desde las 10:40 (una sola sesión hasta el fin del vuelo)",
         fontsize=7, color=TXT2, ha="left", va="center")
axA.text(datetime(2026, 10, 28, 20, 30, tzinfo=LOCAL), 0.0, "sin downlinks encolados (Clase A)",
         fontsize=7.5, color=TXT2, va="center", ha="center")
axA.annotate("último join 21:41", (loc(max(r["_t"] for r in jn if r["sonda"] == TAES)), 2.0), xytext=(0, 12),
             textcoords="offset points", fontsize=7, color=TXT2, ha="center")
axA.annotate("17 join-requests rechazados", (datetime(2026, 10, 28, 18, 33, tzinfo=LOCAL), 2.0), xytext=(-10, 12),
             textcoords="offset points", fontsize=7, color=TXT2, ha="center")
axA.annotate("huecos y llegadas\nfuera de orden", (datetime(2026, 10, 28, 17, 50, tzinfo=LOCAL), 4.3), xytext=(0, -30),
             textcoords="offset points", fontsize=7, color=TXT2, ha="center", va="top",
             arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.6))
zA, zB = datetime(2026, 10, 28, 22, 20, tzinfo=LOCAL), datetime(2026, 10, 28, 23, 50, tzinfo=LOCAL)
axA.axvspan(zA, zB, facecolor="none", edgecolor=TXT2, lw=0.8, ls="-", zorder=6)
axA.set_title("Noche del 28-oct: uplinks, joins y comandos de las dos sondas frente al horario de la noche", loc="left")
axA.set_xlabel("Hora local (UTC-6)")

carriles(axB, zA, zB, True)
axB.xaxis.set_major_locator(mdates.MinuteLocator(byminute=[0, 10, 20, 30, 40, 50], tz=LOCAL))
axB.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M", tz=LOCAL))
axB.set_title("Detalle 22:20-23:50: ventanas de vuelo y Fase 2", loc="left")
axB.set_xlabel("Hora local del 28-oct-2026 (UTC-6)")
etiq = [(T0_EST, 4.3, f"despegue\n{hl(T0_EST)[7:12]}", 0), (t(vu[ORB]["t_corte_inicio_utc"]), 4.3, "corte\n23:21", -10),
        (t(vu[ORB]["t_ultima_recepcion_utc"]), 4.3, "última\n23:34", 10),
        (T0_TAES, 1.0, f"despegue\n{hl(T0_TAES)[7:12]}", 0), (t(vu[TAES]["t_alt_max_utc"]), 1.0, "alt. máx.\n23:21", 0),
        (t(vu[TAES]["t_ultima_recepcion_utc"]), 1.0, "última\n23:36", 8)]
for x, y, n, dx in etiq:
    axB.annotate(n, (loc(x), y + 0.31), xytext=(dx, 8), textcoords="offset points", ha="center", va="bottom",
                 fontsize=7, color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.6))
for obj in (5, 15, 30):
    x = loc(T0_EST + timedelta(minutes=obj))
    axB.annotate(f"T+{obj}", (x, 3.3), xytext=(0, -24), textcoords="offset points", ha="center", fontsize=7,
                 color=TXT2, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.6))
axB.text(loc(T0_EST + timedelta(minutes=47)), 2.75, f"T+ contado desde el despegue\nestimado ({hl(T0_EST)[7:15]})",
         fontsize=7, color=TXT2, ha="left", va="top")
ley = [Line2D([], [], marker="|", ls="", color=TXT2, ms=9, label="uplink recibido"),
       Line2D([], [], marker="D", ls="", color=TXT2, ms=5, label="join aceptado"),
       Line2D([], [], marker="x", ls="", color=C_FALLO, ms=6, label="join-request rechazado"),
       Line2D([], [], marker="o", ls="", color=TXT2, ms=6, label="Marco"),
       Line2D([], [], marker="s", ls="", color=TXT2, ms=6, label="prueba de corte (TEST)"),
       Line2D([], [], marker="^", ls="", color=TXT2, ms=6, label="armado del temporizador"),
       Line2D([], [], marker="v", ls="", color=TXT2, ms=6, label="corte (CORT)"),
       Line2D([], [], marker="o", ls="", markerfacecolor=SUP, markeredgecolor=C_FALLO, ms=6, label="comando sin respuesta"),
       Line2D([], [], color=C_ORB, lw=6, alpha=0.3, label="vuelo Sonda Orbitium (Sci-Fi), ITPA"),
       Line2D([], [], color=C_TAES, lw=6, alpha=0.3, label="vuelo Sonda TAES (Instituto Tepeyac/UPA)")]
axB.legend(handles=ley, loc="upper center", bbox_to_anchor=(0.5, -0.16), ncol=5, fontsize=7.5)
fig.tight_layout()
fig.savefig(os.path.join(FIG, "a3_linea_tiempo_noche.png"), dpi=150)
plt.close(fig)

# ---------------------------------------------------------------- FIGURA 2: Fase 2
fig, (a1, a2) = plt.subplots(2, 1, figsize=(10, 6.2), sharex=True, gridspec_kw={"height_ratios": [1.3, 1]})
pos = sorted([r for r in up if r["sonda"] == ORB and r["en_ventana_vuelo"] == "1" and r["calidad_gps"] in ("ok", "solo_alt")
              and r["alt_m"]], key=lambda r: r["_t"])
xs = [(r["_t"] - T0_EST).total_seconds() / 60 for r in pos]
a1.plot(xs, [float(r["alt_m"]) / 1000 for r in pos], color=C_ORB, marker="o", ms=4)
for obj in (5, 15, 30):
    for a in (a1, a2):
        a.axvline(obj, color=TXT2, lw=0.6, ls=":")
    a1.text(obj, 9.1, f"T+{obj}", ha="center", fontsize=8, color=TXT2)
for f in tabla_dl:
    if f["t_rel_s"] == "" or f["t_rel_s"] < -120:
        continue
    x = f["t_rel_s"] / 60
    m = mk[f["tipo"]]
    if f["respuesta"] == "si":
        a1.scatter([x], [0.35], marker=m, s=46, color=C_ORB, edgecolor=SUP, lw=1, zorder=5)
    else:
        a1.scatter([x], [0.35], marker=m, s=46, facecolor=SUP, edgecolor=C_FALLO, lw=1.4, zorder=5)
    if f["rssi_dl_en_sonda_dbm"]:
        a2.scatter([x], [float(f["rssi_dl_en_sonda_dbm"])], marker="o", s=40, color=C_ORB, edgecolor=SUP, lw=1, zorder=5)
t_cut = (T_CORTE - T0_EST).total_seconds() / 60
a1.annotate("corte por downlink\n8478 m", (t_cut, 8.478), xytext=(-70, -6), textcoords="offset points", fontsize=7.5,
            arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.6))
a1.text(-3.5, 0.8, "comandos enviados", fontsize=7, color=TXT2)
xe = (T0_FW - T0_EST).total_seconds() / 60
a1.axvline(xe, color=TXT2, lw=0.8, ls="--")
a1.text(xe + 0.4, 1.5, f"detección de despegue del firmware ({hl(T0_FW)[7:15]});\n"
        f"el despegue quedó entre el armado ({hl(T0_MIN)[7:15]}) y esa detección",
        fontsize=7, color=TXT2, ha="left")
a1.text(46, 2.2, "fixes cada ~10 min: la recta une\nfixes, no es la trayectoria", fontsize=7, color=TXT2)
a1.set_ylabel("Altitud GPS (km)")
a1.set_ylim(0, 9.6)
a1.set_title("Fase 2 de la Sonda Orbitium (Sci-Fi), ITPA: comandos frente a los tiempos objetivo T+5/15/30", loc="left")
a2.set_ylabel("RSSI del Marco en la sonda (dBm)")
a2.set_xlabel(f"Minutos desde el despegue estimado ({hl(T0_EST)[7:15]} local)")
a2.set_ylim(-130, -85)
ley = [Line2D([], [], marker="o", ls="", color=C_ORB, ms=6, label="Marco con Polo"),
       Line2D([], [], marker="o", ls="", markerfacecolor=SUP, markeredgecolor=C_FALLO, ms=6, label="Marco sin Polo"),
       Line2D([], [], marker="^", ls="", color=C_ORB, ms=6, label="armado del temporizador"),
       Line2D([], [], marker="v", ls="", color=C_ORB, ms=6, label="corte (CORT)")]
a2.legend(handles=ley, loc="lower left", fontsize=7.5, ncol=2)
fig.tight_layout()
fig.savefig(os.path.join(FIG, "a3_fase2_comandos.png"), dpi=150)
plt.close(fig)

# ---------------------------------------------------------------- FIGURA 3: métricas diarias
fig, axs = plt.subplots(3, 1, figsize=(9, 6.4), sharex=True)
xd = [datetime.fromisoformat(d) for d in dias]
for a, g in zip(axs, GWS):
    v = [day[(g, "rx_paquetes")].get(d, 0) for d in dias]
    a.bar(xd, v, width=0.75, color=C_GW[g], zorder=3)
    a.set_title(g, loc="left", fontsize=9)
    a.set_ylabel("paquetes rx / día")
    a.axvspan(datetime(2026, 10, 28, 12), datetime(2026, 10, 29, 12), color="#e9e8e3", lw=0, zorder=0)
    i = dias.index("2026-10-29")
    a.annotate(f"{v[i]}", (xd[i], v[i]), xytext=(0, 2), textcoords="offset points", ha="center", fontsize=7)
axs[0].text(datetime(2026, 10, 28, 9), axs[0].get_ylim()[1] * 0.78, "día UTC\ndel vuelo", ha="right", fontsize=7,
            color=TXT2)
MES = {"10": "oct", "11": "nov"}
axs[-1].set_xticks([datetime.fromisoformat(d) for d in dias[::2]])
axs[-1].set_xticklabels([f"{int(d[8:])}-{MES[d[5:7]]}" for d in dias[::2]])
axs[-1].set_xlabel("Día UTC (el 29-oct UTC abarca de las 18:00 del 28-oct a las 18:00 del 29-oct, hora local)")
fig.suptitle("Paquetes recibidos por día y gateway (todo el tráfico, métricas DAY del LNS)", x=0.01, ha="left",
             fontweight="bold", fontsize=10)
fig.tight_layout()
fig.savefig(os.path.join(FIG, "a3_gateways_diario.png"), dpi=150)
plt.close(fig)
print("\nFiguras escritas en", FIG)
print("Tablas derivadas en", DER)
