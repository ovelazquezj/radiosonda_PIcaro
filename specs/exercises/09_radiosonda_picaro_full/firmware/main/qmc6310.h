/**
 * ============================================================================
 *  qmc6310.h  -  Magnetometro QMC6310 (I2C0 @0x1C)  [CODIFICADO, apagado]
 * ============================================================================
 *  Da el rumbo (heading) a partir del campo magnetico. Apagado por defecto:
 *  activa USE_QMC6310 y TELEMETRY_HEADING en sensores_config.h para usarlo.
 *  Nota: para un rumbo fiable hay que calibrar los offsets de hard/soft-iron.
 * ============================================================================
 */
#pragma once
#include <stdbool.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t qmc6310_init(void);
bool qmc6310_present(void);
/* Lee el campo (uT aprox) y calcula el rumbo en grados [0..360). */
esp_err_t qmc6310_read(float *heading_deg, float *x, float *y, float *z);
#ifdef __cplusplus
}
#endif
