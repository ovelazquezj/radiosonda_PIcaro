"""
theme.py  -  Look & Feel "control de mision" (estilo NASA).

Paleta de ALTO CONTRASTE, pensada para leerse con distinta luz ambiental:
fondo casi negro, texto casi blanco, y acentos saturados (verde/cian/ambar/rojo).
El estado NUNCA depende solo del color (siempre hay texto/etiqueta) para no
excluir a personas con daltonismo. Fuentes claras: monoespaciada para los
valores (tabular y legible) y sans del sistema para etiquetas.
"""
from tkinter import ttk

# --- Paleta ---
BG      = "#0B0E14"   # fondo (espacio profundo)
PANEL   = "#131824"   # panel
PANEL2  = "#1A2130"   # panel alterno / cabecera
BORDER  = "#2A3446"   # bordes / rejilla
TEXT    = "#E6EDF3"   # texto principal (alto contraste)
MUTED   = "#8B98A9"   # texto secundario
GREEN   = "#3FE07A"   # nominal / OK
CYAN    = "#35D6E8"   # datos / enlaces
AMBER   = "#FFB84D"   # precaucion
RED     = "#FF5C5C"   # falla / alerta
VIOLET  = "#B98CFF"   # acento secundario

# --- Fuentes ---
MONO = "Consolas"     # Windows; clara y tabular
SANS = "Segoe UI"

F_TITLE   = (SANS, 15, "bold")
F_SECTION = (SANS, 10, "bold")
F_LABEL   = (SANS, 9)
F_VALUE   = (MONO, 22, "bold")
F_VALUE_S = (MONO, 14, "bold")
F_UNIT    = (SANS, 9)
F_MONO    = (MONO, 10)
F_CLOCK   = (MONO, 13, "bold")


def apply_ttk(root):
    """Aplica el tema oscuro a los widgets ttk."""
    style = ttk.Style(root)
    try:
        style.theme_use("clam")
    except Exception:
        pass
    style.configure(".", background=BG, foreground=TEXT,
                    fieldbackground=PANEL, bordercolor=BORDER)
    style.configure("TFrame", background=BG)
    style.configure("Panel.TFrame", background=PANEL)
    style.configure("Head.TFrame", background=PANEL2)
    style.configure("TLabel", background=PANEL, foreground=TEXT, font=F_LABEL)
    style.configure("Bg.TLabel", background=BG, foreground=TEXT)
    style.configure("Head.TLabel", background=PANEL2, foreground=TEXT)
    style.configure("Section.TLabel", background=PANEL, foreground=CYAN, font=F_SECTION)
    style.configure("Muted.TLabel", background=PANEL, foreground=MUTED, font=F_LABEL)

    style.configure("TCombobox", fieldbackground=PANEL2, background=PANEL2,
                    foreground=TEXT, arrowcolor=CYAN, bordercolor=BORDER)
    style.map("TCombobox", fieldbackground=[("readonly", PANEL2)],
              foreground=[("readonly", TEXT)])

    # Tabla de eventos (Treeview)
    style.configure("Treeview", background=PANEL, fieldbackground=PANEL,
                    foreground=TEXT, bordercolor=BORDER, font=F_MONO, rowheight=20)
    style.configure("Treeview.Heading", background=PANEL2, foreground=CYAN,
                    font=(SANS, 9, "bold"))
    style.map("Treeview", background=[("selected", "#243049")])
    return style


def apply_matplotlib():
    """Tema oscuro para las graficas matplotlib."""
    import matplotlib as mpl
    mpl.rcParams.update({
        "figure.facecolor": PANEL,
        "axes.facecolor": PANEL,
        "axes.edgecolor": BORDER,
        "axes.labelcolor": MUTED,
        "axes.titlecolor": TEXT,
        "text.color": TEXT,
        "xtick.color": MUTED,
        "ytick.color": MUTED,
        "grid.color": BORDER,
        "grid.linewidth": 0.5,
        "font.family": "monospace",
        "font.size": 8,
        "lines.linewidth": 1.6,
        "figure.autolayout": True,
    })
