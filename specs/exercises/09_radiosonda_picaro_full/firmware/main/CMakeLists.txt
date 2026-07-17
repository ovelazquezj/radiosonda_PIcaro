# ============================================================================
#  main/CMakeLists.txt  -  componente principal de la aplicacion
# ----------------------------------------------------------------------------
#  La dependencia RadioLib se declara en idf_component.yml (la baja el
#  IDF Component Manager). Doc build-system:
#  https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-guides/build-system.html
# ============================================================================
idf_component_register(
    SRCS
        "app_main.cpp"
        "radio.cpp"
        "lorawan.cpp"
        "telemetry.c"
        "pmu_axp2101.c"
        "i2c0.c"
        "spi3.c"
        "bme280.c"
        "gps_l76k.c"
        "qmc6310.c"
        "pcf8563.c"
        "qmi8658.c"
        "oled_sh1106.c"
        "sdcard.c"
        "wifi_demo.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        esp_timer
        nvs_flash
        fatfs
        sdmmc
        esp_driver_sdspi
        esp_wifi
        esp_netif
        esp_event
)
