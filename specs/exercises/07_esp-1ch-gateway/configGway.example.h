/*
 * configGway.example.h — Ejemplo de configuración para ESP-1ch-Gateway
 * =====================================================================
 *
 * Fragmento de referencia para el fichero `src/configGway.h` del firmware DE TERCEROS
 * **ESP-1ch-Gateway** (Maarten Westenberg / things4u, licencia MIT):
 *     https://github.com/things4u/ESP-1ch-Gateway
 *
 * ESTE NO ES CÓDIGO DE radiosonda_PIcaro NI DE SEMTECH. Es solo un ejemplo comentado, aportado
 * por radiosonda_PIcaro con fines educativos, de cómo ajustar los #define de ese firmware para:
 *   - placa TTGO ESP32 (SX1276),
 *   - región EU868, canal 0 (868.1 MHz), SF9,
 *   - reenviar los paquetes a un ChirpStack v4 por Semtech UDP (puerto 1700).
 *
 * Cópialo/adáptalo sobre tu configGway.h real. Ajusta la WiFi y la ubicación en configNode.h.
 * Sustituye <IP_DE_CHIRPSTACK> por la IP del host donde corre el ChirpStack Gateway Bridge.
 */

/* ---------------------------------------------------------------------------
 * 1) PLACA / PINES DEL RADIO
 * ------------------------------------------------------------------------- */
/* _PIN_OUT selecciona el mapa de pines SX1276<->MCU.
 *   1 = Hallard (ESP8266)   2 = ComResult (ESP8266)   4 = ESP32 / TTGO / Heltec  */
#define _PIN_OUT      4          // TTGO ESP32: el SX1276 ya viene cableado internamente.

/* Pantalla OLED integrada de la TTGO (opcional; SSD1306 por I2C). 0 = desactivada. */
#define _OLED         1

/* ---------------------------------------------------------------------------
 * 2) REGIÓN, CANAL Y FACTOR DE DISPERSIÓN (SF)
 * ------------------------------------------------------------------------- */
/* Plan de frecuencias. Activa SOLO uno. Para Europa: EU863_870.
 * (Si tu región es US915, usa en su lugar:  #define US902_928  1 ) */
#define EU863_870     1

/* Canal ÚNICO que escucha el gateway. En EU, _CHANNEL 0 = 868.100 MHz. */
#define _CHANNEL      0

/* Factor de dispersión por defecto (SF7..SF12). Debe coincidir con el del nodo. */
#define _SPREADING    SF9

/* CAD = Channel Activity Detection.
 *   1 = escucha TODOS los SF en la frecuencia elegida (recomendado).
 *   0 = un único SF fijo (_SPREADING). */
#define _CAD          1

/* Salto de frecuencia. Déjalo en 0: en single-channel queremos canal FIJO. */
#define _HOP          0

/* Downlinks (ACK, Join Accept, confirmaciones) en el MISMO canal/SF del uplink,
 * en vez de las ventanas RX1/RX2 estándar. IMPRESCINDIBLE en gateway de 1 canal. */
#define _STRICT_1CH   1

/* ---------------------------------------------------------------------------
 * 3) NETWORK SERVER — AQUÍ APUNTAMOS A CHIRPSTACK
 * ------------------------------------------------------------------------- */
/* El firmware habla el protocolo Semtech UDP Packet Forwarder (puerto 1700/UDP),
 * que es exactamente el backend "Semtech UDP" del ChirpStack Gateway Bridge.
 *
 * Ruta SIMPLE (un solo Network Server): apunta el servidor PRIMARIO a tu ChirpStack.
 * _TTNSERVER / _TTNPORT SÍ viven en configGway.h. */
#define _TTNSERVER    "<IP_DE_CHIRPSTACK>"   // p.ej. "192.168.1.50" (host del Gateway Bridge)
#define _TTNPORT      1700                   // Semtech UDP

/* Servidor SECUNDARIO (conservar TTN y AÑADIR ChirpStack): NO va aquí.
 * _THINGSERVER / _THINGPORT se definen en configNode.h (ver configNode.example.h). */

/* ---------------------------------------------------------------------------
 * 4) INTERFAZ WEB DE ADMINISTRACIÓN
 * ------------------------------------------------------------------------- */
/* _SERVER 1 activa el servidor web integrado en http://<IP_del_gateway>:80
 * (estadísticas, log en vivo, y ajuste en caliente de SF/canal/debug). */
#define _SERVER       1
#define _SERVERPORT   80

/* Nivel de depuración por serie (0..3). Sube a 2-3 mientras integras. */
#define _DUSB         1
#define debug         1

/*
 * RECORDATORIO:
 *   - La WiFi (AP_NAME / AP_PASSWD o _WIFIMANAGER) y la ubicación (_LAT/_LON/_ALT)
 *     se configuran en configNode.h, no aquí.
 *   - El Gateway EUI (8 bytes) lo GENERA el firmware a partir de la MAC del ESP:
 *     léelo en el monitor serie (115200) o en la web, y regístralo TAL CUAL en ChirpStack.
 */
