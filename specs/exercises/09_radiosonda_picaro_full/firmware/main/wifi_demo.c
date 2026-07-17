/**
 * ============================================================================
 *  wifi_demo.c  -  Escaneo WiFi (demo). Solo se compila si USE_WIFI_DEMO=1,
 *  asi el binario por defecto NO enlaza la pila WiFi.
 * ============================================================================
 */
#include "sensores_config.h"

#if USE_WIFI_DEMO
#include "wifi_demo.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "wifi_demo";

void wifi_demo_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Escaneando redes WiFi...");
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));   /* bloqueante */

    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    if (n > 12) n = 12;
    wifi_ap_record_t aps[12];
    esp_wifi_scan_get_ap_records(&n, aps);
    ESP_LOGI(TAG, "Encontradas %u redes:", n);
    for (int i = 0; i < n; i++) {
        ESP_LOGI(TAG, "  %2d) %-24s  RSSI %d  ch %d", i + 1,
                 (char *) aps[i].ssid, aps[i].rssi, aps[i].primary);
    }

    /* Apaga el WiFi para no interferir con LoRaWAN. */
    esp_wifi_stop();
    esp_wifi_deinit();
}
#endif /* USE_WIFI_DEMO */
