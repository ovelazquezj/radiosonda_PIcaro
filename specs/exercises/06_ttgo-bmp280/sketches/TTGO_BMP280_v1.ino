// radiosonda_PIcaro — sketch educativo original (NO forma parte de Semtech SWL2001).
// Provisto bajo Clear BSD License. (c) 2026 radiosonda_PIcaro contributors.
/*
 * =====================================================================
 * Ejercicio 06 — TTGO ESP32 LoRa v1 + BMP280  (OTAA, US915)
 * Envía temperatura y presión del BMP280 cada 60 segundos por LoRaWAN.
 * =====================================================================
 * Librerías (Arduino Library Manager):
 *   - MCCI LoRaWAN LMIC library
 *   - U8g2 (OLED)
 *   - Adafruit BMP280 Library  (+ Adafruit Unified Sensor)
 *
 * Config LMIC (arduino_lmic_project_config.h):  #define CFG_us915 1 , #define CFG_sx1276_radio 1
 * =====================================================================
 */
#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>
#include <Adafruit_BMP280.h>

// ---------------------------------------------------------------------
// Pines LoRa (SX1276) — TTGO ESP32 LoRa v1
// ---------------------------------------------------------------------
#define LORA_SCK   5
#define LORA_MISO  19
#define LORA_MOSI  27
#define LORA_SS    18
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_DIO1  33   // en algunas placas requiere puente físico del pad DIO1 a GPIO33
#define LORA_DIO2  32

const lmic_pinmap lmic_pins = {
    .nss  = LORA_SS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst  = LORA_RST,
    .dio  = {LORA_DIO0, LORA_DIO1, LORA_DIO2},
};

// ---------------------------------------------------------------------
// I2C compartido (OLED 0x3C + BMP280 0x76) — SDA=21, SCL=22
// ---------------------------------------------------------------------
#define I2C_SDA 21
#define I2C_SCL 22
#define BMP280_ADDR 0x76        // SDO->GND. Si SDO->VDD, usar 0x77.

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Adafruit_BMP280 bmp;            // usa Wire por defecto

// ---------------------------------------------------------------------
// Credenciales OTAA  (LAB — reemplazar en producción)
//   ChirpStack (MSB): DevEUI aabbccdd60915001 | JoinEUI aabbccddeeff0000
//                     AppKey 60156015601560156015601560156015
//   En LMIC: DevEUI y AppEUI en LSB; AppKey en MSB.
// ---------------------------------------------------------------------
static const u1_t PROGMEM DEVEUI[8]  = { 0x01, 0x50, 0x91, 0x60, 0xDD, 0xCC, 0xBB, 0xAA };
static const u1_t PROGMEM APPEUI[8]  = { 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };
static const u1_t PROGMEM APPKEY[16] = { 0x60, 0x15, 0x60, 0x15, 0x60, 0x15, 0x60, 0x15,
                                         0x60, 0x15, 0x60, 0x15, 0x60, 0x15, 0x60, 0x15 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

// ---------------------------------------------------------------------
// Estado
// ---------------------------------------------------------------------
static osjob_t sendjob;
const unsigned TX_INTERVAL = 60;     // segundos entre uplinks
bool  joined = false;
float lastT = 0, lastP = 0;
uint32_t sentCount = 0;

void updateDisplay(const char* status) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(0, 10, "TTGO + BMP280");
    u8g2.drawStr(0, 24, status);
    char line[24];
    snprintf(line, sizeof(line), "T: %.2f C", lastT);   u8g2.drawStr(0, 38, line);
    snprintf(line, sizeof(line), "P: %.1f hPa", lastP);  u8g2.drawStr(0, 50, line);
    snprintf(line, sizeof(line), "TX: %lu", (unsigned long)sentCount); u8g2.drawStr(0, 62, line);
    u8g2.sendBuffer();
}

// ---------------------------------------------------------------------
// Lee el BMP280 y envía 5 bytes: version(0x01) | temp(int16 BE, C*100) | pres(uint16 BE, hPa)
// ---------------------------------------------------------------------
void do_send(osjob_t* j) {
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, uplink en curso"));
        return;
    }
    lastT = bmp.readTemperature();          // grados C
    lastP = bmp.readPressure() / 100.0;     // Pa -> hPa

    int16_t  ti = (int16_t)  lround(lastT * 100.0);
    uint16_t pi = (uint16_t) lround(lastP);

    uint8_t payload[5];
    payload[0] = 0x01;
    payload[1] = (ti >> 8) & 0xFF;  payload[2] = ti & 0xFF;
    payload[3] = (pi >> 8) & 0xFF;  payload[4] = pi & 0xFF;

    LMIC_setTxData2(2, payload, sizeof(payload), 0);   // fPort 2
    Serial.printf("Encolado: T=%.2fC P=%.1fhPa -> %02X%02X%02X%02X%02X\n",
                  lastT, lastP, payload[0], payload[1], payload[2], payload[3], payload[4]);
    updateDisplay("Enviando...");
}

void onEvent(ev_t ev) {
    switch (ev) {
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));  updateDisplay("Joining...");  break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            joined = true;
            LMIC_setLinkCheckMode(0);         // ADR sin link-check
            updateDisplay("Joined!");
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));  updateDisplay("Join FAILED"); break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));
            sentCount++;
            updateDisplay("TX done");
            // programa el siguiente envío en TX_INTERVAL segundos
            os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
            break;
        default:
            Serial.printf("Evento LMIC: %d\n", (int)ev);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.println(F("\n== TTGO + BMP280 (OTAA US915) =="));

    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.begin();
    updateDisplay("Init...");

    if (!bmp.begin(BMP280_ADDR)) {
        Serial.println(F("ERROR: BMP280 no detectado (revisa cableado y direccion 0x76)"));
        updateDisplay("BMP280 ERROR");
    } else {
        // Modo normal, oversampling estándar
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                        Adafruit_BMP280::SAMPLING_X2,   // temp
                        Adafruit_BMP280::SAMPLING_X16,  // presión
                        Adafruit_BMP280::FILTER_X16,
                        Adafruit_BMP280::STANDBY_MS_500);
        Serial.println(F("BMP280 OK"));
    }

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    os_init();
    LMIC_reset();
    LMIC_selectSubBand(1);                         // US915 sub-banda 2 (canales 8-15) = FSB2 del gateway
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); // tolerancia del reloj del ESP32

    do_send(&sendjob);                             // dispara join + primer uplink
}

void loop() {
    os_runloop_once();
}
