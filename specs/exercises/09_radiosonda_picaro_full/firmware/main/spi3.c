/**
 * ============================================================================
 *  spi3.c  -  Bus SPI3 compartido (IMU + microSD)
 * ============================================================================
 */
#include "spi3.h"
#include "board_pins.h"
#include "esp_log.h"

static const char *TAG = "spi3";
static bool s_ready;

esp_err_t spi3_init(void)
{
    if (s_ready) return ESP_OK;
    spi_bus_config_t buscfg = {
        .mosi_io_num = HSPI_MOSI_PIN,
        .miso_io_num = HSPI_MISO_PIN,
        .sclk_io_num = HSPI_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t e = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (e != ESP_OK) { ESP_LOGE(TAG, "spi_bus_initialize: %s", esp_err_to_name(e)); return e; }
    s_ready = true;
    ESP_LOGI(TAG, "Bus SPI3 listo (SCK=%d MISO=%d MOSI=%d)", HSPI_SCK_PIN, HSPI_MISO_PIN, HSPI_MOSI_PIN);
    return ESP_OK;
}
