// radiosonda_PIcaro — ejemplo educativo original (NO forma parte de Semtech SWL2001).
// Provisto bajo Clear BSD License. (c) 2026 radiosonda_PIcaro contributors.
//
// Configuracion de hardware y credenciales del TTGO ESP32 LoRa v1 (SX1276 + OLED).
#pragma once

// =====================================================================
// PINES — TTGO ESP32 LoRa v1 (identicos al sketch Arduino)
// =====================================================================
// Radio SX1276 por SPI
#define PIN_LORA_SCK    5    // SPI Clock
#define PIN_LORA_MISO   19   // SPI Master In Slave Out
#define PIN_LORA_MOSI   27   // SPI Master Out Slave In
#define PIN_LORA_NSS    18   // Chip Select (NSS)
#define PIN_LORA_RST    14   // Reset del modulo
#define PIN_LORA_DIO0   26   // DIO0: TxDone / RxDone  (IRQ principal de RadioLib)
#define PIN_LORA_DIO1   33   // DIO1: RxTimeout        (gpio auxiliar de RadioLib)
// DIO2 (GPIO32) no lo necesita RadioLib para LoRaWAN Clase A.

// Display OLED SSD1306 por I2C
#define USE_OLED        1    // pon 0 para compilar sin OLED (y quita la dep en idf_component.yml)
#define PIN_OLED_SDA    21
#define PIN_OLED_SCL    22
#define PIN_OLED_RST    -1   // el TTGO v1 no expone el reset del OLED

// =====================================================================
// TEMPORIZACION
// =====================================================================
#define TX_INTERVAL_S   30   // segundos entre uplinks (igual que el sketch)

// =====================================================================
// CREDENCIALES LoRaWAN — ORDEN NATURAL (MSB), tal como en ChirpStack
// =====================================================================
// IMPORTANTE: a diferencia de LMIC (sketch Arduino), RadioLib toma DevEUI y
// JoinEUI en su orden natural de lectura (MSB) — NO se invierten. El AppKey
// tampoco se invierte. Son exactamente los valores de credentials.json.
//
//   DevEUI  (MSB): 02389205358e71db
//   JoinEUI (MSB): 505246f87143fd8a
//   AppKey  (MSB): 8ac583dfeec76c81ffd19ccfe76b73bf
#define JOIN_EUI  0x505246F87143FD8AULL
#define DEV_EUI   0x02389205358E71DBULL
#define APP_KEY   { 0x8A, 0xC5, 0x83, 0xDF, 0xEE, 0xC7, 0x6C, 0x81, \
                    0xFF, 0xD1, 0x9C, 0xCF, 0xE7, 0x6B, 0x73, 0xBF }
