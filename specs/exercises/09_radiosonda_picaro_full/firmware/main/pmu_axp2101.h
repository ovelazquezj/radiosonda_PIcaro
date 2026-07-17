/**
 * ============================================================================
 *  pmu_axp2101.h  -  Gestor de energia AXP2101 (I2C bus1) del T-Beam Supreme
 * ============================================================================
 *
 *  El AXP2101 es CRITICO y va PRIMERO: sus rieles LDO alimentan al radio, al
 *  GPS, a los sensores y a la microSD. Si no se enciende ALDO3, el SX1262 no
 *  tiene energia y `radio.begin()` falla. Mapeo de rieles del Supreme (3.3V):
 *     ALDO3 -> LoRa (SX1262)     ALDO4 -> GPS (L76K)
 *     ALDO1 / ALDO2 -> sensores / OLED     BLDO1 -> microSD
 *  (tomado del LoRaBoards.cpp oficial de LILYGO, bloque T_BEAM_S3_SUPREME).
 * ============================================================================
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el bus I2C1, detecta el AXP2101 y enciende los rieles. */
esp_err_t pmu_init(void);

/* Handle del bus I2C1 (PMU + RTC) para que otros drivers (PCF8563) lo reusen. */
i2c_master_bus_handle_t pmu_i2c_bus1(void);

/* true si el AXP2101 respondio en el bus. */
bool pmu_present(void);

/* Bateria: milivolts (0 si no hay) y porcentaje 0..100 (0xFF = desconocido). */
uint16_t pmu_battery_mv(void);
uint8_t  pmu_battery_percent(void);

/* Estado de alimentacion. */
bool pmu_usb_present(void);
bool pmu_charging(void);

/* Enciende el LED de carga del AXP2101 en modo parpadeo 1 Hz (senal de "vivo"). */
void pmu_charge_led_blink(void);

#ifdef __cplusplus
}
#endif
