# ============================================================================
#  Radiosonda PICARO FULL - proyecto ESP-IDF (LILYGO T-Beam Supreme, ESP32-S3)
# ----------------------------------------------------------------------------
#  Este es el CMakeLists de nivel superior de un proyecto ESP-IDF estandar.
#  No hay que editarlo como estudiante. El target se fija con:
#     idf.py set-target esp32s3
#  Doc oficial: https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-guides/build-system.html
# ============================================================================
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(radiosonda_picaro_full)
