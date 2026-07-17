/**
 * ============================================================================
 *  qmi8658.c  -  Driver QMI8658 por SPI3. CODIFICADO pero apagado.
 * ============================================================================
 *  Lectura SPI: bit7 del registro a 1 para leer. Config: accel +-8g,
 *  giroscopio +-512 dps, ambos habilitados en CTRL7.
 * ============================================================================
 */
#include "qmi8658.h"
#include "spi3.h"
#include "board_pins.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "qmi8658";

#define REG_WHOAMI 0x00
#define REG_CTRL1  0x02
#define REG_CTRL2  0x03
#define REG_CTRL3  0x04
#define REG_CTRL7  0x08
#define REG_STATUS0 0x2E
#define REG_TEMP_L 0x33
#define REG_AX_L   0x35
#define REG_RESET  0x60
#define RESET_VAL  0xB0

#define ACC_SENS  4096.0f   /* LSB/g   a +-8g  */
#define GYR_SENS  64.0f     /* LSB/dps a +-512 */

static spi_device_handle_t s_dev;
static bool s_present;
static uint8_t s_whoami = 0xEE;

static void qmi_read(uint8_t reg, uint8_t *buf, size_t n)
{
    /* WORD_ALIGNED_ATTR: el bus SPI3 usa DMA; los buffers deben ir alineados a
     * 4 bytes o el DMA lee/escribe basura (por eso el WHO_AM_I salia 0x3E). */
    WORD_ALIGNED_ATTR uint8_t tx[16] = {0}, rx[16] = {0};
    tx[0] = reg | 0x80;
    spi_transaction_t t = {};
    t.length = 8 * (n + 1);
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    spi_device_polling_transmit(s_dev, &t);
    memcpy(buf, rx + 1, n);
}
static void qmi_write(uint8_t reg, uint8_t val)
{
    WORD_ALIGNED_ATTR uint8_t tx[4] = { (uint8_t)(reg & 0x7F), val, 0, 0 };
    spi_transaction_t t = {};
    t.length = 16;
    t.tx_buffer = tx;
    spi_device_polling_transmit(s_dev, &t);
}

esp_err_t qmi8658_init(void)
{
    esp_err_t e = spi3_init();
    if (e != ESP_OK) return e;
    spi_device_interface_config_t dc = {};
    dc.clock_speed_hz = 1000000;
    dc.mode = 0;                  /* QMI8658 admite modo 0 o 3 */
    dc.spics_io_num = IMU_CS_PIN;
    dc.queue_size = 1;
    if (spi_bus_add_device(SPI3_HOST, &dc, &s_dev) != ESP_OK) return ESP_FAIL;

    /* WHO_AM_I con reintentos (la 1a lectura SPI tras encender puede fallar). */
    for (int i = 0; i < 10; i++) {
        qmi_read(REG_WHOAMI, &s_whoami, 1);
        if (s_whoami == 0x05) break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (s_whoami != 0x05) {
        ESP_LOGE(TAG, "WHO_AM_I=0x%02X (esperaba 0x05)", s_whoami);
        return ESP_FAIL;
    }

    qmi_write(REG_CTRL1, 0x40);   /* SPI 4 hilos + auto-increment de direccion (bit6) */
    qmi_write(REG_CTRL2, 0x25);   /* accel +-8g @250Hz  (range<<4 | odr = 0x2|0x5) */
    qmi_write(REG_CTRL3, 0x55);   /* gyro  +-512dps @250Hz */
    qmi_write(REG_CTRL7, 0x03);   /* habilita accel (bit0) + gyro (bit1) */
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t c1=0,c2=0,c7=0,stat=0;
    qmi_read(REG_CTRL1,&c1,1); qmi_read(REG_CTRL2,&c2,1);
    qmi_read(REG_CTRL7,&c7,1); qmi_read(REG_STATUS0,&stat,1);
    ESP_LOGI(TAG, "QMI8658 OK: CTRL1=0x%02X CTRL2=0x%02X CTRL7=0x%02X STATUS0=0x%02X",
             c1, c2, c7, stat);
    s_present = true;
    return ESP_OK;
}

bool qmi8658_present(void) { return s_present; }
uint8_t qmi8658_whoami(void) { return s_whoami; }

esp_err_t qmi8658_read(imu_sample_t *s)
{
    if (!s_present) return ESP_FAIL;
    uint8_t d[12];
    uint8_t tmp[2];
    qmi_read(REG_TEMP_L, tmp, 2);
    qmi_read(REG_AX_L, d, 12);
    int16_t ax = (int16_t)(d[1] << 8 | d[0]);
    int16_t ay = (int16_t)(d[3] << 8 | d[2]);
    int16_t az = (int16_t)(d[5] << 8 | d[4]);
    int16_t gx = (int16_t)(d[7] << 8 | d[6]);
    int16_t gy = (int16_t)(d[9] << 8 | d[8]);
    int16_t gz = (int16_t)(d[11] << 8 | d[10]);
    s->ax = ax / ACC_SENS; s->ay = ay / ACC_SENS; s->az = az / ACC_SENS;
    s->gx = gx / GYR_SENS; s->gy = gy / GYR_SENS; s->gz = gz / GYR_SENS;
    s->temp_c = (int16_t)(tmp[1] << 8 | tmp[0]) / 256.0f;
    return ESP_OK;
}
