/**
 * ============================================================================
 *  i2c0.c  -  Bus I2C0 (sensores + OLED)
 * ============================================================================
 */
#include "i2c0.h"
#include "board_pins.h"
#include "esp_log.h"

static const char *TAG = "i2c0";
static i2c_master_bus_handle_t s_bus0;
static bool s_ready;

esp_err_t i2c0_init(void)
{
    if (s_ready) return ESP_OK;
    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C0_SDA_PIN,
        .scl_io_num = I2C0_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    cfg.flags.enable_internal_pullup = true;
    esp_err_t e = i2c_new_master_bus(&cfg, &s_bus0);
    if (e != ESP_OK) { ESP_LOGE(TAG, "no pude crear I2C0: %s", esp_err_to_name(e)); return e; }
    s_ready = true;
    ESP_LOGI(TAG, "Bus I2C0 listo (SDA=%d SCL=%d)", I2C0_SDA_PIN, I2C0_SCL_PIN);
    return ESP_OK;
}

i2c_master_bus_handle_t i2c0_bus(void) { return s_bus0; }

bool i2c0_probe(uint8_t addr)
{
    return s_ready && (i2c_master_probe(s_bus0, addr, 10) == ESP_OK);
}

void i2c0_scan(void)
{
    if (!s_ready) return;
    ESP_LOGI(TAG, "Escaneando bus I2C0 (esperados: BME280 0x77, QMC6310 0x1C, OLED 0x3C)...");
    int found = 0;
    for (uint8_t a = 0x08; a < 0x78; a++) {
        if (i2c_master_probe(s_bus0, a, 5) == ESP_OK) {
            ESP_LOGI(TAG, "  -> dispositivo en 0x%02X", a);
            found++;
        }
    }
    if (!found) ESP_LOGW(TAG, "  (ningun dispositivo respondio en I2C0)");
}
