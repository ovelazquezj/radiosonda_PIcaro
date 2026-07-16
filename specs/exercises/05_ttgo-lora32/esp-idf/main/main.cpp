// radiosonda_PIcaro — ejemplo educativo original (NO forma parte de Semtech SWL2001).
// Provisto bajo Clear BSD License. (c) 2026 radiosonda_PIcaro contributors.
//
// =====================================================================
// TTGO T3 LoRaWAN — VERSION ESP-IDF v5 + RadioLib
// =====================================================================
// Port del sketch Arduino sketches/TTGO_LoRaWAN_v3.ino al framework nativo
// ESP-IDF v5, usando RadioLib como stack LoRaWAN (en lugar de MCCI LMIC).
//
// Comportamiento identico al sketch Arduino:
//   - Join OTAA en US915, sub-banda 2 (FSB2, canales 8-15)
//   - Uplink de 8 bytes en fPort 1 cada 30 s
//   - Mismo formato de payload (magic 0x99 | counter | timestamp | checksum)
//   - Estado en el OLED SSD1306
//
// Diferencia clave con LMIC: RadioLib toma DevEUI/JoinEUI en orden NATURAL
// (MSB) — no hay que invertir los bytes. Ver config.h.
// =====================================================================

#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include <RadioLib.h>

#include "EspHal.h"
#include "config.h"

#if USE_OLED
#include "ssd1306.h"
static SSD1306_t oled;
#endif

static const char* TAG = "ttgo-lorawan";

// ---------------------------------------------------------------------
// HAL + radio SX1276 + nodo LoRaWAN
// ---------------------------------------------------------------------
// EspHal implementa GPIO/SPI/timing sobre ESP-IDF (ver EspHal.h).
static EspHal* hal = new EspHal(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI);

// Module(hal, NSS/CS, DIO0/IRQ, RST, DIO1/gpio)
static SX1276 radio = new Module(hal, PIN_LORA_NSS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1);

// Nodo LoRaWAN en la banda US915. El 3er argumento es la SUB-BANDA (1..8):
// 2 = FSB2 (canales 8-15) — equivale a LMIC_selectSubBand(1) del sketch Arduino.
static LoRaWANNode node(&radio, &US915, 2);

// Credenciales (orden natural MSB — ver config.h).
static uint64_t joinEUI = JOIN_EUI;
static uint64_t devEUI  = DEV_EUI;
static uint8_t  appKey[16] = APP_KEY;

static uint32_t packetCount = 0;

// ---------------------------------------------------------------------
// OLED — muestra estado, contador y banda
// ---------------------------------------------------------------------
static void oledStatus(const char* estado) {
#if USE_OLED
  char line[24];
  ssd1306_clear_screen(&oled, false);
  ssd1306_display_text(&oled, 0, (char*)"TTGO LoRaWAN", 12, false);
  ssd1306_display_text(&oled, 1, (char*)"IDF5 + RadioLib", 15, false);

  snprintf(line, sizeof(line), "Estado:%s", estado);
  ssd1306_display_text(&oled, 3, line, strlen(line), false);

  snprintf(line, sizeof(line), "Paquetes: %u", (unsigned)packetCount);
  ssd1306_display_text(&oled, 5, line, strlen(line), false);

  ssd1306_display_text(&oled, 7, (char*)"US915 FSB2", 10, false);
#else
  (void)estado;
#endif
}

// ---------------------------------------------------------------------
// Construye el payload de 8 bytes (identico al sketch Arduino)
//   [0]    magic     = 0x99
//   [1..2] counter   = uint16 little-endian
//   [3..6] timestamp = uint32 little-endian (uptime en segundos)
//   [7]    checksum  = (suma de bytes 0..6) & 0xFF
// ---------------------------------------------------------------------
static void buildPayload(uint8_t* p) {
  p[0] = 0x99;
  p[1] = packetCount & 0xFF;
  p[2] = (packetCount >> 8) & 0xFF;

  uint32_t ts = (uint32_t)(esp_timer_get_time() / 1000000ULL);  // us -> s
  p[3] = ts & 0xFF;
  p[4] = (ts >> 8) & 0xFF;
  p[5] = (ts >> 16) & 0xFF;
  p[6] = (ts >> 24) & 0xFF;

  p[7] = 0;
  for (int i = 0; i < 7; i++) p[7] += p[i];
}

// ---------------------------------------------------------------------
// app_main — punto de entrada de ESP-IDF
// ---------------------------------------------------------------------
extern "C" void app_main(void) {
  // NVS: RadioLib puede usarla para persistir sesion/nonces. Aqui solo la
  // inicializamos; este ejemplo mantiene la sesion en RAM (como el sketch).
  esp_err_t nvs = nvs_flash_init();
  if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "  TTGO T3 LoRaWAN — ESP-IDF v5 + RadioLib");
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "  Region:       US915");
  ESP_LOGI(TAG, "  Sub-banda:    2 (FSB2, canales 8-15)");
  ESP_LOGI(TAG, "  Activacion:   OTAA");
  ESP_LOGI(TAG, "  Intervalo TX: %d s", TX_INTERVAL_S);

#if USE_OLED
  i2c_master_init(&oled, PIN_OLED_SDA, PIN_OLED_SCL, PIN_OLED_RST);
  ssd1306_init(&oled, 128, 64);
  ssd1306_contrast(&oled, 0xff);
#endif
  oledStatus("JOINING");

  // 1) Inicializar el radio SX1276.
  ESP_LOGI(TAG, "Inicializando radio SX1276...");
  int16_t st = radio.begin();
  if (st != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "radio.begin() fallo (codigo %d). Revisa el cableado/antena.", st);
    while (true) vTaskDelay(pdMS_TO_TICKS(1000));
  }
  ESP_LOGI(TAG, "  Radio OK (SX1276 detectado).");

  // 2) Cargar credenciales OTAA. Para LoRaWAN 1.0.x, nwkKey = NULL y appKey = AppKey.
  node.beginOTAA(joinEUI, devEUI, NULL, appKey);

  // 3) Procedimiento de JOIN (reintenta indefinidamente, como el sketch).
  ESP_LOGI(TAG, "Iniciando JOIN OTAA (esto puede tardar 10-30 s)...");
  while (true) {
    st = node.activateOTAA();
    if (st == RADIOLIB_LORAWAN_NEW_SESSION || st == RADIOLIB_LORAWAN_SESSION_RESTORED) {
      break;
    }
    ESP_LOGW(TAG, "JOIN fallo (codigo %d). Reintento en 10 s...", st);
    ESP_LOGW(TAG, "  Verifica: gateway US915 en FSB2 online, credenciales y device en ChirpStack.");
    oledStatus("JOIN ERR");
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "  CONECTADO A CHIRPSTACK (JOINED)");
  ESP_LOGI(TAG, "========================================");
  oledStatus("JOINED");

  // 4) Bucle de uplinks periodicos.
  while (true) {
    packetCount++;

    uint8_t payload[8];
    buildPayload(payload);

    ESP_LOGI(TAG,
             "TX #%u | fPort 1 | payload: %02X %02X %02X %02X %02X %02X %02X %02X",
             (unsigned)packetCount,
             payload[0], payload[1], payload[2], payload[3],
             payload[4], payload[5], payload[6], payload[7]);

    // sendReceive: envia el uplink y abre las ventanas RX1/RX2.
    //   < 0  -> error
    //   == 0 -> enviado, sin downlink
    //   > 0  -> downlink recibido en esa ventana (1 o 2)
    int16_t rs = node.sendReceive(payload, sizeof(payload), 1);
    if (rs < RADIOLIB_ERR_NONE) {
      ESP_LOGW(TAG, "  sendReceive error (codigo %d)", rs);
    } else if (rs == 0) {
      ESP_LOGI(TAG, "  Uplink OK — sin downlink");
    } else {
      ESP_LOGI(TAG, "  Uplink OK — downlink recibido en ventana RX%d", rs);
    }

    oledStatus("JOINED");
    ESP_LOGI(TAG, "  Proximo uplink en %d s", TX_INTERVAL_S);
    vTaskDelay(pdMS_TO_TICKS((uint32_t)TX_INTERVAL_S * 1000));
  }
}
