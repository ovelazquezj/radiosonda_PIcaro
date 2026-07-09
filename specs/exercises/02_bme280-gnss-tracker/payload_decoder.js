/*!
 * \file      payload_decoder.js
 *
 * \brief     Chirpstack/TTN-style uplink decoder for the BMP280 environmental
 *            payload (temperature + pressure, no humidity).
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

// LoRaWAN fport carrying the BMP280 environmental payload.
var BMP280_PAYLOAD_FPORT = 10;

// Expected payload length and format version (must match the C encoder).
var BMP280_PAYLOAD_SIZE = 5;
var BMP280_PAYLOAD_VERSION = 0x02;

/**
 * Chirpstack/TTN uplink decoder entry point.
 *
 * Reference test vector (inverse of bmp280_payload_encode):
 *   bytes  = [0x02, 0x09, 0x29, 0x03, 0xF5]  (fport 10)
 *   result = { version: 2, temperature: 23.45, pressure: 1013 }
 *
 * @param {{bytes: number[], fPort: number}} input Uplink descriptor.
 * @returns {{data: object, errors?: string[]}} Decoded payload.
 */
function decodeUplink(input) {
  var fport = input.fPort;
  if (fport === undefined) {
    fport = input.fport; // tolerate lowercase variants
  }

  // Only the environmental fport is handled here; other fports (e.g. GNSS)
  // are handled elsewhere and must not break this decoder.
  if (fport !== BMP280_PAYLOAD_FPORT) {
    return { data: {} };
  }

  var bytes = input.bytes;
  if (!bytes || bytes.length < BMP280_PAYLOAD_SIZE) {
    return { data: {}, errors: ["invalid payload length"] };
  }

  var version = bytes[0];

  // int16 big-endian, physical value = raw / 100.
  var tempRaw = (bytes[1] << 8) | bytes[2];
  if (tempRaw & 0x8000) {
    tempRaw -= 0x10000; // two's complement sign extension
  }
  var temperature = tempRaw / 100;

  // uint16 big-endian, physical value = raw hPa.
  var pressure = (bytes[3] << 8) | bytes[4];

  return {
    data: {
      version: version,
      temperature: temperature,
      pressure: pressure,
    },
  };
}

// Optional export for host-side unit testing (ignored by Chirpstack/TTN).
if (typeof module !== "undefined" && module.exports) {
  module.exports = { decodeUplink: decodeUplink };
}
