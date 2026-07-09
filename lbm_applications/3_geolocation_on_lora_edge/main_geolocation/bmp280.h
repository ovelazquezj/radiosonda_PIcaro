/*!
 * \file      bmp280.h
 *
 * \brief     Driver for the Bosch BMP280 temperature & pressure sensor
 *            (no humidity channel). Uses exclusively the smtc_hal_i2c HAL.
 *
 * Original contribution to radiosonda_PIcaro — an educational fork of Semtech
 * LoRa Basics Modem (SWL2001). This file is NOT part of the original Semtech
 * distribution; it was written for radiosonda_PIcaro and is provided under the
 * same Clear BSD License as the rest of the project.
 *
 * The Clear BSD License
 * Copyright (c) 2026 radiosonda_PIcaro contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BMP280_H
#define BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */
#include <stdint.h>
#include <stdbool.h>

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*!
 * \brief Expected value of the BMP280 chip-id register (0xD0).
 */
#define BMP280_CHIP_ID ( 0x58 )

/*!
 * \brief Common 7-bit I2C addresses of the BMP280 (SDO pin dependent).
 *        These are the raw 7-bit device addresses; the HAL is responsible for
 *        the read/write bit handling.
 */
#define BMP280_I2C_ADDR_PRIMARY ( 0x76 )   //!< SDO tied to GND
#define BMP280_I2C_ADDR_SECONDARY ( 0x77 ) //!< SDO tied to VDDIO

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

/*!
 * \brief Compensated (physical) measurement produced by the BMP280 driver.
 */
typedef struct
{
    float temperature_c;  //!< Temperature in degrees Celsius
    float pressure_hpa;   //!< Pressure in hectopascals (hPa)
} bmp280_data_t;

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/*!
 * \brief Initialize the BMP280 driver: verify chip-id and cache calibration.
 *
 * Reads the id register (0xD0) and checks it equals \ref BMP280_CHIP_ID
 * (0x58). On success, reads and caches the factory calibration coefficients
 * (registers 0x88..0x9F) needed for the Bosch compensation formulas.
 *
 * \param [in] i2c_id       Identifier of the I2C peripheral to use (passed to
 *                          the smtc_hal_i2c HAL).
 * \param [in] device_addr  7-bit I2C address of the BMP280
 *                          (\ref BMP280_I2C_ADDR_PRIMARY or SECONDARY).
 *
 * \retval true   Sensor present, chip-id valid and calibration cached.
 * \retval false  I2C failure or chip-id mismatch (sensor absent/incorrect).
 */
bool bmp280_init( uint32_t i2c_id, uint8_t device_addr );

/*!
 * \brief Perform a single forced-mode measurement and return compensated data.
 *
 * Triggers a forced conversion (oversampling x1 on temperature and pressure),
 * waits for completion by polling the status register (with timeout), reads the
 * raw ADC values and applies the official Bosch compensation formulas.
 *
 * \param [out] out  Destination for the compensated temperature and pressure.
 *
 * \retval true   Valid measurement written to \p out.
 * \retval false  I2C failure, conversion timeout, or a compensated value out of
 *                the valid physical range (T: -40..85 C, P: 300..1100 hPa).
 */
bool bmp280_read( bmp280_data_t* out );

#ifdef __cplusplus
}
#endif

#endif  // BMP280_H

/* --- EOF ------------------------------------------------------------------ */
