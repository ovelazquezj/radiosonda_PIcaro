# Diccionario de datos · SatLink 2026

Dataset canónico del reporte "Misiones Espaciales - SatLink · Edición 2026" (Instituto Tecnológico de Pabellón de Arteaga (ITPA), 28 de octubre de 2026).
Lo usan los análisis de RF, de vuelo y de operaciones.

- **Origen.** Grabador MQTT del servidor LoRaWAN del proyecto (ChirpStack 4.19, US915 sub-banda 2), eventos `up`, `join`, `down`, `txack`, `log` y `status` de los días UTC 27, 28 y 29 de octubre de 2026. Las métricas de los gateways salen de la API de ChirpStack. El historial del mapa público se revisó y no trae uplinks que falten en el grabador.
- **Anonimización.** No hay DevEUI, EUI de gateway, JoinEUI, DevAddr, claves, UUID ni IP. Las sondas aparecen por su alias público y los gateways por su nombre. Los extractores que leen los crudos no se publican.
- **Sondas.** Esa noche volaron tres sondas. Estos archivos tienen las dos que transmitieron telemetría por la red LoRaWAN: la del ITPA y la del Instituto Tepeyac con la Universidad Politécnica de Aguascalientes (UPA); la de la Universidad Tecnológica del Norte de Aguascalientes (UTNA) no transmitió y no aparece. Otra sonda preparada en el ITPA no voló y tampoco está aquí.
  - `Sonda Orbitium (Sci-Fi), ITPA` (Equipo Orbitium (Sci-Fi), ITPA): Heltec WiFi LoRa 32 V3, Clase C, SF10/125 kHz (DR0). Hace una lectura cada 10 min, repartida en los fPorts 20-23. Usa además el fPort 30 (Marco Polo), el 31 (corte) y el 32 (temporizador, cada ~60 s).
  - `Sonda TAES (Instituto Tepeyac/UPA)` (equipo TAES, colaboración entre el Instituto Tepeyac y la UPA): LilyGO T-Beam Supreme, SF7/125 kHz (DR3), fPort 1, un uplink cada 15-30 s, Clase A, sin downlinks. Se liberó por un corte por tiempo de su propio sistema; su paquete no trae campo de liberación.
- **Tiempo.** `t_utc` va en ISO 8601 con milisegundos y sufijo `Z`. `t_local` es la hora del centro de México, UTC-6 sin horario de verano, con sufijo `-06:00`. El lanzamiento nocturno empezó a las 22:00 locales del 28-oct, que son las 04:00 UTC del 29-oct.
- **Valores vacíos.** Una celda vacía significa "no disponible": el sensor mandó su valor sentinela, el campo no aplica a ese fPort o no hubo fix. Nunca se rellena con 0.
- **Booleanos.** `1` = verdadero, `0` = falso.

## uplinks.csv

Una fila por uplink deduplicado por el LNS: 2758 filas, 1569 de Orbitium y 1189 de TAES.

| columna | unidad | origen / cálculo |
|---|---|---|
| sonda | — | alias público de la sonda |
| t_utc, t_local | ISO 8601 | campo `time` del evento `up` (hora de recepción en el LNS) |
| sesion | entero | número de joins anteriores dentro del dataset. El `fcnt` se reinicia en cada join, así que un uplink se identifica con (sonda, sesion, fcnt). 0 = antes del primer join del 27-oct |
| fcnt | entero | contador de trama de subida |
| fport | entero | puerto LoRaWAN |
| sf | — | spreading factor (`txInfo.modulation.lora`) |
| bw_khz | kHz | ancho de banda |
| frec_mhz | MHz | frecuencia de subida (`txInfo.frequency`) |
| dr | — | data rate US915 que asignó el LNS |
| canal_us915 | 0-71 | canal US915 calculado a partir de la frecuencia: (f − 902.3 MHz)/0.2 MHz para 125 kHz. La sub-banda 2 corresponde a los canales 8-15 y 65 |
| n_gateways | entero | gateways que recibieron el uplink (largo de `rxInfo`) |
| bytes_payload | bytes | largo del FRMPayload |
| rssi_max_dbm, snr_max_db | dBm, dB | mejor RSSI y mejor SNR entre los gateways |
| tipo_paquete | — | Orbitium: `posicion`(20), `atmosfera`(21), `ambiente`(22), `interno`(23), `polo`(30), `corte`(31), `temporizador`(32). TAES: `telemetria` |
| decodificacion | — | `codec_lns` = objeto decodificado por el codec del LNS (Orbitium). `reinterpretado_26B_v1` / `reinterpretado_27B_v2` = TAES decodificada de nuevo desde los bytes con el formato **inferido** (ver más abajo) |
| lat, lon | grados WGS84 | Orbitium: fPort 20. TAES: bytes 1-8 (27 B) o 0-7 (26 B). Vacío si no hay fix |
| alt_m | m (GPS, sobre el nivel del mar) | Orbitium: fPort 20 (GPS) y fPort 31 (GPS o barométrica, según `alt_fuente`). TAES: GPS. En TAES se conserva aunque no haya fix horizontal; en ese caso `calidad_gps`=`sin_fix` |
| alt_fuente | — | `gps` o `barometrica` (fPort 31, bit 0 de las banderas) |
| gps_fix | 0/1 | Orbitium: bandera del codec. TAES: lat y lon distintas de 0 |
| satelites | entero | satélites en uso |
| temp_capsula_c, pres_capsula_hpa, hum_capsula_pct | °C, hPa, % | Sensor del paquete "atmósfera", montado **dentro de la cápsula**: en vuelo no se midió nada en el exterior. Orbitium: BME280 del fPort 21. TAES: campos inferidos (temperatura x100, HR x100, presión x10). La temperatura y la humedad son las del interior. La presión sigue a la del aire porque la cápsula no era estanca. La temperatura del aire solo se estima de forma indirecta: es la temperatura virtual por hipsometría, a partir de esta presión y de la altitud GPS |
| alt_baro_isa_m | m | **cálculo**: altitud de la atmósfera estándar ISA a partir de `pres_capsula_hpa`, h = 44330.8·(1 − (p/1013.25)^0.190263). Sirve para validar la altitud GPS. No está corregida por temperatura: en una noche cálida queda por debajo de la altitud real |
| temp_int_c, pres_int_hpa, hum_int_pct | °C, hPa, % | Orbitium: BME280 de placa (fPort 23). Vino siempre vacío (sensor ausente) |
| temp_aht_c, hum_aht_pct | °C, % | Orbitium: AHT2x (fPort 22) |
| temp_dht_c, hum_dht_pct | °C, % | Orbitium: DHT11 (fPort 22) |
| luz_lux, luz_saturada | lx, 0/1 | Orbitium: BH1750 (fPort 22) |
| tvoc_ppb, eco2_ppm, aqi | ppb, ppm, índice | Orbitium: ENS160 (fPort 21) |
| bateria_pct | % | Orbitium (fPort 21) |
| bateria_v | V | Orbitium: fPort 22. TAES: campo inferido en mV/1000 (vacío cuando el byte vale 0) |
| ina_bus_v, ina_corriente_ma | V, mA | Orbitium: INA219 (fPort 23) |
| uptime_s | s | Orbitium: tiempo encendida (fPort 22, resolución de 4 s) |
| usb, cargando, gps_activo, en_vuelo, corte_en_curso, cortado | 0/1 | Orbitium: banderas del fPort 23. `en_vuelo` también sale en el fPort 31 |
| lectura | entero | Orbitium: contador de lecturas (fPort 23, u8) |
| polo_seq, polo_rssi_dbm, polo_snr_db | —, dBm, dB | Orbitium, fPort 30: respuesta "Polo" a un Marco. RSSI y SNR son los del **downlink** medidos en la sonda |
| corte_evento, corte_motivo, corte_seq, corte_intento | — | Orbitium, fPort 31. Evento: `inicio`, `confirmado`, `sin_confirmar` o `prueba_finalizada`. Motivo: `altura`, `tiempo`, `reventon`, `downlink`, `consola` o `prueba` |
| vel_vertical_mps | m/s | Orbitium, fPort 31: velocidad vertical que estimó la sonda (positiva = sube) |
| tmr_* | — | Orbitium, fPort 32, temporizador de liberación. Los campos se **dedujeron del objeto decodificado por el LNS**; no hay documentación publicada de este fPort. Son `tmr_fase` (`desarmado`, `armado`, `vuelo`, `cortando`, `terminado`), `tmr_motivo`, `tmr_armado`, `tmr_activo`, `tmr_t_vuelo_s` (s desde que empieza la fase de vuelo), `tmr_restante_s` y `tmr_limite_s` (s), `tmr_orden_aceptada` y `tmr_orden_rechazada` (respuesta a la orden ARM), `tmr_seq`, `tmr_suelo_gps_listo` y `tmr_suelo_baro_listo` (referencia de suelo lista), `tmr_reloj_incierto`, `tmr_fallo_almac`. En el fPort 31, `tmr_t_vuelo_s` es el `t_vuelo_s` del evento de corte |
| upa_tipo | entero | TAES, formato 27 B: primer byte (vale 0 en todos los datos). Las columnas `upa_*` conservan su nombre original y se refieren a la Sonda TAES |
| upa_banderas | entero (byte) | TAES: byte de banderas sin interpretar. El bit 0 coincide con el fix GPS |
| upa_formato | — | `26B_v1` o `27B_v2` (inferido por el largo) |
| lns_latitud, lns_longitud, lns_altitud_m | grados, m | TAES: valores tal como los decodificó su codec en el LNS. Coinciden en los 342 paquetes de 27 B y **no son válidos** en los 847 de 26 B. Se conservan para trazabilidad |
| calidad_gps | — | ver "Filtros de calidad". Vale `no_aplica` en los paquetes sin posición y `no_aplica_baro` en las altitudes barométricas del fPort 31 |
| calidad_hum | — | `pegada_100` si la HR de la cápsula (`hum_capsula_pct`) vale ≥ 100 % (en estas sondas indica un BME280 dañado), `ok` en otro caso, `no_aplica` si no hay HR |
| en_ventana_vuelo | 0/1 | 1 si `t_utc` está entre `t_lanzamiento_est_utc` y `t_ultima_recepcion_utc` de [`vuelos.csv`](vuelos.csv) |

### Formato inferido de la Sonda TAES

**Es una inferencia.** El codec de este equipo no estaba disponible. El formato se dedujo de los bytes crudos: las coordenadas caen sobre el campus, la altitud coincide con la que entrega su codec, la presión baja de forma coherente con la altitud y la batería queda en 4.0-4.1 V. Todos los campos van en little-endian.

| bytes (27 B, v2) | bytes (26 B, v1) | campo | escala |
|---|---|---|---|
| 0 | — | tipo (u8) | — |
| 1-4 / 5-8 | 0-3 / 4-7 | lat / lon (i32) | 27 B: ×1e-7°. 26 B: ×1e-6° |
| 9-10 | 8-9 | altitud (u16) | m |
| 11-12 | 10-11 | temperatura (i16) | ×0.01 °C |
| 13-14 | 12-13 | humedad relativa (i16) | ×0.01 % |
| 15-16 | 14-15 | presión (i16) | ×0.1 hPa |
| 17-18 | 16-17 | batería (i16) | mV (0 = no disponible) |
| 19 | 18 | satélites (u8) | — |
| 20 | 19 | banderas (u8) | bit 0 = fix |
| 21-26 | 20-25 | relleno 0xFF | — |

El formato de 26 B aparece del 28-oct 18:45 al 28-oct 20:46 hora local. Desde las 20:51 locales todos los paquetes son de 27 B, incluido todo el vuelo. Su codec lee los de 26 B como si fueran de 27 B, y por eso latitud, longitud y altitud salen mal en esos paquetes.

## recepciones.csv

Una fila por pareja (uplink, gateway que lo recibió): 5420 filas.

| columna | unidad | origen / cálculo |
|---|---|---|
| sonda, t_utc, t_local, sesion, fcnt, fport, sf | — | las del uplink (sirven para unirlo con [`uplinks.csv`](uplinks.csv)) |
| gateway | — | nombre del gateway ([`gateways.csv`](gateways.csv)) |
| rssi_dbm, snr_db | dBm, dB | `rxInfo[].rssi` / `rxInfo[].snr` |
| snr_omitido | 0/1 | 1 si el evento no traía `snr`. El JSON de ChirpStack omite los valores por defecto, así que se tomó **0.0 dB**. Pasa en 38 filas |
| frec_mhz, canal_us915 | MHz, — | las del uplink |
| canal_gw, rf_chain | entero | canal y cadena de RF internos del concentrador (`rxInfo[].channel`, `rfChain`). Si faltan, se toma 0 (la misma omisión de valores por defecto) |
| crc | — | `CRC_OK` |
| n_gateways | entero | gateways que recibieron ese uplink |
| pos_origen | — | `propia` = fix "ok" del mismo uplink. `interpolada` = interpolación lineal en el tiempo entre los dos fixes "ok" que lo rodean, separados ≤ 11 min (solo aplica a Orbitium, que manda posición cada 10 min). `interpolada_caida` = la interpolación cruzaría el corte de Orbitium (fixes de las 23:20:36 y 23:30:36 locales, a ambos lados del inicio del corte a las 23:21:16): **no es utilizable** y su geometría va vacía. Vacío = sin posición. **La interpolada es una estimación** |
| usar_geometria | 0/1 | 1 = la posición y la geometría de la fila se pueden usar. 0 = no utilizable (`interpolada_caida`: 38 filas de Orbitium entre las 23:20:42 y las 23:29:48 locales; en ese tramo la sonda subió hasta 8478 m y cayó en caída libre, y una recta entre los dos fixes daría altitudes y distancias falsas, por ejemplo más de 53 km al laboratorio). Vacío = sin posición |
| lat_sonda, lon_sonda, alt_sonda_m | grados, m | posición usada para la geometría |
| dist_horiz_m | m | **cálculo**: distancia sobre la esfera (R = 6371 km) entre el gateway y la sonda. Laboratorio: ver la nota de privacidad |
| dist_slant_m | m | **cálculo**: distancia en línea recta entre la antena y la sonda, con las alturas del gateway ([`gateways.csv`](gateways.csv)) y de la sonda |
| elev_deg | grados | **cálculo**: ángulo de elevación de la sonda visto desde el gateway, geométrico, en Tierra esférica y sin refracción. Laboratorio: entero (redondeado a 1°) |
| en_ventana_vuelo | 0/1 | el del uplink |

**Nota de privacidad del laboratorio.** En las filas de `picaro-lab-ags-001`, `dist_horiz_m`, `dist_slant_m` y `elev_deg` **no** se calculan con la posición exacta de la antena, sino con el punto de referencia público de [`gateways.csv`](gateways.csv) (desplazado 1-2 km de la antena a propósito), y se redondean a 1 km y a 1°. Con la posición exacta, aunque se redondearan, un ajuste por mínimos cuadrados sobre las posiciones de la sonda permitía reconstruir la ubicación del laboratorio a pocos metros. Consecuencia para el análisis: las distancias al laboratorio tienen un error de hasta ~2 km (≤ 0.6 dB en la pérdida en espacio libre a 29-51 km) y sus elevaciones, de ~1-2°. Las filas de los gateways del campus llevan la geometría completa.

## downlinks.csv

Un evento `down` (orden encolada) por fila, emparejado con su `txack` mediante el identificador de la cola, y con la respuesta de la sonda: 50 filas, todas de Orbitium.

| columna | unidad | origen / cálculo |
|---|---|---|
| dl_n | entero | número correlativo dentro del dataset (sustituye a los identificadores internos) |
| sonda | — | alias |
| tipo | — | `marco` (fPort 30, 1 byte = seq), `corte` (fPort 31, "CORT"+seq), `prueba_corte` (fPort 31, "TEST"+seq: prueba de la salida de corte), `armar_temporizador` (fPort 32, "ARM!"+seq+límite) |
| fport, seq | — | puerto y número de secuencia que lleva la orden |
| parametro | s | en `armar_temporizador`: el límite del temporizador (u16 LE) |
| payload_ascii, payload_bytes | —, bytes | carga útil de la orden (los bytes no imprimibles salen como `.`) |
| confirmado | 0/1 | downlink confirmado (siempre 0) |
| t_encolado_utc, t_encolado_local | ISO 8601 | hora en que el grabador vio la orden. **Resolución de 1 s**: el evento no trae `time` |
| t_txack_utc | ISO 8601 | `time` del `txack`, cuando el gateway confirmó la transmisión |
| gateway_tx | — | gateway que transmitió |
| frec_tx_mhz, sf_tx, bw_tx_khz, potencia_tx_dbm | MHz, —, kHz, dBm | parámetros de TX del `txack` (RX de Clase C: 923.3 MHz, SF12, 500 kHz) |
| fcnt_down | entero | contador de bajada |
| resultado_txack | — | `ok` si hubo `txack` y `sin_txack` si no. Solo dice que el gateway transmitió, no que la sonda recibiera |
| respuesta | si/no | la sonda contestó por uplink en ≤ 90 s después del `txack`. Marco → Polo (fPort 30) con el mismo seq. Corte o prueba → fPort 31 `inicio` con el mismo seq. ARM → fPort 32 con `orden_aceptada` y el mismo seq |
| t_respuesta_utc, latencia_resp_s, resp_fcnt | ISO, s, — | uplink que respondió y su retardo contado desde el `txack` |
| polo_rssi_dbm, polo_snr_db | dBm, dB | RSSI y SNR del Marco medidos en la sonda (eso reporta el Polo) |
| t_desde_lanz_s | s | **cálculo**: `t_encolado` − `t_lanzamiento_est_utc` de la sonda. Solo se llena cuando la orden cae hasta 3 h antes del lanzamiento o después; un valor negativo indica que fue antes del lanzamiento |

## joins.csv

Joins que el LNS aceptó: 47 filas (35 de Orbitium y 12 de TAES). El DevNonce no se incluye.

| columna | unidad | origen |
|---|---|---|
| sonda, t_utc, t_local | — | evento `join` |
| sesion | entero | número de sesión que abre ese join |
| gateway | — | **siempre vacío**: el evento `join` de ChirpStack v4 no trae `rxInfo` y no se puede saber qué gateway lo recibió |
| clase | — | clase del dispositivo (A/C) |

## eventos_red.csv

Eventos `log` (errores y avisos del LNS) y `status` (DevStatusAns): 66 filas.

| columna | unidad | origen |
|---|---|---|
| sonda, t_utc, t_local | — | evento |
| tipo | — | `log` o `status` |
| nivel, codigo, descripcion | — | del `log`, por ejemplo `UPLINK_MIC` (MIC del join-request inválido), `OTAA` (DevNonce ya usado), `UPLINK_F_CNT_RETRANSMISSION` o `UPLINK_F_CNT_RESET` |
| fcnt | entero | `context.f_cnt_up`, si viene |
| margen_db | dB | `status`: margen del enlace (DevStatusAns) |
| bateria_no_disponible, alim_externa | 0/1 | `status` |

## gateways.csv

Los tres gateways activos.

| columna | unidad | origen |
|---|---|---|
| gateway, modelo, protocolo, sitio | — | inventario del LNS |
| antena | — | tipo y ganancia de la antena. ITPA-IOT-GW00 (dipolo, ~2.5 dBi) e ITPA-IOT-GW01-kibts (colineal, 6 dBi): valores nominales, no medidos. picaro-lab-ags-001: dipolo RAIGEN de 5.8 dBi, la antena instalada; `a1_rf.py` usa esta ganancia en el presupuesto de enlace del laboratorio |
| lat, lon | grados | tabla de gateways del LNS. **El del laboratorio es un punto de referencia aproximado, desplazado 1-2 km de la antena** (ver la nota de privacidad de [`recepciones.csv`](recepciones.csv)) |
| alt_m | m s. n. m. | altitud de la antena. Gateways del campus: suelo a 1890 m más la altura del mástil (ITPA-IOT-GW00: + 12 m = 1902 m; ITPA-IOT-GW01-kibts: + 22 m = 1912 m). Laboratorio: altitud de la antena registrada en el LNS. Es la altura que usan `dist_slant_m` y `elev_deg` |
| alt_ref, precision_publicada | — | aclaraciones sobre altitud y precisión |

## vuelos.csv

Una fila por sonda que voló.

| columna | unidad | método |
|---|---|---|
| suelo_m, n_fix_suelo | m, — | mediana de la altitud GPS de los fixes "ok" en los 60 min previos al primer fix por encima de 2201 m (umbral fijo, unos 300 m sobre el suelo del campus, a 1890 m) |
| t_ultimo_suelo_utc, alt_ultimo_suelo_m | ISO, m | último fix "ok" antes del despegue |
| t_primer_aire_utc, alt_primer_aire_m | ISO, m | **criterio de despegue**: primer fix "ok" con alt > suelo + 100 m, seguido de dos fixes "ok" que siguen subiendo y también superan suelo + 100 m (ascenso sostenido) |
| vz_inicial_mps | m/s | velocidad vertical media entre los dos primeros fixes en el aire |
| vz_primer_tramo_mps | m/s | velocidad vertical media del despegue (`t_lanzamiento_est`, a la altitud `suelo_m`) al primer fix en el aire. En TAES coincide con `vz_inicial_mps` |
| t_lanzamiento_est_utc, t_lanzamiento_est_local | ISO | **despegue estimado, el único T0 del reporte** (ver `metodo_t0`). **TAES**: t_primer_aire − (alt_primer_aire − suelo)/vz_inicial, nunca antes de t_ultimo_suelo (fixes cada 30 s). **Orbitium**: el firmware declara el despegue cuando la altitud supera su suelo en 100 m durante 3 lecturas seguidas de 1 s; el despegue es esa detección (`t_fase_vuelo_firmware_utc`) menos el tiempo de subir 100 m a `vz_primer_tramo_mps` y menos 2 s de confirmación, resuelto por iteración |
| t_lanzamiento_min_utc, t_lanzamiento_max_utc | ISO | Orbitium: cota del despegue. Mínimo = ARM aceptado (se supone que el armado se hizo en tierra); máximo = detección del firmware. Vacío en TAES |
| metodo_t0 | texto | método del T0 de esa sonda |
| t_armado_firmware_utc, t_fase_vuelo_firmware_utc | ISO | Orbitium: primer fPort 32 con la orden ARM aceptada y primer fPort 32 en fase `vuelo` (la detección de despegue del firmware; arranca su reloj de 55 min) |
| t_alt_max_utc, t_alt_max_local, alt_max_m, fport_alt_max | ISO, m | máximo de altitud GPS válida dentro de la ventana. Incluye la altitud GPS del fPort 31 (`solo_alt`) |
| t_alt_max_con_pos_utc, alt_max_con_pos_m, lat/lon_alt_max_con_pos | ISO, m, ° | máximo entre los fixes con posición completa |
| t_corte_inicio_utc, alt_corte_m, corte_motivo, t_corte_confirmado_utc | ISO, m | Orbitium: eventos de corte del fPort 31 dentro de la ventana |
| t_ultimo_fix_utc, alt_ultimo_fix_m, lat/lon_ultimo_fix | ISO, m, ° | último fix "ok" |
| t_ultima_recepcion_utc, t_ultima_recepcion_local | ISO | último uplink recibido, que marca el **fin de la ventana** |
| duracion_min | min | fin − t_lanzamiento_est |
| n_uplinks_ventana, n_fix_ok_ventana | entero | uplinks y fixes "ok" dentro de la ventana |
| metodo | texto | descripción del método |

## gateway_metricas.csv

Métricas agregadas de los gateways según la API de ChirpStack, del 20-oct al 2-nov (UTC), en formato largo.

| columna | unidad | origen |
|---|---|---|
| gateway | — | nombre |
| agregacion | — | `DAY` (cubeta de 1 día UTC) o `HOUR` |
| t_utc, t_local | ISO | inicio de la cubeta |
| metrica | — | `rx_paquetes`, `tx_paquetes`, `rx_por_frec_mhz`, `tx_por_frec_mhz`, `rx_por_dr`, `tx_por_dr`, `tx_por_estado` |
| etiqueta | — | `total`, la frecuencia en MHz, el DR o el estado de TX (`OK`) |
| valor | paquetes | conteo |

Se omiten las filas con valor 0, salvo los totales `DAY`. **Las cubetas `HOUR` solo llegan a ~48 h antes de la consulta** (31-oct/1-nov a 2-nov), así que **no cubren el evento**. Las `DAY` sí lo cubren. Los gateways cuentan todo lo que reciben, también el tráfico de otras redes y de otros dispositivos, y no solo las sondas.

## Filtros de calidad (`calidad_gps`)

No se borra ninguna fila; cada fix se marca en este orden:

1. `sin_fix`: sin lat/lon o con lat = lon = 0.
2. `pos_imposible`: fuera de [−90, 90]/[−180, 180] o a más de 300 km del campus (por ejemplo, un signo de longitud invertido).
3. `alt_imposible`: altitud vacía, < 1700 m (por debajo del terreno de la región, que está a ~1850-2000 m) o > 40 000 m.
4. `salto`: contra el último fix "ok", velocidad horizontal implícita > 60 m/s o vertical > 40 m/s.
5. `ok`: pasa todos los filtros anteriores.
6. `solo_alt` (fPort 31 de Orbitium con fuente GPS): altitud plausible sin lat/lon.

Las posiciones `ok` de TAES en tierra siguen teniendo un error vertical de decenas de metros (rango 1700-1988 m antes del vuelo).

## recuperacion.csv

Punto donde se recuperó cada cápsula. Solo hay dato de la Sonda Orbitium (Sci-Fi), del Instituto Tecnológico de Pabellón de Arteaga (ITPA).

| Columna | Unidad | Origen |
|---|---|---|
| fecha_recuperacion, lugar | — | reporte de la recuperación |
| lat, lon | grados WGS84, 4 decimales | posición reportada por los habitantes que encontraron la cápsula |
| elev_terreno_srtm_m | m s. n. m. | SRTM de 30 m (consulta a OpenTopoData) |
| distancia_ultimo_fix_m, rumbo_desde_ultimo_fix_deg | m, grados desde el norte | desde el último fix GPS de vuelo ([`vuelos.csv`](vuelos.csv)) |
| distancia_punto_corte_m, rumbo_desde_punto_corte_deg | m, grados | desde el fix de altitud máxima con posición (8390 m) |
| distancia_itpa_iot_gw00_m | m | distancia horizontal al gateway ITPA-IOT-GW00 |
| error_prediccion_a2_m | m | distancia entre la zona de aterrizaje que estimó el análisis A2 (22.39, −102.07) y el punto real |
