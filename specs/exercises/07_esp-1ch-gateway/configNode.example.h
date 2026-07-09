/*
 * configNode.example.h — Ejemplo de WiFi/identidad/ubicación para ESP-1ch-Gateway
 * ================================================================================
 *
 * Fragmento de referencia para el fichero `src/configNode.h` del firmware DE TERCEROS
 * **ESP-1ch-Gateway** (Maarten Westenberg / things4u, licencia MIT):
 *     https://github.com/things4u/ESP-1ch-Gateway
 *
 * ESTE NO ES CÓDIGO DE radiosonda_PIcaro NI DE SEMTECH. Es solo un ejemplo comentado, aportado
 * por radiosonda_PIcaro, de cómo rellenar los campos que un alumno DEBE editar en configNode.h:
 * la WiFi, la identidad y la ubicación del gateway (y, opcionalmente, el servidor secundario).
 *
 * Copia/adapta estos valores sobre tu configNode.h real. Sustituye <...> por tus datos.
 * La configuración de radio/región/canal va en configGway.h (ver configGway.example.h).
 */

/* ---------------------------------------------------------------------------
 * 1) WiFi (OBLIGATORIO) — rellena el array wpa[] con tu red.
 *    En el repo viene VACÍO (`wpas wpa[] = {};`) y el gateway NO conecta hasta rellenarlo.
 *    Sustituye el array vacío de tu configNode.h por uno con tu(s) red(es):
 * ------------------------------------------------------------------------- */
wpas wpa[] = {
    { "<TU_SSID>", "<TU_CLAVE_WIFI>" }      // puedes añadir más: , { "SSID2", "clave2" }
};

/* Alternativa: portal de configuración por Access Point. `_WIFIMANAGER` es un flag de
 * COMPILACIÓN (vive en configGway.h; por defecto 0, y platformio.ini lo fija con -D _WIFIMANAGER=0).
 * Ponlo a 1 para configurar la WiFi por portal en vez del array wpa[]. */

/* ---------------------------------------------------------------------------
 * 2) IDENTIDAD del gateway (se muestra en la web del gateway y ayuda a identificarlo)
 * ------------------------------------------------------------------------- */
#define _DESCRIPTION  "<Nombre de tu gateway>"   // p.ej. "Gateway TTGO aula"  (default repo: "ESP Gateway")
#define _EMAIL        "<tu_correo>"              // propietario/contacto        (default repo: mw12554@hotmail.com)
#define _PLATFORM     "ESP32"                    // TTGO/Heltec = ESP32         (default repo: "ESP8266")

/* ---------------------------------------------------------------------------
 * 3) UBICACIÓN del gateway (ChirpStack la usa en el mapa)
 * ------------------------------------------------------------------------- */
#define _LAT          <TU_LATITUD>               // p.ej. 19.4326    (default repo: 52.237367)
#define _LON          <TU_LONGITUD>              // p.ej. -99.1332   (default repo: 5.978654)
#define _ALT          <TU_ALTITUD_M>             // metros, entero   (default repo: 14)

/* ---------------------------------------------------------------------------
 * 4) (OPCIONAL) Servidor SECUNDARIO -> ChirpStack
 *    En el repo estas dos líneas vienen COMENTADAS en configNode.h. Úsalas SOLO si quieres
 *    conservar el servidor primario (`_TTNSERVER`, en configGway.h) y ADEMÁS enviar a ChirpStack.
 *    Para un ÚNICO Network Server es más simple poner `_TTNSERVER` = ChirpStack (ver configGway.example.h).
 * ------------------------------------------------------------------------- */
#define _THINGSERVER  "<IP_DE_CHIRPSTACK>"       // host del ChirpStack Gateway Bridge
#define _THINGPORT    1700                       // Semtech UDP

/*
 * RECORDATORIO:
 *   - El Gateway EUI (8 bytes) NO se define aquí: lo genera el firmware desde la MAC del ESP.
 *     Léelo en el monitor serie (115200) o en la web del gateway y regístralo TAL CUAL en ChirpStack.
 *   - La región, canal, SF, CAD y pines (`_PIN_OUT 4`) van en configGway.h (ver configGway.example.h).
 */
