/**
 * ============================================================================
 *  oled_sh1106.h  -  Pantalla OLED SH1106 128x64 (I2C0 @0x3C)
 * ============================================================================
 *  Muestra el estado (join, uplinks, GPS, bateria). Fuente 5x7 -> 21 chars x
 *  8 lineas. Requiere i2c0_init() y el riel de sensores encendido (PMU).
 * ============================================================================
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t oled_init(void);
bool oled_present(void);
void oled_clear(void);
/* Escribe texto en la linea (0..7). Se recorta a 21 caracteres. */
void oled_line(uint8_t line, const char *text);
#ifdef __cplusplus
}
#endif
