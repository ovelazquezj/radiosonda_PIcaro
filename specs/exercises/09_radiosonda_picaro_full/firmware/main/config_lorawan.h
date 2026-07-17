/**
 * ============================================================================
 *  config_lorawan.h  -  CONFIGURACION LoRaWAN de la Radiosonda PICARO FULL
 * ============================================================================
 *
 *  ESTE ES EL UNICO ARCHIVO QUE NECESITAS EDITAR COMO ESTUDIANTE.
 *
 *  Aqui van las 3 credenciales OTAA que registras en ChirpStack para que la
 *  placa pueda "unirse" (join) a la red:
 *     1) DEV_EUI   -> identificador unico del dispositivo (8 bytes)
 *     2) JOIN_EUI  -> identificador de aplicacion, antes "AppEUI" (8 bytes)
 *     3) APP_KEY   -> llave secreta de cifrado (16 bytes)
 *
 *  Los valores de abajo son de LABORATORIO: sirven para tu primera prueba y ya
 *  vienen dados de alta. Cuando hagas tu propio dispositivo, cambia estos 3 y
 *  registralos identicos en ChirpStack (ver README, seccion ChirpStack E2E).
 * ============================================================================
 */
#pragma once

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * 1) DEV_EUI  (8 bytes, MSB primero)  ->  en ChirpStack: "Device EUI"
 *    ChirpStack lo muestra como:  70 B3 D5 7E D0 09 09 01
 * ------------------------------------------------------------------------- */
#define PICARO_DEV_EUI    0x70B3D57ED0090901ULL

/* ---------------------------------------------------------------------------
 * 2) JOIN_EUI  (8 bytes, MSB primero)  ->  en ChirpStack: "Join EUI"
 *    Para practicar se deja en TODO CEROS (lo mas comun).
 * ------------------------------------------------------------------------- */
#define PICARO_JOIN_EUI   0x0000000000000000ULL

/* ---------------------------------------------------------------------------
 * 3) APP_KEY  (16 bytes, MSB primero)  ->  llave SECRETA. No la publiques.
 *    En ChirpStack (LoRaWAN 1.0.x) se pega en el campo "Application key".
 *    (Por REST/provision.sh ese mismo valor va en el campo "nwkKey").
 *    Valor de lab: los ASCII de "PICARO09" + relleno 0xA5.
 * ------------------------------------------------------------------------- */
#define PICARO_APP_KEY    { 0x50, 0x49, 0x43, 0x41, 0x52, 0x4F, 0x30, 0x39, \
                            0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5 }

/* ---------------------------------------------------------------------------
 * Parametros de red (NO cambiar salvo que sepas lo que haces).
 *   - Region US915 + sub-banda 2 (canales 8-15): el default de ChirpStack y
 *     del gateway del curso. RadioLib enumera las sub-bandas desde 1, asi que
 *     "sub-banda 2" se pide como selectSubBand(2).
 *   - LoRaWAN 1.0.x -> solo se usa AppKey.
 * ------------------------------------------------------------------------- */
#define PICARO_SUBBAND            2       /* US915 FSB2 (canales 8-15) */
#define PICARO_UPLINK_FPORT       10      /* puerto de aplicacion del uplink  */
#define PICARO_UPLINK_INTERVAL_S  30      /* cada cuantos segundos se transmite */

/* Poner en 1 UNA vez si ChirpStack rechaza el join por "DevNonce ya usado":
 * borra los nonces guardados en NVS, arranca, y regresa a 0 y reflashea. */
#define PICARO_RESET_NONCES       0

/* Si 1: IGNORA la sesion guardada y fuerza un JOIN OTAA REAL por aire en cada
 * arranque. Asi "JOIN OK" solo aparece si de verdad hubo JoinAccept (necesita
 * gateway en rango). Con 0 (por defecto) se reusa la sesion de NVS si existe,
 * pero el firmware NO la marca como enlace confirmado hasta recibir un ACK. */
#define PICARO_FORCE_FRESH_JOIN   0
