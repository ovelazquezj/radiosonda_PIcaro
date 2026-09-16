/**
 * ============================================================================
 *  board_heltec.h  -  Pinout de la Heltec WiFi LoRa 32 (V3 y V4)
 * ============================================================================
 *
 *  Este archivo NO se edita como estudiante. La version de la placa se elige
 *  en config_lorawan.h con HELTEC_BOARD_VERSION (3 o 4).
 *
 *  Ambas versiones llevan el mismo chip (ESP32-S3), la misma radio (SX1262)
 *  y la misma pantalla (OLED SSD1306 0.96" por I2C), con los MISMOS pines de
 *  radio y de pantalla. Lo que cambia entre V3 y V4:
 *
 *    | Que                    | V3                    | V4 (4.2 / 4.3)          |
 *    |------------------------|-----------------------|-------------------------|
 *    | USB                    | CP2102 (UART)         | USB nativo del S3       |
 *    | Flash / PSRAM          | 8 MB / sin PSRAM      | 16 MB / 2 MB PSRAM      |
 *    | Potencia TX            | 21 dBm (SX1262 solo)  | 28 dBm (SX1262 + PA)    |
 *    | Control ADC bateria    | GPIO37 en BAJO = mide | GPIO37 en ALTO = mide   |
 *    | LED                    | GPIO35 (simple)       | GPIO38 (WS2812 RGB)     |
 *    | Amplificador de RF     | no tiene              | GC1109 (V4.2) o         |
 *    |                        |                       | KCT8103L (V4.3)         |
 *
 *  Fuentes: variantes oficiales de Heltec para Arduino, definiciones de la
 *  placa en el firmware de Meshtastic (heltec_v3 / heltec_v4) y la guia de
 *  desarrollo de la V4 de OpenELAB.
 * ============================================================================
 */
#pragma once

#include "config_lorawan.h"

#if (HELTEC_BOARD_VERSION != 3) && (HELTEC_BOARD_VERSION != 4)
#error "HELTEC_BOARD_VERSION debe ser 3 o 4 (ver config_lorawan.h)"
#endif

/* ---------------------------------------------------------------------------
 * Radio LoRa SX1262 por SPI (iguales en V3 y V4)
 * ------------------------------------------------------------------------- */
#define LORA_NSS_PIN     8    /* Chip select                                  */
#define LORA_SCK_PIN     9    /* SPI clock                                    */
#define LORA_MOSI_PIN   10    /* SPI MOSI                                     */
#define LORA_MISO_PIN   11    /* SPI MISO                                     */
#define LORA_RST_PIN    12    /* Reset del SX1262                             */
#define LORA_BUSY_PIN   13    /* BUSY del SX1262 (obligatorio en SX126x)      */
#define LORA_DIO1_PIN   14    /* IRQ del SX1262 (TxDone/RxDone/Timeout)       */

/* El SX1262 de la Heltec usa un TCXO alimentado desde DIO3 (1.8 V) y el pin
 * DIO2 como control del conmutador de antena TX/RX. Sin estos dos ajustes el
 * radio "arranca" pero no transmite ni recibe nada. */
#define LORA_TCXO_VOLTAGE   1.8f
#define LORA_DIO2_RF_SWITCH true
#define LORA_USE_LDO        false   /* regulador DC-DC (como en Meshtastic)  */

/* ---------------------------------------------------------------------------
 * Pantalla OLED SSD1306 128x64 por I2C (iguales en V3 y V4)
 * ------------------------------------------------------------------------- */
#define OLED_SDA_PIN    17
#define OLED_SCL_PIN    18
#define OLED_RST_PIN    21
#define OLED_I2C_ADDR   0x3C

/* Vext: regulador que alimenta la OLED (y en la V4 tambien parte del RF).
 * Es ACTIVO EN BAJO: hay que ponerlo en LOW antes de tocar la pantalla. Es el
 * olvido clasico con estas placas: "la OLED no enciende". */
#define VEXT_PIN        36
#define VEXT_ON         LOW
#define VEXT_OFF        HIGH

/* ---------------------------------------------------------------------------
 * Medida de bateria: divisor 390k/100k sobre GPIO1 (ADC1_CH0), habilitado por
 * GPIO37. Vbat = Vadc * (390+100)/100 = Vadc * 4.9
 * ------------------------------------------------------------------------- */
#define VBAT_ADC_PIN     1
#define ADC_CTRL_PIN    37
#define VBAT_DIVIDER    4.9f
#if HELTEC_BOARD_VERSION == 3
#define ADC_CTRL_ON     LOW     /* V3: el transistor del divisor se activa en bajo */
#define ADC_CTRL_OFF    HIGH
#else
#define ADC_CTRL_ON     HIGH    /* V4: polaridad INVERTIDA respecto a la V3  */
#define ADC_CTRL_OFF    LOW
#endif

/* ---------------------------------------------------------------------------
 * LED de usuario. En la V3 es un LED normal; en la V4 es un WS2812 (RGB
 * direccionable) que necesita su propia libreria, asi que en la V4 no se usa.
 * ------------------------------------------------------------------------- */
#if HELTEC_BOARD_VERSION == 3
#define LED_PIN         35
#endif

/* ---------------------------------------------------------------------------
 * V4: amplificador de RF (FEM) entre el SX1262 y la antena.
 *
 *  V4.2 lleva un GC1109 y V4.3 un KCT8103L. Comparten el pin de alimentacion
 *  (GPIO7 -> LDO Vfem) y el de habilitacion (GPIO2 -> CSD). Los otros pines
 *  de control no coinciden, asi que se configuran AMBOS: el que no exista en
 *  tu revision queda sin conectar y no molesta.
 *
 *    GC1109  (V4.2): CPS = GPIO46 -> HIGH = PA completo, LOW = bypass.
 *                    CTX = DIO2 del SX1262 (conmuta TX/RX solo).
 *    KCT8103L(V4.3): CTX = GPIO5  -> HIGH = TX con PA / RX en bypass,
 *                                    LOW  = RX con LNA.
 *                    CPS = DIO2 del SX1262 (conmuta TX/RX solo).
 *
 *  Con GPIO7=HIGH, GPIO2=HIGH, GPIO46=HIGH y GPIO5=HIGH las dos revisiones
 *  transmiten a traves del PA. El PA anade unos +11 dB, por eso en la V4 el
 *  SX1262 se ajusta a una potencia BAJA (ver PICARO_TX_POWER_DBM).
 * ------------------------------------------------------------------------- */
#if HELTEC_BOARD_VERSION == 4
#define FEM_VCC_PIN      7    /* Vfem_Ctrl: alimenta el amplificador         */
#define FEM_CSD_PIN      2    /* CSD: habilita el chip (HIGH = encendido)     */
#define FEM_CPS_PIN     46    /* GC1109  CPS: HIGH = PA completo              */
#define FEM_CTX_PIN      5    /* KCT8103L CTX: HIGH = TX PA / RX bypass       */
#endif

/* ---------------------------------------------------------------------------
 * Potencia de salida por defecto del SX1262 (dBm) segun la placa.
 *   V3: 14 dBm directos a la antena. Sobra para un salon de clase.
 *   V4:  2 dBm al PA (+11 dB) => unos 13 dBm en la antena. NO subas esto a
 *        ciegas: 22 dBm al PA salen como ~28 dBm y saturan el receptor del
 *        gateway si esta a pocos metros.
 * Si PICARO_TX_POWER_DBM esta definido en config_lorawan.h, manda ese valor.
 * ------------------------------------------------------------------------- */
#ifndef PICARO_TX_POWER_DBM
#if HELTEC_BOARD_VERSION == 3
#define PICARO_TX_POWER_DBM   14
#else
#define PICARO_TX_POWER_DBM    2
#endif
#endif

/* Nombre legible de la placa para la OLED y el Monitor Serie. */
#if HELTEC_BOARD_VERSION == 3
#define BOARD_NAME "Heltec LoRa32 V3"
#else
#define BOARD_NAME "Heltec LoRa32 V4"
#endif
