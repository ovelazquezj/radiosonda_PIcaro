/**
 * ============================================================================
 *  sensores_config.h  -  QUE SENSORES SE USAN Y QUE VA EN EL PAYLOAD
 * ============================================================================
 *
 *  Filosofia didactica de este ejercicio:
 *    - TODOS los sensores de la T-Beam Supreme tienen su driver escrito
 *      (axp2101, l76k, bme280, qmi8658, qmc6310, pcf8563, sh1106, sdcard).
 *    - Pero solo se ACTIVAN los que entran en la telemetria LoRaWAN. El resto
 *      queda listo para que TU lo enciendas cambiando un 0 por un 1 aqui.
 *
 *  Dos niveles de control:
 *    (A) USE_*        -> compila e inicializa el driver del periferico.
 *    (B) TELEMETRY_*  -> incluye ese dato en el PAYLOAD que se transmite.
 *
 *  IMPORTANTE (payload <-> datarate): cada dato que activas agranda el payload.
 *  En US915, con mala senal el ADR baja el datarate y el payload maximo se
 *  encoge (DR0/SF10 solo admite ~11 bytes utiles). Si activas demasiados
 *  campos y el uplink no cabe, o subes el datarate o quitas campos. Jugar con
 *  esto es parte del ejercicio. Ver README, seccion "payload <-> datarate".
 *
 *  NOTA: la microSD guarda SIEMPRE la telemetria COMPLETA (todos los sensores
 *  activados con USE_*), independientemente de lo que quepa en el uplink.
 * ============================================================================
 */
#pragma once

/* ============================ (A) DRIVERS =============================== */
/* Perifericos que se inicializan. Los 3 primeros son imprescindibles para la
 * telemetria por defecto; los demas vienen APAGADOS para que los enciendas. */

#define USE_AXP2101     1   /* PMU / bateria. OBLIGATORIO: da energia al radio */
#define USE_L76K_GPS    1   /* GNSS: lat/lon/alt/sats                          */
#define USE_BME280      1   /* temperatura y presion (y humedad, ver abajo)    */

/* IMU QMI8658 (SPI3, comparte bus con la microSD). En la unidad probada el
 * WHO_AM_I sale intermitente (0x05/0x3E): problema de integridad de senal en el
 * bus SPI compartido que requiere analizador logico para depurar. Se deja
 * CODIFICADO pero APAGADO. Si lo activas y ves whoami=0x3E, es este tema. */
#define USE_QMI8658     0   /* IMU 6 ejes  [codificado; SPI inestable en esta placa] */
#define USE_QMC6310     1   /* magnetometro -> rumbo             [ACTIVO->CSV] */
#define USE_PCF8563     0   /* RTC -> timestamp (se puede sincronizar del GPS) */
#define USE_SH1106_OLED 1   /* pantalla de estado                              */
#define USE_SDCARD      1   /* log CSV de telemetria completa                  */

/* Demos de radios internas del ESP32-S3 (NO se usan en la telemetria; se
 * dejan como demo opcional para no perturbar el timing de LoRaWAN). */
#define USE_WIFI_DEMO   0   /* escaneo WiFi al arrancar (ver wifi_demo.c)  [demo] */
/* BLE: reservado. El ESP32-S3 tiene BLE 5; para un beacon habilita Bluetooth
 * (NimBLE) en `idf.py menuconfig` y parte del ejemplo oficial de ESP-IDF.
 * Se deja como ejercicio para no enlazar la pila BT en el binario por defecto. */
#define USE_BLE_DEMO    0

/* ========================= (B) CAMPOS DEL PAYLOAD ====================== */
/* Que se incluye en el uplink LoRaWAN (fPort 10). Cada TELEMETRY_* requiere
 * que su USE_* correspondiente este en 1. */

#define TELEMETRY_STATUS      1   /* byte de flags (fix/charging/usb/...)      */
#define TELEMETRY_GPS         1   /* lat, lon, alt, sats  (requiere USE_L76K)  */
#define TELEMETRY_BATTERY     1   /* mV + %               (requiere USE_AXP2101)*/
#define TELEMETRY_TEMP        1   /* temperatura x100     (requiere USE_BME280) */
#define TELEMETRY_PRESSURE    1   /* presion x10          (requiere USE_BME280) */

#define TELEMETRY_HUMIDITY    0   /* humedad %            (requiere USE_BME280) */
#define TELEMETRY_IMU         0   /* accel/gyro           (requiere USE_QMI8658)*/
#define TELEMETRY_HEADING     0   /* rumbo grados         (requiere USE_QMC6310)*/

/* Chequeos de coherencia: no dejes activar un campo sin su driver. */
#if TELEMETRY_GPS && !USE_L76K_GPS
#  error "TELEMETRY_GPS requiere USE_L76K_GPS=1"
#endif
#if TELEMETRY_BATTERY && !USE_AXP2101
#  error "TELEMETRY_BATTERY requiere USE_AXP2101=1"
#endif
#if (TELEMETRY_TEMP || TELEMETRY_PRESSURE || TELEMETRY_HUMIDITY) && !USE_BME280
#  error "Los campos ambientales requieren USE_BME280=1"
#endif
#if TELEMETRY_IMU && !USE_QMI8658
#  error "TELEMETRY_IMU requiere USE_QMI8658=1"
#endif
#if TELEMETRY_HEADING && !USE_QMC6310
#  error "TELEMETRY_HEADING requiere USE_QMC6310=1"
#endif
