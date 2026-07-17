/**
 * ============================================================================
 *  lorawan.h  -  Capa LoRaWAN (OTAA) sobre RadioLib para la Radiosonda PICARO
 * ============================================================================
 *
 *  IMPORTANTE (joins honestos): RadioLib puede "restaurar" una sesion guardada
 *  en NVS SIN contactar la red (devuelve SESSION_RESTORED). Eso NO prueba que
 *  haya gateway en rango. Por eso distinguimos:
 *    - LORAWAN_JOIN_NEW      -> hubo handshake OTAA REAL por aire (JoinAccept).
 *    - LORAWAN_JOIN_RESTORED -> se reuso una sesion de NVS, SIN confirmar enlace.
 *  Y el "enlace confirmado" solo se declara cuando llega un downlink/ACK real
 *  (ver lorawan_send con confirmed=true).
 * ============================================================================
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LORAWAN_JOIN_FAIL = 0,   /* no hay sesion */
    LORAWAN_JOIN_NEW,        /* join OTAA real por aire (confirmado) */
    LORAWAN_JOIN_RESTORED,   /* sesion restaurada de NVS (SIN confirmar) */
} lorawan_join_t;

/* Intenta unir/restaurar. Ver el enum para el significado honesto. */
lorawan_join_t lorawan_join(void);

/* Envia un uplink. Si confirmed=true, exige ACK de la red.
 * Devuelve: <0 error · 0 enviado sin downlink · >0 llego downlink/ACK. */
int lorawan_send(const uint8_t *data, uint8_t len, uint8_t fport, bool confirmed);

/* true si hay una sesion (nueva o restaurada) cargada. */
bool lorawan_is_joined(void);

#ifdef __cplusplus
}
#endif
