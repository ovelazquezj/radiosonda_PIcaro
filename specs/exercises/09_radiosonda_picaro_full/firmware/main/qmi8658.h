/**
 * ============================================================================
 *  qmi8658.h  -  IMU 6 ejes QMI8658 (SPI3, CS 34)  [CODIFICADO, apagado]
 * ============================================================================
 *  Acelerometro + giroscopio + temperatura. Apagado por defecto: activa
 *  USE_QMI8658 (y TELEMETRY_IMU para meter accel en el payload).
 * ============================================================================
 */
#pragma once
#include <stdbool.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { float ax, ay, az;   /* g   */
                 float gx, gy, gz;    /* dps */
                 float temp_c; } imu_sample_t;
esp_err_t qmi8658_init(void);
bool qmi8658_present(void);
uint8_t qmi8658_whoami(void);   /* ultimo WHO_AM_I leido (0x05 = OK) */
esp_err_t qmi8658_read(imu_sample_t *s);
#ifdef __cplusplus
}
#endif
