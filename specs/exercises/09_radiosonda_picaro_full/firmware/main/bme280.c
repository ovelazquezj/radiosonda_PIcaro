/**
 * ============================================================================
 *  bme280.c  -  Driver BME280 (compensacion oficial Bosch, enteros)
 * ============================================================================
 *  Secuencia: leer chip-id (0x60) -> leer calibracion -> configurar
 *  oversampling x1 y modo forzado -> por cada lectura, disparar medida y
 *  compensar los datos crudos con las formulas del datasheet de Bosch.
 * ============================================================================
 */
#include "bme280.h"
#include "i2c0.h"
#include "board_pins.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bme280";

#define REG_ID        0xD0
#define REG_RESET     0xE0
#define REG_CTRL_HUM  0xF2
#define REG_STATUS    0xF3
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG    0xF5
#define REG_PRESS     0xF7
#define REG_CALIB0    0x88   /* 0x88..0xA1 (26 bytes) */
#define REG_CALIB_H   0xE1   /* 0xE1..0xE7 (7 bytes)  */

static i2c_master_dev_handle_t s_dev;
static bool s_present;
static int32_t t_fine;

/* Coeficientes de calibracion. */
static uint16_t dig_T1; static int16_t dig_T2, dig_T3;
static uint16_t dig_P1; static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3; static int16_t dig_H2, dig_H4, dig_H5; static int8_t dig_H6;

static esp_err_t rd(uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, pdMS_TO_TICKS(100));
}
static esp_err_t wr(uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_transmit(s_dev, b, 2, pdMS_TO_TICKS(100));
}

static esp_err_t read_calibration(void)
{
    uint8_t c[26];
    if (rd(REG_CALIB0, c, 26) != ESP_OK) return ESP_FAIL;
    dig_T1 = (uint16_t)(c[1] << 8 | c[0]);
    dig_T2 = (int16_t)(c[3] << 8 | c[2]);
    dig_T3 = (int16_t)(c[5] << 8 | c[4]);
    dig_P1 = (uint16_t)(c[7] << 8 | c[6]);
    dig_P2 = (int16_t)(c[9] << 8 | c[8]);
    dig_P3 = (int16_t)(c[11] << 8 | c[10]);
    dig_P4 = (int16_t)(c[13] << 8 | c[12]);
    dig_P5 = (int16_t)(c[15] << 8 | c[14]);
    dig_P6 = (int16_t)(c[17] << 8 | c[16]);
    dig_P7 = (int16_t)(c[19] << 8 | c[18]);
    dig_P8 = (int16_t)(c[21] << 8 | c[20]);
    dig_P9 = (int16_t)(c[23] << 8 | c[22]);
    dig_H1 = c[25];
    uint8_t h[7];
    if (rd(REG_CALIB_H, h, 7) != ESP_OK) return ESP_FAIL;
    dig_H2 = (int16_t)(h[1] << 8 | h[0]);
    dig_H3 = h[2];
    dig_H4 = (int16_t)((h[3] << 4) | (h[4] & 0x0F));
    dig_H5 = (int16_t)((h[5] << 4) | (h[4] >> 4));
    dig_H6 = (int8_t)h[6];
    return ESP_OK;
}

/* --- compensaciones oficiales Bosch (enteros) --- */
static int32_t comp_T(int32_t adc_T)
{
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t v2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
                  ((int32_t)dig_T3)) >> 14;
    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8;   /* centi-grados */
}
static uint32_t comp_P(int32_t adc_P)
{
    int64_t v1 = ((int64_t)t_fine) - 128000;
    int64_t v2 = v1 * v1 * (int64_t)dig_P6;
    v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
    v2 = v2 + (((int64_t)dig_P4) << 35);
    v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
    v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)dig_P1) >> 33;
    if (v1 == 0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    v2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + v1 + v2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)p;   /* Q24.8 Pa -> /256 = Pa */
}
static uint32_t comp_H(int32_t adc_H)
{
    int32_t v = (t_fine - ((int32_t)76800));
    v = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v)) +
           ((int32_t)16384)) >> 15) *
         (((((((v * ((int32_t)dig_H6)) >> 10) * (((v * ((int32_t)dig_H3)) >> 11) +
             ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4);
    v = (v < 0) ? 0 : v;
    v = (v > 419430400) ? 419430400 : v;
    return (uint32_t)(v >> 12);   /* Q22.10 %RH */
}

esp_err_t bme280_init(void)
{
    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR_BME280,   /* 0x77 */
        .scl_speed_hz = 400000,
    };
    if (i2c_master_bus_add_device(i2c0_bus(), &dc, &s_dev) != ESP_OK) return ESP_FAIL;

    uint8_t id = 0;
    if (rd(REG_ID, &id, 1) != ESP_OK) {
        ESP_LOGE(TAG, "BME280 no responde en 0x%02X", I2C_ADDR_BME280);
        return ESP_FAIL;
    }
    /* 0x60 = BME280, 0x58 = BMP280 (sin humedad). */
    ESP_LOGI(TAG, "detectado chip-id 0x%02X (%s)", id, id == 0x60 ? "BME280" : id == 0x58 ? "BMP280" : "?");
    if (read_calibration() != ESP_OK) return ESP_FAIL;

    wr(REG_CTRL_HUM, 0x01);    /* humedad oversampling x1 (aplicar antes de ctrl_meas) */
    wr(REG_CONFIG, 0xA0);      /* t_standby 1000ms, filtro off */
    s_present = true;
    return ESP_OK;
}

bool bme280_present(void) { return s_present; }

esp_err_t bme280_read(float *temp_c, float *press_hpa, float *hum_pct)
{
    if (!s_present) return ESP_FAIL;
    /* Modo forzado: osrs_t=x1, osrs_p=x1, mode=forced (0b001). */
    if (wr(REG_CTRL_MEAS, (0x01 << 5) | (0x01 << 2) | 0x01) != ESP_OK) return ESP_FAIL;
    vTaskDelay(pdMS_TO_TICKS(10));   /* espera a que termine la conversion */

    uint8_t d[8];
    if (rd(REG_PRESS, d, 8) != ESP_OK) return ESP_FAIL;
    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_H = ((int32_t)d[6] << 8) | d[7];

    int32_t T = comp_T(adc_T);            /* debe ir primero: fija t_fine */
    uint32_t P = comp_P(adc_P);
    uint32_t H = comp_H(adc_H);

    if (temp_c)    *temp_c = T / 100.0f;
    if (press_hpa) *press_hpa = (P / 256.0f) / 100.0f;   /* Pa -> hPa */
    if (hum_pct)   *hum_pct = H / 1024.0f;
    return ESP_OK;
}
