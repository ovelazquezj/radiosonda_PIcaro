/**
 * ============================================================================
 *  app_main.cpp  -  Radiosonda PICARO FULL (LILYGO T-Beam Supreme / ESP32-S3)
 * ============================================================================
 *
 *  Flujo:
 *    1) PMU AXP2101 (energia a radio/GPS/sensores/SD)   -> va PRIMERO
 *    2) Buses I2C0 / SPI3 y sensores (los activos por sensores_config.h)
 *    3) OLED + microSD
 *    4) Radio SX1262 + JOIN OTAA a la red LoRaWAN
 *    5) Bucle: leer telemetria -> uplink (subset) -> CSV completo en SD -> OLED
 *
 *  Que se envia y que se guarda lo controlas en sensores_config.h.
 * ============================================================================
 */
#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_idf_version.h"
#include "nvs_flash.h"

#include "board_pins.h"
#include "config_lorawan.h"
#include "sensores_config.h"
#include "pmu_axp2101.h"
#include "i2c0.h"
#include "spi3.h"
#include "radio.h"
#include "lorawan.h"
#include "telemetry.h"
#include "bme280.h"
#include "gps_l76k.h"
#include "qmc6310.h"
#include "qmi8658.h"
#include "pcf8563.h"
#include "oled_sh1106.h"
#include "sdcard.h"
#include "wifi_demo.h"

static const char *TAG = "picaro";

static void nvs_bootstrap(void)
{
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

extern "C" void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(200));
    printf("\n#############################################\n");
    printf("#   RADIOSONDA PICARO FULL - arrancando...  #\n");
    printf("#   LILYGO T-Beam Supreme (ESP32-S3+SX1262) #\n");
    printf("#############################################\n");

    esp_chip_info_t chip; esp_chip_info(&chip);
    ESP_LOGI(TAG, "ESP-IDF v%s · %d nucleos · rev %d", esp_get_idf_version(), chip.cores, chip.revision);

    /* 1) PMU primero (da energia al radio/GPS/SD/sensores). */
    if (pmu_init() != ESP_OK) ESP_LOGE(TAG, "PMU fallo (el radio podria no encender)");
    pmu_charge_led_blink();   /* LED del PMU parpadeando 1 Hz = "estoy vivo" */

    /* 2) Buses y sensores. */
    i2c0_init();
    i2c0_scan();              /* diagnostico: que hay realmente en el bus I2C0 */
#if USE_BME280
    bme280_init();
#endif
#if USE_QMC6310
    qmc6310_init();
#endif
#if USE_PCF8563
    pcf8563_init();
#endif
#if USE_L76K_GPS
    gps_init();
#endif

    /* 3) OLED + microSD.
     * OJO con el ORDEN: la microSD va ANTES que el IMU QMI8658. Ambos comparten
     * el bus SPI3 (MISO=37). Al encender, la tarjeta SD "ocupa" MISO hasta que se
     * inicializa; si el IMU se lee antes, recibe basura (WHO_AM_I salia 0x3E).
     * Inicializando la SD primero, esta libera el bus y el IMU ya lee bien. */
#if USE_SH1106_OLED
    oled_init();
    oled_line(0, "PICARO FULL");
    oled_line(1, "Arrancando...");
#endif
#if USE_SDCARD
    if (sdcard_init() == ESP_OK) {
        char hdr[256];
        telemetry_csv_header(hdr, sizeof hdr);
        if (sdcard_log_is_new()) sdcard_log_line(hdr);   /* cabecera si el CSV es nuevo */
        ESP_LOGI(TAG, "Caja negra (SD) activa. Columnas CSV:");
        ESP_LOGI(TAG, "  %s", hdr);
    } else {
        ESP_LOGE(TAG, "Caja negra (SD) NO disponible: no se pudo montar la microSD.");
    }
#endif
#if USE_QMI8658
    qmi8658_init();   /* DESPUES de la SD (ver nota de arriba sobre MISO) */
#endif

#if USE_WIFI_DEMO
    /* Demo opcional: un escaneo WiFi antes del join (no interfiere el timing). */
    wifi_demo_scan();
#endif

    /* 4) Radio + JOIN OTAA. */
    int rst = picaro_radio_begin_test();
    if (rst != RADIOLIB_ERR_NONE) ESP_LOGE(TAG, "Radio begin=%d", rst);

    nvs_bootstrap();
    ESP_LOGI(TAG, "======= DAR DE ALTA EN CHIRPSTACK =======");
    ESP_LOGI(TAG, "  DevEUI  (MSB): 70 B3 D5 7E D0 09 09 01");
    ESP_LOGI(TAG, "  JoinEUI (MSB): 00 00 00 00 00 00 00 00");
    ESP_LOGI(TAG, "  US915 sub-banda %d · LoRaWAN 1.0.x · fPort %d", PICARO_SUBBAND, PICARO_UPLINK_FPORT);
    ESP_LOGI(TAG, "=========================================");
#if USE_SH1106_OLED
    oled_line(1, "Uniendo OTAA...");
#endif

    lorawan_join_t js = (rst == RADIOLIB_ERR_NONE) ? lorawan_join() : LORAWAN_JOIN_FAIL;
    bool joined  = (js != LORAWAN_JOIN_FAIL);
    /* "enlace confirmado" solo es verdad con un join NUEVO por aire; una sesion
     * restaurada NO se confirma hasta recibir un ACK/downlink (loop abajo). */
    bool link_ok = (js == LORAWAN_JOIN_NEW);
    const char *join_txt = (js == LORAWAN_JOIN_NEW)      ? "JOIN NUEVO OK" :
                           (js == LORAWAN_JOIN_RESTORED) ? "SESION REST."  : "JOIN FALLIDO";
    ESP_LOGI(TAG, "Estado join: %s", join_txt);
#if USE_SH1106_OLED
    oled_line(1, join_txt);
#endif

    /* 5) Bucle de telemetria. */
    uint32_t n = 0, sent = 0;
    while (true) {
        telemetry_t tel;
        telemetry_collect(&tel, (uint32_t)(esp_log_timestamp() / 1000));

        uint8_t payload[32];
        size_t len = telemetry_build_payload(&tel, payload, sizeof payload);

        ESP_LOGI(TAG, "---- ciclo #%lu ----", (unsigned long) n);
        ESP_LOGI(TAG, "  bat %u mV(%u%%) USB=%d | %.2fC %.1fhPa | GPS %s sats=%u | payload=%uB",
                 tel.batt_mv, tel.batt_pct, (int)tel.usb, tel.temp_c, tel.press_hpa,
                 !tel.gps_active ? "OFF" : tel.gps_fix ? "FIX" : "ON", tel.sats, (unsigned) len);
        ESP_LOGI(TAG, "  GPS: %s", !tel.gps_active ? "APAGADO / no responde (revisa boton)"
                                    : tel.gps_fix ? "ACTIVO con FIX" : "ACTIVO (buscando fix)");
        ESP_LOGI(TAG, "  perifericos: SD=%d OLED=%d BME280=%d joined=%d",
                 (int) sdcard_present(), (int) oled_present(), (int) bme280_present(), (int) joined);
        ESP_LOGI(TAG, "  I2C0 ACK: 0x1C(QMC)=%d 0x3C=%d 0x3D=%d 0x77(BME)=%d",
                 (int) i2c0_probe(0x1C), (int) i2c0_probe(0x3C),
                 (int) i2c0_probe(0x3D), (int) i2c0_probe(0x77));
#if USE_QMI8658
        ESP_LOGI(TAG, "  IMU QMI8658: present=%d whoami=0x%02X (0x05=OK)",
                 (int) qmi8658_present(), qmi8658_whoami());
#endif

        if (joined) {
            /* Pide ACK (uplink confirmado) mientras el enlace NO este confirmado,
             * y luego cada 5 ciclos para re-verificar. Solo un ACK/downlink real
             * (del gateway, via ChirpStack) marca "ENLACE OK" -> nada de joins falsos. */
            bool confirm = (!link_ok) || (n % 5 == 0);
            int r = lorawan_send(payload, (uint8_t) len, PICARO_UPLINK_FPORT, confirm);
            if (r > 0) { link_ok = true; sent++; }
            else if (r == 0) { sent++; if (confirm) link_ok = false; }
        }
        ESP_LOGI(TAG, "  ENLACE: %s", link_ok ? "OK (ACK/downlink real recibido)"
                                              : "SIN ENLACE (sin ACK del gateway)");

#if USE_SDCARD
        /* CAJA NEGRA: se registra SIEMPRE, haya o no join/enlace. Guarda incluso
         * si el GPS estaba apagado (columna gps_active=0). */
        {
            char line[256];
            telemetry_csv_line(&tel, line, sizeof line);
            esp_err_t se = sdcard_log_line(line);
            ESP_LOGI(TAG, "  SD(caja negra) %s: %s", (se == ESP_OK) ? "OK" : "FALLO", line);
        }
#endif
#if USE_SH1106_OLED
        {
            char l[24];
            snprintf(l, sizeof l, "%s up:%lu", link_ok ? "ENLACE OK" : "SIN ENLACE",
                     (unsigned long) sent);
            oled_line(2, l);
            if (!tel.gps_active)  snprintf(l, sizeof l, "GPS: APAGADO");
            else if (tel.gps_fix) snprintf(l, sizeof l, "GPS FIX sat:%u", tel.sats);
            else                  snprintf(l, sizeof l, "GPS ON s/fix s:%u", tel.sats);
            oled_line(3, l);
            snprintf(l, sizeof l, "T:%.1fC P:%.0f", tel.temp_c, tel.press_hpa);
            oled_line(4, l);
        }
#endif
        n++;
        vTaskDelay(pdMS_TO_TICKS(PICARO_UPLINK_INTERVAL_S * 1000));
    }
}
