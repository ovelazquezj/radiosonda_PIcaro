/*!
 * \file      smtc_hal_i2c.h
 *
 * \brief     I2C Hardware Abstraction Layer definition
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
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
#ifndef SMTC_HAL_I2C_H
#define SMTC_HAL_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>  // C99 types

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*!
 * \brief Maximum blocking time, in milliseconds, granted to any single I2C
 *        transfer. Guarantees a finite timeout (never an infinite loop) on bus
 *        error / NACK / clock-stretch stall (SRS-001 REQ-001-4, value <= 100 ms).
 */
#define HAL_I2C_TIMEOUT_MS ( 100U )

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/*!
 * \brief Initializes the MCU I2C peripheral in master mode (Standard mode,
 *        >= 100 kHz). Enables the I2C and GPIO clocks and configures the SDA/SCL
 *        lines as alternate-function open-drain with internal pull-ups.
 *
 * \param [IN] id  I2C interface id [1:N] (id == 1 maps to I2C1)
 * \param [IN] sda SDA pin name to be used (hal_gpio_pin_names_t value)
 * \param [IN] scl SCL pin name to be used (hal_gpio_pin_names_t value)
 */
void smtc_hal_i2c_init( uint32_t id, uint32_t sda, uint32_t scl );

/*!
 * \brief De-initializes the MCU I2C peripheral.
 *
 * \param [IN] id I2C interface id [1:N] (id == 1 maps to I2C1)
 */
void smtc_hal_i2c_deinit( uint32_t id );

/*!
 * \brief Writes a buffer to an internal register of an I2C slave device.
 *
 * \param [IN] id          I2C interface id [1:N]
 * \param [IN] device_addr 7-bit I2C slave address (e.g. 0x76)
 * \param [IN] mem_addr    Internal register address of the slave (1-byte wide)
 * \param [IN] buffer      Pointer to the data to be written
 * \param [IN] size        Number of bytes to write
 *
 * \retval 0 on success, non-zero on error (timeout / NACK / bus error)
 */
uint8_t smtc_hal_i2c_write_buffer( uint32_t id, uint8_t device_addr, uint16_t mem_addr, const uint8_t* buffer,
                                   uint16_t size );

/*!
 * \brief Reads a buffer from an internal register of an I2C slave device.
 *
 * \param [IN]  id          I2C interface id [1:N]
 * \param [IN]  device_addr 7-bit I2C slave address (e.g. 0x76)
 * \param [IN]  mem_addr    Internal register address of the slave (1-byte wide)
 * \param [OUT] buffer      Pointer to the destination buffer
 * \param [IN]  size        Number of bytes to read
 *
 * \retval 0 on success, non-zero on error (timeout / NACK / bus error)
 */
uint8_t smtc_hal_i2c_read_buffer( uint32_t id, uint8_t device_addr, uint16_t mem_addr, uint8_t* buffer,
                                  uint16_t size );

#ifdef __cplusplus
}
#endif

#endif  // SMTC_HAL_I2C_H

/* --- EOF ------------------------------------------------------------------ */
