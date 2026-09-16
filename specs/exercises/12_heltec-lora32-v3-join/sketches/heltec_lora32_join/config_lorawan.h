/**
 * ============================================================================
 *  config_lorawan.h  -  CONFIGURACION DEL EJERCICIO 12 (Heltec WiFi LoRa 32)
 * ============================================================================
 *
 *  ESTE ES EL UNICO ARCHIVO QUE NECESITAS EDITAR COMO ESTUDIANTE.
 *
 *  En este ejercicio las tres credenciales OTAA las GENERA ChirpStack (boton
 *  de "generar" de cada campo en la consola https://lns.pi-caro.org) y tu las
 *  copias aqui. Flujo:
 *
 *     ChirpStack genera  ->  tu copias aqui  ->  compilas  ->  flasheas
 *
 *     1) DEV_EUI   -> "Device EUI (EUI64)"   8 bytes
 *     2) JOIN_EUI  -> "Join EUI (EUI64)"     8 bytes
 *     3) APP_KEY   -> "Application key"      16 bytes
 *
 *  ChirpStack muestra los valores en hexadecimal y en orden MSB (el boton
 *  MSB/LSB del campo debe estar en MSB). RadioLib tambien los toma en MSB,
 *  asi que NO hay que invertir nada: se copian tal cual, de izquierda a
 *  derecha. Ver el README, seccion "Paso 3".
 * ============================================================================
 */
#pragma once

#include <stdint.h>

/* ===========================================================================
 *  A) VERSION DE LA PLACA:  3  (Heltec WiFi LoRa 32 V3)
 *                           4  (Heltec WiFi LoRa 32 V4, revisiones 4.2 y 4.3)
 *
 *  Como saberlo: la V4 tiene conector USB-C con el chip USB integrado en el
 *  ESP32-S3 (aparece como "USB JTAG/serial debug unit" o "USB Serial Device"),
 *  conector de panel solar y conector GNSS. La V3 aparece como "Silicon Labs
 *  CP210x" y no tiene esos conectores. Tambien esta serigrafiado en la placa.
 * ========================================================================= */
#define HELTEC_BOARD_VERSION   3

/* ===========================================================================
 *  B) CREDENCIALES OTAA (las tres las genera ChirpStack)
 * ========================================================================= */

/* ---------------------------------------------------------------------------
 * 1) DEV_EUI  (8 bytes, MSB)  ->  ChirpStack: "Device EUI (EUI64)"
 *
 *    ChirpStack te muestra, por ejemplo:   a1 b2 c3 d4 e5 f6 07 18
 *    Aqui se escribe con 0x delante, todo junto y ULL al final:
 *                                          0xA1B2C3D4E5F60718ULL
 * ------------------------------------------------------------------------- */
#define PICARO_DEV_EUI    0x0000000000000000ULL   /* <-- PON AQUI TU DevEUI  */

/* ---------------------------------------------------------------------------
 * 2) JOIN_EUI  (8 bytes, MSB)  ->  ChirpStack: "Join EUI (EUI64)"
 *
 *    En este ejercicio TAMBIEN se genera en ChirpStack (mismo boton de
 *    generar). Mismo formato que el DevEUI.
 * ------------------------------------------------------------------------- */
#define PICARO_JOIN_EUI   0x0000000000000000ULL   /* <-- PON AQUI TU JoinEUI */

/* ---------------------------------------------------------------------------
 * 3) APP_KEY  (16 bytes, MSB)  ->  ChirpStack: pestana "OTAA keys",
 *                                   campo "Application key"
 *
 *    ChirpStack te muestra, por ejemplo:
 *        00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff
 *    Aqui va cada byte con 0x delante y separados por comas, en el mismo
 *    orden. Es la llave SECRETA: no la subas a ningun repositorio.
 * ------------------------------------------------------------------------- */
#define PICARO_APP_KEY    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

/* ===========================================================================
 *  C) PARAMETROS DE RED (normalmente NO se tocan)
 * ========================================================================= */

/* Region US915 y sub-banda 2 (canales 8-15, 903.9-905.3 MHz): la del gateway
 * del curso y la que espera ChirpStack. RadioLib numera las sub-bandas desde
 * 1, asi que la "sub-banda 2" se pide como 2. */
#define PICARO_SUBBAND             2

/* Datarate inicial del uplink. DR3 = SF7/BW125: el mas rapido de US915 y el
 * ideal con el gateway cerca (salon). ADR lo ajustara despues si hace falta. */
#define PICARO_UPLINK_DR           3

/* Puerto de aplicacion del uplink. El codec de ChirpStack lo usa para saber
 * como interpretar los bytes (ver payload_decoder.js). */
#define PICARO_UPLINK_FPORT        1

/* Cada cuantos segundos se envia un paquete. 30 s va bien para la demo. */
#define PICARO_UPLINK_INTERVAL_S   30

/* Cada cuantos uplinks se pide uno CONFIRMADO (con ACK del servidor). Sirve
 * para comprobar que el enlace sigue vivo y para ver un downlink real en el
 * Monitor Serie y en la OLED. 0 = nunca. */
#define PICARO_CONFIRMED_EVERY     5

/* Si 1, en cada arranque se hace un JOIN OTAA REAL por aire aunque haya una
 * sesion guardada. Es lo que queremos en este ejercicio: ver el join. Con 0
 * RadioLib reutiliza la sesion de la flash y "JOIN OK" saldria sin transmitir. */
#define PICARO_FORCE_FRESH_JOIN    1

/* Poner en 1 UNA sola vez si ChirpStack rechaza el join por "DevNonce ya
 * usado" (por ejemplo tras borrar y recrear el device): borra los contadores
 * guardados. Flashea, arranca, regresa a 0 y vuelve a flashear. */
#define PICARO_RESET_NONCES        0

/* Potencia del SX1262 en dBm. Si NO lo defines, board_heltec.h pone un valor
 * seguro segun la placa (V3: 14 dBm, V4: 2 dBm porque lleva amplificador). */
/* #define PICARO_TX_POWER_DBM     14 */
