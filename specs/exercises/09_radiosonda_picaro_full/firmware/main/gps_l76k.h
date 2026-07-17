/**
 * ============================================================================
 *  gps_l76k.h  -  GNSS Quectel L76K por UART (NMEA) del T-Beam Supreme
 * ============================================================================
 *  UART: MCU RX=9 <- GPS TX ; MCU TX=8 -> GPS RX ; EN=7 ; 9600 bps.
 *  Una tarea de fondo lee NMEA y mantiene el ultimo fix. La telemetria llama
 *  gps_get() para copiar el estado actual (sin bloquear).
 * ============================================================================
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool     valid;      /* true si hay fix 2D/3D (RMC 'A' y GGA quality>0) */
    double   lat;        /* grados decimales (+N, -S) */
    double   lon;        /* grados decimales (+E, -W) */
    float    alt_m;      /* altitud MSL en metros */
    uint8_t  sats;       /* satelites en uso */
    float    hdop;       /* dilucion horizontal */
    float    speed_kmh;  /* velocidad sobre el suelo */
} gps_fix_t;

/* Enciende el modulo y arranca la tarea de lectura NMEA. */
esp_err_t gps_init(void);

/* Copia el ultimo fix conocido. Devuelve true si es valido. */
bool gps_get(gps_fix_t *out);

/* true si el GPS esta ACTIVO = llegaron sentencias NMEA hace poco (<5 s).
 * Si alguien apaga el GPS (p. ej. por boton) o el modulo no responde, pasa a
 * false aunque el fix previo siguiera "valido". */
bool gps_active(void);

#ifdef __cplusplus
}
#endif
