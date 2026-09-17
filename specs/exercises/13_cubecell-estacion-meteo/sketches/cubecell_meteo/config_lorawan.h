/**
 * ============================================================================
 *  config_lorawan.h  -  CREDENCIALES Y RED DEL EJERCICIO 13 (CubeCell HTCC-AB01)
 * ============================================================================
 *
 *  Aqui van las tres credenciales OTAA que ChirpStack tiene para tu device.
 *  Se escriben como arreglos de bytes, en el MISMO orden en que ChirpStack
 *  los muestra (boton MSB/LSB del campo en "MSB"):
 *
 *     ChirpStack muestra   62 38 51 f5 18 c9 21 35
 *     aqui se escribe      { 0x62, 0x38, 0x51, 0xF5, 0x18, 0xC9, 0x21, 0x35 }
 *
 *  Los valores de abajo son los del device "itpa-meteor-lora" del curso. Si
 *  creas tu propio device, cambia los tres.
 * ============================================================================
 */
#pragma once

/* 1) Device EUI (8 bytes, MSB) -> ChirpStack: "Device EUI (EUI64)" */
#define PICARO_DEV_EUI    { 0x62, 0x38, 0x51, 0xF5, 0x18, 0xC9, 0x21, 0x35 }

/* 2) Join EUI (8 bytes, MSB)   -> ChirpStack: "Join EUI (EUI64)"
 *    En la pila de Heltec se llama "appEui": es el mismo dato. */
#define PICARO_JOIN_EUI   { 0x28, 0x80, 0x61, 0x53, 0xB3, 0xDD, 0xAD, 0xEB }

/* 3) Application key (16 bytes, MSB) -> ChirpStack: pestana "OTAA keys",
 *    campo "Application key". Es la llave SECRETA: no la publiques. */
#define PICARO_APP_KEY    { 0x26, 0x32, 0xCF, 0xED, 0xC2, 0x22, 0x36, 0x8E, \
                            0x33, 0x01, 0x49, 0x32, 0x4E, 0xE7, 0xED, 0x82 }

/* ===========================================================================
 *  Parametros de red (normalmente NO se tocan)
 * ========================================================================= */

/* Sub-banda de US915 que usan los gateways del curso: la 2 (canales 8-15,
 * 903.9-905.3 MHz). En la pila de Heltec se expresa como una mascara de
 * canales; el sketch la construye a partir de este numero (1..8). */
#define PICARO_SUBBAND             2

/* Cada cuantos SEGUNDOS se envia un paquete. Para la demo, 60 s. La pila
 * anade hasta 1 s aleatorio para no chocar con otros nodos. */
#define PICARO_UPLINK_INTERVAL_S   60

/* Puerto de aplicacion (fPort) del uplink. El codec de ChirpStack lo usa. */
#define PICARO_UPLINK_FPORT        2

/* Cada cuantos uplinks se pide uno CONFIRMADO (con ACK del servidor), para
 * ver un downlink real y comprobar que el enlace sigue vivo. 0 = nunca. */
#define PICARO_CONFIRMED_EVERY     5

/* Reintentos de un uplink confirmado sin ACK antes de darlo por perdido.
 * A partir del 3o la pila baja el datarate (ver comentario en el .ino). */
#define PICARO_CONFIRMED_TRIALS    4
