/**
 * ============================================================================
 *  sdcard.c  -  microSD por SPI (esp_vfs_fat_sdspi) + log CSV
 * ============================================================================
 *  Doc: https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/storage/fatfs.html
 * ============================================================================
 */
#include "sdcard.h"
#include "spi3.h"
#include "board_pins.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include <sys/stat.h>
#include <stdio.h>

static const char *TAG = "sdcard";
#define MOUNT_POINT "/sdcard"
#define CSV_PATH    MOUNT_POINT "/PICARO.CSV"

static sdmmc_card_t *s_card;
static bool s_present;
static bool s_new;

esp_err_t sdcard_init(void)
{
    esp_err_t e = spi3_init();
    if (e != ESP_OK) return e;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = (gpio_num_t)SDCARD_CS_PIN;
    slot.host_id = SPI3_HOST;

    esp_vfs_fat_mount_config_t mcfg = {
        .format_if_mount_failed = true,   /* la tarjeta viene virgen -> formatear */
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    e = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot, &mcfg, &s_card);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "no pude montar la microSD (%s). Revisa tarjeta/BLDO1.", esp_err_to_name(e));
        return e;
    }
    s_present = true;

    struct stat st;
    s_new = (stat(CSV_PATH, &st) != 0);   /* true si el CSV aun no existe */

    ESP_LOGI(TAG, "microSD montada: %llu MB. CSV %s",
             ((uint64_t)s_card->csd.capacity * s_card->csd.sector_size) / (1024 * 1024),
             s_new ? "nuevo" : "existente");
    return ESP_OK;
}

bool sdcard_present(void) { return s_present; }
bool sdcard_log_is_new(void) { return s_new; }

esp_err_t sdcard_log_line(const char *line)
{
    if (!s_present) return ESP_FAIL;
    FILE *f = fopen(CSV_PATH, "a");
    if (!f) { ESP_LOGE(TAG, "no pude abrir %s", CSV_PATH); return ESP_FAIL; }
    fprintf(f, "%s\n", line);
    fclose(f);
    s_new = false;
    return ESP_OK;
}
