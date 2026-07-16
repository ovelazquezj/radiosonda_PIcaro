// EspHal — capa de abstraccion de hardware (HAL) de RadioLib para ESP-IDF v5.
//
// Adaptado del ejemplo oficial de RadioLib:
//   RadioLib/examples/NonArduino/ESP-IDF/main/EspHal.h  (licencia MIT).
// Implementa GPIO, SPI y temporizacion usando las APIs nativas de ESP-IDF,
// de modo que RadioLib funciona sin el framework Arduino.
#ifndef ESP_HAL_H
#define ESP_HAL_H

#include <cstring>

#include <RadioLib.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Host SPI y velocidad de reloj para el SX1276.
#define LORA_SPI_HOST   SPI2_HOST
#define LORA_SPI_SPEED  2000000  // 2 MHz — conservador y de sobra para el SX1276

class EspHal : public RadioLibHal {
  public:
    // Constructor: mapea los modos/estados de RadioLib a los enums de ESP-IDF.
    EspHal(int8_t sck, int8_t miso, int8_t mosi)
        : RadioLibHal(GPIO_MODE_INPUT, GPIO_MODE_OUTPUT,
                      0, 1,
                      GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE),
          spiSCK(sck), spiMISO(miso), spiMOSI(mosi) {}

    void init() {
      spiBegin();
    }

    void term() {
      spiEnd();
    }

    // ---- GPIO -------------------------------------------------------
    void pinMode(uint32_t pin, uint32_t mode) {
      if (pin == RADIOLIB_NC) return;
      gpio_config_t cfg = {};
      cfg.pin_bit_mask = 1ULL << pin;
      cfg.mode = (gpio_mode_t)mode;
      cfg.pull_up_en = GPIO_PULLUP_DISABLE;
      cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
      cfg.intr_type = GPIO_INTR_DISABLE;
      gpio_config(&cfg);
    }

    void digitalWrite(uint32_t pin, uint32_t value) {
      if (pin == RADIOLIB_NC) return;
      gpio_set_level((gpio_num_t)pin, value);
    }

    uint32_t digitalRead(uint32_t pin) {
      if (pin == RADIOLIB_NC) return 0;
      return gpio_get_level((gpio_num_t)pin);
    }

    // ---- Interrupciones --------------------------------------------
    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) {
      if (interruptNum == RADIOLIB_NC) return;
      // El servicio ISR es global; instalarlo es idempotente (si ya existe,
      // devuelve ESP_ERR_INVALID_STATE, que ignoramos a proposito).
      gpio_install_isr_service(0);
      gpio_set_intr_type((gpio_num_t)interruptNum, (gpio_int_type_t)mode);
      gpio_isr_handler_add((gpio_num_t)interruptNum, (gpio_isr_t)interruptCb, NULL);
    }

    void detachInterrupt(uint32_t interruptNum) {
      if (interruptNum == RADIOLIB_NC) return;
      gpio_isr_handler_remove((gpio_num_t)interruptNum);
      gpio_set_intr_type((gpio_num_t)interruptNum, GPIO_INTR_DISABLE);
    }

    // ---- Temporizacion ---------------------------------------------
    void delay(unsigned long ms) {
      vTaskDelay(pdMS_TO_TICKS(ms));
    }

    void delayMicroseconds(unsigned long us) {
      esp_rom_delay_us(us);
    }

    unsigned long millis() {
      return (unsigned long)(esp_timer_get_time() / 1000ULL);
    }

    unsigned long micros() {
      return (unsigned long)esp_timer_get_time();
    }

    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) {
      if (pin == RADIOLIB_NC) return 0;
      unsigned long start = micros();
      unsigned long t0 = micros();
      while (digitalRead(pin) == state) {
        if ((micros() - start) > timeout) return 0;
        t0 = micros();
      }
      return (long)(micros() - t0);
    }

    // ---- SPI --------------------------------------------------------
    void spiBegin() {
      spi_bus_config_t buscfg = {};
      buscfg.mosi_io_num = spiMOSI;
      buscfg.miso_io_num = spiMISO;
      buscfg.sclk_io_num = spiSCK;
      buscfg.quadwp_io_num = -1;
      buscfg.quadhd_io_num = -1;
      buscfg.max_transfer_sz = 0;

      spi_device_interface_config_t devcfg = {};
      devcfg.mode = 0;
      devcfg.clock_speed_hz = LORA_SPI_SPEED;
      devcfg.spics_io_num = -1;   // el CS lo maneja RadioLib por GPIO (digitalWrite)
      devcfg.queue_size = 1;

      spi_bus_initialize(LORA_SPI_HOST, &buscfg, SPI_DMA_DISABLED);
      spi_bus_add_device(LORA_SPI_HOST, &devcfg, &spiHandle);
    }

    void spiBeginTransaction() {}

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
      spi_transaction_t t;
      memset(&t, 0, sizeof(t));
      t.length = 8 * len;         // longitud en bits
      t.tx_buffer = out;
      t.rx_buffer = in;
      spi_device_polling_transmit(spiHandle, &t);
    }

    void spiEndTransaction() {}

    void spiEnd() {
      if (spiHandle) {
        spi_bus_remove_device(spiHandle);
        spi_bus_free(LORA_SPI_HOST);
        spiHandle = NULL;
      }
    }

  private:
    int8_t spiSCK, spiMISO, spiMOSI;
    spi_device_handle_t spiHandle = NULL;
};

#endif  // ESP_HAL_H
