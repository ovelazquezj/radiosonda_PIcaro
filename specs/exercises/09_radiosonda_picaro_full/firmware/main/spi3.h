/**
 * ============================================================================
 *  spi3.h  -  Bus SPI compartido (SPI3_HOST) para IMU QMI8658 + microSD
 * ============================================================================
 *  Pines: SCK 36 / MISO 37 / MOSI 35. CS: IMU=34, SD=47 (cada driver pone el
 *  suyo). Init idempotente para que SD e IMU puedan convivir en el mismo bus.
 * ============================================================================
 */
#pragma once

#include "esp_err.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el bus SPI3 una sola vez (SD e IMU lo comparten). */
esp_err_t spi3_init(void);

#ifdef __cplusplus
}
#endif
