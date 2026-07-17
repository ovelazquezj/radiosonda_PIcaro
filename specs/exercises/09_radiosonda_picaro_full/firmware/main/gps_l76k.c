/**
 * ============================================================================
 *  gps_l76k.c  -  Parser NMEA minimo para el Quectel L76K (UART1)
 * ============================================================================
 *  Parsea $--GGA (fix/sats/altitud) y $--RMC (lat/lon/validez/velocidad).
 *  El L76K arranca a 9600 bps hablando NMEA estandar.
 * ============================================================================
 */
#include "gps_l76k.h"
#include "board_pins.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "gps";
#define GPS_UART   UART_NUM_1
#define LINE_MAX   128

static gps_fix_t s_fix;
static SemaphoreHandle_t s_mtx;
static volatile int64_t s_last_nmea_us = 0;   /* ultima sentencia NMEA recibida */

/* Copia el campo idx (separado por comas) de la sentencia a out (permite vacios). */
static bool nmea_field(const char *s, int idx, char *out, size_t outsz)
{
    int f = 0;
    const char *p = s;
    while (f < idx) {
        p = strchr(p, ',');
        if (!p) { out[0] = 0; return false; }
        p++; f++;
    }
    const char *end = strchr(p, ',');
    if (!end) end = p + strlen(p);
    size_t n = (size_t)(end - p);
    if (n >= outsz) n = outsz - 1;
    memcpy(out, p, n);
    out[n] = 0;
    return n > 0;
}

/* ddmm.mmmm + hemisferio -> grados decimales. */
static double nmea_coord(const char *val, const char *hemi)
{
    if (!val[0]) return 0.0;
    double raw = atof(val);
    int deg = (int)(raw / 100.0);
    double min = raw - deg * 100.0;
    double dec = deg + min / 60.0;
    if (hemi[0] == 'S' || hemi[0] == 'W') dec = -dec;
    return dec;
}

static void parse_sentence(const char *s)
{
    char f1[16], f2[16], f3[16], f4[16], f5[16];
    /* Reconocer el tipo por los 3 caracteres tras "$--". */
    const char *type = (s[0] == '$') ? s + 3 : s;

    if (strncmp(type, "GGA", 3) == 0) {
        /* GGA: 1=hora 2=lat 3=N/S 4=lon 5=E/W 6=fixQ 7=sats 8=hdop 9=alt */
        nmea_field(s, 6, f1, sizeof f1);   /* fix quality */
        nmea_field(s, 7, f2, sizeof f2);   /* sats */
        nmea_field(s, 8, f3, sizeof f3);   /* hdop */
        nmea_field(s, 9, f4, sizeof f4);   /* altitud */
        xSemaphoreTake(s_mtx, portMAX_DELAY);
        s_fix.sats  = (uint8_t)atoi(f2);
        s_fix.hdop  = (float)atof(f3);
        s_fix.alt_m = (float)atof(f4);
        if (atoi(f1) == 0) s_fix.valid = false;
        xSemaphoreGive(s_mtx);
    } else if (strncmp(type, "RMC", 3) == 0) {
        /* RMC: 1=hora 2=status(A/V) 3=lat 4=N/S 5=lon 6=E/W 7=vel(nudos) */
        char st[4], lat[16], ns[4], lon[16], ew[4], spd[16];
        nmea_field(s, 2, st, sizeof st);
        nmea_field(s, 3, lat, sizeof lat);
        nmea_field(s, 4, ns, sizeof ns);
        nmea_field(s, 5, lon, sizeof lon);
        nmea_field(s, 6, ew, sizeof ew);
        nmea_field(s, 7, spd, sizeof spd);
        (void)f1; (void)f2; (void)f3; (void)f4; (void)f5;
        xSemaphoreTake(s_mtx, portMAX_DELAY);
        if (st[0] == 'A') {
            s_fix.valid = true;
            s_fix.lat = nmea_coord(lat, ns);
            s_fix.lon = nmea_coord(lon, ew);
            s_fix.speed_kmh = (float)atof(spd) * 1.852f;   /* nudos -> km/h */
        } else {
            s_fix.valid = false;
        }
        xSemaphoreGive(s_mtx);
    }
}

static void gps_task(void *arg)
{
    char line[LINE_MAX];
    int len = 0;
    uint8_t ch;
    for (;;) {
        int r = uart_read_bytes(GPS_UART, &ch, 1, pdMS_TO_TICKS(1000));
        if (r <= 0) continue;
        if (ch == '\n' || ch == '\r') {
            if (len > 5) {
                line[len] = 0;
                s_last_nmea_us = esp_timer_get_time();   /* GPS esta enviando datos */
                parse_sentence(line);
            }
            len = 0;
        } else if (len < LINE_MAX - 1) {
            line[len++] = (char)ch;
        } else {
            len = 0;   /* linea muy larga: descartar */
        }
    }
}

esp_err_t gps_init(void)
{
    s_mtx = xSemaphoreCreateMutex();

    /* Habilita el modulo (EN alto). */
    gpio_config_t en = {
        .pin_bit_mask = 1ULL << GPS_EN_PIN,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&en);
    gpio_set_level((gpio_num_t)GPS_EN_PIN, 1);

    uart_config_t cfg = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GPS_UART, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART, GPS_UART_TX_PIN, GPS_UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(gps_task, "gps", 3072, NULL, 5, NULL);
    ESP_LOGI(TAG, "L76K en UART1 (RX=%d TX=%d, %d bps). Esperando fix...",
             GPS_UART_RX_PIN, GPS_UART_TX_PIN, GPS_BAUD_RATE);
    return ESP_OK;
}

bool gps_get(gps_fix_t *out)
{
    if (!s_mtx) return false;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    *out = s_fix;
    xSemaphoreGive(s_mtx);
    return out->valid;
}

bool gps_active(void)
{
    if (s_last_nmea_us == 0) return false;
    return (esp_timer_get_time() - s_last_nmea_us) < 5000000LL;   /* datos < 5 s */
}
