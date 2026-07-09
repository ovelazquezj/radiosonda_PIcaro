/**
 * @file      creds_eu868.h
 * @brief     Credenciales LoRaWAN + región para el build:
 *            tracker-bmp280-gnss_lr1110_eu868
 *
 * ⚠️ LAB — valores de laboratorio deterministas. Reemplazar antes de una red productiva.
 *
 * Esquema: prefijo de lab AA:BB:CC:DD + tag de banda (86 80 = 868) + índice (01).
 * Este header es la FUENTE DE VERDAD; sus valores se copian a
 * main_geolocation/example_options.h antes de compilar este build.
 */
#ifndef CREDS_EU868_H
#define CREDS_EU868_H

#include "smtc_modem_api.h"  // SMTC_MODEM_REGION_EU_868

// DevEUI (8 bytes, MSB first)
#define USER_LORAWAN_DEVICE_EUI                        \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x86, 0x80, 0x01 \
    }

// JoinEUI (= AppEUI en 1.0.x) — join server de lab, compartido entre bandas
#define USER_LORAWAN_JOIN_EUI                          \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 \
    }

// AppKey (16 bytes) — patrón A8:68 (tag banda 868), único de este build
#define USER_LORAWAN_APP_KEY                                                                           \
    {                                                                                                  \
        0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68, 0xA8, 0x68 \
    }

#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_EU_868

#endif  // CREDS_EU868_H
