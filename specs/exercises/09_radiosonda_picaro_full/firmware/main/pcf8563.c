/**
 * ============================================================================
 *  pcf8563.c  -  Driver RTC PCF8563 (BCD). CODIFICADO pero apagado.
 * ============================================================================
 */
#include "pcf8563.h"
#include "pmu_axp2101.h"   /* comparte el bus I2C1 con el PMU */
#include "board_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pcf8563";
#define REG_SEC 0x02   /* 0x02..0x08: sec,min,hour,day,wday,month,year */

static i2c_master_dev_handle_t s_dev;
static bool s_present;

static uint8_t bcd2dec(uint8_t b) { return (uint8_t)((b >> 4) * 10 + (b & 0x0F)); }
static uint8_t dec2bcd(uint8_t d) { return (uint8_t)(((d / 10) << 4) | (d % 10)); }

esp_err_t pcf8563_init(void)
{
    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR_PCF8563,   /* 0x51 */
        .scl_speed_hz = 400000,
    };
    if (i2c_master_bus_add_device(pmu_i2c_bus1(), &dc, &s_dev) != ESP_OK) return ESP_FAIL;
    uint8_t reg = 0x00, v = 0;
    if (i2c_master_transmit_receive(s_dev, &reg, 1, &v, 1, pdMS_TO_TICKS(100)) != ESP_OK) {
        ESP_LOGE(TAG, "no responde"); return ESP_FAIL;
    }
    s_present = true;
    ESP_LOGI(TAG, "PCF8563 presente");
    return ESP_OK;
}

bool pcf8563_present(void) { return s_present; }

esp_err_t pcf8563_get(rtc_time_t *t)
{
    if (!s_present) return ESP_FAIL;
    uint8_t reg = REG_SEC, d[7];
    if (i2c_master_transmit_receive(s_dev, &reg, 1, d, 7, pdMS_TO_TICKS(100)) != ESP_OK) return ESP_FAIL;
    t->sec  = bcd2dec(d[0] & 0x7F);
    t->min  = bcd2dec(d[1] & 0x7F);
    t->hour = bcd2dec(d[2] & 0x3F);
    t->day  = bcd2dec(d[3] & 0x3F);
    t->mon  = bcd2dec(d[5] & 0x1F);
    t->year = 2000 + bcd2dec(d[6]);
    return ESP_OK;
}

esp_err_t pcf8563_set(const rtc_time_t *t)
{
    if (!s_present) return ESP_FAIL;
    uint8_t b[8] = {
        REG_SEC,
        dec2bcd(t->sec), dec2bcd(t->min), dec2bcd(t->hour),
        dec2bcd(t->day), 0x00 /*wday*/, dec2bcd(t->mon),
        dec2bcd((uint8_t)(t->year % 100)),
    };
    return i2c_master_transmit(s_dev, b, sizeof b, pdMS_TO_TICKS(100));
}
