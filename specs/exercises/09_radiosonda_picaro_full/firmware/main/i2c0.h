/**
 * ============================================================================
 *  i2c0.h  -  Bus I2C0 compartido (sensores + OLED) del T-Beam Supreme
 * ============================================================================
 *  Bus 0: SDA 17 / SCL 18. Cuelgan: BME280 (0x77), QMC6310 (0x1C), OLED (0x3C).
 *  El bus 1 (PMU/RTC) lo gestiona pmu_axp2101 (ver pmu_i2c_bus1()).
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

/* Crea el bus I2C0 (idempotente). */
esp_err_t i2c0_init(void);

/* Handle del bus I2C0 para que cada driver agregue su dispositivo. */
i2c_master_bus_handle_t i2c0_bus(void);

/* Escanea el bus e imprime las direcciones que responden (diagnostico). */
void i2c0_scan(void);

/* true si algo responde (ACK) en esa direccion del bus I2C0. */
bool i2c0_probe(uint8_t addr);

#ifdef __cplusplus
}
#endif
