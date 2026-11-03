# Metodología y datos

*Cómo se obtuvieron las cifras del [reporte SatLink 2026](REPORTE-SATLINK-2026.md), qué limitaciones tienen y cómo rehacerlas.*

## 1. Fuentes

| Fuente | Qué aporta | ¿Se publica? |
|---|---|---|
| Grabador de eventos del servidor de red (ChirpStack 4.19, US915 sub-banda 2) | Eventos `up`, `join`, `down`, `txack`, `log` y `status` de las aplicaciones de las dos sondas que transmitieron por la red (Orbitium y TAES), días UTC 27 a 29 de octubre de 2026 | Solo anonimizados, en [`datos/`](datos/) |
| API de métricas de ChirpStack | Paquetes recibidos y transmitidos por gateway, del 20-oct al 2-nov (UTC) | Sí: [`datos/gateway_metricas.csv`](datos/gateway_metricas.csv) |
| Inventario de gateways del servidor | Modelo, protocolo, antena, posición y altitud de la antena (en el campus, suelo a 1890 m más el mástil) | Sí: [`datos/gateways.csv`](datos/gateways.csv) (el laboratorio, como punto de referencia aproximado, desplazado 1-2 km) |
| Historial del mapa público | Se revisó y no aporta uplinks que falten en el grabador | No se usa |
| Codec de la Sonda Orbitium (Sci-Fi), del Instituto Tecnológico de Pabellón de Arteaga (ITPA) | Decodificación de los fPorts 20 a 32 | Lo aplica el servidor |
| Datos que declaró el Equipo Orbitium (Sci-Fi) | Criterio de detección del despegue de su firmware (suelo + 100 m durante 3 lecturas seguidas de 1 s), hardware, masa y pruebas de la cápsula | Se citan los datos en el reporte |
| Datos que declaró el equipo TAES, del Instituto Tepeyac y la Universidad Politécnica de Aguascalientes (UPA) | Electrónica (LilyGO T-Beam Supreme) y corte por tiempo de su propio sistema | Se citan los datos en el reporte |
| SRTM de 30 m (consulta a OpenTopoData) | Elevación del terreno en el punto de recuperación | Sí: [`datos/recuperacion.csv`](datos/recuperacion.csv) |

Los datos originales de la red (sin anonimizar) no se publican porque contienen identificadores de los dispositivos y de la infraestructura. Quien los necesite para investigación puede solicitarlos a través de <https://pi-caro.org>.

## 2. Procesamiento

1. **Extracción privada.** Unos scripts que no se publican leen los eventos crudos y escriben los CSV de `datos/`. Esos scripts quitan DevEUI, EUI de gateways, JoinEUI, DevAddr, claves, UUID e IP. Nombran las sondas por su alias público y los gateways por su nombre.
2. **Tiempo.** Se usa el campo `time` del evento, que es la hora de recepción en el servidor. Las fracciones de 7 a 9 dígitos se recortan a milisegundos. `t_utc` va en UTC y `t_local` en UTC−6.
3. **Uplinks.** Una fila por uplink, ya deduplicado por el servidor ([`datos/uplinks.csv`](datos/uplinks.csv), 2758 filas). Además, una fila por cada pareja de uplink y gateway ([`datos/recepciones.csv`](datos/recepciones.csv), 5420 filas). Un uplink se identifica por (sonda, sesión, fCnt), porque el fCnt se reinicia en cada join.
4. **Sonda TAES, del Instituto Tepeyac y la Universidad Politécnica de Aguascalientes (UPA).** Su codec no estaba disponible, así que el payload se reinterpretó desde los bytes con un formato **inferido**. Hay dos versiones: 26 B y 27 B. Su codec en el servidor coincide con esa reinterpretación en los 342 paquetes de 27 B y falla en los 847 de 26 B. El formato está en [`datos/DICCIONARIO.md`](datos/DICCIONARIO.md).
5. **Filtros de calidad del GPS** (`calidad_gps`). No se borra ninguna fila; cada una se marca, en este orden:
   - sin fix;
   - posición imposible (a más de 300 km del campus);
   - altitud imposible (menor de 1700 m o mayor de 40 000 m);
   - salto (más de 60 m/s en horizontal o más de 40 m/s en vertical);
   - ok.

   Los análisis usan solo los fixes "ok", más las altitudes GPS del fPort 31 de Orbitium.
6. **Humedad.** La humedad relativa de 100 % o más se marca como `pegada_100`, porque en estas placas indica un BME280 dañado.
7. **Ventana de vuelo y T0.** Va del despegue estimado a la última recepción. El despegue estimado es el **único T0** de todas las tablas y figuras. En TAES, que manda posición cada 30 s, se estima extrapolando hacia atrás, hasta la altitud del suelo, con la velocidad de sus dos primeros fixes en el aire. En Orbitium, con fixes cada 10 min, esa extrapolación daba un despegue anterior al armado del temporizador y a la detección de su firmware, así que se usa el firmware: el despegue es la detección (fase `vuelo` del fPort 32, 22:37:30) menos el tiempo de subir 100 m a la velocidad media del primer tramo (4.1 m/s) y menos 2 s de lecturas de confirmación. Sale a las 22:37:04, acotado entre el armado aceptado (22:37:02, que se supone hecho en tierra) y la detección ([`datos/vuelos.csv`](datos/vuelos.csv), `t_lanzamiento_min_utc` y `t_lanzamiento_max_utc`).
8. **Geometría.** La distancia oblicua y la elevación se calculan en Tierra esférica, sin refracción y con la altitud de la antena de cada gateway: en el campus, el suelo (1890 m) más el mástil, 1902 m para ITPA-IOT-GW00 (12 m de mástil) y 1912 m para ITPA-IOT-GW01-kibts (22 m); en el laboratorio, la que registra el servidor ([`datos/gateways.csv`](datos/gateways.csv)). **Privacidad del laboratorio:** sus distancias y elevaciones se calculan desde un punto de referencia público desplazado 1-2 km de la antena y se redondean a 1 km y 1°. Antes de publicar se comprobó que un ajuste por mínimos cuadrados sobre esas columnas (distancias horizontal y oblicua y elevación, solas o combinadas, con la altura libre o fija) no ubica la antena con un error menor de 1.1 km. Con la posición exacta, aunque se redondearan las columnas, el mismo ajuste la ubicaba a unos metros. Las posiciones de Orbitium entre fixes se **interpolan** en el tiempo (pos_origen = `interpolada`). Las que caen entre los dos fixes que rodean el corte (23:20:36 y 23:30:36), es decir, entre la subida a 8478 m y la caída libre, se marcan como no utilizables (pos_origen = `interpolada_caida`, `usar_geometria` = 0: 38 filas) y van sin geometría.
9. **Entrega.** Es el número de uplinks recibidos entre los fCnt esperados de la sesión de vuelo: 1031-1128 en Orbitium y 144-230 en TAES.
10. **Márgenes.** El margen de SNR es el SNR menos el límite del SF: −7.5 dB en SF7, −15 dB en SF10 y −20 dB en SF12. La señal se estima como RSSI + mín(SNR, 0). El presupuesto de enlace previo al vuelo suponía 22 dBm de TX, 5 dBi por extremo, 1 dB de cable por extremo y 3 dB de margen por patrón, así que se calcula como P_rx = 27 dBm − FSPL a 915 MHz. En picaro-lab-ags-001 se usa la ganancia real de su antena, un dipolo RAIGEN de 5.8 dBi ([`datos/gateways.csv`](datos/gateways.csv)): P_rx = 27.8 dBm − FSPL. Eso sube en 0.8 dB su margen previsto y su pérdida en exceso estimada (`PRESUP_DBM` en [`scripts/a1_rf.py`](scripts/a1_rf.py)). El tiempo en el aire usa la fórmula de Semtech AN1200.13.
11. **Atmósfera.** En vuelo no se midió nada en el exterior: los sensores de temperatura, humedad y presión de las dos sondas iban dentro de la cápsula (columnas `temp_capsula_c`, `pres_capsula_hpa` y `hum_capsula_pct`). La temperatura del aire solo se estima de forma indirecta: la temperatura virtual de cada capa sale de la ecuación hipsométrica, con la altitud GPS y la presión de la cápsula, que no era estanca y por eso sigue a la del aire. Se compara con la atmósfera estándar ISA, y la humedad, con la fórmula de Magnus. El descenso se modela con una velocidad terminal escalada con la densidad.
12. **Comandos.** Cada downlink se empareja con su `txack` por el identificador de la cola. Se considera respondido cuando la sonda contesta con el mismo seq en 90 s o menos. Para los objetivos T+5, T+15 y T+30 se da el Marco respondido más cercano y su desfase, con el rango que deja la incertidumbre del T0. **No se evalúa** si un comando cumplió una ventana: ni un T0 de referencia ni una tolerancia quedaron registrados.

## 3. Notas de edición (cifras conciliadas)

| Tema | Valores en conflicto | Valor que se usa y por qué |
|---|---|---|
| Recepción en el laboratorio | 59.6 % / 23.2 % contra 57.1 % / 21.8 % | 57.1 % y 21.8 %, sobre los fCnt esperados (56/98 y 19/87). Las otras cifras se calcularon sobre los uplinks recibidos |
| Déficit de margen frente al presupuesto | "25-35 dB" | 26-40 dB: margen previsto menos margen de SNR medido, por mediana ([`datos/derivados/a1_resumen.json`](datos/derivados/a1_resumen.json)) |
| Despegue de Orbitium | 22:36:02 (extrapolado de fixes cada 10 min) contra 22:37:30 (fase de vuelo del firmware) | 22:37:04, acotado entre 22:37:02 y 22:37:30 (punto 7). Cambian, respecto a la primera versión, el ascenso medio (2.43 → 2.49 m/s), el primer tramo (3.19 → 4.1 m/s), el retraso del primer despegue (36 → 37 min), la duración (58.8 → 57.8 min) y los uplinks en vuelo (100 → 98 esperados) |
| Altitud de los dos Marcos con −20 dB | "5-7 km" | Unos 5.8 y 7.7 km, interpolados ([`datos/recepciones.csv`](datos/recepciones.csv), fCnt 1065 y 1093) |
| SNR de GW01-kibts frente a GW00 | "3-6 dB" y "6-7 dB" | 6-7 dB de mediana en vuelo ([`datos/derivados/a1_por_gateway.csv`](datos/derivados/a1_por_gateway.csv)) |
| Momento del corte | 2623 s contra 2625 s | 2625 s, el `t_vuelo` del firmware. La cifra de 2623 s se contaba desde el encolado |
| Antena del laboratorio | 5 dBi supuestos contra 5.8 dBi reales (dipolo RAIGEN) | 5.8 dBi. El margen previsto y la pérdida en exceso del laboratorio suben 0.8 dB: pérdida en exceso mediana de 35.8 → 36.6 dB (Orbitium) y de 30.4 → 31.2 dB (TAES); el margen previsto mediano pasa de 29-49 a 29.5-49 dB. El déficit de 26-40 dB no cambia |
| Altitud de las antenas del campus | 1901 m en ITPA-IOT-GW00 (terreno, sin mástil) y 1932 m en ITPA-IOT-GW01-kibts, registradas en el servidor, contra 1902 m y 1912 m | 1902 m y 1912 m: suelo a 1890 m más 12 y 22 m de mástil. Cambian la pérdida en exceso con la sonda en el suelo (38-49 → 38-50 dB, a 100-155 m en lugar de 100-160 m) y la elevación máxima (52.7° → 53.3°; 52.6° en los uplinks pareados). La entrega, los márgenes de SNR, las distancias máximas y el resto de las cifras del reporte no cambian |

## 4. Limitaciones

- **Dos vuelos con telemetría, cortos y bajos**: 43 y 58 min, a 5.1 y 8.5 km. La tercera sonda que voló, de la Universidad Tecnológica del Norte de Aguascalientes (UTNA), no transmitió por la red y no está en los datos. Las conclusiones sobre el patrón de las antenas son tentativas.
- **No se midió la potencia de TX ni la ganancia de antena** de las sondas. Por eso la pérdida en exceso mezcla el efecto del canal con el del equipo.
- **Las distancias y elevaciones del laboratorio** llevan un error de hasta ~2 km y ~1-2° por el tratamiento de privacidad (punto 8).
- **El SNR de −20 dB** que reportó dos veces Orbitium para la bajada puede ser un piso de lo que reporta la radio, y no una medida en el límite.
- **El RSSI no se puede comparar en términos absolutos** entre concentradores distintos. Las conclusiones usan el SNR y diferencias pareadas.
- **Sin medición exterior.** Todos los sensores iban dentro de la cápsula; la temperatura del aire es una estimación hipsométrica.
- **Orbitium tiene pocos fixes en vuelo**: 6 con posición y 2 solo con altitud. Sus velocidades y vientos son promedios de unos 10 min.
- **En 38 recepciones faltaba el SNR**, y se tomó como 0 dB.
- **Las métricas por hora** de los gateways no cubren el evento, y la API no da el tiempo de actividad.
- **El evento de join no dice qué gateway lo recibió, y el de downlink no trae hora** (resolución de 1 s). El registro tampoco guarda la hora del gateway.
- **Inferencias que siguen abiertas**: el formato del payload de TAES, el significado de los campos `tmr_*` del fPort 32, la hora exacta del corte por tiempo de TAES (su telemetría no trae campo de liberación, así que solo se ve en el perfil de velocidad, con resolución de 30 s), la altitud a la que se perdió la señal de Orbitium y su zona de aterrizaje, y el bloqueo del trayecto al laboratorio por el terreno.
- **El punto de recuperación** solo se conoce para Orbitium. Es el lugar del hallazgo que reportaron los habitantes de El Rascón, Zacatecas ([`datos/recuperacion.csv`](datos/recuperacion.csv)), no un registro de la red. La hora de aterrizaje no se midió.
- **Los datos declarados por los equipos** (hardware, masa y pruebas de Orbitium; electrónica y corte por tiempo de TAES) se usaron solo para contrastar e interpretar. No se verificaron por separado.

## 5. Cómo reproducir

Los datos y los scripts están en el repositorio público: <https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/docs/reportes/satlink-2026> (subcarpetas [`datos/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/docs/reportes/satlink-2026/datos) y [`scripts/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/docs/reportes/satlink-2026/scripts)).

Requisitos: Python 3.10 o posterior, con `matplotlib`. `a1_rf.py` necesita además `numpy`. Las cifras se verificaron con Python 3.13, numpy 2.4 y matplotlib 3.10.

```
git clone https://github.com/ovelazquezj/radiosonda_PIcaro.git
cd radiosonda_PIcaro/docs/reportes/satlink-2026/scripts
python -I a1_rf.py          # enlace y gateways
python -I a2_vuelo.py       # perfil de vuelo y atmósfera
python -I a3_operaciones.py # operaciones y comandos
```

Cada script lee solo `../datos/*.csv` y regenera `../figuras/a1_*`, `a2_*` o `a3_*` (PNG a 150 dpi) y `../datos/derivados/a1_*`, `a2_*` o `a3_*`. Las cifras salen por la consola y, en el caso de A1, también en [`datos/derivados/a1_resumen.json`](datos/derivados/a1_resumen.json). `-I` aísla al intérprete del directorio de trabajo.

## 6. Licencia sugerida

- **Texto, figuras y datos** (`*.md`, `figuras/`, `datos/`): **Creative Commons Atribución 4.0 Internacional (CC BY 4.0)**, la misma que el material educativo del repositorio. Ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
- **Scripts** (`scripts/`): The Clear BSD License, como el resto del código propio del repositorio, porque CC no recomienda sus licencias para software.
- **Atribución sugerida:** «Misiones Espaciales - SatLink 2026: datos y reporte de vuelo», proyecto PiCARO / radiosonda_PIcaro, Instituto Tecnológico de Pabellón de Arteaga (ITPA), CC BY 4.0. Para los datos de cada sonda, se cita también al equipo correspondiente.
