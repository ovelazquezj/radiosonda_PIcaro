# SatLink 2026: reporte y datos de vuelo

Proyecto PiCARO: <https://pi-caro.org>

Reporte técnico de los vuelos de **Misiones Espaciales - SatLink · Edición 2026**, celebrados en el Instituto Tecnológico de Pabellón de Arteaga (ITPA) el 28 de octubre de 2026. Esa noche volaron tres sondas. El reporte cubre las dos que transmitieron telemetría por la red LoRaWAN del proyecto PiCARO, cada una en su propio inciso:

- **a) Sonda Orbitium (Sci-Fi)**, del ITPA: 8478 m de altitud, Clase C, comandos Marco Polo, corte por radio y recuperación en El Rascón, Zacatecas.
- **b) Sonda TAES**, del Instituto Tepeyac y la Universidad Politécnica de Aguascalientes (UPA): posición cada ~30 s, de la que salen el perfil de viento y la temperatura virtual de la noche, y corte por tiempo.

La tercera, de la Universidad Tecnológica del Norte de Aguascalientes (UTNA), no transmitió por la red.

## Documentos

| Archivo | Contenido |
|---|---|
| [REPORTE-SATLINK-2026.md](REPORTE-SATLINK-2026.md) | **Reporte principal**: el evento, la infraestructura, el perfil de vuelo, cómo terminó cada vuelo, el enlace, las operaciones, lo que aprendimos y el resumen de recomendaciones para 2027. Cada sección trata por separado a Orbitium (a), a TAES (b) y lo comparativo (c) |
| [METODOLOGIA-Y-DATOS.md](METODOLOGIA-Y-DATOS.md) | Fuentes, procesamiento, filtros, limitaciones, cómo reproducir los resultados y licencia |

## Carpetas

| Carpeta | Contenido |
|---|---|
| [`datos/`](datos/) | CSV anonimizados: `uplinks`, `recepciones`, `downlinks`, `joins`, `eventos_red`, `vuelos`, `gateways`, `gateway_metricas` y `recuperacion`. Incluye el [diccionario](datos/DICCIONARIO.md) |
| [`datos/derivados/`](datos/derivados/) | Tablas calculadas por los scripts (`a1_*`, `a2_*` y `a3_*`) |
| [`scripts/`](scripts/) | `a1_rf.py`, `a2_vuelo.py` y `a3_operaciones.py`. Se ejecutan con `python -I` y leen solo `datos/` |
| [`figuras/`](figuras/) | 19 figuras en PNG generadas por los scripts. Todos los "T+" se cuentan desde el despegue estimado de [`datos/vuelos.csv`](datos/vuelos.csv) |

## Cómo reproducir

El reporte, los datos y los scripts están en <https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/docs/reportes/satlink-2026>.

```
git clone https://github.com/ovelazquezj/radiosonda_PIcaro.git
cd radiosonda_PIcaro/docs/reportes/satlink-2026/scripts
python -I a1_rf.py
python -I a2_vuelo.py
python -I a3_operaciones.py
```

Hacen falta Python 3.10 o posterior, `matplotlib` y `numpy`. Los datos originales de la red (sin anonimizar) no se publican porque contienen identificadores de los dispositivos y de la infraestructura. Quien los necesite para investigación puede solicitarlos a través de <https://pi-caro.org>.

## Licencia

El texto, las figuras y los datos van bajo CC BY 4.0 (ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md)), y los scripts, bajo The Clear BSD License. Los detalles están en [METODOLOGIA-Y-DATOS.md](METODOLOGIA-Y-DATOS.md#6-licencia-sugerida).
