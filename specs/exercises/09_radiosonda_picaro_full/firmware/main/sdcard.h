/**
 * ============================================================================
 *  sdcard.h  -  microSD (SPI3, CS 47) con FAT para el log de telemetria
 * ============================================================================
 *  Guarda la telemetria COMPLETA (todos los sensores) en /sdcard/PICARO.CSV,
 *  una linea por ciclo. Si la tarjeta viene virgen, se formatea a FAT.
 * ============================================================================
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Monta la microSD (formatea si hace falta). Requiere BLDO1 encendido (PMU). */
esp_err_t sdcard_init(void);
bool sdcard_present(void);

/* true si el CSV no existia al montar (el llamador debe escribir la cabecera). */
bool sdcard_log_is_new(void);

/* Agrega una linea (se le anade el salto de linea) al CSV. */
esp_err_t sdcard_log_line(const char *line);

#ifdef __cplusplus
}
#endif
