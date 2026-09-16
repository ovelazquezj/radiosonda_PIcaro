/**
 * ============================================================================
 *  EJERCICIO 12  -  JOIN OTAA con Heltec WiFi LoRa 32 (V3 / V4) + ChirpStack
 * ============================================================================
 *
 *  Que hace este programa (en palabras simples):
 *    1. Enciende la placa: pantalla OLED, radio SX1262 y medidor de bateria.
 *    2. Se "une" a la red LoRaWAN de ChirpStack con OTAA, usando el DevEUI,
 *       JoinEUI y AppKey que ChirpStack genero y tu copiaste a
 *       config_lorawan.h.
 *    3. Cada 30 s arma un paquete pequeno (contador, tiempo encendido y
 *       voltaje de bateria) y lo envia al gateway -> ChirpStack.
 *    4. Cuenta TODO lo que pasa por el Monitor Serie (115200 baudios) y
 *       resume el estado en la OLED.
 *
 *  Archivos de esta carpeta:
 *    - heltec_lora32_join.ino  -> ESTE archivo (la logica).
 *    - config_lorawan.h        -> AQUI pones la version de placa y las llaves.
 *    - board_heltec.h          -> pines de la V3 y la V4 (no se edita).
 *
 *  Librerias (Arduino IDE -> Tools -> Manage Libraries):
 *    - RadioLib (Jan Gromes) 7.x   -> radio SX1262 + pila LoRaWAN
 *    - U8g2 (oliver)               -> pantalla OLED SSD1306
 *
 *  Placa (Arduino IDE -> Tools -> Board -> esp32):
 *    - "Heltec WiFi LoRa 32(V3)"  para la V3 y para la V4 si tu core no lista
 *      la V4. Ver README, Paso 4, para las opciones de USB y particion.
 *
 *  Licencia: MIT. Basado en los ejemplos LoRaWAN de RadioLib.
 * ============================================================================
 */

#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include <Preferences.h>

#include "config_lorawan.h"   /* <-- TU version de placa y TUS credenciales */
#include "board_heltec.h"     /* pines segun HELTEC_BOARD_VERSION          */

/* Version de este sketch (se imprime al arrancar). */
#define SKETCH_VERSION "1.0"

// ===========================================================================
//  1) OBJETOS PRINCIPALES
// ===========================================================================

/* Radio SX1262:  Module(NSS, DIO1, RST, BUSY). Usa el bus SPI por defecto,
 * que en setup() se arranca con los pines de la Heltec. */
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

/* Nodo LoRaWAN: junta el radio con la region US915 y la sub-banda. */
LoRaWANNode node(&radio, &US915, PICARO_SUBBAND);

/* Credenciales, en el formato que espera RadioLib (MSB, sin invertir). */
uint64_t joinEUI  = PICARO_JOIN_EUI;
uint64_t devEUI   = PICARO_DEV_EUI;
uint8_t  appKey[] = PICARO_APP_KEY;

/* Pantalla OLED SSD1306 128x64 por I2C (rotacion 0, reset, SCL, SDA). */
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, OLED_RST_PIN, OLED_SCL_PIN, OLED_SDA_PIN);

/* Memoria flash (NVS) para guardar los "nonces" de union entre reinicios. */
Preferences store;

/* Estado que se muestra en pantalla. */
static uint32_t g_packetCount = 0;      /* paquetes enviados desde el arranque */
static int16_t  g_lastRssi    = 0;      /* del ultimo downlink recibido        */
static float    g_lastSnr     = 0.0f;
static bool     g_haveDownlink = false;
static uint16_t g_vbatMv      = 0;

// ===========================================================================
//  2) FORMATO DEL PAYLOAD (los bytes que se envian)
// ===========================================================================
//
//  9 bytes, "big-endian" (byte mas significativo primero). ChirpStack los
//  vuelve a numeros con payload_decoder.js.
//
//   Byte(s) | Campo         | Tipo    | Como se codifica
//   --------+---------------+---------+---------------------------------
//     0     | magic         | uint8   | 0x48 ('H' de Heltec). Sirve para
//           |               |         | detectar paquetes ajenos.
//    1..2   | counter       | uint16  | numero de paquete (1, 2, 3, ...)
//    3..6   | uptime_s      | uint32  | segundos desde el arranque
//    7..8   | vbat_mv       | uint16  | voltaje de bateria en milivolts
//
#define PICARO_PAYLOAD_SIZE   9
#define PICARO_PAYLOAD_MAGIC  0x48

// ===========================================================================
//  3) FUNCIONES DE AYUDA
// ===========================================================================

/* Traduce los codigos de RadioLib a un texto que explique QUE revisar. */
String stateDecode(int16_t result)
{
    switch (result) {
    case RADIOLIB_ERR_NONE:                 return F("OK");
    case RADIOLIB_ERR_CHIP_NOT_FOUND:       return F("RADIO NO ENCONTRADO: revisa HELTEC_BOARD_VERSION y que la placa sea una Heltec LoRa 32");
    case RADIOLIB_ERR_SPI_CMD_TIMEOUT:      return F("EL RADIO NO RESPONDE POR SPI (BUSY no baja): revisa la placa");
    case RADIOLIB_ERR_PACKET_TOO_LONG:      return F("PAYLOAD DEMASIADO LARGO para este datarate");
    case RADIOLIB_ERR_NETWORK_NOT_JOINED:   return F("AUN NO UNIDO A LA RED");
    case RADIOLIB_ERR_NO_JOIN_ACCEPT:       return F("NO LLEGO EL JOIN-ACCEPT: revisa llaves en ChirpStack, sub-banda y que el gateway este online");
    case RADIOLIB_ERR_RX_TIMEOUT:           return F("SIN RESPUESTA (RX timeout)");
    case RADIOLIB_ERR_MIC_MISMATCH:         return F("MIC INCORRECTO: la AppKey de la placa no es la de ChirpStack");
    case RADIOLIB_ERR_JOIN_NONCE_INVALID:   return F("JOIN NONCE INVALIDO: pon PICARO_RESET_NONCES=1 una vez");
    case RADIOLIB_ERR_DOWNLINK_MALFORMED:   return F("DOWNLINK MAL FORMADO (se ignora)");
    case RADIOLIB_ERR_NO_CHANNEL_AVAILABLE: return F("SIN CANAL DISPONIBLE (duty cycle / dwell time)");
    case RADIOLIB_ERR_UPLINK_UNAVAILABLE:   return F("UPLINK NO DISPONIBLE todavia (espera timeUntilUplink)");
    case RADIOLIB_LORAWAN_NEW_SESSION:      return F("NUEVA SESION (join exitoso por aire)");
    case RADIOLIB_LORAWAN_SESSION_RESTORED: return F("SESION RESTAURADA de la flash (sin transmitir)");
    }
    return "codigo " + String(result) +
           " (ver https://jgromes.github.io/RadioLib/group__status__codes.html)";
}

/* Imprime un numero de 64 bits en hex, byte a byte, MSB primero. */
static void printEui(uint64_t eui)
{
    for (int i = 7; i >= 0; i--) {
        uint8_t b = (uint8_t)(eui >> (i * 8));
        if (b < 0x10) Serial.print('0');
        Serial.print(b, HEX);
        if (i) Serial.print(' ');
    }
}

/* Imprime un buffer en hex ("A1 B2 ..."). */
static void printHex(const uint8_t *buf, size_t len, bool spaces = true)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print('0');
        Serial.print(buf[i], HEX);
        if (spaces && i + 1 < len) Serial.print(' ');
    }
}

/* Nombre del datarate de US915 para el Monitor Serie. */
static const char *drName(uint8_t dr)
{
    switch (dr) {
    case 0: return "DR0 SF10/125k";
    case 1: return "DR1 SF9/125k";
    case 2: return "DR2 SF8/125k";
    case 3: return "DR3 SF7/125k";
    case 4: return "DR4 SF8/500k";
    case 8: return "DR8 SF12/500k";
    case 9: return "DR9 SF11/500k";
    case 10: return "DR10 SF10/500k";
    case 11: return "DR11 SF9/500k";
    case 12: return "DR12 SF8/500k";
    case 13: return "DR13 SF7/500k";
    }
    return "DR?";
}

/* Muestra por Serie la identidad que debe coincidir con ChirpStack. */
static void printIdentity()
{
    Serial.println(F("\n========== CREDENCIALES (deben ser IGUALES en ChirpStack) =========="));
    Serial.print(F("  DevEUI  (MSB): ")); printEui(devEUI);  Serial.println();
    Serial.print(F("  JoinEUI (MSB): ")); printEui(joinEUI); Serial.println();
    Serial.print(F("  AppKey  (MSB): ")); printHex(appKey, sizeof(appKey)); Serial.println();
    Serial.println(F("  Region: US915   Sub-banda: 2 (canales 8-15)   LoRaWAN: 1.0.x   Clase: A"));
    Serial.println(F("====================================================================\n"));

    if (devEUI == 0 || joinEUI == 0) {
        Serial.println(F("[AVISO] DevEUI o JoinEUI estan en CEROS. Copia los valores generados"));
        Serial.println(F("        por ChirpStack en config_lorawan.h antes de seguir."));
    }
    bool keyZero = true;
    for (unsigned i = 0; i < sizeof(appKey); i++) if (appKey[i]) { keyZero = false; break; }
    if (keyZero) {
        Serial.println(F("[AVISO] La AppKey esta en CEROS. ChirpStack rechazara el join (MIC)."));
    }
}

// ---------------------------------------------------------------------------
//  Bateria
// ---------------------------------------------------------------------------

/* Lee el voltaje de bateria en milivolts. Habilita el divisor (GPIO37) solo
 * durante la medida para no gastar bateria. Sin bateria conectada, el pin
 * queda flotando y la lectura es basura pequena (< 1000 mV): se reporta 0. */
static uint16_t readBatteryMv()
{
    digitalWrite(ADC_CTRL_PIN, ADC_CTRL_ON);
    delay(20);                                  /* deja asentar el divisor */
    uint32_t acc = 0;
    const int N = 8;
    for (int i = 0; i < N; i++) {
        acc += analogReadMilliVolts(VBAT_ADC_PIN);
        delay(2);
    }
    digitalWrite(ADC_CTRL_PIN, ADC_CTRL_OFF);

    float adcMv  = (float)acc / N;
    float vbatMv = adcMv * VBAT_DIVIDER;
    if (vbatMv < 1000.0f) return 0;             /* sin bateria: flotando */
    return (uint16_t)vbatMv;
}

/* 0..254 para DevStatusAns (LoRaWAN): 0 = alimentado por USB, 255 = no se
 * puede medir. Aproximacion lineal entre 3.3 V (vacio) y 4.2 V (lleno). */
static uint8_t batteryLevelForNetwork(uint16_t mv)
{
    if (mv == 0) return 0;
    long pct = map((long)mv, 3300, 4200, 1, 254);
    if (pct < 1) pct = 1;
    if (pct > 254) pct = 254;
    return (uint8_t)pct;
}

// ---------------------------------------------------------------------------
//  OLED
// ---------------------------------------------------------------------------

/* Dibuja hasta 4 lineas bajo un titulo fijo. Fuente 6x12: 21 caracteres. */
static void oledShow(const char *l1, const char *l2 = NULL,
                     const char *l3 = NULL, const char *l4 = NULL)
{
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 10, BOARD_NAME);
    oled.drawHLine(0, 13, 128);
    if (l1) oled.drawStr(0, 25, l1);
    if (l2) oled.drawStr(0, 37, l2);
    if (l3) oled.drawStr(0, 49, l3);
    if (l4) oled.drawStr(0, 61, l4);
    oled.sendBuffer();
}

/* Pantalla "CONECTADO" con los datos de la sesion. */
static void oledShowJoined(uint32_t devAddr, uint8_t dr, bool restored)
{
    char l2[24], l3[24], l4[24];
    snprintf(l2, sizeof(l2), "DevAddr %08lX", (unsigned long)devAddr);
    snprintf(l3, sizeof(l3), "%s", drName(dr));
    snprintf(l4, sizeof(l4), "US915 SB2 %s", restored ? "(restaur.)" : "JoinAccept");
    oledShow(restored ? "SESION RESTAURADA" : "CONECTADO (JOIN OK)", l2, l3, l4);
}

/* Pantalla de cada envio. */
static void oledShowTx(uint32_t fcnt, const char *estado)
{
    char l1[24], l2[24], l3[24], l4[24];
    snprintf(l1, sizeof(l1), "TX #%lu  fCnt %lu", (unsigned long)g_packetCount, (unsigned long)fcnt);
    if (g_vbatMv) snprintf(l2, sizeof(l2), "Bat %u.%02u V  up %lus", g_vbatMv / 1000, (g_vbatMv % 1000) / 10,
                           (unsigned long)(millis() / 1000UL));
    else          snprintf(l2, sizeof(l2), "Bat USB       up %lus", (unsigned long)(millis() / 1000UL));
    if (g_haveDownlink) snprintf(l3, sizeof(l3), "DL RSSI%4d SNR%5.1f", g_lastRssi, g_lastSnr);
    else                snprintf(l3, sizeof(l3), "DL: ninguno aun");
    snprintf(l4, sizeof(l4), "%s", estado);
    oledShow(l1, l2, l3, l4);
}

// ---------------------------------------------------------------------------
//  Placa: Vext, OLED, bateria, amplificador (V4)
// ---------------------------------------------------------------------------
static void setupBoard()
{
    /* Vext alimenta la OLED (y en V4 parte del RF). Activo en BAJO. */
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, VEXT_ON);
    delay(100);

    /* Medida de bateria: divisor apagado hasta que se mida. */
    pinMode(ADC_CTRL_PIN, OUTPUT);
    digitalWrite(ADC_CTRL_PIN, ADC_CTRL_OFF);
    analogReadResolution(12);
    analogSetPinAttenuation(VBAT_ADC_PIN, ADC_2_5db);   /* Vbat/4.9 < 0.9 V */

#ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
#endif

#if HELTEC_BOARD_VERSION == 4
    /* Amplificador de RF: alimentar, habilitar y dejarlo en modo PA. */
    pinMode(FEM_VCC_PIN, OUTPUT);  digitalWrite(FEM_VCC_PIN, HIGH);
    pinMode(FEM_CSD_PIN, OUTPUT);  digitalWrite(FEM_CSD_PIN, HIGH);
    pinMode(FEM_CPS_PIN, OUTPUT);  digitalWrite(FEM_CPS_PIN, HIGH);
    pinMode(FEM_CTX_PIN, OUTPUT);  digitalWrite(FEM_CTX_PIN, HIGH);
    delay(10);
    Serial.println(F("[placa] V4: amplificador de RF alimentado (GPIO7/2/46/5 en alto)."));
#endif

    /* OLED por I2C. U8g2 hace el reset con OLED_RST_PIN. */
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    oled.setI2CAddress(OLED_I2C_ADDR << 1);
    oled.begin();
    oled.setContrast(200);
    oledShow("Iniciando...", "Vext ON, OLED OK");
    Serial.printf("[placa] %s: Vext=GPIO%d (ON), OLED SDA=%d SCL=%d RST=%d @0x%02X\n",
                  BOARD_NAME, VEXT_PIN, OLED_SDA_PIN, OLED_SCL_PIN, OLED_RST_PIN, OLED_I2C_ADDR);
    Serial.printf("[placa] Radio SX1262: NSS=%d SCK=%d MOSI=%d MISO=%d RST=%d BUSY=%d DIO1=%d\n",
                  LORA_NSS_PIN, LORA_SCK_PIN, LORA_MOSI_PIN, LORA_MISO_PIN,
                  LORA_RST_PIN, LORA_BUSY_PIN, LORA_DIO1_PIN);
    Serial.printf("[placa] Bateria: ADC=GPIO%d, control=GPIO%d (activo en %s), divisor x%.1f\n",
                  VBAT_ADC_PIN, ADC_CTRL_PIN, (ADC_CTRL_ON == LOW) ? "BAJO" : "ALTO", VBAT_DIVIDER);
}

// ---------------------------------------------------------------------------
//  Nonces (contadores de union) en flash
// ---------------------------------------------------------------------------
static void saveNonces()
{
    uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    memcpy(buffer, node.getBufferNonces(), sizeof(buffer));
    store.putBytes("nonces", buffer, sizeof(buffer));
}

// ===========================================================================
//  4) SETUP  (una sola vez al encender)
// ===========================================================================
void setup()
{
    Serial.begin(115200);
    delay(3000);   /* tiempo para abrir el Monitor Serie */

    Serial.println(F("\n\n##################################################"));
    Serial.println(F("#  EJERCICIO 12 - JOIN OTAA  (Heltec WiFi LoRa 32) #"));
    Serial.println(F("##################################################"));
    Serial.printf("[info] Sketch v%s | placa: %s (HELTEC_BOARD_VERSION=%d) | RadioLib %d.%d.%d\n",
                  SKETCH_VERSION, BOARD_NAME, HELTEC_BOARD_VERSION,
                  RADIOLIB_VERSION_MAJOR, RADIOLIB_VERSION_MINOR, RADIOLIB_VERSION_PATCH);
    Serial.printf("[info] Uplink cada %d s en fPort %d, DR inicial %d, confirmado cada %d, potencia %d dBm\n",
                  PICARO_UPLINK_INTERVAL_S, PICARO_UPLINK_FPORT, PICARO_UPLINK_DR,
                  PICARO_CONFIRMED_EVERY, PICARO_TX_POWER_DBM);

    /* ---- [1/4] Placa ---- */
    Serial.println(F("\n[1/4] Encendiendo la placa (Vext, OLED, bateria)..."));
    setupBoard();
    g_vbatMv = readBatteryMv();
    if (g_vbatMv) Serial.printf("      Bateria: %u mV\n", g_vbatMv);
    else          Serial.println(F("      Bateria: no detectada (alimentacion por USB)"));

    /* ---- [2/4] Radio ---- */
    Serial.println(F("\n[2/4] Inicializando radio SX1262 (TCXO 1.8 V, DIO2 = conmutador RF)..."));
    oledShow("Iniciando radio...");
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);

    /* begin(freq, bw, sf, cr, syncWord, power, preamble, tcxoVoltage, useLDO).
     * La frecuencia/SF los reconfigura el nodo LoRaWAN; lo importante aqui es
     * el TCXO y el regulador. */
    int16_t state = radio.begin(915.0, 125.0, 7, 5, RADIOLIB_SX126X_SYNC_WORD_PUBLIC,
                                PICARO_TX_POWER_DBM, 8, LORA_TCXO_VOLTAGE, LORA_USE_LDO);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[ERROR] radio.begin() -> ")); Serial.println(stateDecode(state));
        oledShow("ERROR RADIO", "ver Monitor Serie");
        while (true) delay(1000);
    }
    radio.setDio2AsRfSwitch(LORA_DIO2_RF_SWITCH);
    Serial.println(F("      Radio OK."));

    /* ---- [3/4] Credenciales y nonces ---- */
    Serial.println(F("\n[3/4] Configurando OTAA (LoRaWAN 1.0.x: solo AppKey)..."));
    state = node.beginOTAA(joinEUI, devEUI, NULL, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[ERROR] beginOTAA() -> ")); Serial.println(stateDecode(state));
        while (true) delay(1000);
    }
    printIdentity();

    store.begin("picaro12");
#if PICARO_RESET_NONCES
    store.clear();
    Serial.println(F("[i] PICARO_RESET_NONCES=1: contadores de union BORRADOS. Regresa a 0 y reflashea."));
#endif
    if (store.isKey("nonces")) {
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
        store.getBytes("nonces", buffer, sizeof(buffer));
        node.setBufferNonces(buffer);
        Serial.println(F("[i] Contadores de union (DevNonce) restaurados desde la flash."));
#if !PICARO_FORCE_FRESH_JOIN
        if (store.isKey("session")) {
            uint8_t sess[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
            store.getBytes("session", sess, sizeof(sess));
            if (node.setBufferSession(sess) == RADIOLIB_ERR_NONE)
                Serial.println(F("[i] Sesion anterior restaurada desde la flash."));
        }
#endif
    } else {
        Serial.println(F("[i] Primer arranque: sin contadores de union guardados."));
    }
#if PICARO_FORCE_FRESH_JOIN
    Serial.println(F("[i] PICARO_FORCE_FRESH_JOIN=1: se hara un JOIN REAL por aire."));
#endif

    /* ---- [4/4] JOIN ---- */
    Serial.println(F("\n[4/4] JOIN OTAA: enviando Join-Request y esperando Join-Accept..."));
    Serial.println(F("      (RX1 a los 5 s, RX2 a los 6 s; cada intento tarda ~7 s)"));

    uint16_t intentos = 0;
    while (true) {
        intentos++;
        char l1[24], l2[24];
        snprintf(l1, sizeof(l1), "JOIN OTAA  intento %u", intentos);
        snprintf(l2, sizeof(l2), "DevEUI ..%04X%04X",
                 (unsigned)((devEUI >> 16) & 0xFFFF), (unsigned)(devEUI & 0xFFFF));
        oledShow(l1, l2, "esperando JoinAccept", "US915 sub-banda 2");

        Serial.printf("      Intento #%u: Join-Request (DevNonce nuevo) ...\n", intentos);
        uint32_t t0 = millis();
        state = node.activateOTAA();
        uint32_t dt = millis() - t0;
        saveNonces();   /* siempre: aunque falle, el DevNonce ya se gasto */

        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial.printf("      JOIN EXITOSO en %lu ms: llego el Join-Accept.\n", (unsigned long)dt);
            break;
        }
        if (state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
            Serial.println(F("      Sesion RESTAURADA de la flash (no se transmitio nada)."));
            break;
        }
        Serial.printf("      Fallo (%lu ms): ", (unsigned long)dt);
        Serial.println(stateDecode(state));
        Serial.println(F("      Reintento en 15 s. Mientras, revisa: gateway ONLINE en la consola,"));
        Serial.println(F("      device creado con ESTAS llaves, region US915 y sub-banda 2."));
        snprintf(l1, sizeof(l1), "JOIN fallo (%u)", intentos);
        oledShow(l1, "sin JoinAccept", "revisa llaves/gateway", "reintento en 15 s");
        delay(15000);
    }

    /* Datos de la sesion recien creada. */
    bool restored = (state == RADIOLIB_LORAWAN_SESSION_RESTORED);
    node.setADR(true);
    node.setDatarate(PICARO_UPLINK_DR);
    node.setTxPower(PICARO_TX_POWER_DBM);
#if !PICARO_FORCE_FRESH_JOIN
    {   uint8_t sess[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
        memcpy(sess, node.getBufferSession(), sizeof(sess));
        store.putBytes("session", sess, sizeof(sess)); }
#endif

    Serial.println(F("\n---------- DATOS DE LA SESION ----------"));
    Serial.printf("  DevAddr (lo asigno ChirpStack): %08lX\n", (unsigned long)node.getDevAddr());
    Serial.printf("  Tipo de sesion : %s\n", restored ? "restaurada de flash" : "NUEVA (Join-Accept por aire)");
    if (!restored) {
        Serial.printf("  Join-Accept    : RSSI %.0f dBm, SNR %.1f dB\n", radio.getRSSI(), radio.getSNR());
    }
    Serial.printf("  Datarate uplink: %s (ADR activo, la red puede cambiarlo)\n", drName(PICARO_UPLINK_DR));
    Serial.printf("  Potencia TX    : %d dBm en el SX1262%s\n", PICARO_TX_POWER_DBM,
                  (HELTEC_BOARD_VERSION == 4) ? " (+~11 dB del amplificador)" : "");
    Serial.printf("  fCnt uplink    : %lu\n", (unsigned long)node.getFCntUp());
    Serial.println(F("  Ahora en la consola de ChirpStack: device -> LoRaWAN frames -> JoinRequest / JoinAccept"));
    Serial.println(F("----------------------------------------\n"));

    oledShowJoined(node.getDevAddr(), PICARO_UPLINK_DR, restored);
    delay(3000);
}

// ===========================================================================
//  5) LOOP  (para siempre)
// ===========================================================================
void loop()
{
    /* 1) Leer bateria e informar a la red (DevStatusReq). */
    g_vbatMv = readBatteryMv();
    node.setDeviceStatus(batteryLevelForNetwork(g_vbatMv));

    /* 2) Armar el payload. */
    g_packetCount++;
    uint32_t uptime = millis() / 1000UL;
    uint8_t payload[PICARO_PAYLOAD_SIZE];
    payload[0] = PICARO_PAYLOAD_MAGIC;
    payload[1] = (uint8_t)(g_packetCount >> 8);
    payload[2] = (uint8_t)(g_packetCount);
    payload[3] = (uint8_t)(uptime >> 24);
    payload[4] = (uint8_t)(uptime >> 16);
    payload[5] = (uint8_t)(uptime >> 8);
    payload[6] = (uint8_t)(uptime);
    payload[7] = (uint8_t)(g_vbatMv >> 8);
    payload[8] = (uint8_t)(g_vbatMv);

    bool confirmed = (PICARO_CONFIRMED_EVERY > 0) && (g_packetCount % PICARO_CONFIRMED_EVERY == 0);

    Serial.printf("=============== UPLINK #%lu %s===============\n",
                  (unsigned long)g_packetCount, confirmed ? "(CONFIRMADO) " : "");
    Serial.printf("  counter=%lu  uptime=%lu s  vbat=%u mV%s\n",
                  (unsigned long)g_packetCount, (unsigned long)uptime, g_vbatMv,
                  g_vbatMv ? "" : " (USB, sin bateria)");
    Serial.print(F("  Payload (hex, ")); Serial.print(PICARO_PAYLOAD_SIZE);
    Serial.print(F(" bytes): ")); printHex(payload, PICARO_PAYLOAD_SIZE); Serial.println();
    Serial.printf("  fPort=%d  fCnt que va a usar=%lu\n", PICARO_UPLINK_FPORT, (unsigned long)node.getFCntUp());

#ifdef LED_PIN
    digitalWrite(LED_PIN, HIGH);
#endif
    oledShowTx(node.getFCntUp(), "enviando...");

    /* 3) Enviar y escuchar las ventanas RX1/RX2. */
    uint8_t  downlink[64];
    size_t   downlinkLen = 0;
    LoRaWANEvent_t evUp, evDown;
    int16_t state = node.sendReceive(payload, PICARO_PAYLOAD_SIZE, PICARO_UPLINK_FPORT,
                                     downlink, &downlinkLen, confirmed, &evUp, &evDown);
#ifdef LED_PIN
    digitalWrite(LED_PIN, LOW);
#endif

    char estado[24];
    if (state >= RADIOLIB_ERR_NONE) {
        /* Detalle del uplink tal como salio por la antena. */
        Serial.printf("  TX OK: %.3f MHz, %s, %d dBm, fCnt=%lu, nbTrans=%u, tiempo en aire %lu ms\n",
                      evUp.freq, drName(evUp.datarate), evUp.power, (unsigned long)evUp.fCnt,
                      evUp.nbTrans, (unsigned long)node.getLastToA());

        if (state > 0) {
            /* Llego algo en RX1 (1) o RX2 (2). */
            g_haveDownlink = true;
            g_lastRssi = (int16_t)radio.getRSSI();
            g_lastSnr  = radio.getSNR();
            Serial.printf("  DOWNLINK en RX%d: %.3f MHz, %s, RSSI %d dBm, SNR %.1f dB, fCnt=%lu, fPort=%u%s%s\n",
                          state, evDown.freq, drName(evDown.datarate), g_lastRssi, g_lastSnr,
                          (unsigned long)evDown.fCnt, evDown.fPort,
                          evDown.confirming ? " [ACK]" : "",
                          evDown.frmPending ? " [mas pendientes]" : "");
            if (downlinkLen > 0) {
                Serial.print(F("  Datos del downlink: ")); printHex(downlink, downlinkLen); Serial.println();
            } else {
                Serial.println(F("  Downlink sin datos de aplicacion (solo MAC/ACK)."));
            }
            snprintf(estado, sizeof(estado), "RX%d %s", state, evDown.confirming ? "ACK" : "downlink");
        } else {
            Serial.println(confirmed
                ? F("  Sin ACK: el servidor no confirmo. Si se repite, no hay enlace real.")
                : F("  Sin downlink (normal en Clase A si el servidor no tiene nada que decir)."));
            snprintf(estado, sizeof(estado), confirmed ? "sin ACK!" : "OK sin downlink");
        }
    } else {
        Serial.print(F("  [ERROR] sendReceive -> ")); Serial.println(stateDecode(state));
        snprintf(estado, sizeof(estado), "ERROR %d", state);
    }
    oledShowTx(node.getFCntUp(), estado);

    /* 4) Esperar al siguiente envio, respetando lo que pida la pila. */
    uint32_t wanted = (uint32_t)PICARO_UPLINK_INTERVAL_S * 1000UL;
    uint32_t byLaw  = node.timeUntilUplink();
    uint32_t waitMs = (byLaw > wanted) ? byLaw : wanted;
    Serial.printf("  Siguiente uplink en %lu s.\n\n", (unsigned long)(waitMs / 1000UL));
    delay(waitMs);
}
