/**
 * @file      creds_us915.h
 * @brief     Credenciales LoRaWAN + región para el build:
 *            tracker-bme280-gnss_lr1110_us915
 *
 * ⚠️ LAB — valores de laboratorio deterministas. Reemplazar antes de una red productiva.
 *
 * Esquema: prefijo de lab AA:BB:CC:DD + tag de banda (91 50 = 915) + índice (01).
 * Este header es la FUENTE DE VERDAD; sus valores se copian a
 * main_geolocation/example_options.h antes de compilar este build.
 */
#ifndef CREDS_US915_H
#define CREDS_US915_H

#include "smtc_modem_api.h"  // SMTC_MODEM_REGION_US_915

// DevEUI (8 bytes, MSB first)
#define USER_LORAWAN_DEVICE_EUI                        \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x91, 0x50, 0x01 \
    }

// JoinEUI (= AppEUI en 1.0.x) — join server de lab, compartido entre bandas
#define USER_LORAWAN_JOIN_EUI                          \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 \
    }

// AppKey (16 bytes) — patrón A9:15 (tag banda 915), único de este build
#define USER_LORAWAN_APP_KEY                                                                           \
    {                                                                                                  \
        0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15, 0xA9, 0x15 \
    }

#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_US_915

#endif  // CREDS_US915_H
