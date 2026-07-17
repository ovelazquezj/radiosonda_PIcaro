/**
 * ============================================================================
 *  qmc6310.c  -  Driver QMC6310 (magnetometro). CODIFICADO pero apagado.
 * ============================================================================
 */
#include "qmc6310.h"
#include "i2c0.h"
#include "board_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "qmc6310";

#define REG_CHIPID 0x00
#define REG_DATA   0x01   /* X_L,X_H,Y_L,Y_H,Z_L,Z_H */
#define REG_STATUS 0x09
#define REG_CTRL1  0x0A
#define REG_CTRL2  0x0B

static i2c_master_dev_handle_t s_dev;
static bool s_present;

static esp_err_t rd(uint8_t reg, uint8_t *buf, size_t n)
{ return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, pdMS_TO_TICKS(100)); }
static esp_err_t wr(uint8_t reg, uint8_t val)
{ uint8_t b[2] = { reg, val }; return i2c_master_transmit(s_dev, b, 2, pdMS_TO_TICKS(100)); }

esp_err_t qmc6310_init(void)
{
    /* El QMC6310 puede estar en 0x1C o 0x3C segun la variante (U/N). En esta
     * T-Beam Supreme esta en 0x3C (el OLED se movio a 0x3D). Autoselecciona. */
    uint8_t addr = i2c0_probe(0x1C) ? 0x1C : 0x3C;
    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    ESP_LOGI(TAG, "QMC6310 en 0x%02X", addr);
    if (i2c_master_bus_add_device(i2c0_bus(), &dc, &s_dev) != ESP_OK) return ESP_FAIL;

    uint8_t id = 0;
    if (rd(REG_CHIPID, &id, 1) != ESP_OK) { ESP_LOGE(TAG, "no responde"); return ESP_FAIL; }

    /* Secuencia de arranque del QMC6310:
     *   0x29 = 0x06  -> registro de "signo" de ejes (REQUERIDO; sin el, datos malos)
     *   CR2  = 0x08  -> rango 8 Gauss, modo set/reset ON
     *   CR1  = 0xC3  -> OSR1 alto, ODR 10Hz, MODO CONTINUO (bits[1:0]=11) */
    wr(0x29, 0x06);
    wr(REG_CTRL2, 0x08);
    wr(REG_CTRL1, 0xC3);

    uint8_t c1 = 0, c2 = 0;
    rd(REG_CTRL1, &c1, 1); rd(REG_CTRL2, &c2, 1);
    ESP_LOGI(TAG, "QMC6310 OK: id=0x%02X CR1=0x%02X CR2=0x%02X", id, c1, c2);
    s_present = true;
    return ESP_OK;
}

bool qmc6310_present(void) { return s_present; }

esp_err_t qmc6310_read(float *heading_deg, float *x, float *y, float *z)
{
    if (!s_present) return ESP_FAIL;
    uint8_t d[6];
    if (rd(REG_DATA, d, 6) != ESP_OK) return ESP_FAIL;
    int16_t mx = (int16_t)(d[1] << 8 | d[0]);
    int16_t my = (int16_t)(d[3] << 8 | d[2]);
    int16_t mz = (int16_t)(d[5] << 8 | d[4]);
    if (x) *x = mx;
    if (y) *y = my;
    if (z) *z = mz;
    if (heading_deg) {
        float h = atan2f((float)my, (float)mx) * 180.0f / (float)M_PI;
        if (h < 0) h += 360.0f;
        *heading_deg = h;   /* OJO: sin calibrar hard/soft-iron */
    }
    return ESP_OK;
}
