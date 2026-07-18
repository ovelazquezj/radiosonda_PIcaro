"""
dashboard.py  -  PICARO Mission Control (dashboard tkinter estilo NASA).

Consume la telemetria del Ejercicio 09 (que llega a ChirpStack), la guarda en
SQLite local y la muestra con paneles, graficas de tendencia, un mapa real con
el track de posiciones (filtro de tiempo + decimado) y un log de eventos.
Ademas exporta a CSV (todos los campos) los datos de la ventana de tiempo.

Fuentes de datos (config o --mode):
  mqtt    en vivo desde ChirpStack por MQTT   (necesita gateway + placa emitiendo)
  sim     simulador de telemetria sintetica   (para probar sin gateway)
  replay  reproduce lo ya guardado en la SQLite

Uso:
  py -3 dashboard.py                 # usa config.json (o config.example.json)
  py -3 dashboard.py --mode sim      # demo sin hardware
  py -3 dashboard.py --mode mqtt
"""
import argparse
import csv
import json
import os
import queue
import time
import tkinter as tk
from datetime import datetime, timezone
from tkinter import ttk, filedialog, messagebox

import theme as T
from storage import TelemetryStore, decimate, FIELDS
from mqtt_ingest import MqttIngest
from replay import SimIngest, ReplayIngest

from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

try:
    from tkintermapview import TkinterMapView
    HAVE_MAP = True
except Exception:
    HAVE_MAP = False

# Ventanas de tiempo (nombre -> segundos; None = todo). 1 h por defecto.
WINDOWS = [("15 min", 900), ("1 h", 3600), ("6 h", 21600),
           ("24 h", 86400), ("Todo", None)]
MAP_MAX_POINTS = 500        # tope de puntos dibujados (decimado)


def _fmt(v, spec="{:.1f}", dash="--"):
    return dash if v is None else spec.format(v)


class Tile:
    """Etiqueta + valor grande + unidad (una celda de telemetria)."""
    def __init__(self, parent, label, unit="", color=T.TEXT):
        self.frame = tk.Frame(parent, bg=T.PANEL)
        tk.Label(self.frame, text=label, bg=T.PANEL, fg=T.MUTED, font=T.F_LABEL).pack(anchor="w")
        row = tk.Frame(self.frame, bg=T.PANEL); row.pack(anchor="w")
        self.val = tk.Label(row, text="--", bg=T.PANEL, fg=color, font=T.F_VALUE)
        self.val.pack(side="left")
        self._unit = tk.Label(row, text=(" " + unit) if unit else "", bg=T.PANEL,
                              fg=T.MUTED, font=T.F_UNIT)
        self._unit.pack(side="left", anchor="s", pady=(0, 5))

    def set(self, text, color=None):
        self.val.config(text=text)
        if color:
            self.val.config(fg=color)


class StatusLED:
    """Indicador redondo + texto (el estado no depende solo del color)."""
    def __init__(self, parent, label, bg=T.PANEL):
        self.frame = tk.Frame(parent, bg=bg)
        self.canvas = tk.Canvas(self.frame, width=16, height=16, bg=bg, highlightthickness=0)
        self.dot = self.canvas.create_oval(3, 3, 13, 13, fill=T.MUTED, outline="")
        self.canvas.pack(side="left")
        self.lbl = tk.Label(self.frame, text=label, bg=bg, fg=T.TEXT, font=T.F_LABEL)
        self.lbl.pack(side="left", padx=(5, 0))

    def set(self, color, text=None):
        self.canvas.itemconfig(self.dot, fill=color)
        if text is not None:
            self.lbl.config(text=text)


class Dashboard:
    def __init__(self, root, config):
        self.root = root
        self.cfg = config
        self.mode = config.get("mode", "sim")
        self.store = TelemetryStore(config.get("db_path", "picaro_telemetry.db"))
        self.q = queue.Queue()
        self._last = None
        self._ingest_status = (False, "iniciando...")
        self._t_start = time.time()
        self._last_map_key = None
        self._map_refresh_due = 0

        root.title("PICARO · Mission Control")
        root.configure(bg=T.BG)
        root.geometry("1180x800")
        root.minsize(1040, 700)
        T.apply_ttk(root)
        T.apply_matplotlib()

        self._build_topbar()
        self._build_linkstrip()
        body = tk.Frame(root, bg=T.BG); body.pack(fill="both", expand=True, padx=8, pady=(0, 8))
        self._build_panels(body)     # 3 columnas: Power | Environment | GNSS
        self._build_middle(body)     # Trend | Position Track (50/50, area principal)
        self._build_log(body)

        self._start_ingest()
        self.root.after(150, self._poll)
        self.root.after(1000, self._tick)
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ---------------- UI ----------------
    def _panel(self, parent, title):
        outer = tk.Frame(parent, bg=T.BORDER)
        inner = tk.Frame(outer, bg=T.PANEL)
        inner.pack(fill="both", expand=True, padx=1, pady=1)
        tk.Label(inner, text=title, bg=T.PANEL, fg=T.CYAN, font=T.F_SECTION).pack(anchor="w", padx=10, pady=(8, 2))
        return outer, inner

    def _build_topbar(self):
        bar = tk.Frame(self.root, bg=T.PANEL2, height=52)
        bar.pack(fill="x", side="top")
        tk.Label(bar, text="◇ PICARO", bg=T.PANEL2, fg=T.CYAN, font=(T.SANS, 16, "bold")).pack(side="left", padx=(14, 6), pady=10)
        tk.Label(bar, text="MISSION CONTROL", bg=T.PANEL2, fg=T.TEXT, font=(T.SANS, 12)).pack(side="left", pady=10)
        self.mode_lbl = tk.Label(bar, text="", bg=T.PANEL2, fg=T.VIOLET, font=(T.MONO, 10, "bold"))
        self.mode_lbl.pack(side="left", padx=16)

        self.dev_lbl = tk.Label(bar, text="DEV --", bg=T.PANEL2, fg=T.MUTED, font=T.F_MONO)
        self.dev_lbl.pack(side="right", padx=14)
        self.conn = StatusLED(bar, "sin datos", bg=T.PANEL2); self.conn.frame.pack(side="right", padx=10)
        self.met_lbl = tk.Label(bar, text="MET 00:00:00", bg=T.PANEL2, fg=T.TEXT, font=T.F_CLOCK)
        self.met_lbl.pack(side="right", padx=14)
        self.clock_lbl = tk.Label(bar, text="UTC --:--:--", bg=T.PANEL2, fg=T.GREEN, font=T.F_CLOCK)
        self.clock_lbl.pack(side="right", padx=6)

    def _build_linkstrip(self):
        """Franja fina con las metricas de enlace (antes panel 'Link')."""
        strip = tk.Frame(self.root, bg=T.BG)
        strip.pack(fill="x", padx=10, pady=(6, 0))
        tk.Label(strip, text="LINK", bg=T.BG, fg=T.CYAN, font=T.F_SECTION).pack(side="left", padx=(2, 12))
        self.link_lbl = tk.Label(strip, text="RSSI --   SNR --   fCnt --   DR --   gw --",
                                 bg=T.BG, fg=T.TEXT, font=(T.MONO, 11, "bold"))
        self.link_lbl.pack(side="left")

    def _build_panels(self, parent):
        row = tk.Frame(parent, bg=T.BG); row.pack(fill="x", pady=(8, 0))
        # Tk 8.6 no acepta columnconfigure con tupla -> configurar 1 por 1.
        for _c in (0, 1, 2):
            row.columnconfigure(_c, weight=1, uniform="p")

        # --- POWER ---
        outer, p = self._panel(row, "POWER"); outer.grid(row=0, column=0, sticky="nsew", padx=(0, 6))
        grid = tk.Frame(p, bg=T.PANEL); grid.pack(fill="x", padx=10, pady=(0, 8))
        self.t_batt_v = Tile(grid, "BATERIA", "V", T.GREEN); self.t_batt_v.frame.grid(row=0, column=0, sticky="w", padx=(0, 18))
        self.t_batt_p = Tile(grid, "CARGA", "%", T.GREEN); self.t_batt_p.frame.grid(row=0, column=1, sticky="w")
        pw = tk.Frame(p, bg=T.PANEL); pw.pack(fill="x", padx=10, pady=(0, 12))
        self.led_usb = StatusLED(pw, "USB"); self.led_usb.frame.pack(side="left", padx=(0, 14))
        self.led_chg = StatusLED(pw, "CARGANDO"); self.led_chg.frame.pack(side="left")

        # --- ENVIRONMENT ---
        outer, e = self._panel(row, "ENVIRONMENT"); outer.grid(row=0, column=1, sticky="nsew", padx=6)
        grid = tk.Frame(e, bg=T.PANEL); grid.pack(fill="x", padx=10, pady=(0, 20))
        self.t_temp = Tile(grid, "TEMPERATURA", "°C", T.AMBER); self.t_temp.frame.grid(row=0, column=0, sticky="w", padx=(0, 18))
        self.t_press = Tile(grid, "PRESION", "hPa", T.CYAN); self.t_press.frame.grid(row=0, column=1, sticky="w")

        # --- GNSS ---
        outer, g = self._panel(row, "GNSS"); outer.grid(row=0, column=2, sticky="nsew", padx=(6, 0))
        leds = tk.Frame(g, bg=T.PANEL); leds.pack(fill="x", padx=10, pady=(0, 4))
        self.led_gps_act = StatusLED(leds, "ACTIVO"); self.led_gps_act.frame.pack(side="left", padx=(0, 16))
        self.led_gps_fix = StatusLED(leds, "FIX"); self.led_gps_fix.frame.pack(side="left")
        self.g_lat = tk.Label(g, text="LAT   --", bg=T.PANEL, fg=T.TEXT, font=(T.MONO, 12, "bold"))
        self.g_lat.pack(anchor="w", padx=10)
        self.g_lon = tk.Label(g, text="LON   --", bg=T.PANEL, fg=T.TEXT, font=(T.MONO, 12, "bold"))
        self.g_lon.pack(anchor="w", padx=10)
        self.g_sat = tk.Label(g, text="ALT -- m   SAT --", bg=T.PANEL, fg=T.MUTED, font=(T.MONO, 11))
        self.g_sat.pack(anchor="w", padx=10, pady=(0, 8))

    def _build_middle(self, parent):
        mid = tk.Frame(parent, bg=T.BG); mid.pack(fill="both", expand=True, pady=(8, 0))
        mid.columnconfigure(0, weight=1, uniform="m")
        mid.columnconfigure(1, weight=1, uniform="m")
        mid.rowconfigure(0, weight=1)

        # --- TREND (mitad izquierda) ---
        outer, pl = self._panel(mid, "TREND"); outer.grid(row=0, column=0, sticky="nsew", padx=(0, 6))
        self._fig = Figure(figsize=(4.6, 3.6), dpi=100)
        axes = self._fig.subplots(3, 1, sharex=True)
        self._plot_axes = [
            (axes[0], "battery_v", T.GREEN, "Bat V"),
            (axes[1], "temperature_c", T.AMBER, "T °C"),
            (axes[2], "pressure_hpa", T.CYAN, "hPa"),
        ]
        self._canvas = FigureCanvasTkAgg(self._fig, master=pl)
        self._canvas.get_tk_widget().pack(fill="both", expand=True, padx=8, pady=8)

        # --- POSITION TRACK (mitad derecha) ---
        outer, mp = self._panel(mid, "POSITION TRACK"); outer.grid(row=0, column=1, sticky="nsew", padx=(6, 0))
        ctl = tk.Frame(mp, bg=T.PANEL); ctl.pack(fill="x", padx=10, pady=(0, 4))
        tk.Label(ctl, text="Ventana:", bg=T.PANEL, fg=T.MUTED, font=T.F_LABEL).pack(side="left")
        self.win_var = tk.StringVar(value=self.cfg.get("default_window", "1 h"))
        cb = ttk.Combobox(ctl, textvariable=self.win_var, width=8, state="readonly",
                          values=[w[0] for w in WINDOWS])
        cb.pack(side="left", padx=6)
        cb.bind("<<ComboboxSelected>>", lambda e: self._refresh_view(force_map=True))
        tk.Button(ctl, text="⬇ CSV", command=self._export_csv, font=T.F_LABEL,
                  bg=T.PANEL2, fg=T.CYAN, activebackground=T.BORDER, activeforeground=T.TEXT,
                  relief="flat", bd=0, padx=10, pady=2, cursor="hand2").pack(side="left", padx=8)
        self.trk_lbl = tk.Label(ctl, text="0 puntos", bg=T.PANEL, fg=T.MUTED, font=T.F_LABEL)
        self.trk_lbl.pack(side="right")

        base = self.cfg.get("map_start", {"lat": 21.9337, "lon": -102.2983, "zoom": 13})
        if HAVE_MAP:
            self.map = TkinterMapView(mp, corner_radius=0)
            self.map.pack(fill="both", expand=True, padx=8, pady=(0, 8))
            self.map.set_position(base["lat"], base["lon"])
            self.map.set_zoom(base.get("zoom", 13))
        else:
            self.map = None
            tk.Label(mp, text="Instala 'tkintermapview' para el mapa\n(pip install tkintermapview)",
                     bg=T.PANEL, fg=T.AMBER, font=T.F_MONO).pack(expand=True)

    def _build_log(self, parent):
        outer, lg = self._panel(parent, "EVENT LOG"); outer.pack(fill="x", pady=(8, 0))
        cols = ("utc", "fcnt", "temp", "press", "batt", "rssi", "snr", "gps")
        heads = ("UTC", "fCnt", "T°C", "hPa", "Bat%", "RSSI", "SNR", "GPS")
        widths = (150, 60, 70, 80, 60, 70, 60, 90)
        self.tree = ttk.Treeview(lg, columns=cols, show="headings", height=6)
        for c, h, w in zip(cols, heads, widths):
            self.tree.heading(c, text=h)
            self.tree.column(c, width=w, anchor="center")
        self.tree.pack(fill="x", padx=8, pady=8)

    # ---------------- Ingesta ----------------
    def _on_record(self, rec):      # llamado desde el hilo de ingesta
        self.q.put(rec)

    def _on_status(self, ok, msg):  # llamado desde el hilo de ingesta
        self._ingest_status = (ok, msg)

    def _start_ingest(self):
        self.mode_lbl.config(text=f"[{self.mode.upper()}]")
        if self.mode == "mqtt":
            m = self.cfg.get("mqtt", {})
            self.ingest = MqttIngest(m.get("host", "localhost"), m.get("port", 1883),
                                     self.cfg.get("app_id", ""), self.cfg.get("dev_eui", ""),
                                     self._on_record, self._on_status)
        elif self.mode == "replay":
            self.ingest = ReplayIngest(self.store, self._on_record, self._on_status,
                                       period_s=self.cfg.get("replay_period_s", 0.5))
        else:
            base = self.cfg.get("map_start", {})
            self.ingest = SimIngest(self._on_record, self._on_status,
                                    period_s=self.cfg.get("sim_period_s", 2.0),
                                    base_lat=base.get("lat", 21.9337),
                                    base_lon=base.get("lon", -102.2983))
        self.ingest.start()

    # ---------------- Loops ----------------
    def _poll(self):
        n = 0
        while True:
            try:
                rec = self.q.get_nowait()
            except queue.Empty:
                break
            try:
                self.store.insert(rec)
            except Exception:
                pass
            self._last = rec
            self._update_panels(rec)
            self._append_log(rec)
            n += 1
        if n:
            self._map_refresh_due = 0
        self.root.after(150, self._poll)

    def _tick(self):
        now = datetime.now(timezone.utc)
        self.clock_lbl.config(text=now.strftime("UTC %H:%M:%S"))
        met = int(time.time() - self._t_start)
        self.met_lbl.config(text=f"MET {met // 3600:02d}:{(met % 3600) // 60:02d}:{met % 60:02d}")

        ok, msg = self._ingest_status
        age = (time.time() - self._last["ts"]) if self._last else 1e9
        if self._last and age < 90:
            self.conn.set(T.GREEN, "enlace activo")
        elif self._last and age < 300:
            self.conn.set(T.AMBER, f"ultimo hace {int(age)}s")
        elif ok:
            self.conn.set(T.CYAN, msg[:26])
        else:
            self.conn.set(T.RED if self._last else T.MUTED, (msg or "sin datos")[:26])

        if time.time() >= self._map_refresh_due:
            self._refresh_view()
            self._map_refresh_due = time.time() + 3
        self.root.after(1000, self._tick)

    # ---------------- Actualizacion ----------------
    def _update_panels(self, r):
        self.dev_lbl.config(text=f"DEV {(r.get('dev_eui') or '--')}")
        # LINK (franja)
        rssi = r.get("rssi"); snr = r.get("snr")
        self.link_lbl.config(text="RSSI {}   SNR {}   fCnt {}   DR {}   gw {}".format(
            f"{rssi} dBm" if rssi is not None else "--",
            f"{snr:.1f} dB" if snr is not None else "--",
            r.get("f_cnt", "--"), r.get("dr", "--"), (r.get("gateway_id") or "--")[:12]))
        # POWER
        self.t_batt_v.set(_fmt(r.get("battery_v"), "{:.2f}"))
        bp = r.get("battery_pct")
        self.t_batt_p.set("--" if bp is None else str(bp),
                          T.GREEN if (bp or 0) >= 40 else T.AMBER if (bp or 0) >= 15 else T.RED)
        self.led_usb.set(T.CYAN if r.get("usb_powered") else T.MUTED)
        self.led_chg.set(T.GREEN if r.get("charging") else T.MUTED)
        # ENVIRONMENT
        self.t_temp.set(_fmt(r.get("temperature_c"), "{:.1f}"))
        self.t_press.set(_fmt(r.get("pressure_hpa"), "{:.1f}"))
        # GNSS
        self.led_gps_act.set(T.GREEN if r.get("gps_active") else T.RED,
                             "ACTIVO" if r.get("gps_active") else "APAGADO")
        self.led_gps_fix.set(T.GREEN if r.get("gps_fix") else T.AMBER,
                             "FIX" if r.get("gps_fix") else "SIN FIX")
        if r.get("gps_fix") and r.get("latitude") is not None:
            self.g_lat.config(text=f"LAT  {r['latitude']:.6f}")
            self.g_lon.config(text=f"LON {r['longitude']:.6f}")
            self.g_sat.config(text=f"ALT {_fmt(r.get('altitude_m'),'{:.0f}')} m   SAT {r.get('satellites','--')}")
        else:
            self.g_lat.config(text="LAT   -- (sin fix)")
            self.g_lon.config(text="LON   --")
            self.g_sat.config(text=f"ALT -- m   SAT {r.get('satellites','--')}")

    def _append_log(self, r):
        vals = (r.get("iso", "")[-8:], r.get("f_cnt", ""),
                _fmt(r.get("temperature_c")), _fmt(r.get("pressure_hpa")),
                r.get("battery_pct", ""), r.get("rssi", ""), _fmt(r.get("snr")),
                "FIX" if r.get("gps_fix") else ("ON" if r.get("gps_active") else "OFF"))
        self.tree.insert("", 0, values=vals)
        kids = self.tree.get_children()
        if len(kids) > 200:
            self.tree.delete(kids[-1])

    def _window_seconds(self):
        for name, secs in WINDOWS:
            if name == self.win_var.get():
                return secs
        return 3600

    def _refresh_view(self, force_map=False):
        secs = self._window_seconds()
        since = 0 if secs is None else (time.time() - secs)
        self._refresh_plots(since)
        if self.map is not None:
            self._refresh_map(since)

    def _refresh_plots(self, since):
        data = self.store.series(since)
        for ax, key, color, ylabel in self._plot_axes:
            ax.clear()
            ax.set_ylabel(ylabel, fontsize=8)
            ax.grid(True, alpha=0.3)
            if data:
                t0 = data[0]["ts"]
                xs = [(d["ts"] - t0) / 60.0 for d in data]
                ys = [d[key] for d in data]
                ax.plot(xs, ys, color=color)
        self._plot_axes[-1][0].set_xlabel("min", fontsize=8)
        try:
            self._canvas.draw_idle()
        except Exception:
            pass

    def _refresh_map(self, since):
        pts = self.store.track(since)
        self.trk_lbl.config(text=f"{len(pts)} puntos")
        key = (len(pts), self.win_var.get())
        if key == self._last_map_key:
            return
        self._last_map_key = key
        self.map.delete_all_path()
        self.map.delete_all_marker()
        if not pts:
            return
        latlon = decimate([(p[1], p[2]) for p in pts], MAP_MAX_POINTS)
        if len(latlon) >= 2:
            self.map.set_path(latlon, color=T.CYAN, width=3)
        self.map.set_marker(latlon[0][0], latlon[0][1], text="inicio", marker_color_circle=T.MUTED)
        self.map.set_marker(latlon[-1][0], latlon[-1][1], text="ahora", marker_color_circle=T.GREEN)
        self.map.set_position(latlon[-1][0], latlon[-1][1])

    # ---------------- Export CSV ----------------
    def _export_csv(self):
        """Exporta a CSV TODOS los campos de los registros dentro de la ventana
        de tiempo seleccionada en Position Track."""
        secs = self._window_seconds()
        since = 0 if secs is None else (time.time() - secs)
        rows = self.store.records_since(since)
        if not rows:
            messagebox.showinfo("Exportar CSV", "No hay datos en la ventana seleccionada.")
            return
        win = self.win_var.get().replace(" ", "")
        default = f"picaro_telemetry_{win}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        path = filedialog.asksaveasfilename(
            defaultextension=".csv", initialfile=default,
            filetypes=[("CSV", "*.csv"), ("Todos", "*.*")],
            title="Guardar telemetria filtrada (todos los campos)")
        if not path:
            return
        try:
            with open(path, "w", newline="", encoding="utf-8") as f:
                w = csv.DictWriter(f, fieldnames=FIELDS, extrasaction="ignore")
                w.writeheader()
                for r in rows:
                    w.writerow({k: r.get(k) for k in FIELDS})
            messagebox.showinfo("Exportar CSV",
                                f"Guardadas {len(rows)} filas (ventana {self.win_var.get()}).\n{path}")
        except Exception as e:
            messagebox.showerror("Exportar CSV", str(e))

    def _on_close(self):
        try:
            self.ingest.stop()
        except Exception:
            pass
        self.store.close()
        self.root.destroy()


def load_config(path):
    cfg = {}
    for p in (path, "config.json", "config.example.json"):
        if p and os.path.exists(p):
            with open(p, "r", encoding="utf-8") as f:
                cfg = json.load(f)
            break
    return cfg


def main():
    ap = argparse.ArgumentParser(description="PICARO Mission Control dashboard")
    ap.add_argument("--config", default=None)
    ap.add_argument("--mode", choices=["mqtt", "sim", "replay"], default=None)
    ap.add_argument("--db", default=None)
    args = ap.parse_args()
    cfg = load_config(args.config)
    if args.mode:
        cfg["mode"] = args.mode
    if args.db:
        cfg["db_path"] = args.db
    cfg.setdefault("mode", "sim")

    root = tk.Tk()
    Dashboard(root, cfg)
    root.mainloop()


if __name__ == "__main__":
    main()
