// radiosonda_PIcaro — sketch educativo original (NO forma parte de Semtech SWL2001).
// Provisto bajo Clear BSD License. (c) 2026 radiosonda_PIcaro contributors.
 /*
 * =====================================================================
 * TTGO T3 LoRaWAN - EJEMPLO DIDÁCTICO PARA INGENIERÍA
 * =====================================================================
 * 
 * OBJETIVO:
 * Demostrar el funcionamiento de LoRaWAN usando activación OTAA y
 * envío periódico de datos a un Network Server (ChirpStack).
 * 
 * CONCEPTOS CLAVE:
 * - LoRaWAN: Protocolo de comunicación de largo alcance y bajo consumo
 * - OTAA: Over-The-Air Activation (activación segura mediante Join)
 * - Uplink: Transmisión del dispositivo al servidor
 * - Downlink: Transmisión del servidor al dispositivo
 * - Sub-banda: Grupo de canales de frecuencia dentro de una región
 * 
 * TOPOLOGÍA DE RED LoRaWAN:
 * [Dispositivo] --RF--> [Gateway] --Internet--> [Network Server]
 *                                                      |
 *                                               [Application Server]
 * 
 * Autor: [Tu nombre]
 * Curso: Comunicaciones e IoT
 * =====================================================================
 */

// =====================================================================
// 1. LIBRERÍAS NECESARIAS
// =====================================================================

#include <Wire.h>        // Comunicación I2C para el display OLED
#include <U8g2lib.h>     // Librería gráfica para pantallas OLED
#include <SPI.h>         // Comunicación SPI para el módulo LoRa
#include <lmic.h>        // IBM LMIC (LoraMAC-in-C) - Stack LoRaWAN
#include <hal/hal.h>     // Hardware Abstraction Layer para LMIC

/*
 * NOTA SOBRE LMIC:
 * LMIC es la implementación del stack LoRaWAN que maneja:
 * - Capa física (PHY): Modulación LoRa, frecuencias, potencia
 * - Capa MAC: Join procedure, gestión de canales, duty cycle
 * - Cifrado: AES-128 para seguridad end-to-end
 */

// =====================================================================
// 2. CONFIGURACIÓN DEL DISPLAY OLED
// =====================================================================

/*
 * Display: SSD1306 de 128x64 píxeles
 * Interfaz: I2C (2 cables: SDA y SCL)
 * Dirección I2C: 0x3C (por defecto)
 * Pines ESP32: SDA=GPIO21, SCL=GPIO22
 */
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// =====================================================================
// 3. CONFIGURACIÓN DE PINES - MÓDULO LoRa SX1276
// =====================================================================

/*
 * El módulo LoRa se comunica mediante SPI (Serial Peripheral Interface)
 * 
 * SPI es un bus síncrono con 4 señales principales:
 * - SCK (Clock):  Señal de reloj generada por el maestro (ESP32)
 * - MISO (Master In, Slave Out): Datos del SX1276 al ESP32
 * - MOSI (Master Out, Slave In): Datos del ESP32 al SX1276
 * - SS (Slave Select): Selecciona el dispositivo SPI activo
 * 
 * Además, el SX1276 tiene pines de interrupción (DIO) para eventos:
 * - DIO0: Transmisión/Recepción completa
 * - DIO1: Timeout de recepción, FHSS change channel
 * - DIO2: FHSS change channel (modo FDD)
 */

#define LORA_SCK   5    // GPIO 5  - SPI Clock
#define LORA_MISO  19   // GPIO 19 - SPI Master In Slave Out
#define LORA_MOSI  27   // GPIO 27 - SPI Master Out Slave In
#define LORA_SS    18   // GPIO 18 - SPI Slave Select (Chip Select)
#define LORA_RST   14   // GPIO 14 - Reset del módulo LoRa
#define LORA_DIO0  26   // GPIO 26 - Interrupción DIO0 (TX/RX Done)
#define LORA_DIO1  33   // GPIO 33 - Interrupción DIO1 (RX Timeout)
#define LORA_DIO2  32   // GPIO 32 - Interrupción DIO2 (FHSS)

// =====================================================================
// 4. CREDENCIALES LoRaWAN - MODO OTAA
// =====================================================================

/*
 * OTAA (Over-The-Air Activation) - Proceso de Join
 * ================================================
 * 
 * OTAA es el método recomendado para activación segura. El proceso:
 * 
 * 1. El dispositivo envía un Join Request con:
 *    - DevEUI: Identificador único del dispositivo (EUI-64)
 *    - JoinEUI/AppEUI: Identificador de la aplicación (EUI-64)
 *    - Nonce: Número aleatorio para evitar replay attacks
 * 
 * 2. El Network Server verifica las credenciales y responde con Join Accept:
 *    - DevAddr: Dirección de red asignada al dispositivo (32 bits)
 *    - Session Keys: Claves de sesión derivadas de AppKey
 *      * NwkSKey: Network Session Key (cifrado MAC)
 *      * AppSKey: Application Session Key (cifrado payload)
 * 
 * 3. El dispositivo ya puede enviar y recibir datos cifrados
 * 
 * VENTAJAS DE OTAA vs ABP:
 * - Mayor seguridad: Claves de sesión únicas por Join
 * - Sin riesgo de Frame Counter rollover
 * - Permite re-Join si se pierde la sesión
 */

/*
 * DevEUI (Device Extended Unique Identifier)
 * ------------------------------------------
 * Identificador único de 64 bits (8 bytes) del dispositivo.
 * Similar al MAC address en redes Ethernet.
 * 
 * FORMATO: LSB (Least Significant Byte first)
 * Valor original:  0x02 0x38 0x92 0x05 0x35 0x8E 0x71 0xDB
 * Array en código:  DB   71   8E   35   05   92   38   02  (invertido)
 */
/*static const u1_t PROGMEM DEVEUI[8] = { 
    0x2E,0x44,0x5E,0xCD,0xB4,0x53,0xA0,0xCB
};*/

/*
 * JoinEUI / AppEUI (Application Extended Unique Identifier)
 * ---------------------------------------------------------
 * Identificador de 64 bits que identifica la aplicación en el Join Server.
 * En LoRaWAN 1.1+ se llama JoinEUI, en 1.0.x se llamaba AppEUI.
 * 
 * FORMATO: LSB (Least Significant Byte first)
 * Valor original:  0x50 0x52 0x46 0xF8 0x71 0x43 0xFD 0x8A
 * Array en código:  8A   FD   43   71   F8   46   52   50  (invertido)
 */
/*static const u1_t PROGMEM APPEUI[8] = { 
    #0x8A, 0xFD, 0x43, 0x71, 0xF8, 0x46, 0x52, 0x50
    0x62, 0xDF, 0xC9, 0x83, 0xE5, 0x15, 0x99, 0x6A 
};*/

/*
 * AppKey (Application Key)
 * ------------------------
 * Clave secreta de 128 bits (16 bytes) compartida entre el dispositivo
 * y el Join Server. Se usa para derivar las claves de sesión.
 * 
 * FORMATO: MSB (Most Significant Byte first)
 * Esta clave NO se transmite por el aire. Se usa para:
 * 1. Cifrar el Join Request
 * 2. Verificar el Join Accept
 * 3. Derivar NwkSKey y AppSKey
 * 
 * SEGURIDAD: Esta clave debe mantenerse secreta y almacenarse de forma
 * segura (idealmente en hardware seguro como Secure Element).
 */
/*static const u1_t PROGMEM APPKEY[16] = { 
    #0x8A, 0xC5, 0x83, 0xDF, 0xEE, 0xC7, 0x6C, 0x81, 
    #0xFF, 0xD1, 0x9C, 0xCF, 0xE7, 0x6B, 0x73, 0xBF 
    0x62, 0xDF, 0xC9, 0x83, 0xE5, 0x15, 0x99, 0x6A
};*/

// DevEUI:  02389205358e71db (convertido a LSB)
static const u1_t PROGMEM DEVEUI[8] = { 0xDB, 0x71, 0x8E, 0x35, 0x05, 0x92, 0x38, 0x02 };

// JoinEUI/AppEUI: 505246f87143fd8a (convertido a LSB)
static const u1_t PROGMEM APPEUI[8] = { 0x8A, 0xFD, 0x43, 0x71, 0xF8, 0x46, 0x52, 0x50 };

// AppKey: (ya en MSB)
static const u1_t PROGMEM APPKEY[16] = { 0x8A, 0xC5, 0x83, 0xDF, 0xEE, 0xC7, 0x6C, 0x81, 0xFF, 0xD1, 0x9C, 0xCF, 0xE7, 0x6B, 0x73, 0xBF };


/*
 * Funciones callback requeridas por LMIC
 * ---------------------------------------
 * LMIC llama estas funciones cuando necesita las credenciales.
 * Se usan PROGMEM para almacenar las credenciales en Flash y ahorrar RAM.
 */
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

// =====================================================================
// 5. VARIABLES GLOBALES DEL SISTEMA
// =====================================================================

/*
 * Job de transmisión
 * ------------------
 * LMIC usa un sistema de jobs (tareas programadas) para manejar eventos.
 * osjob_t es una estructura que representa una tarea en el scheduler.
 */
static osjob_t sendjob;

/*
 * Configuración de temporización
 * ------------------------------
 * LoRaWAN tiene restricciones de duty cycle (tiempo de transmisión permitido)
 * En Europa: 1% duty cycle (36 segundos de TX por hora)
 * En USA: No hay restricción de duty cycle, pero hay fair use policy
 */
const unsigned TX_INTERVAL = 30; // Intervalo de transmisión en segundos

/*
 * Variables de estado
 * -------------------
 */
bool deviceJoined = false;      // ¿El dispositivo completó el Join?
unsigned long packetCount = 0;  // Contador de paquetes enviados

// =====================================================================
// 6. CONFIGURACIÓN DEL HARDWARE (PINMAP)
// =====================================================================

/*
 * Pin Mapping para LMIC
 * ---------------------
 * Esta estructura le indica a LMIC qué pines usar para comunicarse
 * con el módulo LoRa SX1276/SX1278.
 * 
 * LMIC_UNUSED_PIN se usa para pines que no están conectados o no se usan.
 */
const lmic_pinmap lmic_pins = {
    .nss = LORA_SS,           // Slave Select (Chip Select)
    .rxtx = LMIC_UNUSED_PIN,  // No usado en SX1276 (solo en SX1272)
    .rst = LORA_RST,          // Reset del módulo
    .dio = {                  // Array de pines de interrupción
        LORA_DIO0,            // DIO0: TxDone / RxDone
        LORA_DIO1,            // DIO1: RxTimeout / FhssChangeChannel
        LORA_DIO2             // DIO2: FhssChangeChannel
    }
};

// =====================================================================
// 7. FUNCIÓN: ACTUALIZAR DISPLAY OLED
// =====================================================================

void updateDisplay() {
    /*
     * Esta función actualiza el contenido del display OLED.
     * 
     * Flujo de trabajo con U8g2:
     * 1. clearBuffer(): Limpia el buffer de memoria (no la pantalla física)
     * 2. setFont(): Selecciona la fuente tipográfica
     * 3. drawStr(): Dibuja texto en el buffer
     * 4. sendBuffer(): Transfiere el buffer a la pantalla física
     * 
     * Este enfoque (buffered) permite actualizaciones rápidas sin parpadeo.
     */
    
    u8g2.clearBuffer();
    
    // Título principal con fuente más grande
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 12, "TTGO LoRaWAN");
    
    // Cambiar a fuente más pequeña para detalles
    u8g2.setFont(u8g2_font_6x12_tf);
    
    // Mostrar estado de conexión
    if (deviceJoined) {
        u8g2.drawStr(0, 28, "Estado: CONECTADO");
    } else {
        u8g2.drawStr(0, 28, "Estado: JOINING...");
    }

    // Mostrar contador de paquetes
    char countStr[32];
    sprintf(countStr, "Paquetes: %lu", packetCount);
    u8g2.drawStr(0, 44, countStr);

    // Información de configuración
    u8g2.drawStr(0, 60, "US915 SubBand 2");

    // Enviar buffer a la pantalla física
    u8g2.sendBuffer();
}

// =====================================================================
// 8. DECLARACIÓN ANTICIPADA DE FUNCIONES
// =====================================================================

/*
 * Forward declaration de do_send()
 * --------------------------------
 * En C/C++, las funciones deben declararse antes de usarse.
 * Como onEvent() necesita llamar a do_send(), pero do_send() se define
 * después, necesitamos esta declaración anticipada.
 */
void do_send(osjob_t* j);

// =====================================================================
// 9. MANEJADOR DE EVENTOS LoRaWAN
// =====================================================================

void onEvent (ev_t ev) {
    /*
     * CALLBACK DE EVENTOS
     * ===================
     * 
     * LMIC llama esta función cuando ocurren eventos importantes en el
     * stack LoRaWAN. Es el corazón de la máquina de estados del protocolo.
     * 
     * EVENTOS PRINCIPALES:
     * - EV_JOINING: Iniciando procedimiento de Join
     * - EV_JOINED: Join exitoso, dispositivo activado
     * - EV_JOIN_FAILED: Join falló, se reintentará
     * - EV_TXCOMPLETE: Transmisión completada (y ventanas RX cerradas)
     * 
     * NOTA: Esta función se ejecuta en contexto de interrupción,
     * por lo que debe ser rápida y no bloqueante.
     */
    
    // Imprimir timestamp del evento (en ticks del sistema LMIC)
    Serial.print(os_getTime());
    Serial.print(": ");
    
    // Switch para manejar cada tipo de evento
    switch(ev) {
        
        // -----------------------------------------------------------------
        // EVENTOS DE BEACONING (Clase B)
        // -----------------------------------------------------------------
        /*
         * Clase B permite downlinks programados usando beacons del gateway.
         * No se usa en este ejemplo (Clase A es suficiente para la mayoría).
         */
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        
        // -----------------------------------------------------------------
        // EVENTO: INICIANDO JOIN
        // -----------------------------------------------------------------
        case EV_JOINING:
            /*
             * El dispositivo está enviando un Join Request.
             * 
             * QUÉ SUCEDE:
             * 1. Se construye el mensaje Join Request con DevEUI y JoinEUI
             * 2. Se añade un nonce aleatorio para evitar replay attacks
             * 3. Se calcula el MIC (Message Integrity Code) usando AppKey
             * 4. Se transmite en un canal aleatorio de la sub-banda
             * 5. Se abre ventana RX1 (5 seg después) y RX2 (6 seg después)
             * 
             * El dispositivo esperará un Join Accept del Network Server.
             */
            Serial.println(F("EV_JOINING - Intentando unirse a la red..."));
            Serial.println(F("  → Enviando Join Request con DevEUI y JoinEUI"));
            Serial.println(F("  → Esperando Join Accept del Network Server..."));
            deviceJoined = false;
            updateDisplay();
            break;
        
        // -----------------------------------------------------------------
        // EVENTO: JOIN EXITOSO
        // -----------------------------------------------------------------
        case EV_JOINED:
            /*
             * ¡Join exitoso! El Network Server aceptó al dispositivo.
             * 
             * QUÉ SUCEDIÓ:
             * 1. Network Server verificó DevEUI, JoinEUI y MIC
             * 2. Asignó un DevAddr (dirección de red) al dispositivo
             * 3. Derivó las claves de sesión (NwkSKey y AppSKey)
             * 4. Envió Join Accept cifrado con AppKey
             * 5. LMIC descifró el Join Accept y extrajo DevAddr y claves
             * 
             * AHORA EL DISPOSITIVO PUEDE:
             * - Enviar uplinks cifrados con AppSKey
             * - Recibir downlinks del servidor
             * - El Frame Counter se inicializa en 0
             */
            Serial.println(F("========================================"));
            Serial.println(F("✅ EV_JOINED - ¡CONECTADO A CHIRPSTACK!"));
            Serial.println(F("========================================"));
            Serial.println(F("  → DevAddr asignado por el servidor"));
            Serial.println(F("  → Claves de sesión establecidas:"));
            Serial.println(F("     - NwkSKey: Para integridad MAC"));
            Serial.println(F("     - AppSKey: Para cifrado payload"));
            Serial.println(F("  → Frame Counter inicializado en 0"));
            
            /*
             * LMIC_setLinkCheckMode(0)
             * ------------------------
             * Desactiva el Link Check MAC command.
             * Link Check se usa para verificar la conectividad, pero
             * consume airtime y no es necesario para este ejemplo.
             */
            LMIC_setLinkCheckMode(0);
            
            deviceJoined = true;
            updateDisplay();
            
            // Programar primer envío después de 2 segundos
            Serial.println(F("  → Programando primer envío de datos..."));
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(2), do_send);
            break;
        
        // -----------------------------------------------------------------
        // EVENTO: JOIN FALLIDO
        // -----------------------------------------------------------------
        case EV_JOIN_FAILED:
            /*
             * El Join Request falló. Posibles causas:
             * 
             * 1. No se recibió Join Accept (gateway fuera de alcance)
             * 2. Credenciales incorrectas (DevEUI, JoinEUI o AppKey)
             * 3. Interferencia en el canal
             * 4. Gateway no está conectado al Network Server
             * 5. Dispositivo no está registrado en ChirpStack
             * 
             * LMIC reintentará automáticamente con backoff exponencial.
             */
            Serial.println(F("❌ EV_JOIN_FAILED - Join falló"));
            Serial.println(F("  Posibles causas:"));
            Serial.println(F("  - Gateway fuera de alcance"));
            Serial.println(F("  - Credenciales incorrectas"));
            Serial.println(F("  - Gateway no conectado al servidor"));
            Serial.println(F("  → LMIC reintentará automáticamente..."));
            deviceJoined = false;
            updateDisplay();
            break;
        
        // -----------------------------------------------------------------
        // EVENTO: RE-JOIN FALLIDO
        // -----------------------------------------------------------------
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        
        // -----------------------------------------------------------------
        // EVENTO: TRANSMISIÓN COMPLETA
        // -----------------------------------------------------------------
        case EV_TXCOMPLETE:
            /*
             * EVENTO MÁS IMPORTANTE: Transmisión completada
             * ==============================================
             * 
             * Este evento se dispara cuando:
             * 1. El uplink fue transmitido completamente
             * 2. Las ventanas de recepción RX1 y RX2 cerraron
             * 3. Se procesó cualquier downlink recibido
             * 
             * CLASE A - VENTANAS DE RECEPCIÓN:
             * --------------------------------
             * Después de cada uplink, se abren 2 ventanas de recepción:
             * 
             * RX1: Se abre 1 segundo después del uplink
             *      - Usa la misma frecuencia que el uplink (con offset)
             *      - Usa el mismo Data Rate que el uplink
             * 
             * RX2: Se abre 2 segundos después del uplink
             *      - Usa una frecuencia fija (923.3 MHz en US915)
             *      - Usa Data Rate fijo (SF12 en US915)
             * 
             * Si no hay downlink en RX1 ni RX2, se cierra y ocurre EV_TXCOMPLETE.
             */
            Serial.println(F("========================================"));
            Serial.println(F("📤 EV_TXCOMPLETE - Transmisión completa"));
            Serial.println(F("========================================"));
            Serial.println(F("  → Uplink enviado exitosamente"));
            Serial.println(F("  → Ventanas RX1 y RX2 cerradas"));
            
            // Verificar si se recibió ACK (Acknowledged uplink)
            if (LMIC.txrxFlags & TXRX_ACK) {
                /*
                 * ACK recibido del Network Server
                 * --------------------------------
                 * Esto confirma que el servidor recibió el uplink.
                 * Los ACKs se usan en uplinks "confirmed" (no en este ejemplo).
                 */
                Serial.println(F("  ✅ ACK recibido del Network Server"));
            }
            
            // Verificar si se recibió downlink
            if (LMIC.dataLen) {
                /*
                 * DOWNLINK RECIBIDO
                 * -----------------
                 * El servidor envió datos al dispositivo.
                 * Los downlinks pueden contener:
                 * - Datos de aplicación
                 * - MAC commands (configuración del dispositivo)
                 * 
                 * El payload está en: LMIC.frame[LMIC.dataBeg ... dataBeg+dataLen]
                 */
                Serial.print(F("  📥 Downlink recibido ("));
                Serial.print(LMIC.dataLen);
                Serial.print(F(" bytes) en puerto "));
                Serial.print(LMIC.frame[LMIC.dataBeg-1]); // Puerto está antes del payload
                Serial.println(F(":"));
                Serial.print(F("     Payload: "));
                
                // Imprimir payload en hexadecimal
                for (int i = 0; i < LMIC.dataLen; i++) {
                    if (LMIC.frame[LMIC.dataBeg + i] < 0x10) 
                        Serial.print("0");
                    Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
            } else {
                Serial.println(F("  No se recibió downlink"));
            }
            
            // Programar siguiente transmisión
            if (deviceJoined) {
                Serial.print(F("  ⏱️  Próximo uplink en "));
                Serial.print(TX_INTERVAL);
                Serial.println(F(" segundos"));
                Serial.println(F("========================================"));
                
                /*
                 * os_setTimedCallback()
                 * ---------------------
                 * Programa la ejecución de do_send() después de TX_INTERVAL.
                 * sec2osticks() convierte segundos a ticks del sistema LMIC.
                 */
                os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            }
            updateDisplay();
            break;
        
        // -----------------------------------------------------------------
        // OTROS EVENTOS
        // -----------------------------------------------------------------
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC - Sincronización temporal perdida"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET - Stack LMIC reiniciado"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE - Recepción completa (Clase B/C)"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD - Enlace muerto (sin respuesta del servidor)"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE - Enlace activo (servidor respondió)"));
            break;
        
        // Evento desconocido
        default:
            Serial.print(F("⚠️  Evento desconocido: "));
            Serial.println((unsigned) ev);
            break;
    }
}

// =====================================================================
// 10. FUNCIÓN: ENVIAR PAQUETE DE PRUEBA
// =====================================================================

void do_send(osjob_t* j) {
    /*
     * FUNCIÓN DE TRANSMISIÓN
     * ======================
     * 
     * Esta función construye y envía un uplink LoRaWAN.
     * 
     * ESTRUCTURA DE UN UPLINK LoRaWAN:
     * --------------------------------
     * [MHDR][DevAddr][FCtrl][FCnt][FOpts][FPort][Payload][MIC]
     * 
     * Donde:
     * - MHDR: MAC Header (tipo de mensaje)
     * - DevAddr: Dirección del dispositivo (asignada en Join)
     * - FCtrl: Frame Control (opciones, ACK, ADR)
     * - FCnt: Frame Counter (contador incremental, anti-replay)
     * - FOpts: MAC commands opcionales
     * - FPort: Puerto de aplicación (0-223)
     * - Payload: Datos de aplicación (cifrados con AppSKey)
     * - MIC: Message Integrity Code (4 bytes, calculado con NwkSKey)
     * 
     * LMIC maneja automáticamente toda esta estructura.
     * Nosotros solo proporcionamos: Puerto, Payload y si queremos ACK.
     */
    
    // -----------------------------------------------------------------
    // VERIFICAR ESTADO DE TRANSMISIÓN
    // -----------------------------------------------------------------
    
    /*
     * OP_TXRXPEND: Transmission/Reception Pending
     * --------------------------------------------
     * Esta bandera indica que hay una transmisión en curso o que
     * las ventanas RX están abiertas. No podemos enviar otro uplink
     * hasta que se complete.
     */
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("⚠️  OP_TXRXPEND - TX/RX en progreso, esperando..."));
        return;
    }

    // -----------------------------------------------------------------
    // INCREMENTAR CONTADOR
    // -----------------------------------------------------------------
    packetCount++;

    // -----------------------------------------------------------------
    // CONSTRUIR PAYLOAD
    // -----------------------------------------------------------------
    
    /*
     * DISEÑO DEL PAYLOAD (8 bytes)
     * =============================
     * 
     * Byte 0:     Tipo de mensaje (0x99 = paquete de prueba)
     * Bytes 1-2:  Número de paquete (16 bits, little-endian)
     * Bytes 3-6:  Timestamp en segundos (32 bits, little-endian)
     * Byte 7:     Checksum (suma de bytes 0-6)
     * 
     * LITTLE-ENDIAN:
     * El byte menos significativo (LSB) se almacena primero.
     * Ejemplo: 0x12345678 se guarda como [78][56][34][12]
     * 
     * POR QUÉ UN CHECKSUM:
     * Aunque LoRaWAN ya tiene MIC para integridad, un checksum
     * simple ayuda a detectar errores en la capa de aplicación.
     */
    
    uint8_t payload[8];
    
    // Byte 0: Tipo de mensaje
    payload[0] = 0x99;  // 0x99 = Identificador de "paquete de prueba"
    
    // Bytes 1-2: Contador de paquetes (16 bits)
    payload[1] = packetCount & 0xFF;         // Byte bajo
    payload[2] = (packetCount >> 8) & 0xFF;  // Byte alto
    
    // Bytes 3-6: Timestamp en segundos desde el inicio (32 bits)
    uint32_t timestamp = millis() / 1000;    // Convertir ms a segundos
    payload[3] = timestamp & 0xFF;           // Byte 0 (LSB)
    payload[4] = (timestamp >> 8) & 0xFF;    // Byte 1
    payload[5] = (timestamp >> 16) & 0xFF;   // Byte 2
    payload[6] = (timestamp >> 24) & 0xFF;   // Byte 3 (MSB)
    
    // Byte 7: Checksum simple (suma de bytes anteriores)
    payload[7] = 0;
    for (int i = 0; i < 7; i++) {
        payload[7] += payload[i];
    }
    // El checksum es de 8 bits, por lo que automáticamente hace modulo 256

    // -----------------------------------------------------------------
    // ENVIAR UPLINK
    // -----------------------------------------------------------------
    
    /*
     * LMIC_setTxData2()
     * -----------------
     * Encola un uplink para transmisión.
     * 
     * Parámetros:
     * 1. FPort (1-223): Puerto de aplicación
     *    - Puerto 0: Reservado para MAC commands
     *    - Puerto 1-223: Uso de aplicación
     *    - Puerto 224-255: Reservado para pruebas
     * 2. Payload: Puntero al array de bytes a enviar
     * 3. Length: Tamaño del payload (máximo 51 bytes en US915 con SF10)
     * 4. Confirmed: 0=Unconfirmed, 1=Confirmed (requiere ACK del servidor)
     * 
     * CONFIRMED vs UNCONFIRMED:
     * - Unconfirmed (0): Más eficiente, no requiere ACK, para datos no críticos
     * - Confirmed (1): Requiere ACK, consume más airtime, para datos críticos
     * 
     * MÁXIMO TAMAÑO DE PAYLOAD:
     * Depende del Data Rate (SF - Spreading Factor):
     * - DR0 (SF10): 11 bytes
     * - DR1 (SF9):  53 bytes
     * - DR2 (SF8):  125 bytes
     * - DR3 (SF7):  242 bytes
     * - DR4 (SF8):  242 bytes en BW500
     */
    LMIC_setTxData2(1, payload, sizeof(payload), 0);
    
    // -----------------------------------------------------------------
    // MOSTRAR INFORMACIÓN EN SERIAL
    // -----------------------------------------------------------------
    
    Serial.println(F("========================================"));
    Serial.print(F("📤 ENVIANDO PAQUETE #"));
    Serial.println(packetCount);
    Serial.println(F("========================================"));
    
    Serial.print(F("  Puerto (FPort):        "));
    Serial.println(1);
    
    Serial.print(F("  Tamaño del payload:    "));
    Serial.print(sizeof(payload));
    Serial.println(F(" bytes"));
    
    Serial.print(F("  Tipo de uplink:        "));
    Serial.println(F("Unconfirmed (sin ACK)"));
    
    Serial.print(F("  Data Rate actual:      DR"));
    Serial.println(LMIC.datarate);
    
    Serial.print(F("  Potencia TX:           "));
    Serial.print(LMIC.txpow);
    Serial.println(F(" dBm"));
    
    Serial.print(F("  Frecuencia:            "));
    Serial.print(LMIC.freq / 1000000.0, 1);
    Serial.println(F(" MHz"));
    
    Serial.println(F("  ----------------------------------------"));
    Serial.print(F("  Payload (HEX):         "));
    for (int i = 0; i < sizeof(payload); i++) {
        if (payload[i] < 0x10) Serial.print("0");
        Serial.print(payload[i], HEX);
        if (i < sizeof(payload) - 1) Serial.print(" ");
    }
    Serial.println();
    
    Serial.println(F("  ----------------------------------------"));
    Serial.println(F("  Decodificación:"));
    Serial.print(F("    Tipo mensaje:        0x"));
    Serial.println(payload[0], HEX);
    
    Serial.print(F("    Número de paquete:   "));
    uint16_t pktNum = payload[1] | (payload[2] << 8);
    Serial.println(pktNum);
    
    Serial.print(F("    Timestamp:           "));
    Serial.print(timestamp);
    Serial.println(F(" segundos"));
    
    Serial.print(F("    Checksum:            0x"));
    Serial.println(payload[7], HEX);
    
    Serial.println(F("========================================"));
    Serial.println();
}

// =====================================================================
// 11. FUNCIÓN: SETUP (INICIALIZACIÓN)
// =====================================================================

void setup() {
    /*
     * FUNCIÓN SETUP
     * =============
     * 
     * Se ejecuta UNA SOLA VEZ al iniciar o reiniciar el ESP32.
     * Aquí configuramos todo el hardware y el stack LoRaWAN.
     */
    
    // -----------------------------------------------------------------
    // INICIALIZAR PUERTO SERIAL
    // -----------------------------------------------------------------
    
    /*
     * Serial a 115200 baud (bits por segundo)
     * ----------------------------------------
     * Baudrate alto para transmisión rápida de datos de debug.
     * IMPORTANTE: Configurar el Serial Monitor a la misma velocidad.
     */
    Serial.begin(115200);
    delay(2000);  // Esperar a que el Serial esté listo
    
    // -----------------------------------------------------------------
    // BANNER DE INICIO
    // -----------------------------------------------------------------
    
    Serial.println(F("\n"));
    Serial.println(F("========================================"));
    Serial.println(F("   TTGO T3 LoRaWAN - EJEMPLO DIDÁCTICO"));
    Serial.println(F("========================================"));
    Serial.println(F("   Curso: Comunicaciones e IoT"));
    Serial.println(F("   Tema: Redes LPWAN - LoRaWAN"));
    Serial.println(F("========================================"));
    Serial.println();
    
    Serial.println(F("📡 CONFIGURACIÓN DE RED:"));
    Serial.println(F("  ├─ Región:           US915 (América)"));
    Serial.println(F("  ├─ Sub-banda:        2 (Canales 8-15)"));
    Serial.println(F("  ├─ Frecuencias:      903.9 - 905.3 MHz"));
    Serial.println(F("  ├─ Modo activación:  OTAA"));
    Serial.println(F("  ├─ Clase:            A (bidireccional)"));
    Serial.println(F("  └─ Intervalo TX:     30 segundos"));
    Serial.println();
    
    // -----------------------------------------------------------------
    // CONFIGURAR GPIOs SEGUROS
    // -----------------------------------------------------------------
    
    /*
     * GPIO 0: Boot button - Configurar como INPUT con pull-up
     * GPIO 2: LED interno - Configurar como OUTPUT en LOW
     * 
     * Estos pines tienen funciones especiales en el ESP32 y es
     * importante configurarlos correctamente para evitar problemas.
     */
    pinMode(0, INPUT_PULLUP);
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    // -----------------------------------------------------------------
    // INICIALIZAR DISPLAY OLED
    // -----------------------------------------------------------------
    
    Serial.println(F("📺 INICIALIZANDO DISPLAY OLED..."));
    
    /*
     * Wire.begin(SDA, SCL)
     * --------------------
     * Inicializa el bus I2C con los pines especificados.
     * En ESP32, los pines I2C son configurables por software.
     * 
     * SDA (GPIO 21): Serial Data - Línea de datos bidireccional
     * SCL (GPIO 22): Serial Clock - Línea de reloj generada por maestro
     * 
     * I2C es un bus multi-maestro, multi-esclavo:
     * - Puede haber múltiples dispositivos en el mismo bus
     * - Cada dispositivo tiene una dirección única (7 bits)
     * - El OLED SSD1306 típicamente usa dirección 0x3C
     */
    Wire.begin(21, 22);
    
    /*
     * Wire.setClock()
     * ---------------
     * Configura la velocidad del bus I2C.
     * - 100 kHz: Modo estándar (Standard Mode)
     * - 400 kHz: Modo rápido (Fast Mode)
     * 
     * Usamos 100 kHz por compatibilidad y menor consumo.
     */
    Wire.setClock(100000);
    
    /*
     * u8g2.begin()
     * ------------
     * Inicializa el controlador del display y lo pone en modo activo.
     */
    u8g2.begin();
    updateDisplay();
    
    Serial.println(F("  └─ ✅ OLED inicializado correctamente"));
    Serial.println();

    // -----------------------------------------------------------------
    // INICIALIZAR BUS SPI Y STACK LoRaWAN
    // -----------------------------------------------------------------
    
    Serial.println(F("📡 INICIALIZANDO MÓDULO LoRa..."));
    
    /*
     * SPI.begin(SCK, MISO, MOSI, SS)
     * ------------------------------
     * Inicializa el bus SPI con los pines especificados.
     * 
     * SPI (Serial Peripheral Interface):
     * - Comunicación síncrona full-duplex
     * - Hasta varios MHz de velocidad
     * - Modo maestro-esclavo
     * - 4 líneas: SCK, MISO, MOSI, SS
     * 
     * El SX1276 soporta hasta 10 MHz en modo SPI.
     */
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    
    /*
     * os_init()
     * ---------
     * Inicializa el sistema operativo de LMIC (scheduler, timers).
     * LMIC tiene su propio mini-RTOS para manejar eventos y tareas.
     */
    os_init();
    
    /*
     * LMIC_reset()
     * ------------
     * Reinicia el stack LoRaWAN a estado inicial:
     * - Reinicia el módulo LoRa (pulso en pin RST)
     * - Limpia todas las variables de sesión
     * - Configura registros del SX1276 a valores por defecto
     * - Limpia la cola de eventos
     */
    LMIC_reset();
    
    Serial.println(F("  ├─ Bus SPI inicializado"));
    Serial.println(F("  ├─ Stack LMIC inicializado"));
    Serial.println(F("  └─ Módulo SX1276 en modo IDLE"));
    Serial.println();
    
    // -----------------------------------------------------------------
    // CONFIGURAR PLAN DE CANALES (CHANNEL PLAN)
    // -----------------------------------------------------------------
    
    Serial.println(F("📻 CONFIGURANDO PLAN DE CANALES..."));
    
    /*
     * PLAN DE CANALES US915
     * =====================
     * 
     * US915 tiene 72 canales de uplink:
     * - 64 canales @ 125 kHz: 902.3 a 914.9 MHz (cada 200 kHz)
     * - 8 canales @ 500 kHz: 903.0 a 914.2 MHz (cada 1.6 MHz)
     * 
     * SUB-BANDAS:
     * Los 64 canales se dividen en 8 sub-bandas de 8 canales cada una.
     * Esto permite que múltiples gateways operen en paralelo sin
     * interferirse, asignando diferentes sub-bandas a cada uno.
     * 
     * SUB-BANDA 2 (índice 1):
     * - Canales 8-15
     * - Frecuencias: 903.9, 904.1, 904.3, 904.5, 904.7, 904.9, 905.1, 905.3 MHz
     * 
     * IMPORTANTE: El gateway debe estar configurado para la misma sub-banda.
     */
    LMIC_selectSubBand(1);  // Sub-banda 2 (índice base-0: 0=SubBand1, 1=SubBand2, etc.)
    
    Serial.println(F("  ├─ Región:            US915"));
    Serial.println(F("  ├─ Sub-banda activa:  2 (índice 1)"));
    Serial.println(F("  ├─ Canales activos:   8-15"));
    Serial.println(F("  └─ Rango frecuencias: 903.9 - 905.3 MHz"));
    Serial.println();
    
    // -----------------------------------------------------------------
    // COMPENSACIÓN DE RELOJ
    // -----------------------------------------------------------------
    
    /*
     * LMIC_setClockError()
     * --------------------
     * Compensa la imprecisión del cristal del ESP32.
     * 
     * Los cristales tienen tolerancia típica de ±10-20 ppm (partes por millón).
     * Esta imprecisión causa drift en las ventanas de recepción RX.
     * 
     * MAX_CLOCK_ERROR típicamente es 65536 (100%).
     * MAX_CLOCK_ERROR * 1 / 100 = 655 (1% de error)
     * 
     * Esto amplía las ventanas RX para compensar el drift del reloj.
     * Mayor error → ventanas RX más largas → mayor consumo de energía.
     */
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    
    Serial.println(F("⚙️  PARÁMETROS ADICIONALES:"));
    Serial.println(F("  ├─ Compensación reloj: 1%"));
    Serial.println(F("  ├─ ADR habilitado:     Sí (Adaptive Data Rate)"));
    Serial.println(F("  └─ Link Check:         Deshabilitado"));
    Serial.println();
    
    // -----------------------------------------------------------------
    // INICIAR PROCEDIMIENTO DE JOIN
    // -----------------------------------------------------------------
    
    /*
     * LMIC_startJoining()
     * -------------------
     * Inicia el procedimiento de activación OTAA.
     * 
     * QUÉ HACE:
     * 1. Construye un Join Request con DevEUI, JoinEUI y nonce aleatorio
     * 2. Calcula el MIC usando AppKey
     * 3. Selecciona un canal aleatorio de la sub-banda activa
     * 4. Transmite el Join Request
     * 5. Abre ventanas RX para esperar Join Accept
     * 
     * Si no recibe Join Accept:
     * - Reintenta con backoff exponencial
     * - Cambia a diferentes canales aleatoriamente
     * - Continúa indefinidamente hasta tener éxito
     */
    Serial.println(F("========================================"));
    Serial.println(F("🚀 INICIANDO PROCEDIMIENTO DE JOIN"));
    Serial.println(F("========================================"));
    Serial.println();
    Serial.println(F("  ⏳ Enviando Join Request..."));
    Serial.println(F("  ⏳ Esperando Join Accept del servidor..."));
    Serial.println(F("  ⏳ Este proceso puede tomar 10-30 segundos..."));
    Serial.println();
    Serial.println(F("  💡 VERIFICAR:"));
    Serial.println(F("     1. Gateway está encendido y conectado"));
    Serial.println(F("     2. Credenciales correctas en ChirpStack"));
    Serial.println(F("     3. Dispositivo registrado en ChirpStack"));
    Serial.println(F("     4. Gateway en sub-banda 2 (US915)"));
    Serial.println();
    
    LMIC_startJoining();
}

// =====================================================================
// 12. FUNCIÓN: LOOP (BUCLE PRINCIPAL)
// =====================================================================

void loop() {
    /*
     * FUNCIÓN LOOP
     * ============
     * 
     * Se ejecuta CONTINUAMENTE en un bucle infinito después de setup().
     * Es el corazón del programa Arduino.
     * 
     * En este caso, el bucle es muy simple porque LMIC maneja
     * todo el trabajo pesado mediante su scheduler interno.
     */
    
    // -----------------------------------------------------------------
    // EJECUTAR SCHEDULER DE LMIC
    // -----------------------------------------------------------------
    
    /*
     * os_runloop_once()
     * -----------------
     * Ejecuta UNA ITERACIÓN del scheduler de LMIC.
     * 
     * QUÉ HACE:
     * 1. Verifica si hay tareas programadas (jobs) listas para ejecutar
     * 2. Procesa interrupciones del módulo LoRa (DIO0, DIO1, DIO2)
     * 3. Maneja timeouts y timers
     * 4. Ejecuta la máquina de estados del protocolo LoRaWAN
     * 5. Retorna inmediatamente (no bloqueante)
     * 
     * IMPORTANTE:
     * Esta función DEBE llamarse frecuentemente (cada pocos ms)
     * para que LMIC funcione correctamente. Si se bloquea el loop
     * con delay() largos, LMIC puede perder eventos y fallar.
     */
    os_runloop_once();

    // -----------------------------------------------------------------
    // ACTUALIZAR DISPLAY PERIÓDICAMENTE
    // -----------------------------------------------------------------
    
    /*
     * Actualizar el display cada 1 segundo para mostrar:
     * - Estado de conexión (Joining / Conectado)
     * - Contador de paquetes enviados
     * - Información de red
     * 
     * Usamos una variable estática para recordar el último update.
     * Las variables estáticas mantienen su valor entre llamadas.
     */
    static unsigned long lastDisplayUpdate = 0;
    
    if (millis() - lastDisplayUpdate >= 1000) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
    /*
     * NOTA SOBRE CONSUMO DE ENERGÍA:
     * ===============================
     * 
     * Este código está optimizado para demostración, no para bajo consumo.
     * 
     * Para aplicaciones con batería, considerar:
     * 1. Deep Sleep entre transmisiones
     * 2. Desactivar display OLED
     * 3. Reducir frecuencia de transmisión
     * 4. Usar Class A con SF más bajo (menor tiempo en aire)
     * 
     * Consumo típico:
     * - TX @ 20 dBm: ~120 mA durante 1-2 segundos
     * - RX: ~12 mA durante ventanas RX
     * - Deep Sleep: ~10 µA
     * - Display OLED: ~20 mA constante
     */
}

// =====================================================================
// FIN DEL PROGRAMA
// =====================================================================

/*
 * RESUMEN DE CONCEPTOS CLAVE
 * ===========================
 * 
 * 1. LoRaWAN es un protocolo LPWAN (Low Power Wide Area Network)
 *    - Largo alcance: 2-15 km dependiendo del entorno
 *    - Bajo consumo: Años de batería en aplicaciones bien diseñadas
 *    - Baja tasa de datos: 0.3 - 50 kbps
 * 
 * 2. Clases de dispositivos LoRaWAN:
 *    - Clase A: Bidireccional, menor consumo (usado aquí)
 *    - Clase B: Downlinks programados con beacons
 *    - Clase C: Recepción continua, mayor consumo
 * 
 * 3. Seguridad:
 *    - Cifrado AES-128 end-to-end
 *    - Claves únicas por dispositivo (OTAA)
 *    - MIC para integridad de mensajes
 *    - Frame Counter para prevenir replay attacks
 * 
 * 4. Arquitectura LoRaWAN:
 *    - End Device (este código)
 *    - Gateway (puente RF-IP)
 *    - Network Server (ChirpStack)
 *    - Application Server (decodifica payloads)
 * 
 * 5. Adaptive Data Rate (ADR):
 *    - El servidor ajusta SF y potencia TX automáticamente
 *    - Optimiza airtime y consumo de energía
 *    - Basado en calidad de señal (SNR, RSSI)
 * 
 * PROXIMOS EJERCICIOS PARA LABORATORIO:
 * ========================================
 * 
 * 1. Modificar el payload para incluir temperatura de un sensor
 * 2. Implementar un comando downlink para cambiar el intervalo TX
 * 3. Medir y graficar RSSI y SNR de cada uplink
 * 4. Calcular el Time-on-Air y verificar duty cycle compliance
 * 5. Experimentar con diferentes Data Rates y medir alcance
 * 6. Implementar deep sleep para optimizar consumo de batería
 * 7. Crear un dashboard en ChirpStack para visualizar los datos
 * 
 * REFERENCIAS:
 * ============
 * - LoRaWAN Specification: https://lora-alliance.org/resource_hub/lorawan-specification-v1-1/
 * - ChirpStack Documentation: https://www.chirpstack.io/docs/
 * - MCCI LMIC Library: https://github.com/mcci-catena/arduino-lmic
 * - Semtech SX1276 Datasheet
 */