/**
 * ============================================================================
 *  config_sensores.h  -  QUE SENSORES ESTAN CONECTADOS Y EN QUE PIN
 * ============================================================================
 *
 *  La estacion se construye por etapas. Cada sensor se activa poniendo su
 *  macro SENSOR_xxx en 1 cuando ya esta cableado. Con todos en 0 (como viene)
 *  la placa hace el join y envia solo el voltaje de bateria; los campos de los
 *  sensores viajan como "no disponible" y el codec los omite.
 *
 *  Alimentacion: TODOS los sensores se alimentan del pin Vext de la CubeCell
 *  (3.3 V conmutados). El sketch enciende Vext solo para medir y lo apaga
 *  despues, asi la estacion aguanta con bateria. Por eso NO los conectes a
 *  3V3 fijo: conectalos a Vext.
 *
 *  Pines de la HTCC-AB01 (serigrafia de la placa -> nombre en el codigo):
 *    SDA, SCL  -> I2C (BMP280 y BH1750 comparten el bus)
 *    ADC       -> entrada analogica (salida AO del modulo de lluvia)
 *    GPIO1     -> entrada digital  (salida DO del modulo de lluvia)
 *    GPIO5     -> entrada digital  (DATA del DHT11)
 *    Vext      -> 3.3 V conmutado para los sensores
 *    GND       -> comun
 * ============================================================================
 */
#pragma once

/* ---------------------------------------------------------------------------
 * DHT11 - temperatura (1 C) y humedad relativa (1 %). Bus de 1 hilo propio.
 *   VCC -> Vext, GND -> GND, DATA -> GPIO5 (con resistencia de 10 kOhm entre
 *   DATA y VCC si tu modulo no la trae; los modulos de 3 pines ya la traen).
 * ------------------------------------------------------------------------- */
#ifndef SENSOR_DHT11
#define SENSOR_DHT11        0
#endif
#define PIN_DHT11           GPIO5

/* ---------------------------------------------------------------------------
 * BMP280 - presion (hPa) y temperatura (0.01 C). I2C, direccion 0x76 o 0x77.
 *   VCC -> Vext, GND -> GND, SDA -> SDA, SCL -> SCL.
 *   Si tu modulo tiene el pin SDO: a GND = 0x76, a VCC = 0x77.
 * ------------------------------------------------------------------------- */
#ifndef SENSOR_BMP280
#define SENSOR_BMP280       0
#endif
#define BMP280_I2C_ADDR     0x76

/* ---------------------------------------------------------------------------
 * BH1750 - luz ambiente (lux). I2C, direccion 0x23 (ADDR al aire o a GND).
 *   VCC -> Vext, GND -> GND, SDA -> SDA, SCL -> SCL.
 * ------------------------------------------------------------------------- */
#ifndef SENSOR_BH1750
#define SENSOR_BH1750       0
#endif

/* ---------------------------------------------------------------------------
 * MH-RD - modulo detector de lluvia (placa sensora + comparador LM393).
 *   VCC -> Vext, GND -> GND, AO -> ADC, DO -> GPIO1.
 *   AO baja cuando hay agua sobre la placa; DO se pone en BAJO al superar el
 *   umbral del potenciometro del modulo.
 *   MHRD_DRY_RAW y MHRD_WET_RAW son las lecturas de analogRead() con la
 *   placa seca y empapada: se calibran en el Paso de los sensores del README
 *   mirando el Monitor Serie. Con ellas el sketch calcula un porcentaje.
 * ------------------------------------------------------------------------- */
#ifndef SENSOR_MHRD
#define SENSOR_MHRD         0
#endif
#define PIN_MHRD_DO         GPIO1
#define PIN_MHRD_AO         ADC
#define MHRD_DRY_RAW        4095
#define MHRD_WET_RAW        0

/* Tiempo que se deja encendido Vext antes de leer, para que los sensores
 * arranquen (el DHT11 necesita ~1 s tras alimentarse). */
#define SENSORES_WARMUP_MS  1200
