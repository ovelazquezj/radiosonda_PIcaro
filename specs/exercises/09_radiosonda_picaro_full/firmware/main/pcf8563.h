/**
 * ============================================================================
 *  pcf8563.h  -  RTC PCF8563 (I2C1 @0x51)  [CODIFICADO, apagado]
 * ============================================================================
 *  Reloj de tiempo real: da la marca de tiempo para el CSV. Se puede
 *  sincronizar con la hora del GPS. Activa USE_PCF8563 para usarlo.
 * ============================================================================
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { uint16_t year; uint8_t mon, day, hour, min, sec; } rtc_time_t;
esp_err_t pcf8563_init(void);
bool pcf8563_present(void);
esp_err_t pcf8563_get(rtc_time_t *t);
esp_err_t pcf8563_set(const rtc_time_t *t);
#ifdef __cplusplus
}
#endif
