/*!
 * \file      bmp280_payload.c
 *
 * \brief     Compact big-endian payload encoder for BMP280 environmental data.
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

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */
#include <stdint.h>
#include <stddef.h>  // NULL
#include <math.h>    // lroundf

#include "bmp280_payload.h"
#include "bmp280.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*! \brief Saturating clamp helper. */
#define BMP280_CLAMP( v, lo, hi ) ( ( v ) < ( lo ) ? ( lo ) : ( ( v ) > ( hi ) ? ( hi ) : ( v ) ) )

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 *
 * Reference test vector (verified by hand and by the JavaScript decoder):
 *   in = { temperature_c = 23.45, pressure_hpa = 1013 }
 *     version     = 0x02
 *     temperature = round(23.45 * 100) = 2345 = 0x0929  -> bytes 0x09 0x29
 *     pressure    = round(1013)        = 1013 = 0x03F5  -> bytes 0x03 0xF5
 *   buffer      = 02 09 29 03 F5
 * -----------------------------------------------------------------------------
 */

uint8_t bmp280_payload_encode( const bmp280_data_t* in, uint8_t* buffer )
{
    long    temp_scaled;
    long    press_int;
    int16_t temperature;
    uint16_t pressure;

    if( ( in == NULL ) || ( buffer == NULL ) )
    {
        return 0;
    }

    /* temperature: degrees C x 100, signed 16-bit, saturated to int16 range. */
    temp_scaled = lroundf( in->temperature_c * 100.0f );
    temp_scaled = BMP280_CLAMP( temp_scaled, (long) INT16_MIN, (long) INT16_MAX );
    temperature = (int16_t) temp_scaled;

    /* pressure: integer hPa, unsigned 16-bit, saturated to uint16 range. */
    press_int = lroundf( in->pressure_hpa );
    press_int = BMP280_CLAMP( press_int, 0L, (long) UINT16_MAX );
    pressure  = (uint16_t) press_int;

    /* Big-endian serialization (MSB first), endianness-independent. */
    buffer[0] = BMP280_PAYLOAD_VERSION;
    buffer[1] = (uint8_t) ( ( (uint16_t) temperature >> 8 ) & 0xFF );
    buffer[2] = (uint8_t) ( (uint16_t) temperature & 0xFF );
    buffer[3] = (uint8_t) ( ( pressure >> 8 ) & 0xFF );
    buffer[4] = (uint8_t) ( pressure & 0xFF );

    return BMP280_PAYLOAD_SIZE;
}

/* --- EOF ------------------------------------------------------------------ */
