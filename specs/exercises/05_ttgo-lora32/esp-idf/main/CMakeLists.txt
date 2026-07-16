# El componente `main` de la aplicacion.
# radiolib y ssd1306 se descargan via el component manager (idf_component.yml).
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer nvs_flash radiolib ssd1306
)
