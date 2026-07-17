/**
 * ============================================================================
 *  board_pins.h  -  Pinout de la LILYGO T-Beam Supreme (ESP32-S3 + SX1262)
 * ============================================================================
 *
 *  Fuente autoritativa: utilities.h del ejemplo oficial de LILYGO
 *  (examples/LoRaWAN/RadioLib_OTAA, macro T_BEAM_S3_SUPREME_SX1262) y la wiki
 *  oficial de LILYGO. NO cambies estos pines salvo que cambies de placa.
 *
 *  La placa tiene TRES buses independientes; respetarlos es clave:
 *    - SPI del RADIO  (SX1262)  ......... SCK 12 / MISO 13 / MOSI 11
 *    - SPI compartido IMU + microSD ..... SCK 36 / MISO 37 / MOSI 35
 *    - I2C bus 0 (sensores + OLED) ...... SDA 17 / SCL 18
 *    - I2C bus 1 (PMU AXP2101 + RTC) .... SDA 42 / SCL 41
 * ============================================================================
 */
#pragma once

/* ---------------------------------------------------------------------------
 * RADIO LoRa SX1262 (bus SPI dedicado)
 * ------------------------------------------------------------------------- */
#define LORA_SCK_PIN    12
#define LORA_MISO_PIN   13
#define LORA_MOSI_PIN   11
#define LORA_CS_PIN     10   /* NSS */
#define LORA_RST_PIN     5
#define LORA_BUSY_PIN    4
#define LORA_DIO1_PIN    1   /* IRQ del radio -> ISR */

/* ---------------------------------------------------------------------------
 * GNSS Quectel L76K (UART). OJO con la direccion:
 *   - El ESP32-S3 RECIBE del GPS por GPIO9  -> UART RX del MCU = 9
 *   - El ESP32-S3 TRANSMITE al  GPS por GPIO8 -> UART TX del MCU = 8
 *   - GPS_EN (=7) habilita el modulo (nivel alto). PPS (=6) es el pulso 1 Hz.
 *   - El L76K arranca a 9600 bps y habla NMEA (RMC/GGA).
 * ------------------------------------------------------------------------- */
#define GPS_UART_RX_PIN  9   /* MCU RX  <- GPS TX */
#define GPS_UART_TX_PIN  8   /* MCU TX  -> GPS RX */
#define GPS_EN_PIN       7
#define GPS_PPS_PIN      6
#define GPS_BAUD_RATE    9600

/* ---------------------------------------------------------------------------
 * I2C bus 0  -> sensores ambientales / magnetometro / OLED
 *   BME280 @0x77 · QMC6310 @0x1C · OLED SH1106 @0x3C
 * ------------------------------------------------------------------------- */
#define I2C0_SDA_PIN    17
#define I2C0_SCL_PIN    18

/* ---------------------------------------------------------------------------
 * I2C bus 1  -> PMU y RTC (energia, no tocar el orden de arranque)
 *   AXP2101 @0x34 · PCF8563 @0x51
 * ------------------------------------------------------------------------- */
#define I2C1_SDA_PIN    42
#define I2C1_SCL_PIN    41
#define PMU_IRQ_PIN     40
#define RTC_INT_PIN     14

/* ---------------------------------------------------------------------------
 * SPI compartido -> IMU QMI8658 + microSD
 * ------------------------------------------------------------------------- */
#define HSPI_MOSI_PIN   35
#define HSPI_MISO_PIN   37
#define HSPI_SCK_PIN    36
#define IMU_CS_PIN      34
#define IMU_INT_PIN     33
#define SDCARD_CS_PIN   47

/* ---------------------------------------------------------------------------
 * Botones
 * ------------------------------------------------------------------------- */
#define BOOT_BUTTON_PIN  0

/* ---------------------------------------------------------------------------
 * Direcciones I2C de 7 bits (referencia).
 * OJO: el QMC6310 puede estar en 0x1C o 0x3C, y el OLED en 0x3C o 0x3D. En las
 * unidades observadas el OLED esta en 0x3D y el QMC6310 en 0x3C; los drivers
 * de oled/qmc AUTODETECTAN la direccion para evitar confundirlos.
 * ------------------------------------------------------------------------- */
#define I2C_ADDR_AXP2101   0x34
#define I2C_ADDR_PCF8563   0x51
#define I2C_ADDR_BME280    0x77
#define I2C_ADDR_QMC6310   0x1C   /* o 0x3C (autodetectado) */
#define I2C_ADDR_OLED      0x3D   /* o 0x3C (autodetectado; 0x3C suele ser el QMC) */
