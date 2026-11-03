# -*- coding: utf-8 -*-
"""
A2 · Perfil de vuelo y atmósfera — Misiones Espaciales SatLink 2026 (ITPA, 28-oct-2026).

Lee SOLO los CSV anonimizados de ../datos/ (uplinks.csv, vuelos.csv, gateways.csv)
y produce:
  - figuras ../figuras/a2_*.png (matplotlib, 150 dpi)
  - tablas derivadas ../datos/derivados/a2_*.csv (cálculos de este script, documentados abajo)
  - un resumen de cifras por la salida estándar

Ejecutar:  python -I a2_vuelo.py      (desde cualquier carpeta)

Cálculos derivados (todos reproducibles con este script):
  * Velocidad vertical: TAES = pendiente de una regresión lineal z(t) en una ventana
    centrada de +-60 s sobre fixes "ok" (5 puntos a 30 s). Orbitium = diferencia entre
    fixes consecutivos (cada ~10 min) y la de los eventos del fPort 31.
  * Viento por capa (método del globo piloto): desplazamiento horizontal entre el primer
    y el último fix "ok" de cada capa de 500 m (TAES) o entre fixes consecutivos (Orbitium),
    dividido entre el tiempo. Dirección meteorológica = de dónde sopla (rumbo + 180°).
  * Atmósfera estándar ISA (troposfera): T = 288.15 - 0.0065 z [K];
    p = 1013.25 (T/288.15)^5.25588 [hPa]; z_ISA(p) = 44330.8 (1 - (p/1013.25)^0.190263).
  * Temperatura virtual media de capa por la ecuación hipsométrica:
    Tv = g dz / (Rd ln(p1/p2)), con dz de la altitud GPS y p del BME280
    (g = 9.80665 m/s², Rd = 287.05 J/(kg K)). Es una ESTIMACIÓN indirecta de la temperatura
    del aire: en vuelo no se midió nada en el exterior. Todos los sensores (temperatura,
    humedad y presión) iban dentro de la cápsula; la presión de la cápsula sirve porque la
    cápsula no era estanca y su presión sigue a la del aire.
  * Razón de mezcla w = 622 e/(p - e) [g/kg], e = HR/100 · es(T), es de Magnus
    (6.112 exp(17.62 T/(243.12 + T)) hPa). Punto de rocío por la inversa de Magnus.
  * Descenso: v(z) = v0 sqrt(rho0/rho(z)) (velocidad terminal), rho = p/(Rd T) con la
    T de la ISA desplazada al perfil estimado; v0 se ajusta a los datos.
"""
import csv
import math
import os
import statistics as st
from datetime import datetime, timedelta, timezone

import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

AQUI = os.path.dirname(os.path.abspath(__file__))
DATOS = os.path.normpath(os.path.join(AQUI, "..", "datos"))
FIG = os.path.normpath(os.path.join(AQUI, "..", "figuras"))
DERIV = os.path.join(DATOS, "derivados")
os.makedirs(FIG, exist_ok=True)
os.makedirs(DERIV, exist_ok=True)

ORB = "Sonda Orbitium (Sci-Fi), ITPA"
TAES = "Sonda TAES (Instituto Tepeyac/UPA)"
NOMBRE_CORTO = {ORB: "Sonda Orbitium (Sci-Fi), ITPA", TAES: "Sonda TAES (Instituto Tepeyac/UPA)"}
CORTA = {ORB: "Orbitium", TAES: "TAES"}  # etiqueta corta para tablas de consola

# ---------------------------------------------------------------- estilo (skill dataviz)
C = {ORB: "#2a78d6", TAES: "#eb6834"}      # categóricas 1 y 2 (validadas, modo claro)
TXT = "#0b0b0b"
TXT2 = "#52514e"
MUTED = "#8a8984"
GRID = "#e4e3dd"
SURF = "#fcfcfb"
REF = "#6f6e69"                               # referencias (ISA, 5 m/s): gris neutro
CRIT = "#d03b3b"                              # estado "critical", solo para el corte

plt.rcParams.update({
    "figure.facecolor": SURF, "axes.facecolor": SURF, "savefig.facecolor": SURF,
    "axes.edgecolor": GRID, "axes.labelcolor": TXT2, "axes.titlecolor": TXT,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8, "grid.linestyle": "-",
    "axes.spines.top": False, "axes.spines.right": False,
    "xtick.color": TXT2, "ytick.color": TXT2, "text.color": TXT,
    "font.size": 10, "axes.titlesize": 11.5, "axes.titleweight": "semibold",
    "axes.titlelocation": "left", "legend.frameon": False, "lines.linewidth": 2,
    "axes.axisbelow": True,
})

try:
    sys.stdout.reconfigure(encoding="utf-8")
except Exception:
    pass

G = 9.80665
RD = 287.05
UTC6 = timezone(timedelta(hours=-6))


# ---------------------------------------------------------------- utilidades
def t(s):
    return datetime.fromisoformat(s.replace("Z", "+00:00"))


def f(x):
    return float(x) if x not in ("", None) else None


def hav(lat1, lon1, lat2, lon2):
    R = 6371000.0
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = p2 - p1, math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * R * math.asin(math.sqrt(a))


def rumbo(lat1, lon1, lat2, lon2):
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dl = math.radians(lon2 - lon1)
    y = math.sin(dl) * math.cos(p2)
    x = math.cos(p1) * math.sin(p2) - math.sin(p1) * math.cos(p2) * math.cos(dl)
    return (math.degrees(math.atan2(y, x)) + 360) % 360


PUNTOS = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
          "S", "SSO", "SO", "OSO", "O", "ONO", "NO", "NNO"]


def cardinal(g):
    return PUNTOS[int((g + 11.25) // 22.5) % 16]


def isa_T(z):
    return 288.15 - 0.0065 * z


def isa_p(z):
    return 1013.25 * (isa_T(z) / 288.15) ** 5.25588


def isa_z(p):
    return 44330.8 * (1 - (p / 1013.25) ** 0.190263)


def es_magnus(Tc):
    return 6.112 * math.exp(17.62 * Tc / (243.12 + Tc))


def rocio(Tc, hr):
    g_ = math.log(hr / 100.0) + 17.62 * Tc / (243.12 + Tc)
    return 243.12 * g_ / (17.62 - g_)


def mezcla(Tc, hr, p):
    e = hr / 100.0 * es_magnus(Tc)
    return 622.0 * e / (p - e)


def regresion(xs, ys):
    n = len(xs)
    mx, my = sum(xs) / n, sum(ys) / n
    sxx = sum((x - mx) ** 2 for x in xs)
    sxy = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    b = sxy / sxx
    a = my - b * mx
    if n > 2:
        res = sum((y - a - b * x) ** 2 for x, y in zip(xs, ys))
        se_b = math.sqrt(res / (n - 2) / sxx)
    else:
        se_b = float("nan")
    return a, b, se_b


def interp(ts, ys, tq, extrap_s=0):
    """Interpolación lineal en el tiempo; extrapola hasta extrap_s s con la última pendiente."""
    if tq < ts[0] or tq > ts[-1] + extrap_s:
        return None
    for i in range(1, len(ts)):
        if tq <= ts[i]:
            w = (tq - ts[i - 1]) / (ts[i] - ts[i - 1])
            return ys[i - 1] + w * (ys[i] - ys[i - 1])
    b = (ys[-1] - ys[-2]) / (ts[-1] - ts[-2])
    return ys[-1] + b * (tq - ts[-1])


def guardar_csv(nombre, filas, cols):
    ruta = os.path.join(DERIV, nombre)
    with open(ruta, "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=cols)
        w.writeheader()
        for r in filas:
            w.writerow({c: ("" if r.get(c) is None else
                            (round(r[c], 4) if isinstance(r[c], float) else r[c])) for c in cols})
    return ruta


def guardar_fig(fig, nombre):
    ruta = os.path.join(FIG, nombre)
    fig.savefig(ruta, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print("  figura:", os.path.relpath(ruta, os.path.join(AQUI, "..")))


def hl(dt):
    return dt.astimezone(UTC6).strftime("%H:%M:%S")


# ---------------------------------------------------------------- carga
def leer(n):
    with open(os.path.join(DATOS, n), encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


UP = leer("uplinks.csv")
VU = {r["sonda"]: r for r in leer("vuelos.csv")}
GW = {r["gateway"]: r for r in leer("gateways.csv")}
for r in UP:
    r["_t"] = t(r["t_utc"])

T0 = {s: t(VU[s]["t_lanzamiento_est_utc"]) for s in VU}
TFIN = {s: t(VU[s]["t_ultima_recepcion_utc"]) for s in VU}
SUELO = {s: float(VU[s]["suelo_m"]) for s in VU}
ITPA = (float(GW["ITPA-IOT-GW00"]["lat"]), float(GW["ITPA-IOT-GW00"]["lon"]))

out = []  # líneas del resumen


def say(*a):
    s = " ".join(str(x) for x in a)
    out.append(s)
    print(s)


def de(sonda, desde_min=-40):
    """Uplinks de la sonda desde 'desde_min' minutos antes del lanzamiento hasta el fin."""
    a = T0[sonda] + timedelta(minutes=desde_min)
    return [r for r in UP if r["sonda"] == sonda and a <= r["_t"] <= TFIN[sonda]]


# ---------------------------------------------------------------- series de altitud
def fixes(sonda, con_solo_alt=True, desde_min=-40):
    q = {"ok"} | ({"solo_alt"} if con_solo_alt else set())
    L = []
    for r in de(sonda, desde_min):
        if r["calidad_gps"] in q and r["alt_m"]:
            L.append(r)
    return L


say("=== A2 · perfil de vuelo y atmósfera ===")
FX = {s: fixes(s) for s in (ORB, TAES)}
for s in FX:
    say(f"{s}: lanzamiento est. {hl(T0[s])} local; fin {hl(TFIN[s])}; fixes usados {len(FX[s])}")

# --- Orbitium: velocidades por segmento
say("\n-- Orbitium: segmentos entre fixes (GPS) --")
# primer tramo: del despegue (T0 de vuelos.csv, a la altitud de 'suelo') al primer fix en el aire;
# no se usa el segmento que empieza en el último fix en tierra (mezclaría reposo y vuelo)
sf = [{"_t": T0[ORB], "alt_m": SUELO[ORB]}] + [r for r in FX[ORB] if r["_t"] > T0[ORB]]
seg_sf = []
for a, b in zip(sf, sf[1:]):
    dt_ = (b["_t"] - a["_t"]).total_seconds()
    dz = float(b["alt_m"]) - float(a["alt_m"])
    seg = {"sonda": ORB, "t_ini_local": hl(a["_t"]), "t_fin_local": hl(b["_t"]),
           "z_ini_m": float(a["alt_m"]), "z_fin_m": float(b["alt_m"]), "dt_s": dt_,
           "vz_mps": dz / dt_, "z_medio_m": (float(a["alt_m"]) + float(b["alt_m"])) / 2,
           "t_medio_min": ((a["_t"] - T0[ORB]).total_seconds() + dt_ / 2) / 60}
    seg_sf.append(seg)
    say(f"  {seg['t_ini_local']} -> {seg['t_fin_local']}: {seg['z_ini_m']:.0f} -> {seg['z_fin_m']:.0f} m, "
        f"{dt_:.0f} s, vz = {seg['vz_mps']:+.2f} m/s")
asc_sf = [s_ for s_ in seg_sf if s_["vz_mps"] > 0.5 and s_["z_ini_m"] > SUELO[ORB] + 50]
z_top_sf = float(VU[ORB]["alt_max_m"])
t_top_sf = t(VU[ORB]["t_alt_max_utc"])
vz_media_sf = (z_top_sf - SUELO[ORB]) / (t_top_sf - T0[ORB]).total_seconds()
say(f"  ascenso medio lanzamiento->alt. máx.: {vz_media_sf:.2f} m/s "
    f"({(t_top_sf - T0[ORB]).total_seconds()/60:.1f} min para {z_top_sf - SUELO[ORB]:.0f} m)")

# --- TAES: velocidad vertical suavizada (+-60 s)
say("\n-- TAES: velocidad vertical suavizada (regresión +-60 s) --")
upa = [r for r in FX[TAES] if r["_t"] >= T0[TAES] - timedelta(minutes=1)]
tu = [(r["_t"] - T0[TAES]).total_seconds() for r in upa]
zu = [float(r["alt_m"]) for r in upa]
vz_upa = []
for i, ti in enumerate(tu):
    idx = [j for j, tj in enumerate(tu) if abs(tj - ti) <= 61]
    if len(idx) >= 3:
        _, b, _ = regresion([tu[j] for j in idx], [zu[j] for j in idx])
        vz_upa.append((ti, zu[i], b, upa[i]))
for ti, zi, b, r in vz_upa[::4]:
    say(f"  T+{ti/60:5.1f} min {hl(r['_t'])}  z={zi:6.0f} m  vz={b:+.2f} m/s")

# fases de la TAES (por velocidad suavizada)
z_top_u = float(VU[TAES]["alt_max_m"])
t_top_u = t(VU[TAES]["t_alt_max_utc"])


def media_vz(sonda_list, a_s, b_s):
    v = [b for ti, zi, b, r in sonda_list if a_s <= ti <= b_s]
    return (sum(v) / len(v), min(v), max(v), len(v)) if v else None


def tmin(hhmmss):
    """minutos desde el lanzamiento TAES para una hora local del 28-oct."""
    h, m, s_ = map(int, hhmmss.split(":"))
    dt_ = datetime(2026, 10, 28, h, m, s_, tzinfo=UTC6)
    return (dt_ - T0[TAES]).total_seconds()


fases_upa = [
    ("ascenso 1", "22:53:44", "23:04:44"),
    ("meseta/descenso leve", "23:05:14", "23:11:14"),
    ("ascenso 2", "23:11:44", "23:21:44"),
    ("descenso lento", "23:22:14", "23:29:14"),
    ("descenso rápido", "23:29:44", "23:36:44"),
]
say("\n-- TAES: fases (velocidad media entre fixes extremos de cada fase) --")
fases_out = []
for nom, a_, b_ in fases_upa:
    ra = min(upa, key=lambda r: abs((r["_t"] - T0[TAES]).total_seconds() - tmin(a_)))
    rb = min(upa, key=lambda r: abs((r["_t"] - T0[TAES]).total_seconds() - tmin(b_)))
    if nom == "ascenso 1":
        za, ta = SUELO[TAES], T0[TAES]
    else:
        za, ta = float(ra["alt_m"]), ra["_t"]
    zb, tb = float(rb["alt_m"]), rb["_t"]
    v = (zb - za) / (tb - ta).total_seconds()
    fases_out.append({"sonda": TAES, "fase": nom, "t_ini_local": hl(ta), "t_fin_local": hl(tb),
                      "z_ini_m": za, "z_fin_m": zb, "vz_media_mps": v})
    say(f"  {nom:22s} {hl(ta)} -> {hl(tb)}  {za:.0f} -> {zb:.0f} m  vz = {v:+.2f} m/s")
vz_media_u = (z_top_u - SUELO[TAES]) / (t_top_u - T0[TAES]).total_seconds()
say(f"  ascenso medio lanzamiento->alt. máx.: {vz_media_u:.2f} m/s "
    f"({(t_top_u - T0[TAES]).total_seconds()/60:.1f} min para {z_top_u - SUELO[TAES]:.0f} m)")
for nom_, vv in (("Orbitium", vz_media_sf), ("TAES", vz_media_u)):
    z_s = SUELO[ORB if nom_ == 'Orbitium' else TAES]
    say(f"  {nom_}: con {vv:.2f} m/s, de su suelo ({z_s:.0f} m) a 15 km de altitud GPS tardaría "
        f"{(15000 - z_s) / vv / 60:.0f} min; con la misma base, 5 m/s -> {(15000 - z_s) / 5 / 60:.1f} min, "
        f"4 m/s -> {(15000 - z_s) / 4 / 60:.1f} min")

# ---------------------------------------------------------------- secuencia de liberación (Orbitium)
say("\n-- Orbitium: temporizador (fPort 32) y corte (fPort 31) --")
tm = [r for r in de(ORB, -40) if r["fport"] in ("31", "32")]
cambios = []
prev = None
for r in tm:
    if r["fport"] == "32":
        clave = (r["tmr_fase"], r["tmr_motivo"])
        if clave != prev:
            cambios.append(r)
            say(f"  {hl(r['_t'])} fPort 32 fase={r['tmr_fase']:10s} motivo={r['tmr_motivo']:9s} "
                f"t_vuelo={r['tmr_t_vuelo_s'] or '-':>7s} s  restante={r['tmr_restante_s'] or '-':>7s} s "
                f"límite={r['tmr_limite_s']} s")
            prev = clave
    else:
        say(f"  {hl(r['_t'])} fPort 31 evento={r['corte_evento']} motivo={r['corte_motivo']} seq={r['corte_seq']} "
            f"alt={r['alt_m']} m ({r['alt_fuente']}) vz={r['vel_vertical_mps']} m/s t_vuelo={r['tmr_t_vuelo_s']} s")
t_fase_vuelo = t(VU[ORB]["t_fase_vuelo_firmware_utc"])
lim = float([r for r in tm if r["fport"] == "32"][0]["tmr_limite_s"])
t_watchdog = t_fase_vuelo + timedelta(seconds=lim)
t_corte = t(VU[ORB]["t_corte_inicio_utc"])
say(f"  el temporizador habría cortado a las {hl(t_watchdog)} local (fase vuelo {hl(t_fase_vuelo)} + {lim:.0f} s); "
    f"el corte manual llegó {(t_watchdog - t_corte).total_seconds():.0f} s antes")
v_last = asc_sf[-1]["vz_mps"]
z_wd = z_top_sf + v_last * (t_watchdog - t_corte).total_seconds()
say(f"  (inferencia) con el ascenso final de {v_last:.2f} m/s, en el disparo del temporizador "
    f"la sonda habría estado a ~{z_wd:.0f} m")

# ---------------------------------------------------------------- presión, ISA e hipsometría
say("\n-- Presión vs GPS: altitud ISA vs altitud GPS --")
# Orbitium: altitud GPS interpolada al instante de cada fPort 21
sfa = sorted(FX[ORB], key=lambda r: r["_t"])
ts_sf = [(r["_t"] - T0[ORB]).total_seconds() for r in sfa]
zs_sf = [float(r["alt_m"]) for r in sfa]
pz = {ORB: [], TAES: []}  # (t_s, z_gps, p, T, HR, rama)
for r in de(ORB, -40):
    if r["fport"] == "21" and r["pres_capsula_hpa"]:
        ts_ = (r["_t"] - T0[ORB]).total_seconds()
        z = interp(ts_sf, zs_sf, ts_, extrap_s=30)
        if z is None:
            continue
        rama = "suelo" if ts_ < 0 else ("descenso" if r["_t"] > t_corte else "ascenso")
        pz[ORB].append((ts_, z, float(r["pres_capsula_hpa"]), f(r["temp_capsula_c"]), f(r["hum_capsula_pct"]), rama, r))
for r in de(TAES, -40):
    if r["calidad_gps"] == "ok" and r["pres_capsula_hpa"] and r["alt_m"]:
        ts_ = (r["_t"] - T0[TAES]).total_seconds()
        if ts_ < 0:
            rama = "suelo"
        elif r["_t"] <= t_top_u:
            rama = "ascenso"
        else:
            rama = "descenso"
        pz[TAES].append((ts_, float(r["alt_m"]), float(r["pres_capsula_hpa"]), f(r["temp_capsula_c"]),
                        f(r["hum_capsula_pct"]), rama, r))

for s in pz:
    suelo = [x for x in pz[s] if x[5] == "suelo"]
    if suelo:
        d = [x[1] - isa_z(x[2]) for x in suelo]
        say(f"  {s}: suelo, {len(suelo)} pares: z_GPS - z_ISA mediana {st.median(d):+.0f} m; "
            f"p mediana {st.median([x[2] for x in suelo]):.1f} hPa")
    for x in pz[s]:
        if x[5] != "suelo":
            pass
    alto = max((x for x in pz[s] if x[5] != "suelo"), key=lambda x: x[1])
    say(f"  {s}: punto más alto con presión: z_GPS={alto[1]:.0f} m, p={alto[2]:.1f} hPa, "
        f"z_ISA={isa_z(alto[2]):.0f} m, diferencia {alto[1]-isa_z(alto[2]):+.0f} m")

# Validación cruzada: z_GPS de la Orbitium contra z(p) de la TAES a la misma presión
say("\n-- Validación cruzada: altitud de la Orbitium vs z(p) de la TAES a la misma presión --")
up_asc = sorted([x for x in pz[TAES] if x[5] == "ascenso"], key=lambda x: -x[2])
cruz = []
for x in pz[ORB]:
    if x[5] == "suelo":
        continue
    p = x[2]
    for a, b in zip(up_asc, up_asc[1:]):
        if a[2] >= p >= b[2]:
            w = (a[2] - p) / (a[2] - b[2])
            zu_ = a[1] + w * (b[1] - a[1])
            cruz.append((p, x[1], zu_, x[5]))
            say(f"  p={p:.1f} hPa ({x[5]}): Orbitium {x[1]:.0f} m, TAES {zu_:.0f} m, diferencia {x[1]-zu_:+.0f} m")
            break

# Hipsometría: temperatura virtual media de capa
say("\n-- Temperatura virtual media de capa (ecuación hipsométrica, ESTIMACIÓN) --")
# ruido vertical del GPS de la TAES en tierra (fixes ok de los 40 min previos)
sz = st.pstdev([x[1] for x in pz[TAES] if x[5] == "suelo"])
szs = st.pstdev([float(r["alt_m"]) for r in FX[ORB] if r["_t"] < T0[ORB]])
say(f"  dispersión vertical GPS en tierra (desv. est.): TAES {sz:.1f} m; Orbitium {szs:.1f} m")
capas = []
# TAES: regresión ln p vs z en capas de 500 m, ascenso (1 y 2 juntos) y descenso
for rama, paso in (("ascenso", 500), ("descenso", 600)):
    pts = [x for x in pz[TAES] if x[5] == rama]
    z0 = 2000 if rama == "ascenso" else 2100
    while z0 < 5100:
        sel = [x for x in pts if z0 <= x[1] < z0 + paso]
        if len(sel) >= 4 and max(x[1] for x in sel) - min(x[1] for x in sel) > 0.6 * paso:
            a, b, se = regresion([x[1] for x in sel], [math.log(x[2]) for x in sel])
            Tv = -G / (RD * b)
            eTv = Tv * se / abs(b)
            zm = sum(x[1] for x in sel) / len(sel)
            capas.append({"sonda": TAES, "rama": rama, "z_base_m": z0, "z_tope_m": z0 + paso,
                          "z_medio_m": zm, "n": len(sel), "Tv_K": Tv, "Tv_C": Tv - 273.15,
                          "err_Tv_K": eTv, "T_isa_C": isa_T(zm) - 273.15,
                          "T_sensor_medio_C": sum(x[3] for x in sel) / len(sel)})
        z0 += paso
# Orbitium: pares consecutivos de niveles
lv = [x for x in pz[ORB]]
lv = [lv[i] for i in range(len(lv)) if lv[i][5] != "suelo" or i == max(j for j, y in enumerate(lv) if y[5] == "suelo")]
for a, b in zip(lv, lv[1:]):
    if b[5] == "descenso":
        rama = "ascenso/descenso"
    else:
        rama = "ascenso"
    dz = b[1] - a[1]
    Tv = G * dz / (RD * math.log(a[2] / b[2]))
    eTv = Tv * math.sqrt(2) * max(szs, 10.0) / abs(dz)
    zm = (a[1] + b[1]) / 2
    capas.append({"sonda": ORB, "rama": rama, "z_base_m": min(a[1], b[1]), "z_tope_m": max(a[1], b[1]),
                  "z_medio_m": zm, "n": 2, "Tv_K": Tv, "Tv_C": Tv - 273.15, "err_Tv_K": eTv,
                  "T_isa_C": isa_T(zm) - 273.15,
                  "T_sensor_medio_C": (a[3] + b[3]) / 2})
for c_ in capas:
    say(f"  {CORTA[c_['sonda']]:12s} {c_['rama']:16s} {c_['z_base_m']:5.0f}-{c_['z_tope_m']:5.0f} m  "
        f"Tv = {c_['Tv_C']:+6.1f} ± {c_['err_Tv_K']:4.1f} °C   ISA {c_['T_isa_C']:+6.1f} °C   "
        f"sensor {c_['T_sensor_medio_C']:5.1f} °C")
# perfil lineal Tv(z) ajustado a las capas TAES de ascenso + Orbitium de ascenso (pesos 1/err²)
cc = [c_ for c_ in capas if c_["rama"] == "ascenso"]
wsum = sum(1 / c_["err_Tv_K"] ** 2 for c_ in cc)
mz = sum(c_["z_medio_m"] / c_["err_Tv_K"] ** 2 for c_ in cc) / wsum
mT = sum(c_["Tv_K"] / c_["err_Tv_K"] ** 2 for c_ in cc) / wsum
sxx = sum((c_["z_medio_m"] - mz) ** 2 / c_["err_Tv_K"] ** 2 for c_ in cc)
sxy = sum((c_["z_medio_m"] - mz) * (c_["Tv_K"] - mT) / c_["err_Tv_K"] ** 2 for c_ in cc)
lapse = sxy / sxx
Tv0 = mT - lapse * mz


def Tv_perfil(z):
    return Tv0 + lapse * z


say(f"  ajuste lineal (ponderado): Tv(z) = {Tv0-273.15:.1f} °C {lapse*1000:+.2f} K/km·z[km]; "
    f"Tv(2 km) = {Tv_perfil(2000)-273.15:+.1f} °C, Tv(5 km) = {Tv_perfil(5000)-273.15:+.1f} °C, "
    f"Tv(8.4 km) = {Tv_perfil(8400)-273.15:+.1f} °C")
say(f"  ISA: 2 km {isa_T(2000)-273.15:+.1f} °C, 5 km {isa_T(5000)-273.15:+.1f} °C, 8.4 km {isa_T(8400)-273.15:+.1f} °C")

# altitud de la Orbitium a partir de la presión con el perfil estimado (prueba de los 8478 m)
def z_hipso(p, p_ref, z_ref):
    """Integra hacia arriba de z_ref (p_ref) a p usando Tv_perfil (pasos de 10 m)."""
    z, pp = z_ref, p_ref
    while pp > p:
        Tv = Tv_perfil(z + 5)
        pp *= math.exp(-G * 10 / (RD * Tv))
        z += 10
    return z


# Prueba independiente de los 8478 m: perfil SOLO de la TAES (capas de ascenso 2.5-5 km),
# extrapolado hacia arriba, e integrado desde el nivel de 580.2 hPa de la Orbitium
# (ya validado contra la TAES) hasta 359.6 hPa.
cu_ = [c_ for c_ in capas if c_["sonda"] == TAES and c_["rama"] == "ascenso" and c_["z_base_m"] >= 2500]
a_u, b_u, _ = regresion([c_["z_medio_m"] for c_ in cu_], [c_["Tv_K"] for c_ in cu_])


def z_hipso_lin(p, p_ref, z_ref, Tz):
    z, pp = z_ref, p_ref
    while pp > p:
        pp *= math.exp(-G * 10 / (RD * Tz(z + 5)))
        z += 10
    return z


ref = [x for x in pz[ORB] if abs(x[2] - 580.2) < 0.05][0]
top = [x for x in pz[ORB] if abs(x[2] - 359.6) < 0.05][0]
z_u = z_hipso_lin(359.6, ref[2], ref[1], lambda z: a_u + b_u * z)
Tv_475 = a_u + b_u * 4750
z_65 = z_hipso_lin(359.6, ref[2], ref[1], lambda z: Tv_475 - 0.0065 * (z - 4750))
z_isa_sh = z_hipso_lin(359.6, ref[2], ref[1], lambda z: isa_T(z))
say(f"  perfil solo TAES (2.5-5 km): Tv(z) = {a_u-273.15:.1f} °C {b_u*1000:+.2f} K/km·z[km]")
say(f"  Orbitium a 359.6 hPa (23:20:46): GPS interpolado {top[1]:.0f} m. Desde 580.2 hPa / {ref[1]:.0f} m: "
    f"con el perfil TAES extrapolado {z_u:.0f} m; con Tv(4.75 km) y -6.5 K/km {z_65:.0f} m; "
    f"con temperaturas ISA {z_isa_sh:.0f} m; ISA pura desde 1013 hPa {isa_z(359.6):.0f} m")
# TAES: z(p) en descenso contra ascenso (retardo de ventilación de la cápsula)
dd = []
for x in [x for x in pz[TAES] if x[5] == "descenso"]:
    for a, b in zip(up_asc, up_asc[1:]):
        if a[2] >= x[2] >= b[2]:
            w = (a[2] - x[2]) / (a[2] - b[2])
            dd.append(x[1] - (a[1] + w * (b[1] - a[1])))
            break
say(f"  TAES: z_GPS(descenso) - z_GPS(ascenso) a la misma presión: mediana {st.median(dd):+.0f} m "
    f"(n={len(dd)}, rango {min(dd):+.0f} a {max(dd):+.0f} m)")

# ---------------------------------------------------------------- viento por capas
say("\n-- Viento por capa (deriva del globo) --")
viento = []
# TAES: capas de 500 m en ascenso; descenso en capas de 500 m
for rama, a_t, b_t in (("ascenso", T0[TAES], t_top_u), ("descenso", t_top_u, TFIN[TAES])):
    pts = [r for r in upa if a_t <= r["_t"] <= b_t and r["lat"]]
    z0 = 1900
    while z0 < 5100:
        sel = [r for r in pts if z0 <= float(r["alt_m"]) < z0 + 500]
        if len(sel) >= 3:
            a, b = sel[0], sel[-1]
            d = hav(float(a["lat"]), float(a["lon"]), float(b["lat"]), float(b["lon"]))
            dt_ = (b["_t"] - a["_t"]).total_seconds()
            rb_ = rumbo(float(a["lat"]), float(a["lon"]), float(b["lat"]), float(b["lon"]))
            viento.append({"sonda": TAES, "rama": rama, "z_base_m": z0, "z_tope_m": z0 + 500,
                           "z_medio_m": (float(a["alt_m"]) + float(b["alt_m"])) / 2,
                           "t_ini_local": hl(a["_t"]), "t_fin_local": hl(b["_t"]), "dt_s": dt_,
                           "dist_m": d, "vel_mps": d / dt_, "rumbo_hacia_deg": rb_,
                           "dir_desde_deg": (rb_ + 180) % 360, "dir_desde": cardinal((rb_ + 180) % 360)})
        z0 += 500
# Orbitium: segmentos entre fixes con posición
# (se descarta el segmento que empieza en tierra, antes del lanzamiento: mezclaría reposo y vuelo)
sfp = [r for r in FX[ORB] if r["lat"] and r["_t"] >= T0[ORB]]
for a, b in zip(sfp, sfp[1:]):
    d = hav(float(a["lat"]), float(a["lon"]), float(b["lat"]), float(b["lon"]))
    dt_ = (b["_t"] - a["_t"]).total_seconds()
    rb_ = rumbo(float(a["lat"]), float(a["lon"]), float(b["lat"]), float(b["lon"]))
    za, zb = float(a["alt_m"]), float(b["alt_m"])
    viento.append({"sonda": ORB, "rama": "ascenso" if zb > za else "ascenso+caída",
                   "z_base_m": min(za, zb), "z_tope_m": max(za, zb), "z_medio_m": (za + zb) / 2,
                   "t_ini_local": hl(a["_t"]), "t_fin_local": hl(b["_t"]), "dt_s": dt_,
                   "dist_m": d, "vel_mps": d / dt_, "rumbo_hacia_deg": rb_,
                   "dir_desde_deg": (rb_ + 180) % 360, "dir_desde": cardinal((rb_ + 180) % 360)})
for v in viento:
    say(f"  {CORTA[v['sonda']]:12s} {v['rama']:14s} {v['z_base_m']:5.0f}-{v['z_tope_m']:5.0f} m "
        f"{v['t_ini_local']}->{v['t_fin_local']}  {v['vel_mps']:5.1f} m/s ({v['vel_mps']*3.6:4.0f} km/h) "
        f"desde {v['dir_desde_deg']:3.0f}° ({v['dir_desde']})")

say("\n-- Deriva total y último punto conocido --")
deriva = {}
for s in (ORB, TAES):
    pp = [r for r in FX[s] if r["lat"] and r["_t"] <= T0[s]]
    lanz = pp[-1]
    ult = [r for r in FX[s] if r["lat"]][-1]
    d = hav(float(lanz["lat"]), float(lanz["lon"]), float(ult["lat"]), float(ult["lon"]))
    rb_ = rumbo(float(lanz["lat"]), float(lanz["lon"]), float(ult["lat"]), float(ult["lon"]))
    dg = hav(ITPA[0], ITPA[1], float(ult["lat"]), float(ult["lon"]))
    deriva[s] = (lanz, ult, d, rb_)
    say(f"  {s}: último fix {hl(ult['_t'])} en {float(ult['lat']):.5f}, {float(ult['lon']):.5f} a "
        f"{float(ult['alt_m']):.0f} m; deriva desde el último fix en tierra {d/1000:.1f} km "
        f"hacia {rb_:.0f}° ({cardinal(rb_)}); a {dg/1000:.1f} km de ITPA-IOT-GW00")

# ---------------------------------------------------------------- descenso (escalado con la densidad)
say("\n-- Descenso: velocidad terminal escalada con la densidad (INFERENCIA) --")


def rho(z):
    """Densidad con la presión ISA ajustada al suelo medido y Tv del perfil estimado."""
    p = z_a_p(z)
    return p * 100 / (RD * Tv_perfil(z))


def z_a_p(z, p_ref=815.4, z_ref=1897.0):
    pp, zz = p_ref, z_ref
    paso = 10 if z > z_ref else -10
    while (zz < z) if paso > 0 else (zz > z):
        pp *= math.exp(-G * paso / (RD * Tv_perfil(zz + paso / 2)))
        zz += paso
    return pp


rho_ref = rho(2000)
# TAES: v0 = vz * sqrt(rho/rho_ref) en el descenso rápido
v0s = []
for ti, zi, b, r in vz_upa:
    if r["_t"] >= datetime(2026, 10, 29, 5, 30, 14, tzinfo=timezone.utc) and b < -3:
        v0s.append((-b) * math.sqrt(rho(zi) / rho_ref))
v0_upa = st.median(v0s)
say(f"  TAES: descenso rápido, v normalizada a 2000 m = {v0_upa:.2f} m/s (mediana de {len(v0s)}; "
    f"rango {min(v0s):.2f}-{max(v0s):.2f}); a 4.5 km correspondería {v0_upa*math.sqrt(rho_ref/rho(4500)):.1f} m/s")


def t_caida(z_a, z_b, v0):
    tt, z = 0.0, z_a
    while z > z_b:
        v = v0 * math.sqrt(rho_ref / rho(z))
        tt += 10 / v
        z -= 10
    return tt


# Orbitium: ajustar v0 a 8408 m (23:21:46) -> 4278 m (23:30:36)
t_conf = t(VU[ORB]["t_corte_confirmado_utc"])
t_ufix = t(VU[ORB]["t_ultimo_fix_utc"])
z_conf = [float(r["alt_m"]) for r in FX[ORB] if r["corte_evento"] == "confirmado"][0]
z_ufix = float(VU[ORB]["alt_ultimo_fix_m"])
obj = (t_ufix - t_conf).total_seconds()
lo, hi = 2.0, 20.0
for _ in range(60):
    mid = (lo + hi) / 2
    if t_caida(z_conf, z_ufix, mid) > obj:
        lo = mid
    else:
        hi = mid
v0_sf = (lo + hi) / 2
say(f"  Orbitium: caída {z_conf:.0f} -> {z_ufix:.0f} m en {obj:.0f} s (media {(z_conf-z_ufix)/obj:.1f} m/s); "
    f"v0 ajustada a 2000 m = {v0_sf:.2f} m/s; a 8.4 km eso da {v0_sf*math.sqrt(rho_ref/rho(8400)):.1f} m/s "
    f"(firmware: -9.6 m/s en 'confirmado')")
# altitud estimada en la última recepción y hora de aterrizaje
dt_ult = (TFIN[ORB] - t_ufix).total_seconds()
z, tt = z_ufix, 0.0
while tt < dt_ult:
    v = v0_sf * math.sqrt(rho_ref / rho(z))
    z -= v * 1.0
    tt += 1.0
z_ultrx = z
aterr = {}
for zt in (1900, 2000, 2100):
    aterr[zt] = t_ufix + timedelta(seconds=t_caida(z_ufix, zt, v0_sf))
say(f"  Orbitium (inferencia): en la última recepción ({hl(TFIN[ORB])}) estaría a ~{z_ultrx:.0f} m; "
    f"aterrizaje ~{hl(aterr[2100])}-{hl(aterr[1900])} local (terreno 2100-1900 m)")
# Orbitium: deriva horizontal probable del tramo final (INFERENCIA): caída desde el último fix
# con v(z) y el viento medido por la TAES en su descenso (capa más cercana en altitud).
vdesc = [v for v in viento if v["sonda"] == TAES and v["rama"] == "descenso"]
z, dx, dy, tt = z_ufix, 0.0, 0.0, 0.0
while z > 2000:
    v = v0_sf * math.sqrt(rho_ref / rho(z))
    capa = min(vdesc, key=lambda c_: abs(c_["z_medio_m"] - z))
    dt_ = 10 / v
    ang = math.radians(capa["rumbo_hacia_deg"])
    dx += capa["vel_mps"] * math.sin(ang) * dt_
    dy += capa["vel_mps"] * math.cos(ang) * dt_
    z -= 10
    tt += dt_
lat_ult, lon_ult = float(VU[ORB]["lat_ultimo_fix"]), float(VU[ORB]["lon_ultimo_fix"])
lat_at = lat_ult + dy / 111320
lon_at = lon_ult + dx / (111320 * math.cos(math.radians(lat_ult)))
d_at = math.hypot(dx, dy)
say(f"  Orbitium (inferencia): con el viento de la TAES en descenso, deriva adicional hasta 2000 m ≈ {d_at/1000:.1f} km "
    f"hacia {(math.degrees(math.atan2(dx, dy))+360)%360:.0f}° en {tt/60:.1f} min -> zona ≈ {lat_at:.3f}, {lon_at:.3f}")
u_last = [r for r in upa if r["_t"] == TFIN[TAES]][0]
z_u_last = float(u_last["alt_m"])
for zt in (1900, 2000, 2100):
    if z_u_last > zt:
        say(f"  TAES (inferencia): de {z_u_last:.0f} m a {zt} m faltaban ~{t_caida(z_u_last, zt, v0_upa):.0f} s")

# ---------------------------------------------------------------- humedad
say("\n-- Humedad --")
hum_sf = [r for r in UP if r["sonda"] == ORB and r["hum_capsula_pct"]]
peg = [r for r in hum_sf if r["calidad_hum"] == "pegada_100"]
say(f"  Orbitium: {len(hum_sf)} lecturas de HR; {len(peg)} pegadas en 100 % "
    f"({peg[0]['t_local'][:16]} a {peg[-1]['t_local'][:16]})" if peg else "  sin HR pegada")
hum_u = [r for r in UP if r["sonda"] == TAES and r["hum_capsula_pct"]]
peg_u = [r for r in hum_u if r["calidad_hum"] == "pegada_100"]
say(f"  TAES: {len(hum_u)} lecturas de HR; {len(peg_u)} pegadas en 100 %")
hum_tab = []
for s in (ORB, TAES):
    for x in pz[s]:
        if x[3] is not None and x[4] is not None and 0 < x[4] < 100:
            w = mezcla(x[3], x[4], x[2])
            td = rocio(x[3], x[4])
            hum_tab.append({"sonda": s, "t_local": hl(x[6]["_t"]), "t_desde_lanz_min": x[0] / 60,
                            "rama": x[5], "z_gps_m": x[1], "p_hpa": x[2], "T_sensor_C": x[3],
                            "HR_pct": x[4], "rocio_C": td, "mezcla_gkg": w})
for s in (ORB, TAES):
    hh = [h for h in hum_tab if h["sonda"] == s and h["rama"] != "suelo"]
    hs = [h for h in hum_tab if h["sonda"] == s and h["rama"] == "suelo"]
    top = max(hh, key=lambda h: h["z_gps_m"])
    say(f"  {s}: suelo HR {st.median([h['HR_pct'] for h in hs]):.1f} %, rocío {st.median([h['rocio_C'] for h in hs]):.1f} °C, "
        f"w {st.median([h['mezcla_gkg'] for h in hs]):.1f} g/kg; en {top['z_gps_m']:.0f} m: HR {top['HR_pct']:.1f} %, "
        f"T {top['T_sensor_C']:.1f} °C, rocío {top['rocio_C']:.1f} °C, w {top['mezcla_gkg']:.1f} g/kg")

# ---------------------------------------------------------------- temperatura de la cápsula y batería
say("\n-- Temperatura de la cápsula y batería --")
for s in (ORB, TAES):
    tt_ = [(x[0], x[3], x[1]) for x in pz[s] if x[3] is not None]
    suelo = [v for ts_, v, z in tt_ if ts_ < 0]
    vuelo = [(ts_, v, z) for ts_, v, z in tt_ if ts_ >= 0]
    mn = min(vuelo, key=lambda x: x[1])
    say(f"  {s}: T sensor en tierra (mediana) {st.median(suelo):.1f} °C; mínima en vuelo {mn[1]:.2f} °C "
        f"a T+{mn[0]/60:.1f} min ({mn[2]:.0f} m)")
    tamb = Tv_perfil(mn[2]) - 273.15
    say(f"     (estimación) aire (Tv por hipsometría) a {mn[2]:.0f} m ≈ {tamb:.1f} °C: la cápsula iba {mn[1]-tamb:+.0f} K por encima")
bat = [(r["_t"], float(r["bateria_v"])) for r in UP if r["sonda"] == TAES and r["bateria_v"]]
bv = [b for tt_, b in bat if T0[TAES] <= tt_ <= TFIN[TAES]]
say(f"  TAES batería en vuelo: {max(bv):.3f} -> {min(bv):.3f} V (caída {1000*(max(bv)-min(bv)):.0f} mV); umbral de la prueba de frío: 3.3 V")
say(f"  TAES batería todo el registro: {bat[0][1]:.3f} V ({hl(bat[0][0])}) -> {bat[-1][1]:.3f} V ({hl(bat[-1][0])}), "
    f"{len(bat)} lecturas")
sfb = [r for r in UP if r["sonda"] == ORB and (r["bateria_v"] or r["bateria_pct"] or r["ina_bus_v"])]
usb = [r["usb"] for r in UP if r["sonda"] == ORB and r["fport"] == "23" and T0[ORB] <= r["_t"] <= TFIN[ORB]]
say(f"  Orbitium: lecturas de batería/INA219 en todo el dataset: {len(sfb)}; bandera usb en vuelo: {usb}")

# ---------------------------------------------------------------- tablas derivadas
guardar_csv("a2_segmentos_vertical.csv",
            seg_sf + [dict(sonda=x["sonda"], t_ini_local=x["t_ini_local"], t_fin_local=x["t_fin_local"],
                           z_ini_m=x["z_ini_m"], z_fin_m=x["z_fin_m"], vz_mps=x["vz_media_mps"],
                           fase=x["fase"]) for x in fases_out],
            ["sonda", "fase", "t_ini_local", "t_fin_local", "z_ini_m", "z_fin_m", "dt_s", "vz_mps",
             "z_medio_m", "t_medio_min"])
guardar_csv("a2_viento_capas.csv", viento,
            ["sonda", "rama", "z_base_m", "z_tope_m", "z_medio_m", "t_ini_local", "t_fin_local", "dt_s",
             "dist_m", "vel_mps", "rumbo_hacia_deg", "dir_desde_deg", "dir_desde"])
guardar_csv("a2_temperatura_capas.csv", capas,
            ["sonda", "rama", "z_base_m", "z_tope_m", "z_medio_m", "n", "Tv_K", "Tv_C", "err_Tv_K",
             "T_isa_C", "T_sensor_medio_C"])
guardar_csv("a2_humedad.csv", hum_tab,
            ["sonda", "t_local", "t_desde_lanz_min", "rama", "z_gps_m", "p_hpa", "T_sensor_C", "HR_pct",
             "rocio_C", "mezcla_gkg"])
print("  tablas: datos/derivados/a2_segmentos_vertical.csv, a2_viento_capas.csv, a2_temperatura_capas.csv, a2_humedad.csv")

# ================================================================= FIGURAS
print("\n-- figuras --")
LBL = {ORB: "Sonda Orbitium (Sci-Fi), ITPA", TAES: "Sonda TAES (Instituto Tepeyac/UPA)"}


def nota(fig, txt, y=-0.02):
    fig.text(0.0, y, txt, ha="left", va="top", fontsize=8, color=MUTED, wrap=True)


# --- Fig 1: perfil altitud-tiempo
fig, ax = plt.subplots(figsize=(9, 5.2))
for s in (ORB, TAES):
    pre = [r for r in FX[s] if r["_t"] < T0[s]]
    post = [r for r in FX[s] if r["_t"] >= T0[s]]
    xs = [((r["_t"] - T0[s]).total_seconds()) / 60 for r in pre] + [0.0] + \
         [((r["_t"] - T0[s]).total_seconds()) / 60 for r in post]
    ys = [float(r["alt_m"]) / 1000 for r in pre] + [SUELO[s] / 1000] + [float(r["alt_m"]) / 1000 for r in post]
    ax.plot(xs, ys, color=C[s], lw=2, marker="o", ms=4 if s == TAES else 6,
            mec=SURF, mew=1, label=LBL[s] + " · GPS", zorder=3)
    ax.plot([0], [SUELO[s] / 1000], marker="o", ms=8, mfc=SURF, mec=C[s], mew=1.5, zorder=4)
    bx = [x[0] / 60 for x in pz[s]]
    by = [isa_z(x[2]) / 1000 for x in pz[s]]
    ax.plot(bx, by, ls="none", marker="x" if s == ORB else ".", ms=6 if s == ORB else 3,
            color=C[s], alpha=0.7, label=LBL[s] + " · presión → ISA", zorder=2)
# descenso estimado de la Orbitium tras el último fix (inferencia)
xe = [(t_ufix - T0[ORB]).total_seconds() / 60, (TFIN[ORB] - T0[ORB]).total_seconds() / 60,
      (aterr[2000] - T0[ORB]).total_seconds() / 60]
ax.plot(xe, [z_ufix / 1000, z_ultrx / 1000, 2.0], color=C[ORB], lw=1.4, ls=(0, (2, 2)), zorder=2,
        label="Sonda Orbitium (Sci-Fi), ITPA · descenso estimado (inferencia)")
ax.plot([xe[1]], [z_ultrx / 1000], marker="o", ms=7, mfc=SURF, mec=C[ORB], mew=1.5, zorder=4)
x5 = [0, 50]
ax.plot(x5, [SUELO[TAES] / 1000, (SUELO[TAES] + 5 * 3000) / 1000], color=REF, lw=1.2, ls=(0, (4, 3)),
        label="supuesto del evento: 5 m/s", zorder=1)
ax.axhline(15, color=REF, lw=0.8)
ax.text(1, 15.15, "liberación objetivo 15 km (GPS)", color=TXT2, fontsize=8.5, va="bottom")
tc = (t_corte - T0[ORB]).total_seconds() / 60
ax.annotate(f"corte por downlink\n{z_top_sf:.0f} m · {hl(t_corte)[:5]}", (tc, z_top_sf / 1000),
            xytext=(tc + 3, 10.6), fontsize=8.5, color=TXT,
            arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
ax.annotate(f"máx. {z_top_u:.0f} m · {hl(t_top_u)[:5]}", ((t_top_u - T0[TAES]).total_seconds() / 60, z_top_u / 1000),
            xytext=(31, 6.4), fontsize=8.5, color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
for s, xt, yt in ((ORB, 57, 4.6), (TAES, 45, 1.2)):
    xl = (TFIN[s] - T0[s]).total_seconds() / 60
    ax.annotate(f"última recepción {hl(TFIN[s])[:5]}" + ("\n(~2.6 km, estimada)" if s == ORB else ""), (xl, (z_ultrx if s == ORB else z_u_last) / 1000),
                xytext=(xt, yt), fontsize=8.5, color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
xw = (t_watchdog - T0[ORB]).total_seconds() / 60
ax.axvline(xw, color=C[ORB], lw=0.8, ls=(0, (2, 2)))
ax.text(xw + 0.5, 12.6, "temporizador Orbitium\n(55 min, no disparó)", fontsize=8, color=TXT2)
ax.set_xlim(-12, 66)
ax.set_ylim(0, 16)
ax.set_xlabel("tiempo desde el despegue estimado de cada sonda (min)")
ax.set_ylabel("altitud sobre el nivel del mar (km)")
ax.set_title("Perfil de vuelo: altitud contra tiempo")
ax.legend(loc="upper left", fontsize=8.3, bbox_to_anchor=(0.0, 0.93))
nota(fig, f"Despegues estimados (círculo hueco en t = 0): Orbitium {hl(T0[ORB])} y TAES {hl(T0[TAES])} (hora local, UTC−6). "
          "Puntos: altitud GPS. Cruces/puntos pequeños: altitud ISA calculada con la presión del BME280. "
          "La línea de 5 m/s parte del suelo de la TAES.")
guardar_fig(fig, "a2_perfil_altitud.png")

# --- Fig 2: velocidad vertical contra altitud
fig, ax = plt.subplots(figsize=(8, 5.6))
vx = [b for ti, zi, b, r in vz_upa]
vy = [zi / 1000 for ti, zi, b, r in vz_upa]
ax.plot(vx, vy, color=C[TAES], lw=1.6, marker="o", ms=3.5, mec=SURF, mew=0.6,
        label=LBL[TAES] + " · regresión ±60 s")
for s_ in seg_sf:
    # el primer segmento empieza en el despegue estimado (T0, a la altitud del suelo)
    ax.plot([s_["vz_mps"]] * 2, [s_["z_ini_m"] / 1000, s_["z_fin_m"] / 1000], color=C[ORB], lw=3,
            solid_capstyle="round")
fp31 = [r for r in FX[ORB] if r["fport"] == "31"]
ax.plot([float(r["vel_vertical_mps"]) for r in fp31], [float(r["alt_m"]) / 1000 for r in fp31],
        ls="none", marker="D", ms=7, color=C[ORB], mec=SURF, mew=1)
ax.axvline(5, color=REF, lw=1.2, ls=(0, (4, 3)))
ax.text(5.15, 7.7, "5 m/s\n(supuesto del evento)", fontsize=8.5, color=TXT2)
ax.axvline(0, color=TXT2, lw=0.8)
ax.annotate("fPort 31: corte confirmado (−9.6 m/s);\nel inicio (+2.5 m/s) es el rombo de la derecha", (-9.6, 8.408), xytext=(-9.3, 9.05),
            fontsize=8.3, color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
ax.annotate("caída media tras el corte\n8408 → 4278 m", (seg_sf[-1]["vz_mps"], 6.2), xytext=(-9.8, 5.0),
            fontsize=8.3, color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
ax.annotate("meseta 4.4-4.5 km", (-0.9, 4.45), xytext=(-6.5, 3.6), fontsize=8.3, color=TXT,
            arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
leg = [Line2D([], [], color=C[ORB], lw=3, label=LBL[ORB] + " · entre fixes (10 min)"),
       Line2D([], [], color=C[ORB], ls="none", marker="D", ms=6, label=LBL[ORB] + " · firmware (fPort 31)"),
       Line2D([], [], color=C[TAES], lw=1.6, marker="o", ms=3.5, label=LBL[TAES] + " · regresión ±60 s")]
ax.legend(handles=leg, loc="lower right", fontsize=8.3)
ax.set_xlim(-11, 7.5)
ax.set_ylim(1.8, 9.7)
ax.set_xlabel("velocidad vertical (m/s; positiva = sube)")
ax.set_ylabel("altitud GPS (km)")
ax.set_title("Velocidad vertical contra altitud")
nota(fig, "Orbitium: cada barra vertical es un segmento entre dos fixes; su posición horizontal es la velocidad media del segmento. "
          "El segmento 8390 → 4278 m mezcla 70 s de ascenso con la caída libre.")
guardar_fig(fig, "a2_velocidad_vertical.png")

# --- Fig 3: secuencia de liberación (temporizador)
fig, ax = plt.subplots(figsize=(9, 4.6))
t32 = [r for r in de(ORB, -20) if r["fport"] == "32"]
FASE_Y = {"desarmado": 0, "armado": 1, "vuelo": 2, "cortando": 3, "terminado": 4}
xs = [r["_t"].astimezone(UTC6) for r in t32]
ys = [FASE_Y[r["tmr_fase"]] for r in t32]
ax.step(xs, ys, where="post", color=C[ORB], lw=2)
ax.plot(xs, ys, ls="none", marker="o", ms=4, color=C[ORB], mec=SURF, mew=0.8)
ax.set_yticks(list(FASE_Y.values()))
ax.set_yticklabels([f"{k}" for k in FASE_Y])
ax.set_ylabel("fase del temporizador (fPort 32)")
ax.set_ylim(-0.6, 5.3)
import matplotlib.dates as mdates
ax.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M", tz=UTC6))
t_arm = t(VU[ORB]["t_armado_firmware_utc"])
ev = [(t_arm, f"ARM aceptado {hl(t_arm)[:5]}\nlímite 3300 s", 1, -17, 0.9),
      (t_fase_vuelo, f"fase vuelo {hl(t_fase_vuelo)[:5]}\n(t_vuelo = 0)", 2, -17, 1.0),
      (t_corte, f"corte manual {hl(t_corte)[:5]} (downlink 'CORT')\nt_vuelo = 2625 s · {z_top_sf:.0f} m", 3, -27, -0.2),
      (t_conf, f"confirmado {hl(t_conf)[:5]} · 8408 m\n−9.6 m/s", 4, -22, 0.55)]
for tt_, txt_, y_, dx, dy in ev:
    ax.annotate(txt_, (tt_.astimezone(UTC6), y_), xytext=(tt_.astimezone(UTC6) + timedelta(minutes=dx), y_ + dy),
                fontsize=8.2, color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
ax.axvline(t_watchdog.astimezone(UTC6), color=REF, lw=1, ls=(0, (4, 3)))
ax.text(t_watchdog.astimezone(UTC6) + timedelta(seconds=30), 0.2,
        f"disparo del temporizador\nprevisto {hl(t_watchdog)[:5]}", fontsize=8.2, color=TXT2)
ax.axvline(TFIN[ORB].astimezone(UTC6), color=TXT2, lw=0.8)
ax.text(TFIN[ORB].astimezone(UTC6) - timedelta(seconds=30), -0.45, "última recepción", fontsize=8.2,
        color=TXT2, ha="right")
ax.set_xlabel("hora local, 28-oct-2026 (UTC−6)")
ax.set_title("Sonda Orbitium (Sci-Fi), ITPA: secuencia de liberación")
nota(fig, "Fases y tiempos del objeto decodificado del fPort 32 (cada ~60 s); eventos de corte del fPort 31. "
          "El campo 'motivo' del fPort 32 vale 'prueba' hasta el corte y 'downlink' después.")
guardar_fig(fig, "a2_secuencia_liberacion.png")

# --- Fig 4: trayectoria horizontal
fig, ax = plt.subplots(figsize=(7.6, 7.2))
coslat = math.cos(math.radians(22.25))
for s in (ORB, TAES):
    pp = [r for r in FX[s] if r["lat"]]
    lo_ = [float(r["lon"]) for r in pp]
    la_ = [float(r["lat"]) for r in pp]
    ax.plot(lo_, la_, color=C[s], lw=1.8, marker="o", ms=3.5 if s == TAES else 6, mec=SURF, mew=0.8,
            label=LBL[s])
    ult = pp[-1]
    ax.plot(float(ult["lon"]), float(ult["lat"]), marker="X", ms=11, color=C[s], mec=TXT, mew=0.8, zorder=5)
    ax.annotate(f"último fix {hl(ult['_t'])[:5]}\n{float(ult['lat']):.4f}, {float(ult['lon']):.4f}\n{float(ult['alt_m']):.0f} m",
                (float(ult["lon"]), float(ult["lat"])),
                xytext=(float(ult["lon"]) + (-0.035 if s == ORB else 0.012), float(ult["lat"]) + (-0.045 if s == ORB else -0.03)),
                fontsize=8.2, color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
# etiquetas de altitud en la Orbitium
for r in [r for r in FX[ORB] if r["lat"] and r["_t"] > T0[ORB]][:-1]:
    ax.text(float(r["lon"]) + 0.004, float(r["lat"]) - 0.004, f"{float(r['alt_m'])/1000:.1f} km",
            fontsize=7.8, color=TXT2)
for r in [r for r in upa if r["lat"] and hl(r["_t"]) in ("23:05:14", "23:21:44", "23:29:14")]:
    ax.text(float(r["lon"]) - 0.006, float(r["lat"]), f"{float(r['alt_m'])/1000:.1f} km", fontsize=7.8,
            color=TXT2, ha="right")
for g, mk in (("ITPA-IOT-GW00", "^"),):
    ax.plot(float(GW[g]["lon"]), float(GW[g]["lat"]), marker=mk, ms=9, color=TXT, ls="none")
    ax.text(float(GW[g]["lon"]) + 0.005, float(GW[g]["lat"]) - 0.004, "ITPA (gateways GW00 y GW01)",
            fontsize=8.2, color=TXT, ha="left", va="top")
ax.set_aspect(1 / coslat)
ax.set_xlim(-102.31, -102.04)
ax.set_ylim(22.13, 22.40)
# barra de escala 5 km
x0, y0 = -102.12, 22.145
dlon = 5000 / (111320 * coslat)
ax.plot([x0, x0 + dlon], [y0, y0], color=TXT, lw=3, solid_capstyle="butt")
ax.text(x0 + dlon / 2, y0 + 0.003, "5 km", ha="center", fontsize=8.5)
ax.annotate("", xy=(-102.05, 22.185), xytext=(-102.05, 22.16), arrowprops=dict(arrowstyle="-|>", color=TXT, lw=1))
ax.text(-102.05, 22.155, "N", ha="center", va="top", fontsize=9)
ax.set_xlabel("longitud (°)")
ax.set_ylabel("latitud (°)")
ax.set_title("Trayectoria horizontal (fixes GPS válidos)")
ax.legend(loc="upper left", fontsize=8.5)
nota(fig, "Sin mapa de fondo. Relación de aspecto corregida por cos(latitud). "
          "La X marca el último fix; las sondas siguieron cayendo después (ver texto).")
guardar_fig(fig, "a2_trayectoria.png")

# --- Fig 5: viento por capa
fig, (a1, a2_) = plt.subplots(1, 2, figsize=(9.5, 5.2), sharey=True)
for s, mk in ((ORB, "s"), (TAES, "o")):
    for rama, fill in (("ascenso", True), ("descenso", False), ("ascenso+caída", False)):
        vv = [v for v in viento if v["sonda"] == s and v["rama"] == rama]
        if not vv:
            continue
        zz = [v["z_medio_m"] / 1000 for v in vv]
        lab = f"{LBL[s]} · {rama}"
        for axx, key in ((a1, "vel_mps"), (a2_, "dir_desde_deg")):
            axx.plot([v[key] for v in vv], zz, ls="-" if rama == "ascenso" else "none", lw=1.5,
                     marker=mk, ms=7, color=C[s], mfc=C[s] if fill else SURF, mec=C[s], mew=1.6, label=lab)
            for v in vv:
                axx.plot([v[key]] * 2, [v["z_base_m"] / 1000, v["z_tope_m"] / 1000], color=C[s], lw=1,
                         alpha=0.45)
a1.set_xlabel("velocidad horizontal del viento (m/s)")
a1.set_ylabel("altitud GPS (km)")
a1.set_xlim(0, 16)
a1.set_title("Rapidez del viento por capa")
a2_.set_xlabel("dirección de la que sopla (°)")
a2_.set_xticks([0, 90, 180, 270, 360])
a2_.set_xticklabels(["N\n0", "E\n90", "S\n180", "O\n270", "N\n360"])
a2_.set_xlim(0, 360)
a2_.set_title("Dirección del viento por capa")
a1.legend(loc="upper left", fontsize=7.8, bbox_to_anchor=(0, 1))
nota(fig, "Método del globo piloto: desplazamiento entre el primer y el último fix de cada capa (TAES, capas de 500 m) "
          "o entre fixes consecutivos (Orbitium, ~10 min). Barra vertical = espesor de la capa. Símbolo hueco = descenso.")
guardar_fig(fig, "a2_viento_capas.png")

# --- Fig 6: presión y temperatura frente a la ISA
fig, (a1, a2_) = plt.subplots(1, 2, figsize=(10, 5.4), sharey=True)
zz = [z / 1000 for z in range(1500, 9001, 100)]
a1.plot([isa_p(z * 1000) for z in zz], zz, color=REF, lw=1.2, ls=(0, (4, 3)), label="ISA")
for s, mk in ((ORB, "s"), (TAES, "o")):
    for rama, fill in (("ascenso", True), ("descenso", False), ("suelo", True)):
        pts = [x for x in pz[s] if x[5] == rama]
        if not pts:
            continue
        a1.plot([x[2] for x in pts], [x[1] / 1000 for x in pts], ls="none", marker=mk,
                ms=7 if s == ORB else 4, color=C[s], mfc=C[s] if fill else SURF, mec=C[s], mew=1.2,
                label=f"{LBL[s]} · {rama}" if rama != "suelo" else None)
a1.invert_xaxis()
a1.set_xlabel("presión del BME280 de la cápsula (hPa)")
a1.set_ylabel("altitud GPS (km)")
a1.set_title("Presión contra altitud")
a1.legend(fontsize=7.6, loc="upper center", bbox_to_anchor=(0.5, -0.13), ncol=1)
a2_.plot([isa_T(z * 1000) - 273.15 for z in zz], zz, color=REF, lw=1.2, ls=(0, (4, 3)), label="ISA")
for s, mk in ((ORB, "s"), (TAES, "o")):
    pts = [x for x in pz[s] if x[3] is not None]
    a2_.plot([x[3] for x in pts], [x[1] / 1000 for x in pts], ls="none", marker=mk, ms=6 if s == ORB else 3.5,
             color=C[s], mec=SURF, mew=0.5, label=f"{LBL[s]} · sensor de la cápsula (interior)")
cu = [c_ for c_ in capas if c_["rama"] == "ascenso"]
for c_ in cu:
    a2_.errorbar(c_["Tv_C"], c_["z_medio_m"] / 1000, xerr=c_["err_Tv_K"], fmt="none", ecolor=TXT2, lw=1, capsize=2)
a2_.plot([c_["Tv_C"] for c_ in cu], [c_["z_medio_m"] / 1000 for c_ in cu], ls="none", marker="o", ms=6,
         mfc=SURF, mec=TXT, mew=1.3, label="temperatura virtual del aire (hipsometría, por capa)")
a2_.plot([Tv_perfil(z * 1000) - 273.15 for z in zz], zz, color=TXT2, lw=1, label="ajuste lineal de Tv")
a2_.set_xlabel("temperatura (°C)")
a2_.set_xlim(-45, 35)
a2_.set_title("Temperatura contra altitud")
a2_.legend(fontsize=7.6, loc="upper center", bbox_to_anchor=(0.5, -0.13), ncol=1)
nota(fig, "En vuelo no se midió nada en el exterior: los sensores de ambas sondas iban dentro de la cápsula (interior). "
          "La del aire es la temperatura virtual que da la ecuación hipsométrica (altitud GPS + presión); las barras son ±1σ. "
          "La ISA es una atmósfera de latitud media: a 22° N lo habitual es una troposfera más cálida que ella.",
     y=-0.17)
guardar_fig(fig, "a2_presion_temperatura_isa.png")

# --- Fig 7: altitud barométrica vs GPS
fig, ax = plt.subplots(figsize=(8, 5))
for s, mk in ((ORB, "s"), (TAES, "o")):
    pts = pz[s]
    ax.plot([x[1] / 1000 for x in pts], [x[1] - isa_z(x[2]) for x in pts], ls="none", marker=mk,
            ms=7 if s == ORB else 3.5, color=C[s], mec=SURF, mew=0.6, label=LBL[s])
# diferencia esperada con el perfil estimado (atmósfera más cálida que la ISA)
zgrid = list(range(1900, 8601, 100))
ax.plot([z / 1000 for z in zgrid], [z - isa_z(z_a_p(z)) for z in zgrid], color=TXT2, lw=1.2,
        label="esperada con el perfil Tv estimado")
ax.axhline(0, color=TXT2, lw=0.8)
xd = [x for x in pz[ORB] if x[5] == "descenso"][0]
ax.annotate("Orbitium en descenso (23:30)", (xd[1] / 1000, xd[1] - isa_z(xd[2])), xytext=(4.6, 60), fontsize=8.3,
            color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
ax.annotate("fixes en tierra\n(error vertical del GPS)", (1.85, 20), xytext=(2.3, -30), fontsize=8.3,
            color=TXT, arrowprops=dict(arrowstyle="-", color=TXT2, lw=0.8))
ax.set_xlabel("altitud GPS (km)")
ax.set_ylabel("altitud GPS − altitud ISA por presión (m)")
ax.set_title("Altitud barométrica (ISA) contra altitud GPS")
ax.legend(fontsize=8.5, loc="upper left")
nota(fig, "La ISA supone 15 °C al nivel del mar y −6.5 K/km; en una atmósfera más cálida la presión baja más despacio y la "
          "altitud ISA se queda corta (a 22° N la troposfera suele ser más cálida que la ISA). "
          "La curva gris integra la presión con el perfil de temperatura virtual estimado.")
guardar_fig(fig, "a2_baro_vs_gps.png")

# --- Fig 8: humedad
fig, (a1, a2_) = plt.subplots(1, 2, figsize=(10, 4.8))
hs = [r for r in UP if r["sonda"] == ORB and r["hum_capsula_pct"]]
ok_ = [r for r in hs if r["calidad_hum"] != "pegada_100"]
pg_ = [r for r in hs if r["calidad_hum"] == "pegada_100"]
a1.plot([r["_t"].astimezone(UTC6) for r in ok_], [float(r["hum_capsula_pct"]) for r in ok_], ls="none", marker="o",
        ms=4, color=C[ORB], label="HR válida")
a1.plot([r["_t"].astimezone(UTC6) for r in pg_], [float(r["hum_capsula_pct"]) for r in pg_], ls="none", marker="o",
        ms=5, mfc=SURF, mec=CRIT, mew=1.4, label=f"pegada en 100 % ({len(pg_)}, sensor dañado)")
a1.axvspan(T0[ORB].astimezone(UTC6), TFIN[ORB].astimezone(UTC6), color=GRID, alpha=0.8, lw=0)
a1.text(T0[ORB].astimezone(UTC6), 102, "vuelo", fontsize=8, color=TXT2, ha="right")
a1.xaxis.set_major_locator(mdates.HourLocator(byhour=[0, 6, 12, 18], tz=UTC6))
a1.xaxis.set_major_formatter(mdates.DateFormatter("%d-oct\n%H h", tz=UTC6))
a1.set_ylim(30, 106)
a1.set_ylabel("humedad relativa del BME280 (%)")
a1.set_xlabel("hora local (UTC−6)")
a1.set_title("Sonda Orbitium (Sci-Fi), ITPA: HR del 27 y 28-oct")
a1.legend(fontsize=8, loc="lower left")
for s, mk in ((ORB, "s"), (TAES, "o")):
    hh = [h for h in hum_tab if h["sonda"] == s and h["rama"] != "suelo"]
    a2_.plot([h["mezcla_gkg"] for h in hh], [h["z_gps_m"] / 1000 for h in hh], ls="none", marker=mk,
             ms=7 if s == ORB else 3.5, color=C[s], mec=SURF, mew=0.5, label=LBL[s])
a2_.set_xlabel("razón de mezcla calculada (g/kg)")
a2_.set_ylabel("altitud GPS (km)")
a2_.set_xlim(0, 20)
a2_.set_title("Vapor de agua en la cápsula durante el vuelo")
a2_.legend(fontsize=8, loc="upper left")
nota(fig, "La razón de mezcla sale de HR, temperatura y presión del mismo BME280 (Magnus). Describe el aire dentro de la cápsula, "
          "no el de la atmósfera.")
guardar_fig(fig, "a2_humedad.png")

# --- Fig 9: temperatura de la cápsula y batería
fig, (a1, a2_) = plt.subplots(1, 2, figsize=(10.5, 4.6))
fig.subplots_adjust(wspace=0.3)
for s, mk in ((ORB, "s"), (TAES, "o")):
    pts = [x for x in pz[s] if x[3] is not None]
    a1.plot([x[0] / 60 for x in pts], [x[3] for x in pts], color=C[s], lw=1.6, marker=mk,
            ms=6 if s == ORB else 3, mec=SURF, mew=0.5, label=f"{LBL[s]} · sensor de la cápsula (interior)")
    est = [(x[0] / 60, Tv_perfil(x[1]) - 273.15) for x in pts if x[0] >= 0]
    a1.plot([e[0] for e in est], [e[1] for e in est], color=C[s], lw=1.2, ls=(0, (3, 2)),
            label=f"{LBL[s]} · aire, estimado por hipsometría")
a1.axhline(-10, color=REF, lw=1, ls=(0, (4, 3)))
a1.text(-38, -9, "−10 °C: referencia de diseño de la batería (prueba de frío del evento)", fontsize=8, color=TXT2)
a1.set_xlabel("tiempo desde el despegue estimado (min)")
a1.set_ylabel("temperatura (°C)")
a1.set_xlim(-40, 62)
a1.set_ylim(-35, 35)
a1.set_title("Sensor de la cápsula (interior) y aire estimado")
a1.legend(fontsize=7.5, loc="lower left")
bt = [(tt_, b) for tt_, b in bat if tt_ >= T0[TAES] - timedelta(minutes=40)]
a2_.plot([(tt_ - T0[TAES]).total_seconds() / 60 for tt_, b in bt], [b for tt_, b in bt], color=C[TAES], lw=1.8)
a2_.axhline(3.3, color=CRIT, lw=1)
a2_.text(-38, 3.33, "3.3 V: umbral de batería de la prueba de frío", fontsize=8, color=TXT2)
a2_.set_ylim(3.2, 4.2)
a2_.set_xlim(-40, 46)
a2_.set_xlabel("tiempo desde el despegue estimado (min)")
a2_.set_ylabel("voltaje de batería (V)")
a2_.set_title("Batería de la Sonda TAES (Instituto Tepeyac/UPA)")
a2_.text(-38, 3.95, f"{max(bv):.3f} → {min(bv):.3f} V en vuelo", fontsize=8.5, color=TXT)
nota(fig, "La Sonda Orbitium (Sci-Fi), ITPA no reportó batería ni INA219 en ningún paquete del dataset y marcó usb=1 en todo el vuelo. "
          "Aire estimado = perfil hipsométrico de Tv (presión de la cápsula + altitud GPS) evaluado en la altitud de la sonda.")
guardar_fig(fig, "a2_capsula_bateria.png")

with open(os.path.join(DERIV, "a2_resumen.txt"), "w", encoding="utf-8") as fh:
    fh.write("\n".join(out) + "\n")
print("\nresumen escrito en datos/derivados/a2_resumen.txt")
