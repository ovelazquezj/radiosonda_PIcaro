/**
 * ============================================================================
 *  bme280.h  -  Sensor ambiental Bosch BME280 (I2C0 @0x77)
 * ============================================================================
 *  Da temperatura, presion y humedad. En la telemetria por defecto se usan
 *  temperatura y presion; la humedad esta codificada pero apagada (ver
 *  sensores_config.h -> TELEMETRY_HUMIDITY).
 * ============================================================================
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el BME280 (requiere i2c0_init() antes). */
esp_err_t bme280_init(void);
bool bme280_present(void);

/* Lee una medida (modo forzado). Cualquier puntero puede ser NULL.
 *   temp_c   -> grados Celsius
 *   press_hpa-> hectopascales (hPa)
 *   hum_pct  -> humedad relativa %
 * Devuelve ESP_OK si la lectura fue valida. */
esp_err_t bme280_read(float *temp_c, float *press_hpa, float *hum_pct);

#ifdef __cplusplus
}
#endif
