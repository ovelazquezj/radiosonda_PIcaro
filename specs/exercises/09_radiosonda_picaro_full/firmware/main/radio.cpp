/**
 * ============================================================================
 *  radio.cpp  -  Instancia del SX1262 (RadioLib) para el T-Beam Supreme
 * ============================================================================
 *
 *  DETALLE CLAVE (TCXO): el SX1262 del T-Beam Supreme usa un oscilador TCXO
 *  alimentado desde el pin DIO3 del propio radio. Si no se le dice a RadioLib
 *  que hay TCXO, el radio no arranca (error -707 / -706 al calibrar). Por eso
 *  a `begin()` le pasamos tcxoVoltage = 1.8 V (valor de las placas LILYGO).
 * ============================================================================
 */
#include "radio.h"
#include "board_pins.h"
#include "esp_log.h"

static const char *TAG = "radio";

/* HAL de RadioLib sobre el SPI del radio (SCK 12 / MISO 13 / MOSI 11).
 * Usa SPI2_HOST por defecto (dejamos SPI3_HOST para IMU+SD en fases 4/5). */
static EspHal *hal = new EspHal(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN);

/* Radio SX1262:  Module(hal, NSS, DIO1, RST, BUSY). */
SX1262 radio = new Module(hal, LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

int picaro_radio_begin_test(void)
{
    ESP_LOGI(TAG, "SX1262.begin(): 915 MHz, BW125, SF9, TCXO 1.8V ...");

    /* Firma: begin(freq, bw, sf, cr, syncWord, power, preamble, tcxo, useLDO) */
    int state = radio.begin(915.0,                                 /* MHz     */
                            125.0,                                 /* kHz BW  */
                            9,                                     /* SF      */
                            7,                                     /* CR 4/7  */
                            RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                            10,                                    /* dBm     */
                            8,                                     /* preamb. */
                            1.8,                                   /* TCXO V  */
                            false);                                /* DC-DC   */
    return state;
}
