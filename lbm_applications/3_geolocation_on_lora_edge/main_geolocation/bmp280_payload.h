/*!
 * \file      bmp280_payload.h
 *
 * \brief     Compact big-endian payload encoder for BMP280 environmental data.
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

#ifndef BMP280_PAYLOAD_H
#define BMP280_PAYLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */
#include <stdint.h>

#include "bmp280.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*! \brief Payload format version byte (offset 0). */
#define BMP280_PAYLOAD_VERSION ( 0x02 )

/*! \brief Encoded payload length in bytes. */
#define BMP280_PAYLOAD_SIZE ( 5 )

/*! \brief LoRaWAN fport used for the BMP280 environmental payload. */
#define BMP280_PAYLOAD_FPORT ( 10 )

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/*!
 * \brief Encode compensated BMP280 data into a compact big-endian payload.
 *
 * Layout (5 bytes, big-endian / MSB first):
 * | Offset | Field       | Type   | Encoding          |
 * |--------|-------------|--------|-------------------|
 * | 0      | version     | uint8  | fixed 0x02        |
 * | 1-2    | temperature | int16  | degrees C x 100   |
 * | 3-4    | pressure    | uint16 | hPa (integer)     |
 *
 * The encoding is written byte by byte so it does not depend on the endianness
 * of the host MCU.
 *
 * \param [in]  in      Compensated measurement to encode (must not be NULL).
 * \param [out] buffer  Destination buffer, capacity >= \ref BMP280_PAYLOAD_SIZE.
 *
 * \returns Number of bytes written (\ref BMP280_PAYLOAD_SIZE, i.e. 5), or 0 on
 *          a NULL argument.
 */
uint8_t bmp280_payload_encode( const bmp280_data_t* in, uint8_t* buffer );

#ifdef __cplusplus
}
#endif

#endif  // BMP280_PAYLOAD_H

/* --- EOF ------------------------------------------------------------------ */
