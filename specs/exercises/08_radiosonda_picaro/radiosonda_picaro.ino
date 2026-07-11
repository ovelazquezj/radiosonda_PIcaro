/**
 * ============================================================================
 *  RADIOSONDA PICARO  -  LoRaWAN OTAA para LilyGo T-Beam (ESP32 + SX1276)
 * ============================================================================
 *
 *  Que hace este programa (en palabras simples):
 *    1. Enciende la placa T-Beam (radio LoRa + GPS + medidor de bateria).
 *    2. Se "une" a la red LoRaWAN de ChirpStack usando el metodo OTAA
 *       (Over-The-Air-Activation), es decir, con DEV_EUI + JOIN_EUI + APP_KEY.
 *    3. Cada cierto tiempo lee el GPS y la bateria, arma un paquete de bytes
 *       (payload) y lo envia por radio hacia el gateway -> ChirpStack.
 *    4. Imprime TODO por el Monitor Serie (115200 baudios) para que veas el
 *       proceso, y tambien lo muestra en la pantalla OLED si la placa la trae.
 *
 *  Que archivos hay en esta carpeta y para que sirven:
 *    - radiosonda_picaro.ino  -> ESTE archivo (la logica principal).
 *    - config_lorawan.h       -> AQUI cambias DEV_EUI / JOIN_EUI / APP_KEY.
 *    - utilities.h            -> pines de la placa (ya viene con T-Beam SX1276).
 *    - LoRaBoards.cpp / .h    -> "motor" de la placa (enciende PMU, GPS, OLED).
 *                                No necesitas editarlo para la practica.
 *    - chirpstack/decoder.js  -> codigo para que ChirpStack "traduzca" el payload.
 *    - README.md              -> guia paso a paso (leela primero).
 *
 *  Como estudiante, el 99% de las veces SOLO tocas config_lorawan.h.
 *
 *  Licencia: MIT.  Basado en los ejemplos oficiales de LilyGo (RadioLib).
 * ============================================================================
 */

#include <RadioLib.h>          // Pila LoRaWAN + control del radio SX1276
#include "LoRaBoards.h"        // Inicializacion de la T-Beam (PMU, GPS, OLED)
#include <TinyGPS++.h>         // Interpreta las tramas NMEA del GPS (clase TinyGPSPlus)
#include <Preferences.h>       // Guarda datos en la memoria flash (NVS)
#include "config_lorawan.h"    // <-- TUS credenciales y parametros

/* ---------------------------------------------------------------------------
 *  Si alguna vez el join falla con un error de "DevNonce" (numero de union ya
 *  usado), pon esta linea en 1, sube el programa UNA vez, luego regresala a 0
 *  y vuelve a subir. Esto borra los contadores de union guardados en la placa.
 * ------------------------------------------------------------------------- */
#define PICARO_RESET_NONCES   0

// ===========================================================================
//  1) OBJETOS PRINCIPALES
// ===========================================================================

// Objeto del radio SX1276. Los pines (CS, DIO0, RST, DIO1) vienen de utilities.h
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

// Region LoRaWAN (US915) y sub-banda, tomadas de config_lorawan.h
const LoRaWANBand_t Region = PICARO_REGION;
const uint8_t subBand      = PICARO_SUBBAND;

// El "nodo" LoRaWAN: junta el radio con la region.
LoRaWANNode node(&radio, &Region, subBand);

// Credenciales, convertidas al formato que espera RadioLib.
uint64_t joinEUI = PICARO_JOIN_EUI;
uint64_t devEUI  = PICARO_DEV_EUI;
uint8_t  appKey[] = { PICARO_APP_KEY };

// Objeto GPS y almacenamiento persistente (para los contadores de union).
TinyGPSPlus gps;
Preferences store;

// ===========================================================================
//  2) ESTRUCTURA DEL PAYLOAD (los bytes que se envian)
// ===========================================================================
//
//  El payload son 15 bytes, en orden "big-endian" (el byte mas significativo
//  primero). ChirpStack lo vuelve a numeros con chirpstack/decoder.js.
//
//   Byte(s) | Campo            | Tipo         | Como se codifica
//   --------+------------------+--------------+---------------------------
//     0     | status           | uint8        | bit0=GPS ok, bit1=cargando,
//           |                  |              | bit2=USB conectado
//    1..4   | latitud          | int32        | grados * 1,000,000
//    5..8   | longitud         | int32        | grados * 1,000,000
//    9..10  | altitud          | int16        | metros
//    11     | satelites        | uint8        | cantidad de satelites
//   12..13  | bateria_mv       | uint16       | milivolts
//    14     | bateria_pct      | uint8        | 0..100 (255 = desconocido)
//
#define PICARO_PAYLOAD_SIZE   15

// Metemos un entero de 32 bits en el buffer, byte mas significativo primero.
static void putInt32BE(uint8_t *buf, int32_t v)
{
    buf[0] = (uint8_t)(v >> 24);
    buf[1] = (uint8_t)(v >> 16);
    buf[2] = (uint8_t)(v >> 8);
    buf[3] = (uint8_t)(v);
}

// Igual pero con 16 bits.
static void putInt16BE(uint8_t *buf, int16_t v)
{
    buf[0] = (uint8_t)(v >> 8);
    buf[1] = (uint8_t)(v);
}

// ===========================================================================
//  3) FUNCIONES DE AYUDA
// ===========================================================================

// Traduce algunos codigos de error de RadioLib a texto legible.
String stateDecode(const int16_t result)
{
    switch (result) {
    case RADIOLIB_ERR_NONE:                 return "OK";
    case RADIOLIB_ERR_CHIP_NOT_FOUND:       return "RADIO NO ENCONTRADO (revisa la placa/pines)";
    case RADIOLIB_ERR_PACKET_TOO_LONG:      return "PAYLOAD DEMASIADO LARGO para este datarate";
    case RADIOLIB_ERR_NETWORK_NOT_JOINED:   return "AUN NO UNIDO A LA RED";
    case RADIOLIB_ERR_NO_JOIN_ACCEPT:       return "NO LLEGO EL JOIN-ACCEPT (revisa llaves/region/gateway)";
    case RADIOLIB_ERR_RX_TIMEOUT:           return "SIN RESPUESTA (RX timeout)";
    case RADIOLIB_LORAWAN_NEW_SESSION:      return "NUEVA SESION (join exitoso)";
    case RADIOLIB_LORAWAN_SESSION_RESTORED: return "SESION RESTAURADA";
    }
    return "codigo " + String(result) +
           " (ver https://jgromes.github.io/RadioLib/group__status__codes.html)";
}

// Muestra un error y, si 'halt' es true, detiene el programa (para depurar).
void debug(bool failed, const __FlashStringHelper *message, int state, bool halt)
{
    if (failed) {
        Serial.print("[ERROR] ");
        Serial.print(message);
        Serial.print(" -> ");
        Serial.println(stateDecode(state));
        while (halt) {
            delay(1000);
        }
    }
}

// Imprime la identidad del dispositivo para que la copies en ChirpStack.
void printIdentityForChirpStack()
{
    Serial.println(F("\n========== DATOS PARA DAR DE ALTA EN CHIRPSTACK =========="));

    Serial.print(F("  DevEUI  (MSB): "));
    for (int i = 7; i >= 0; i--) {
        uint8_t b = (devEUI >> (i * 8)) & 0xFF;
        if (b < 0x10) Serial.print('0');
        Serial.print(b, HEX);
        if (i) Serial.print(' ');
    }
    Serial.println();

    Serial.print(F("  JoinEUI (MSB): "));
    for (int i = 7; i >= 0; i--) {
        uint8_t b = (joinEUI >> (i * 8)) & 0xFF;
        if (b < 0x10) Serial.print('0');
        Serial.print(b, HEX);
        if (i) Serial.print(' ');
    }
    Serial.println();

    Serial.print(F("  AppKey  (MSB): "));
    for (unsigned i = 0; i < sizeof(appKey); i++) {
        if (appKey[i] < 0x10) Serial.print('0');
        Serial.print(appKey[i], HEX);
        if (i + 1 < sizeof(appKey)) Serial.print(' ');
    }
    Serial.println();
    Serial.println(F("  Region: US915   Sub-banda: 2   Version LoRaWAN: 1.0.x"));
    Serial.println(F("=========================================================\n"));
}

// Lee el GPS durante 'ms' milisegundos SIN dejar de alimentar al parser.
// (El GPS envia datos todo el tiempo; hay que vaciarlos o se pierden.)
static void feedGpsFor(uint32_t ms)
{
    uint32_t start = millis();
    do {
        while (SerialGPS.available() > 0) {
            gps.encode(SerialGPS.read());
        }
    } while (millis() - start < ms);
}

// Dibuja el estado en la pantalla OLED (si la placa la trae).
void showOnDisplay(const char *line1, const char *line2, const char *line3)
{
#ifdef DISPLAY_MODEL
    if (!disp) return;
    disp->clearBuffer();
    disp->setFont(u8g2_font_6x12_tf);
    disp->drawStr(0, 12, "Radiosonda PICARO");
    disp->drawHLine(0, 15, disp->getWidth());
    if (line1) disp->drawStr(0, 30, line1);
    if (line2) disp->drawStr(0, 44, line2);
    if (line3) disp->drawStr(0, 58, line3);
    disp->sendBuffer();
#endif
}

// ===========================================================================
//  4) ARMAR EL PAYLOAD CON LOS SENSORES
// ===========================================================================
// Devuelve cuantos bytes se llenaron (siempre PICARO_PAYLOAD_SIZE).
uint8_t buildPayload(uint8_t *buf)
{
    // --- Leer GPS ---
    bool    gpsOk = gps.location.isValid() && gps.location.age() < 5000;
    int32_t lat   = gpsOk ? (int32_t)(gps.location.lat() * 1000000.0) : 0;
    int32_t lon   = gpsOk ? (int32_t)(gps.location.lng() * 1000000.0) : 0;
    int16_t alt   = (gps.altitude.isValid()) ? (int16_t)gps.altitude.meters() : 0;
    uint8_t sats  = (gps.satellites.isValid()) ? (uint8_t)gps.satellites.value() : 0;

    // --- Leer bateria (PMU AXP192/AXP2101) ---
    uint16_t battMv  = 0;
    int      battPct = -1;
    bool     charging = false;
    bool     usb      = false;
#ifdef HAS_PMU
    if (PMU) {
        battMv   = PMU->getBattVoltage();     // milivolts
        battPct  = PMU->getBatteryPercent();  // 0..100, o -1 si no se puede
        charging = PMU->isCharging();
        usb      = PMU->isVbusIn();
    }
#endif

    // --- Byte de estado (flags) ---
    uint8_t status = 0;
    if (gpsOk)    status |= 0x01;   // bit0
    if (charging) status |= 0x02;   // bit1
    if (usb)      status |= 0x04;   // bit2

    // --- Escribir los bytes en orden ---
    buf[0] = status;
    putInt32BE(&buf[1], lat);
    putInt32BE(&buf[5], lon);
    putInt16BE(&buf[9], alt);
    buf[11] = sats;
    buf[12] = (uint8_t)(battMv >> 8);
    buf[13] = (uint8_t)(battMv);
    buf[14] = (battPct < 0) ? 255 : (uint8_t)battPct;

    // --- Copia legible por Serie (para el estudiante) ---
    Serial.println(F("---- Lectura de sensores ----"));
    if (gpsOk) {
        Serial.printf("  GPS: lat=%.6f  lon=%.6f  alt=%dm  sats=%u\n",
                      gps.location.lat(), gps.location.lng(), alt, sats);
    } else {
        Serial.printf("  GPS: SIN FIX todavia (sats vistos=%u). Ponlo cerca de una ventana.\n",
                      (unsigned)(gps.satellites.isValid() ? gps.satellites.value() : 0));
    }
    Serial.printf("  Bateria: %u mV  (%d%%)  %s%s\n",
                  battMv, battPct,
                  charging ? "[cargando] " : "",
                  usb ? "[USB]" : "[bateria]");

    return PICARO_PAYLOAD_SIZE;
}

// ===========================================================================
//  5) SETUP  (se ejecuta una sola vez al encender)
// ===========================================================================
void setup()
{
    Serial.begin(115200);
    delay(3000);   // tiempo para abrir el Monitor Serie

    Serial.println(F("\n\n#############################################"));
    Serial.println(F("#     RADIOSONDA PICARO - arrancando...     #"));
    Serial.println(F("#############################################"));

    // Enciende la placa: PMU (rieles de energia de LoRa y GPS), GPS, OLED, etc.
    setupBoards();
    delay(1500);

    showOnDisplay("Iniciando radio...", NULL, NULL);

    // --- Inicializar el radio SX1276 ---
    Serial.println(F("\n[1/3] Inicializando radio SX1276..."));
    int16_t state = radio.begin();
    debug(state != RADIOLIB_ERR_NONE, F("No se pudo iniciar el radio"), state, true);
    Serial.println(F("      Radio OK."));

    // --- Configurar credenciales OTAA (LoRaWAN 1.0.x: nwkKey = NULL) ---
    // Al pasar NULL como clave de red, RadioLib usa LoRaWAN 1.0.x, que solo
    // necesita AppKey. Es lo que usa ChirpStack en la mayoria de los cursos.
    node.beginOTAA(joinEUI, devEUI, NULL, appKey);

    // Muestra por Serie los datos a copiar en ChirpStack.
    printIdentityForChirpStack();

    // --- Manejo de "nonces" (contadores de union) guardados en flash ---
    store.begin("picaro");
#if PICARO_RESET_NONCES
    store.clear();
    Serial.println(F("[i] Nonces borrados (PICARO_RESET_NONCES=1)."));
#endif
    if (store.isKey("nonces")) {
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
        store.getBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        node.setBufferNonces(buffer);
        Serial.println(F("[i] Contadores de union restaurados desde la flash."));
    }

    // --- Unirse a la red (JOIN) con reintentos ---
    Serial.println(F("\n[2/3] Uniendose a la red LoRaWAN (OTAA)..."));
    showOnDisplay("Join LoRaWAN...", "esperando gateway", NULL);

    state = RADIOLIB_ERR_NETWORK_NOT_JOINED;
    uint8_t intentos = 0;
    while (state != RADIOLIB_LORAWAN_NEW_SESSION &&
           state != RADIOLIB_LORAWAN_SESSION_RESTORED) {

        intentos++;
        Serial.printf("      Intento de union #%u...\n", intentos);
        state = node.activateOTAA();

        // Guardar los nonces (para que el proximo reinicio no reuse numeros).
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
        uint8_t *persist = node.getBufferNonces();
        memcpy(buffer, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        store.putBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

        if (state != RADIOLIB_LORAWAN_NEW_SESSION &&
            state != RADIOLIB_LORAWAN_SESSION_RESTORED) {
            Serial.print(F("      Union fallida: "));
            Serial.println(stateDecode(state));
            Serial.println(F("      Reintentando en 30 s (revisa gateway/llaves/region)..."));
            showOnDisplay("Join fallido", "reintentando 30s", NULL);
            feedGpsFor(30000);   // esperamos, pero seguimos leyendo el GPS
        }
    }

    Serial.println(F("      JOIN EXITOSO! Ya estamos en la red."));
    Serial.print(F("      DevAddr asignado: "));
    Serial.println((unsigned long)node.getDevAddr(), HEX);

    // --- Ajustes finos de la red ---
    node.setADR(true);                        // ADR: la red ajusta el datarate
    node.setDatarate(PICARO_INITIAL_DATARATE); // datarate inicial (config)

    Serial.println(F("\n[3/3] Listo. Comenzando a enviar telemetria.\n"));
    showOnDisplay("JOIN OK!", "enviando datos...", NULL);
}

// ===========================================================================
//  6) LOOP  (se repite para siempre)
// ===========================================================================
void loop()
{
    // Reportar el nivel de bateria a la red (opcional pero recomendado).
#ifdef HAS_PMU
    if (PMU) {
        int pct = PMU->getBatteryPercent();
        node.setDeviceStatus(pct < 0 ? 255 : map(pct, 0, 100, 1, 254));
    }
#endif

    // 1) Armar el payload con los sensores.
    uint8_t payload[PICARO_PAYLOAD_SIZE];
    uint8_t len = buildPayload(payload);

    // Mostrar el payload en hexadecimal (util para comparar con ChirpStack).
    Serial.print(F("  Payload (hex): "));
    for (uint8_t i = 0; i < len; i++) {
        if (payload[i] < 0x10) Serial.print('0');
        Serial.print(payload[i], HEX);
    }
    Serial.println();

    // 2) Enviar el paquete (uplink) y escuchar por si hay respuesta (downlink).
    Serial.println(F("  Enviando uplink..."));
    uint8_t downlink[64];
    size_t  downlinkLen = 0;
    int16_t state = node.sendReceive(payload, len, PICARO_UPLINK_FPORT,
                                     downlink, &downlinkLen);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("  Uplink enviado (sin downlink)."));
    } else if (state > 0) {
        Serial.printf("  Uplink enviado. Downlink recibido en ventana Rx%d, %u bytes.\n",
                      state, (unsigned)downlinkLen);
    } else {
        debug(true, F("Fallo en el envio"), state, false);
    }

    // Mostrar calidad de senal del ultimo enlace.
    char l2[24], l3[24];
    snprintf(l2, sizeof(l2), "RSSI %d dBm", (int)radio.getRSSI());
    snprintf(l3, sizeof(l3), "SNR %.1f dB", radio.getSNR());
    Serial.printf("  RSSI: %d dBm   SNR: %.1f dB   Uplink #%lu\n",
                  (int)radio.getRSSI(), radio.getSNR(),
                  (unsigned long)node.getFCntUp());
    showOnDisplay(gps.location.isValid() ? "GPS OK, enviando" : "Buscando GPS...", l2, l3);

    // 3) Esperar hasta el siguiente envio, respetando el duty cycle legal,
    //    y SIN dejar de leer el GPS mientras tanto.
    uint32_t wanted = (uint32_t)PICARO_UPLINK_INTERVAL_S * 1000UL;
    uint32_t byLaw  = node.timeUntilUplink();   // minimo permitido por regulacion
    uint32_t waitMs = max(wanted, byLaw);
    Serial.printf("  Siguiente uplink en %lu s.\n\n", (unsigned long)(waitMs / 1000UL));
    feedGpsFor(waitMs);
}
