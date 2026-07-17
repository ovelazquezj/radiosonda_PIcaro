/**
 * ============================================================================
 *  pmu_axp2101.c  -  Driver minimo del AXP2101 en ESP-IDF (I2C nuevo, 5.x)
 * ============================================================================
 *
 *  Escrito a nivel de registro (sin librerias externas) para que se vea COMO
 *  se enciende cada riel. Mapa de registros tomado del AXP2101 (XPowersLib):
 *     0x00 STATUS1   0x01 STATUS2   0x03 CHIP_ID
 *     0x30 ADC ch    0x34/0x35 VBAT ADC (mV)   0xA4 % bateria
 *     0x90 LDO on/off0 (ALDO1..4=bit0..3, BLDO1=bit4, BLDO2=bit5)
 *     0x92..0x97 voltajes ALDO1,ALDO2,ALDO3,ALDO4,BLDO1,BLDO2
 *  Voltaje ALDO/BLDO: val = (mV - 500)/100  (0.5V..3.5V, paso 100 mV).
 *  Doc I2C master (IDF 5.x):
 *  https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/peripherals/i2c.html
 * ============================================================================
 */
#include "pmu_axp2101.h"
#include "board_pins.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pmu";

/* --- registros --- */
#define AXP_STATUS1     0x00
#define AXP_STATUS2     0x01
#define AXP_CHIP_ID     0x03
#define AXP_ADC_CH_CTRL 0x30
#define AXP_ADC_VBAT_H  0x34
#define AXP_ADC_VBAT_L  0x35
#define AXP_LDO_ONOFF0  0x90
#define AXP_ALDO1_VOL   0x92
#define AXP_ALDO2_VOL   0x93
#define AXP_ALDO3_VOL   0x94
#define AXP_ALDO4_VOL   0x95
#define AXP_BLDO1_VOL   0x96
#define AXP_BLDO2_VOL   0x97
#define AXP_BAT_PERCENT 0xA4

/* --- bits de encendido en 0x90 --- */
#define EN_ALDO1 (1u << 0)
#define EN_ALDO2 (1u << 1)
#define EN_ALDO3 (1u << 2)
#define EN_ALDO4 (1u << 3)
#define EN_BLDO1 (1u << 4)

static i2c_master_bus_handle_t s_bus1;
static i2c_master_dev_handle_t s_axp;
static bool s_present;

static esp_err_t rd(uint8_t reg, uint8_t *val)
{
    return i2c_master_transmit_receive(s_axp, &reg, 1, val, 1, pdMS_TO_TICKS(100));
}
static esp_err_t wr(uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_transmit(s_axp, b, 2, pdMS_TO_TICKS(100));
}
static void set_bits(uint8_t reg, uint8_t mask)
{
    uint8_t v = 0;
    if (rd(reg, &v) == ESP_OK) {
        wr(reg, (uint8_t)(v | mask));
    }
}
static inline uint8_t vol_aldo(uint16_t mv) { return (uint8_t)((mv - 500) / 100); }

esp_err_t pmu_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = I2C1_SDA_PIN,
        .scl_io_num = I2C1_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    bus_cfg.flags.enable_internal_pullup = true;
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus1);
    if (err != ESP_OK) { ESP_LOGE(TAG, "no pude crear bus I2C1: %s", esp_err_to_name(err)); return err; }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR_AXP2101,   /* 0x34 */
        .scl_speed_hz = 400000,
    };
    err = i2c_master_bus_add_device(s_bus1, &dev_cfg, &s_axp);
    if (err != ESP_OK) { ESP_LOGE(TAG, "no pude agregar AXP2101: %s", esp_err_to_name(err)); return err; }

    uint8_t id = 0;
    if (rd(AXP_CHIP_ID, &id) != ESP_OK) {
        ESP_LOGE(TAG, "AXP2101 no responde en 0x%02X (revisa I2C1 SDA=%d SCL=%d)",
                 I2C_ADDR_AXP2101, I2C1_SDA_PIN, I2C1_SCL_PIN);
        return ESP_FAIL;
    }
    s_present = true;
    ESP_LOGI(TAG, "AXP2101 detectado (chip id 0x%02X)", id);

    /* Voltajes 3.3V y encendido de rieles (mapeo T-Beam Supreme). */
    wr(AXP_ALDO3_VOL, vol_aldo(3300));   /* LoRa  */
    wr(AXP_ALDO4_VOL, vol_aldo(3300));   /* GPS   */
    wr(AXP_ALDO1_VOL, vol_aldo(3300));   /* sensores */
    wr(AXP_ALDO2_VOL, vol_aldo(3300));   /* sensores/OLED */
    wr(AXP_BLDO1_VOL, vol_aldo(3300));   /* microSD */
    set_bits(AXP_LDO_ONOFF0, EN_ALDO3 | EN_ALDO4 | EN_ALDO1 | EN_ALDO2 | EN_BLDO1);

    /* Habilita el ADC de bateria (bit0 de 0x30). */
    set_bits(AXP_ADC_CH_CTRL, 0x01);

    ESP_LOGI(TAG, "Rieles ON @3.3V: ALDO3=LoRa ALDO4=GPS ALDO1/2=sensores BLDO1=SD");
    vTaskDelay(pdMS_TO_TICKS(50));   /* dejar estabilizar antes de tocar el radio */
    return ESP_OK;
}

bool pmu_present(void) { return s_present; }

i2c_master_bus_handle_t pmu_i2c_bus1(void) { return s_bus1; }

uint16_t pmu_battery_mv(void)
{
    uint8_t h = 0, l = 0;
    if (rd(AXP_ADC_VBAT_H, &h) != ESP_OK || rd(AXP_ADC_VBAT_L, &l) != ESP_OK) return 0;
    return (uint16_t)(((h & 0x3F) << 8) | l);
}

uint8_t pmu_battery_percent(void)
{
    uint8_t p = 0;
    if (rd(AXP_BAT_PERCENT, &p) != ESP_OK) return 0xFF;
    return (p > 100) ? 0xFF : p;
}

bool pmu_usb_present(void)
{
    uint8_t s = 0;
    if (rd(AXP_STATUS1, &s) != ESP_OK) return false;
    return (s >> 5) & 0x01;
}

bool pmu_charging(void)
{
    uint8_t s = 0;
    if (rd(AXP_STATUS2, &s) != ESP_OK) return false;
    return ((s >> 5) & 0x03) == 0x01;
}

/* CHGLED (reg 0x69): control manual + parpadeo 1 Hz. Igual que XPowersLib:
 *   val = (val & 0xC8) | 0x05 (manual) | (mode<<4); mode 1 = 1 Hz. */
#define AXP_CHGLED 0x69
void pmu_charge_led_blink(void)
{
    if (!s_present) return;
    uint8_t v = 0;
    if (rd(AXP_CHGLED, &v) != ESP_OK) return;
    v &= 0xC8;
    v |= 0x05;
    v |= (1u << 4);   /* modo 1 = parpadeo 1 Hz */
    wr(AXP_CHGLED, v);
}
