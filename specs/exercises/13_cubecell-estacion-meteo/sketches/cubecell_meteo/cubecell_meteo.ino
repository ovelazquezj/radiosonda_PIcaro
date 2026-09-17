/**
 * ============================================================================
 *  EJERCICIO 13  -  ESTACION METEOROLOGICA LoRaWAN con Heltec CubeCell HTCC-AB01
 * ============================================================================
 *
 *  Que hace este programa:
 *    1. Hace JOIN OTAA al ChirpStack del curso con las credenciales de
 *       config_lorawan.h (pila LoRaWAN de Heltec, incluida en el paquete de
 *       placas CubeCell).
 *    2. Cada PICARO_UPLINK_INTERVAL_S segundos enciende Vext, lee los sensores
 *       que esten activados en config_sensores.h (DHT11, BMP280, BH1750 y
 *       modulo de lluvia MH-RD), apaga Vext y envia un paquete de 11 bytes.
 *    3. Entre envios la placa DUERME (consumo de microamperios): asi funciona
 *       con bateria y panel solar.
 *    4. Cuenta todo por el Monitor Serie (115200 baudios) y lo resume con el
 *       LED RGB de la placa: rojo = transmitiendo, morado = join OK,
 *       azul/amarillo = ventanas de recepcion, verde = downlink recibido.
 *
 *  Archivos de esta carpeta:
 *    - cubecell_meteo.ino  -> ESTE archivo (la logica). No se edita.
 *    - config_lorawan.h    -> credenciales y parametros de red.
 *    - config_sensores.h   -> que sensores hay conectados y en que pin.
 *
 *  Opciones de la placa en Arduino IDE (menu Tools), ver README Paso 4:
 *    Board: CubeCell-Board (HTCC-AB01) · LORAWAN_REGION: REGION_US915 ·
 *    LORAWAN_CLASS: CLASS_A · LORAWAN_DEVEUI: CUSTOM · LORAWAN_NETMODE: OTAA ·
 *    LORAWAN_ADR: ON · LORAWAN_UPLINKMODE: UNCONFIRMED · LORAWAN_Net_Reserve: OFF ·
 *    LORAWAN_AT_SUPPORT: OFF · LORAWAN_RGB: ACTIVE · LoRaWan Debug Level: Freq
 *
 *  Licencia: MIT. Basado en los ejemplos LoRaWAN del CubeCell Development
 *  Framework de Heltec.
 * ============================================================================
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>

#include "config_lorawan.h"
#include "config_sensores.h"

#if SENSOR_BMP280
#include "BMP280.h"     /* libreria Sensor_ThirdParty del paquete CubeCell */
#endif
#if SENSOR_BH1750
#include "BH1750.h"     /* libreria SensorBasic del paquete CubeCell       */
#endif

#define SKETCH_VERSION "1.0"

// ===========================================================================
//  1) VARIABLES QUE EXIGE LA PILA LoRaWAN DE HELTEC
// ===========================================================================
// La pila de Heltec no recibe la configuracion por funciones sino leyendo
// estas variables globales con nombres fijos. Los valores vienen de
// config_lorawan.h y de las opciones del menu Tools (macros LORAWAN_*).

/* OTAA: DevEUI, JoinEUI (aqui "appEui") y AppKey, en MSB. */
uint8_t devEui[] = PICARO_DEV_EUI;
uint8_t appEui[] = PICARO_JOIN_EUI;
uint8_t appKey[] = PICARO_APP_KEY;

/* ABP: no se usa en este ejercicio, pero la pila exige que existan. */
uint8_t  nwkSKey[16] = { 0 };
uint8_t  appSKey[16] = { 0 };
uint32_t devAddr     = 0;

/* Mascara de canales de US915. Cada bit es un canal de 125 kHz: la palabra 0
 * cubre los canales 0-15, la 1 los 16-31, etc. La sub-banda N (1..8) son los
 * 8 canales que empiezan en (N-1)*8. Sub-banda 2 => canales 8-15 => 0xFF00. */
#define SB_WORD  ((PICARO_SUBBAND - 1) / 2)
#define SB_BITS  (((PICARO_SUBBAND - 1) % 2) ? 0xFF00 : 0x00FF)
uint16_t userChannelsMask[6] = {
    (SB_WORD == 0) ? SB_BITS : 0x0000,
    (SB_WORD == 1) ? SB_BITS : 0x0000,
    (SB_WORD == 2) ? SB_BITS : 0x0000,
    (SB_WORD == 3) ? SB_BITS : 0x0000,
    0x0000, 0x0000 };

LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;   /* menu Tools -> REGION_US915 */
DeviceClass_t   loraWanClass  = LORAWAN_CLASS;   /* menu Tools -> CLASS_A       */
bool overTheAirActivation     = LORAWAN_NETMODE; /* menu Tools -> OTAA          */
bool loraWanAdr               = LORAWAN_ADR;     /* menu Tools -> ADR ON        */
bool keepNet                  = LORAWAN_NET_RESERVE;
bool isTxConfirmed            = LORAWAN_UPLINKMODE; /* se cambia por ciclo abajo */
uint8_t confirmedNbTrials     = PICARO_CONFIRMED_TRIALS;
uint32_t appTxDutyCycle       = (uint32_t)PICARO_UPLINK_INTERVAL_S * 1000UL;
uint8_t  appPort              = PICARO_UPLINK_FPORT;

// ===========================================================================
//  2) FORMATO DEL PAYLOAD (11 bytes, big-endian, fPort 2)
// ===========================================================================
//
//  Cabe en DR0 (SF10, maximo 11 bytes en US915), que es el datarate con el
//  que arranca la pila antes de que ADR lo suba. ChirpStack lo decodifica
//  con payload_decoder.js.
//
//   Byte(s) | Campo      | Tipo   | Codificacion                 | "no hay"
//   --------+------------+--------+------------------------------+---------
//     0     | status     | uint8  | bit0 DHT11 ok  bit1 BMP280 ok |
//           |            |        | bit2 BH1750 ok bit3 lluvia DO |
//           |            |        | bit4 MH-RD ok                 |
//     1     | vbat       | uint8  | voltaje bateria / 20 mV       |   0
//     2     | temp_dht   | int8   | grados C enteros              | 0x7F
//     3     | hum_dht    | uint8  | % humedad relativa            | 0xFF
//    4..5   | temp_bmp   | int16  | grados C x 10                 | 0x7FFF
//    6..7   | presion    | uint16 | hPa x 10                      | 0xFFFF
//    8..9   | lux        | uint16 | lux enteros (tope 65534)      | 0xFFFF
//    10     | lluvia_pct | uint8  | 0 seco .. 100 empapado        | 0xFF
//
#define PAYLOAD_SIZE 11

// ===========================================================================
//  3) LECTURA DE SENSORES
// ===========================================================================

struct Lecturas {
    bool     dhtOk, bmpOk, bhOk, mhrdOk, lluviaDO;
    int8_t   tempDht;   uint8_t humDht;
    float    tempBmp;   float   presHpa;
    float    lux;
    uint16_t lluviaRaw; uint8_t lluviaPct;
    uint16_t vbatMv;
};

#if SENSOR_DHT11
/* Lector minimo del DHT11 (protocolo de 1 hilo), sin librerias externas.
 * Devuelve true si la trama de 40 bits llego con checksum correcto. */
static bool leerDht11(int8_t *tempC, uint8_t *humPct)
{
    uint8_t datos[5] = { 0 };

    /* Pulso de arranque: el MCU baja la linea >= 18 ms y la suelta. */
    pinMode(PIN_DHT11, OUTPUT);
    digitalWrite(PIN_DHT11, LOW);
    delay(20);
    digitalWrite(PIN_DHT11, HIGH);
    delayMicroseconds(30);
    pinMode(PIN_DHT11, INPUT_PULLUP);

    /* Respuesta del sensor: 80 us en bajo + 80 us en alto. */
    uint32_t t = micros();
    while (digitalRead(PIN_DHT11) == HIGH) { if (micros() - t > 200) return false; }
    t = micros();
    while (digitalRead(PIN_DHT11) == LOW)  { if (micros() - t > 200) return false; }
    t = micros();
    while (digitalRead(PIN_DHT11) == HIGH) { if (micros() - t > 200) return false; }

    /* 40 bits: cada uno es 50 us en bajo y luego 26-28 us (0) o 70 us (1) en alto. */
    for (int i = 0; i < 40; i++) {
        t = micros();
        while (digitalRead(PIN_DHT11) == LOW)  { if (micros() - t > 100) return false; }
        uint32_t tAlto = micros();
        while (digitalRead(PIN_DHT11) == HIGH) { if (micros() - tAlto > 150) return false; }
        uint32_t dur = micros() - tAlto;
        datos[i / 8] <<= 1;
        if (dur > 45) datos[i / 8] |= 1;
    }

    uint8_t suma = datos[0] + datos[1] + datos[2] + datos[3];
    if (suma != datos[4]) return false;

    *humPct = datos[0];
    *tempC  = (int8_t)datos[2];
    if (datos[3] & 0x80) *tempC = -(*tempC);   /* algunos DHT11 marcan negativo asi */
    return true;
}
#endif

static void leerSensores(Lecturas *L)
{
    memset(L, 0, sizeof(*L));

    /* Bateria: la mide la propia placa (divisor interno). En mV. */
    L->vbatMv = getBatteryVoltage();

    /* Encender la alimentacion de los sensores solo para medir. */
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);          /* Vext es activo en BAJO */
    delay(SENSORES_WARMUP_MS);

#if SENSOR_DHT11
    /* El DHT11 falla a veces la primera lectura: se intenta hasta 3 veces. */
    for (int i = 0; i < 3 && !L->dhtOk; i++) {
        L->dhtOk = leerDht11(&L->tempDht, &L->humDht);
        if (!L->dhtOk) delay(1100);   /* el DHT11 exige >= 1 s entre lecturas */
    }
#endif

#if SENSOR_BMP280 || SENSOR_BH1750
    Wire.begin();
#endif

#if SENSOR_BMP280
    {
        BMP280 bmp;
        if (bmp.begin(BMP280_I2C_ADDR)) {
            bmp.setSampling(BMP280::MODE_FORCED, BMP280::SAMPLING_X2,
                            BMP280::SAMPLING_X16, BMP280::FILTER_X4,
                            BMP280::STANDBY_MS_1);
            delay(50);
            L->tempBmp = bmp.readTemperature();
            L->presHpa = bmp.readPressure() / 100.0f;
            L->bmpOk   = (L->presHpa > 300.0f && L->presHpa < 1200.0f);
        }
    }
#endif

#if SENSOR_BH1750
    {
        BH1750 luz;
        if (luz.begin(BH1750::ONE_TIME_HIGH_RES_MODE_2)) {
            L->lux = luz.readLightLevel();     /* la primera lectura puede ser mala */
            delay(180);
            L->lux = luz.readLightLevel();
            luz.end();
            L->bhOk = (L->lux >= 0.0f);
        }
    }
#endif

#if SENSOR_BMP280 || SENSOR_BH1750
    Wire.end();
#endif

#if SENSOR_MHRD
    pinMode(PIN_MHRD_DO, INPUT);
    L->lluviaDO  = (digitalRead(PIN_MHRD_DO) == LOW);   /* DO en bajo = lluvia */
    L->lluviaRaw = (uint16_t)analogRead(PIN_MHRD_AO);
    long pct = map((long)L->lluviaRaw, MHRD_DRY_RAW, MHRD_WET_RAW, 0, 100);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    L->lluviaPct = (uint8_t)pct;
    L->mhrdOk = true;
#endif

    /* Apagar los sensores y dejar los pines en alta impedancia para dormir. */
    digitalWrite(Vext, HIGH);
#if SENSOR_DHT11
    pinMode(PIN_DHT11, ANALOG);
#endif
#if SENSOR_MHRD
    pinMode(PIN_MHRD_DO, ANALOG);
#endif
}

static void imprimirLecturas(const Lecturas *L)
{
    Serial.println("---- Lecturas ----");
    Serial.printf("  Bateria : %u mV\r\n", L->vbatMv);
#if SENSOR_DHT11
    if (L->dhtOk) Serial.printf("  DHT11   : %d C, %u %%HR\r\n", L->tempDht, L->humDht);
    else          Serial.println("  DHT11   : SIN RESPUESTA (revisa DATA->GPIO5, VCC->Vext, resistencia pull-up)");
#else
    Serial.println("  DHT11   : desactivado (SENSOR_DHT11=0)");
#endif
#if SENSOR_BMP280
    if (L->bmpOk) Serial.printf("  BMP280  : %.2f C, %.1f hPa\r\n", L->tempBmp, L->presHpa);
    else          Serial.printf("  BMP280  : NO RESPONDE en 0x%02X (revisa SDA/SCL, direccion 0x76/0x77)\r\n", BMP280_I2C_ADDR);
#else
    Serial.println("  BMP280  : desactivado (SENSOR_BMP280=0)");
#endif
#if SENSOR_BH1750
    if (L->bhOk) Serial.printf("  BH1750  : %.0f lux\r\n", L->lux);
    else         Serial.println("  BH1750  : NO RESPONDE en 0x23 (revisa SDA/SCL)");
#else
    Serial.println("  BH1750  : desactivado (SENSOR_BH1750=0)");
#endif
#if SENSOR_MHRD
    Serial.printf("  MH-RD   : AO=%u (raw) -> %u %% lluvia, DO=%s\r\n",
                  L->lluviaRaw, L->lluviaPct, L->lluviaDO ? "LLUVIA" : "seco");
#else
    Serial.println("  MH-RD   : desactivado (SENSOR_MHRD=0)");
#endif
}

// ===========================================================================
//  4) ARMAR EL PAYLOAD
// ===========================================================================
static uint32_t g_ciclo = 0;   /* uplinks preparados desde el arranque */

static void prepareTxFrame(uint8_t port)
{
    (void)port;
    Lecturas L;
    leerSensores(&L);
    imprimirLecturas(&L);

    uint8_t status = 0;
    if (L.dhtOk)    status |= 0x01;
    if (L.bmpOk)    status |= 0x02;
    if (L.bhOk)     status |= 0x04;
    if (L.lluviaDO) status |= 0x08;
    if (L.mhrdOk)   status |= 0x10;

    uint16_t vbat20  = L.vbatMv / 20;                 if (vbat20 > 255) vbat20 = 255;
    int8_t   tDht    = L.dhtOk ? L.tempDht : (int8_t)0x7F;
    uint8_t  hDht    = L.dhtOk ? L.humDht  : 0xFF;
    int16_t  tBmp10  = L.bmpOk ? (int16_t)(L.tempBmp * 10.0f) : (int16_t)0x7FFF;
    uint16_t pres10  = L.bmpOk ? (uint16_t)(L.presHpa * 10.0f) : 0xFFFF;
    uint16_t lux     = L.bhOk  ? (uint16_t)((L.lux > 65534.0f) ? 65534 : L.lux) : 0xFFFF;
    uint8_t  lluvia  = L.mhrdOk ? L.lluviaPct : 0xFF;

    appDataSize = PAYLOAD_SIZE;
    appData[0]  = status;
    appData[1]  = (uint8_t)vbat20;
    appData[2]  = (uint8_t)tDht;
    appData[3]  = hDht;
    appData[4]  = (uint8_t)(tBmp10 >> 8);
    appData[5]  = (uint8_t)(tBmp10);
    appData[6]  = (uint8_t)(pres10 >> 8);
    appData[7]  = (uint8_t)(pres10);
    appData[8]  = (uint8_t)(lux >> 8);
    appData[9]  = (uint8_t)(lux);
    appData[10] = lluvia;

    Serial.print("  Payload (hex, 11 bytes): ");
    for (int i = 0; i < PAYLOAD_SIZE; i++) Serial.printf("%02X ", appData[i]);
    Serial.println();
}

// ===========================================================================
//  5) DOWNLINKS (la pila llama a estas funciones)
// ===========================================================================
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
    Serial.printf("[downlink] datos en %s: fPort=%d, %d bytes: ",
                  mcpsIndication->RxSlot ? "RX2" : "RX1",
                  mcpsIndication->Port, mcpsIndication->BufferSize);
    for (uint8_t i = 0; i < mcpsIndication->BufferSize; i++)
        Serial.printf("%02X", mcpsIndication->Buffer[i]);
    Serial.println();
}

void downLinkAckHandle()
{
    Serial.println("[downlink] ACK del servidor: el uplink confirmado llego.");
}

// ===========================================================================
//  6) AYUDAS PARA EL MONITOR SERIE
// ===========================================================================
static void imprimirHex(const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++) Serial.printf("%02X%s", b[i], (i + 1 < n) ? " " : "");
}

static void imprimirIdentidad()
{
    Serial.println("========== CREDENCIALES (deben ser IGUALES en ChirpStack) ==========");
    Serial.print("  DevEUI  (MSB): "); imprimirHex(devEui, 8);  Serial.println();
    Serial.print("  JoinEUI (MSB): "); imprimirHex(appEui, 8);  Serial.println();
    Serial.print("  AppKey  (MSB): "); imprimirHex(appKey, 16); Serial.println();
    Serial.printf("  Region: US915   Sub-banda: %d (mascara 0x%04X 0x%04X 0x%04X 0x%04X)   LoRaWAN 1.0.x   Clase A\r\n",
                  PICARO_SUBBAND, userChannelsMask[0], userChannelsMask[1],
                  userChannelsMask[2], userChannelsMask[3]);
    Serial.println("====================================================================");
}

// ===========================================================================
//  7) SETUP y LOOP  (maquina de estados de la pila de Heltec)
// ===========================================================================
void setup()
{
    Serial.begin(115200);
    delay(1500);
    Serial.println();
    Serial.println("##########################################################");
    Serial.println("#  EJERCICIO 13 - ESTACION METEO LoRaWAN (CubeCell AB01) #");
    Serial.println("##########################################################");
    Serial.printf("[info] Sketch v%s | uplink cada %d s en fPort %d | confirmado cada %d | ADR %s\r\n",
                  SKETCH_VERSION, PICARO_UPLINK_INTERVAL_S, PICARO_UPLINK_FPORT,
                  PICARO_CONFIRMED_EVERY, loraWanAdr ? "ON" : "OFF");
    Serial.printf("[info] Sensores activos: DHT11=%d BMP280=%d BH1750=%d MH-RD=%d\r\n",
                  SENSOR_DHT11, SENSOR_BMP280, SENSOR_BH1750, SENSOR_MHRD);
    imprimirIdentidad();

    /* Vext apagado hasta que toque medir. */
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);

    deviceState = DEVICE_STATE_INIT;
    LoRaWAN.ifskipjoin();   /* con LORAWAN_Net_Reserve=OFF no hace nada */
}

void loop()
{
    switch (deviceState) {

    case DEVICE_STATE_INIT:
        Serial.println("[estado] INIT: arrancando la pila LoRaWAN de Heltec...");
        printDevParam();                       /* la pila imprime sus parametros */
        LoRaWAN.init(loraWanClass, loraWanRegion);
        deviceState = DEVICE_STATE_JOIN;
        break;

    case DEVICE_STATE_JOIN:
        Serial.println("[estado] JOIN: enviando Join-Request (OTAA). Si falla, la pila reintenta a los 30 s.");
        Serial.println("         LED rojo = transmitiendo, azul/amarillo = RX1/RX2, morado = JOIN OK.");
        LoRaWAN.join();                        /* imprime 'joining...' y 'joined' */
        break;

    case DEVICE_STATE_SEND:
        g_ciclo++;
        isTxConfirmed = (PICARO_CONFIRMED_EVERY > 0) && (g_ciclo % PICARO_CONFIRMED_EVERY == 0);
        Serial.printf("\r\n=============== UPLINK #%lu %s===============\r\n",
                      (unsigned long)g_ciclo, isTxConfirmed ? "(CONFIRMADO) " : "");
        prepareTxFrame(appPort);
        LoRaWAN.send();                        /* imprime 'unconfirmed/confirmed uplink sending ...' */
        deviceState = DEVICE_STATE_CYCLE;
        break;

    case DEVICE_STATE_CYCLE:
        txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
        /* OJO: la pila transmite en su siguiente "tick", asi que las lineas
         * "TX on freq..." de la pila aparecen DESPUES de este mensaje. */
        Serial.printf("[estado] CYCLE: proximo uplink en %lu ms (duerme tras RX2)\r\n",
                      (unsigned long)txDutyCycleTime);
        Serial.flush();   /* vaciar la UART antes de que la pila duerma el MCU */
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;

    case DEVICE_STATE_SLEEP:
        LoRaWAN.sleep();                       /* bajo consumo hasta el proximo evento */
        break;

    default:
        deviceState = DEVICE_STATE_INIT;
        break;
    }
}
