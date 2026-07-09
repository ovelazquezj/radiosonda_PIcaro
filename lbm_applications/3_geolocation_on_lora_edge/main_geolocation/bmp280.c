/*!
 * \file      bmp280.c
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

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // NULL

#include "bmp280.h"
#include "smtc_hal_i2c.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*! \brief BMP280 register map (subset used by this driver). */
#define BMP280_REG_ID ( 0xD0 )         //!< Chip-id register (reads 0x58)
#define BMP280_REG_CALIB_START ( 0x88 )//!< First calibration byte (dig_T1 LSB)
#define BMP280_REG_STATUS ( 0xF3 )     //!< Status register
#define BMP280_REG_CTRL_MEAS ( 0xF4 )  //!< Measurement control register
#define BMP280_REG_PRESS_MSB ( 0xF7 )  //!< First raw-data byte (press MSB)

/*! \brief Number of calibration bytes (0x88..0x9F). */
#define BMP280_CALIB_LEN ( 24 )

/*! \brief Number of raw data bytes (0xF7..0xFC: press[3] + temp[3]). */
#define BMP280_RAW_LEN ( 6 )

/*!
 * \brief ctrl_meas value: osrs_t = x1 (001), osrs_p = x1 (001), mode = forced
 *        (01) => 0b001_001_01 = 0x25.
 */
#define BMP280_CTRL_MEAS_FORCED ( 0x25 )

/*! \brief status register "measuring" bit (bit 3). */
#define BMP280_STATUS_MEASURING ( 0x08 )

/*! \brief Polling attempts before declaring a conversion timeout. */
#define BMP280_MEASURE_TIMEOUT ( 100 )

/*! \brief HAL return code meaning success. */
#define BMP280_I2C_OK ( 0 )

/*! \brief Valid physical ranges used to reject implausible readings. */
#define BMP280_TEMP_MIN_C ( -40.0f )
#define BMP280_TEMP_MAX_C ( 85.0f )
#define BMP280_PRESS_MIN_HPA ( 300.0f )
#define BMP280_PRESS_MAX_HPA ( 1100.0f )

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*! \brief Cached calibration coefficients and driver context. */
typedef struct
{
    uint32_t i2c_id;       //!< I2C peripheral id
    uint8_t  device_addr;  //!< 7-bit device address
    bool     initialized;  //!< true once bmp280_init succeeded

    uint16_t dig_t1;
    int16_t  dig_t2;
    int16_t  dig_t3;
    uint16_t dig_p1;
    int16_t  dig_p2;
    int16_t  dig_p3;
    int16_t  dig_p4;
    int16_t  dig_p5;
    int16_t  dig_p6;
    int16_t  dig_p7;
    int16_t  dig_p8;
    int16_t  dig_p9;

    int32_t t_fine;  //!< Carry value from temperature to pressure compensation
} bmp280_ctx_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static bmp280_ctx_t bmp280_ctx;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*!
 * \brief Little-endian 16-bit unsigned load from a byte buffer.
 */
static uint16_t bmp280_u16_le( const uint8_t* p );

/*!
 * \brief Little-endian 16-bit signed load from a byte buffer.
 */
static int16_t bmp280_s16_le( const uint8_t* p );

/*!
 * \brief Apply the Bosch temperature compensation and update t_fine.
 * \returns Temperature in degrees Celsius.
 */
static float bmp280_compensate_temperature( int32_t adc_t );

/*!
 * \brief Apply the Bosch pressure compensation (uses t_fine).
 * \returns Pressure in Pascals.
 */
static float bmp280_compensate_pressure( int32_t adc_p );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

bool bmp280_init( uint32_t i2c_id, uint8_t device_addr )
{
    uint8_t id    = 0;
    uint8_t calib[BMP280_CALIB_LEN];

    bmp280_ctx.i2c_id      = i2c_id;
    bmp280_ctx.device_addr = device_addr;
    bmp280_ctx.initialized = false;

    /* Verify the chip-id register (0xD0) equals 0x58 (BMP280). */
    if( smtc_hal_i2c_read_buffer( i2c_id, device_addr, BMP280_REG_ID, &id, 1 ) != BMP280_I2C_OK )
    {
        return false;
    }
    if( id != BMP280_CHIP_ID )
    {
        return false;
    }

    /* Read and cache the calibration block (0x88..0x9F, little-endian). */
    if( smtc_hal_i2c_read_buffer( i2c_id, device_addr, BMP280_REG_CALIB_START, calib, BMP280_CALIB_LEN ) !=
        BMP280_I2C_OK )
    {
        return false;
    }

    bmp280_ctx.dig_t1 = bmp280_u16_le( &calib[0] );
    bmp280_ctx.dig_t2 = bmp280_s16_le( &calib[2] );
    bmp280_ctx.dig_t3 = bmp280_s16_le( &calib[4] );
    bmp280_ctx.dig_p1 = bmp280_u16_le( &calib[6] );
    bmp280_ctx.dig_p2 = bmp280_s16_le( &calib[8] );
    bmp280_ctx.dig_p3 = bmp280_s16_le( &calib[10] );
    bmp280_ctx.dig_p4 = bmp280_s16_le( &calib[12] );
    bmp280_ctx.dig_p5 = bmp280_s16_le( &calib[14] );
    bmp280_ctx.dig_p6 = bmp280_s16_le( &calib[16] );
    bmp280_ctx.dig_p7 = bmp280_s16_le( &calib[18] );
    bmp280_ctx.dig_p8 = bmp280_s16_le( &calib[20] );
    bmp280_ctx.dig_p9 = bmp280_s16_le( &calib[22] );

    bmp280_ctx.initialized = true;
    return true;
}

bool bmp280_read( bmp280_data_t* out )
{
    uint8_t ctrl = BMP280_CTRL_MEAS_FORCED;
    uint8_t status;
    uint8_t raw[BMP280_RAW_LEN];
    int32_t adc_p;
    int32_t adc_t;
    float   temperature_c;
    float   pressure_hpa;

    if( ( out == NULL ) || ( bmp280_ctx.initialized == false ) )
    {
        return false;
    }

    /* Trigger one forced-mode conversion (osrs x1, forced). */
    if( smtc_hal_i2c_write_buffer( bmp280_ctx.i2c_id, bmp280_ctx.device_addr, BMP280_REG_CTRL_MEAS, &ctrl, 1 ) !=
        BMP280_I2C_OK )
    {
        return false;
    }

    /* Wait for conversion completion by polling the measuring bit (timeout). */
    for( uint32_t attempt = 0; ; attempt++ )
    {
        if( attempt >= BMP280_MEASURE_TIMEOUT )
        {
            return false;  // conversion did not complete in time
        }
        if( smtc_hal_i2c_read_buffer( bmp280_ctx.i2c_id, bmp280_ctx.device_addr, BMP280_REG_STATUS, &status, 1 ) !=
            BMP280_I2C_OK )
        {
            return false;
        }
        if( ( status & BMP280_STATUS_MEASURING ) == 0 )
        {
            break;  // measurement done
        }
    }

    /* Burst-read the raw data: press[0..2] then temp[3..5]. */
    if( smtc_hal_i2c_read_buffer( bmp280_ctx.i2c_id, bmp280_ctx.device_addr, BMP280_REG_PRESS_MSB, raw,
                                  BMP280_RAW_LEN ) != BMP280_I2C_OK )
    {
        return false;
    }

    /* 20-bit raw values, MSB first: (msb<<12) | (lsb<<4) | (xlsb>>4). */
    adc_p = ( (int32_t) raw[0] << 12 ) | ( (int32_t) raw[1] << 4 ) | ( (int32_t) raw[2] >> 4 );
    adc_t = ( (int32_t) raw[3] << 12 ) | ( (int32_t) raw[4] << 4 ) | ( (int32_t) raw[5] >> 4 );

    /* Temperature must be compensated first: it produces t_fine. */
    temperature_c = bmp280_compensate_temperature( adc_t );
    pressure_hpa  = bmp280_compensate_pressure( adc_p ) / 100.0f;  // Pa -> hPa

    /* Reject physically implausible readings. */
    if( ( temperature_c < BMP280_TEMP_MIN_C ) || ( temperature_c > BMP280_TEMP_MAX_C ) )
    {
        return false;
    }
    if( ( pressure_hpa < BMP280_PRESS_MIN_HPA ) || ( pressure_hpa > BMP280_PRESS_MAX_HPA ) )
    {
        return false;
    }

    out->temperature_c = temperature_c;
    out->pressure_hpa  = pressure_hpa;
    return true;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static uint16_t bmp280_u16_le( const uint8_t* p )
{
    return (uint16_t) ( ( (uint16_t) p[1] << 8 ) | (uint16_t) p[0] );
}

static int16_t bmp280_s16_le( const uint8_t* p )
{
    return (int16_t) bmp280_u16_le( p );
}

static float bmp280_compensate_temperature( int32_t adc_t )
{
    /* Official Bosch floating-point compensation (BMP280 datasheet). */
    float var1, var2;

    var1 = ( ( (float) adc_t ) / 16384.0f - ( (float) bmp280_ctx.dig_t1 ) / 1024.0f ) * ( (float) bmp280_ctx.dig_t2 );
    var2 = ( ( ( (float) adc_t ) / 131072.0f - ( (float) bmp280_ctx.dig_t1 ) / 8192.0f ) *
             ( ( (float) adc_t ) / 131072.0f - ( (float) bmp280_ctx.dig_t1 ) / 8192.0f ) ) *
           ( (float) bmp280_ctx.dig_t3 );

    bmp280_ctx.t_fine = (int32_t) ( var1 + var2 );

    return ( var1 + var2 ) / 5120.0f;
}

static float bmp280_compensate_pressure( int32_t adc_p )
{
    /* Official Bosch floating-point compensation (BMP280 datasheet). */
    float var1, var2, p;

    var1 = ( (float) bmp280_ctx.t_fine / 2.0f ) - 64000.0f;
    var2 = var1 * var1 * ( (float) bmp280_ctx.dig_p6 ) / 32768.0f;
    var2 = var2 + var1 * ( (float) bmp280_ctx.dig_p5 ) * 2.0f;
    var2 = ( var2 / 4.0f ) + ( ( (float) bmp280_ctx.dig_p4 ) * 65536.0f );
    var1 = ( ( (float) bmp280_ctx.dig_p3 ) * var1 * var1 / 524288.0f + ( (float) bmp280_ctx.dig_p2 ) * var1 ) /
           524288.0f;
    var1 = ( 1.0f + var1 / 32768.0f ) * ( (float) bmp280_ctx.dig_p1 );

    if( var1 == 0.0f )
    {
        return 0.0f;  // avoid division by zero
    }

    p    = 1048576.0f - (float) adc_p;
    p    = ( p - ( var2 / 4096.0f ) ) * 6250.0f / var1;
    var1 = ( (float) bmp280_ctx.dig_p9 ) * p * p / 2147483648.0f;
    var2 = p * ( (float) bmp280_ctx.dig_p8 ) / 32768.0f;
    p    = p + ( var1 + var2 + ( (float) bmp280_ctx.dig_p7 ) ) / 16.0f;

    return p;  // Pascals
}

/* --- EOF ------------------------------------------------------------------ */
