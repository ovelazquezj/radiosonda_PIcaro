/**
 * ============================================================================
 *  config_lorawan.h  -  CONFIGURACION DE LA RADIOSONDA PICARO
 * ============================================================================
 *
 *  ESTE ES EL UNICO ARCHIVO QUE NECESITAS EDITAR COMO ESTUDIANTE.
 *
 *  Aqui pones las 3 credenciales que ChirpStack te da (o que tu inventas y
 *  luego copias a ChirpStack) para que la placa pueda "unirse" (join) a la red
 *  LoRaWAN:
 *
 *     1) DEV_EUI   -> identificador unico del dispositivo (8 bytes)
 *     2) JOIN_EUI  -> identificador de la aplicacion, antes llamado AppEUI (8 bytes)
 *     3) APP_KEY   -> llave secreta de cifrado (16 bytes)
 *
 *  Ver el README.md (seccion "Paso 4: cambiar las llaves") para el paso a paso
 *  con capturas de donde sale cada valor en ChirpStack.
 * ============================================================================
 */
#pragma once

/* ---------------------------------------------------------------------------
 * 1) DEV_EUI  (Device EUI)  -  8 bytes
 * ---------------------------------------------------------------------------
 *  - Se escribe como un numero hexadecimal de 16 digitos con el prefijo 0x.
 *  - El ORDEN es "MSB primero" (el mismo que ves en la interfaz de ChirpStack
 *    cuando el boton de orden esta en "MSB").
 *  - Ejemplo de ChirpStack:  70 B3 D5 7E D0 06 6A 4C
 *    -> aqui se escribe:      0x70B3D57ED0066A4C
 *
 *  Si aun no tienes uno, puedes usar el que ya viene abajo para tu primera
 *  prueba y darlo de alta con ESE mismo valor en ChirpStack.
 */
#define PICARO_DEV_EUI      0x70B3D57ED0066A4C

/* ---------------------------------------------------------------------------
 * 2) JOIN_EUI  (Join EUI / AppEUI)  -  8 bytes
 * ---------------------------------------------------------------------------
 *  - Mismo formato que DEV_EUI (0x + 16 digitos, MSB primero).
 *  - En muchas instalaciones de ChirpStack se deja en TODO CEROS. Es lo mas
 *    comun para practicar, asi que lo dejamos en ceros por defecto.
 *  - IMPORTANTE: el valor que pongas aqui debe ser EL MISMO que registres en
 *    ChirpStack (campo "Join EUI").
 */
#define PICARO_JOIN_EUI     0x0000000000000000

/* ---------------------------------------------------------------------------
 * 3) APP_KEY  (Application Key)  -  16 bytes
 * ---------------------------------------------------------------------------
 *  - Es la llave SECRETA. NO la compartas ni la subas a internet.
 *  - Se escribe como 16 bytes separados por comas, MSB primero (el mismo orden
 *    que muestra ChirpStack).
 *  - Ejemplo de ChirpStack:  00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
 *    -> aqui se escribe cada byte con 0x delante, separados por comas (abajo).
 *
 *  Debes reemplazar los 16 valores de abajo por los de TU dispositivo.
 */
#define PICARO_APP_KEY      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, \
                            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF

/* ===========================================================================
 *  PARAMETROS DE RED  (normalmente NO se tocan para esta practica)
 * ===========================================================================
 */

/* Region / banda de frecuencia.  Debe COINCIDIR con tu gateway y ChirpStack.
 * Para este curso quedo fijada en US915 (Estados Unidos / Latinoamerica).
 * Otras opciones validas de RadioLib: EU868, AU915, AS923, IN865, KR920, CN500 */
#define PICARO_REGION       US915

/* Sub-banda de canales (SOLO aplica a US915 y AU915).
 * ChirpStack, por defecto, usa la sub-banda 2 (canales 8 a 15) en US915.
 * Si tu gateway usa otra sub-banda, cambia este numero.
 * En EU868/AS923 este valor se ignora. */
#define PICARO_SUBBAND      2

/* Cada cuantos SEGUNDOS se envia un paquete (uplink).
 * Para la demo en el salon: 30 s esta bien. En produccion se usan minutos
 * para respetar las reglas de uso justo del espectro. */
#define PICARO_UPLINK_INTERVAL_S   30

/* Puerto de aplicacion (fPort) del uplink. ChirpStack lo muestra en cada
 * mensaje; el decodificador de payload lo usa para saber como interpretar. */
#define PICARO_UPLINK_FPORT        10

/* Datarate inicial de subida (0 = mas lento/mas alcance, 3 = mas rapido).
 * En US915 el DR3 (SF7BW125) permite paquetes grandes y es ideal cuando el
 * gateway esta cerca (caso del salon). El algoritmo ADR lo ajustara solo. */
#define PICARO_INITIAL_DATARATE    3
