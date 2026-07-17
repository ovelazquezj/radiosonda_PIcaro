/**
 * ============================================================================
 *  radio.h  -  Radio SX1262 del T-Beam Supreme via RadioLib + EspHal
 * ============================================================================
 *
 *  RadioLib (componente oficial de ESP-IDF) trae la clase EspHal que abstrae
 *  el SPI/GPIO del ESP32-S3. Aqui creamos el objeto `radio` (SX1262) con el
 *  pinout del Supreme y lo exponemos para que lo use el resto del firmware
 *  (en la Fase 3 lo envolvera el nodo LoRaWAN).
 *
 *  Constructores usados:
 *    EspHal(sck, miso, mosi)                 -> bus SPI del radio
 *    SX1262 = new Module(hal, NSS, DIO1, RST, BUSY)
 * ============================================================================
 */
#pragma once

#include <RadioLib.h>
#include "esp_hal.h"    /* nuestra HAL de RadioLib para el ESP32-S3 (spi_master) */

/* Objeto global del radio (definido en radio.cpp). */
extern SX1262 radio;

/* Fase 1: prueba de arranque del radio (SPI + TCXO). Devuelve el codigo de
 * RadioLib: 0 == RADIOLIB_ERR_NONE (OK). */
int picaro_radio_begin_test(void);
