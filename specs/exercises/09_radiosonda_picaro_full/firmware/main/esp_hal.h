/**
 * ============================================================================
 *  esp_hal.h  -  HAL de RadioLib para ESP-IDF (ESP32-S3) usando spi_master
 * ============================================================================
 *
 *  RadioLib abstrae el hardware con la clase base RadioLibHal. Aqui la
 *  implementamos para ESP-IDF/ESP32-S3 usando el driver oficial `spi_master`
 *  (en vez de tocar registros como el EspHal de ejemplo, que es solo para el
 *  ESP32 clasico). Asi funciona en el S3 y convive con otros dispositivos SPI.
 *
 *  Implementa los 15 metodos "puros" que exige RadioLib 7.7.x:
 *    GPIO:   pinMode digitalWrite digitalRead attach/detachInterrupt
 *    tiempo: delay delayMicroseconds millis micros pulseIn
 *    SPI:    spiBegin spiBeginTransaction spiTransfer spiEndTransaction spiEnd
 *  (init/term/tone/yield/pullUpDown tienen implementacion por defecto.)
 *
 *  Referencia HAL de RadioLib: https://jgromes.github.io/RadioLib/class_hal.html
 *  Basado en el HAL ESP-IDF oficial de RadioLib (spi_master), adaptado al S3.
 * ============================================================================
 */
#ifndef PICARO_ESP_HAL_H
#define PICARO_ESP_HAL_H

#include <RadioLib.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

/* Constantes estilo Arduino que RadioLib pasa a la HAL. */
#ifndef LOW
#define LOW     (0x0)
#endif
#ifndef HIGH
#define HIGH    (0x1)
#endif
#ifndef INPUT
#define INPUT   (0x01)
#endif
#ifndef OUTPUT
#define OUTPUT  (0x03)
#endif
#ifndef RISING
#define RISING  (0x01)
#endif
#ifndef FALLING
#define FALLING (0x02)
#endif

class EspHal : public RadioLibHal {
  public:
    EspHal(int8_t sck, int8_t miso, int8_t mosi,
           spi_host_device_t host = SPI2_HOST, uint32_t clockHz = 2000000)
        : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING),
          spiSCK(sck), spiMISO(miso), spiMOSI(mosi),
          spiHost(host), spiClockHz(clockHz),
          spiDev(nullptr), busReady(false) {}

    void init() override { spiBegin(); }
    void term() override { spiEnd(); }

    /* ---- GPIO ---- */
    void pinMode(uint32_t pin, uint32_t mode) override {
        if (pin == RADIOLIB_NC) return;
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = 1ULL << pin;
        cfg.mode = (mode == (uint32_t)OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&cfg);
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
        if (pin == RADIOLIB_NC) return;
        gpio_set_level((gpio_num_t)pin, value);
    }

    uint32_t digitalRead(uint32_t pin) override {
        if (pin == RADIOLIB_NC) return 0;
        return gpio_get_level((gpio_num_t)pin);
    }

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
        if (interruptNum == RADIOLIB_NC) return;
        /* El servicio de ISR se instala una sola vez para toda la app. */
        static bool isrServiceInstalled = false;
        if (!isrServiceInstalled) {
            gpio_install_isr_service(0);   /* ignora ESP_ERR_INVALID_STATE si ya estaba */
            isrServiceInstalled = true;
        }
        gpio_set_direction((gpio_num_t)interruptNum, GPIO_MODE_INPUT);
        gpio_int_type_t t = (mode == (uint32_t)FALLING) ? GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE;
        gpio_set_intr_type((gpio_num_t)interruptNum, t);
        /* RadioLib usa callbacks void(void); el ISR de IDF es void(void*). El
         * cast funciona porque el callback ignora el argumento. */
        gpio_isr_handler_add((gpio_num_t)interruptNum, (gpio_isr_t)interruptCb, nullptr);
        gpio_intr_enable((gpio_num_t)interruptNum);
    }

    void detachInterrupt(uint32_t interruptNum) override {
        if (interruptNum == RADIOLIB_NC) return;
        gpio_intr_disable((gpio_num_t)interruptNum);
        gpio_isr_handler_remove((gpio_num_t)interruptNum);
        gpio_set_intr_type((gpio_num_t)interruptNum, GPIO_INTR_DISABLE);
    }

    /* ---- Tiempo ---- */
    void delay(RadioLibTime_t ms) override {
        if (ms == 0) { taskYIELD(); return; }
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
    void delayMicroseconds(RadioLibTime_t us) override { esp_rom_delay_us(us); }
    RadioLibTime_t millis() override { return (RadioLibTime_t)(esp_timer_get_time() / 1000ULL); }
    RadioLibTime_t micros() override { return (RadioLibTime_t)(esp_timer_get_time()); }

    long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override {
        if (pin == RADIOLIB_NC) return 0;
        this->pinMode(pin, INPUT);
        RadioLibTime_t start = this->micros();
        RadioLibTime_t curtick = this->micros();
        while (this->digitalRead(pin) == state) {
            if ((this->micros() - curtick) > timeout) return 0;
        }
        return (long)(this->micros() - start);
    }

    /* ---- SPI (driver spi_master) ---- */
    void spiBegin() {
        if (busReady) return;
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = spiMOSI;
        buscfg.miso_io_num = spiMISO;
        buscfg.sclk_io_num = spiSCK;
        buscfg.quadwp_io_num = -1;
        buscfg.quadhd_io_num = -1;
        buscfg.max_transfer_sz = 0;   /* por defecto */
        /* Sin DMA: transferencias por CPU (hasta 64 bytes), suficiente para el
         * SX1262 y nuestra telemetria. Para payloads mayores, usar DMA. */
        spi_bus_initialize(spiHost, &buscfg, SPI_DMA_DISABLED);

        spi_device_interface_config_t devcfg = {};
        devcfg.clock_speed_hz = spiClockHz;
        devcfg.mode = 0;                 /* SPI modo 0 */
        devcfg.spics_io_num = -1;        /* el NSS lo maneja RadioLib por GPIO */
        devcfg.queue_size = 1;
        spi_bus_add_device(spiHost, &devcfg, &spiDev);
        busReady = true;
    }

    void spiBeginTransaction() {}

    void spiTransfer(uint8_t *out, size_t len, uint8_t *in) {
        if (len == 0 || spiDev == nullptr) return;
        spi_transaction_t t = {};
        t.length = 8 * len;      /* bits */
        t.tx_buffer = out;
        t.rx_buffer = in;
        spi_device_polling_transmit(spiDev, &t);
    }

    void spiEndTransaction() {}

    void spiEnd() {
        if (!busReady) return;
        spi_bus_remove_device(spiDev);
        spi_bus_free(spiHost);
        spiDev = nullptr;
        busReady = false;
    }

  private:
    int8_t spiSCK, spiMISO, spiMOSI;
    spi_host_device_t spiHost;
    uint32_t spiClockHz;
    spi_device_handle_t spiDev;
    bool busReady;
};

#endif /* PICARO_ESP_HAL_H */
