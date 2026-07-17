/**
 * ============================================================================
 *  lorawan.cpp  -  OTAA US915 (RadioLib) con persistencia de nonces/sesion NVS
 * ============================================================================
 *
 *  Flujo canonico de RadioLib:
 *    1) beginOTAA(joinEUI, devEUI, nwkKey, appKey)  -> nwkKey=NULL => 1.0.x
 *    2) (si hay) setBufferNonces()/setBufferSession() desde NVS
 *    3) activateOTAA() -> NEW_SESSION (join nuevo) o SESSION_RESTORED
 *    4) guardar getBufferNonces()/getBufferSession() en NVS
 *    5) sendReceive() para cada uplink; tras cada uno, guardar la sesion.
 * ============================================================================
 */
#include "lorawan.h"
#include "radio.h"
#include "config_lorawan.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

static const char *TAG = "lorawan";

/* Nodo LoRaWAN: se construye PEREZOSAMENTE en lorawan_join() (runtime), no como
 * global. Motivo: el constructor de LoRaWANNode hace phyLayer->getMod()->hal->...,
 * y si el nodo se construye como global ANTES que el objeto `radio` (orden de
 * inicializacion estatica indefinido entre .cpp), `hal` es NULL y crashea. */
static LoRaWANNode *node = nullptr;
static bool s_joined = false;

#define NVS_NS       "picaro_lw"
#define KEY_NONCES   "nonces"
#define KEY_SESSION  "session"

/* ---- helpers NVS (blobs) ---- */
static bool nvs_load_blob(const char *key, uint8_t *buf, size_t len)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    size_t sz = len;
    esp_err_t e = nvs_get_blob(h, key, buf, &sz);
    nvs_close(h);
    return (e == ESP_OK && sz == len);
}
static void nvs_save_blob(const char *key, const uint8_t *buf, size_t len)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, key, buf, len);
    nvs_commit(h);
    nvs_close(h);
}
static void nvs_del_all(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_erase_all(h);
    nvs_commit(h);
    nvs_close(h);
}

lorawan_join_t lorawan_join(void)
{
#if PICARO_RESET_NONCES
    ESP_LOGW(TAG, "PICARO_RESET_NONCES=1 -> borrando nonces/sesion guardados");
    nvs_del_all();
#endif
#if PICARO_FORCE_FRESH_JOIN
    ESP_LOGW(TAG, "PICARO_FORCE_FRESH_JOIN=1 -> ignoro la sesion guardada, join OTAA real");
    nvs_save_blob(KEY_SESSION, (const uint8_t[RADIOLIB_LORAWAN_SESSION_BUF_SIZE]){0},
                  RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
#endif

    /* Construir el nodo ahora (radio ya existe y tiene su HAL). */
    if (node == nullptr) {
        node = new LoRaWANNode(&radio, &US915, PICARO_SUBBAND);
    }

    uint64_t joinEUI = PICARO_JOIN_EUI;
    uint64_t devEUI  = PICARO_DEV_EUI;
    uint8_t  appKey[16] = PICARO_APP_KEY;

    /* nwkKey = NULL => LoRaWAN 1.0.x (solo AppKey). */
    int16_t st = node->beginOTAA(joinEUI, devEUI, NULL, appKey);
    if (st != RADIOLIB_ERR_NONE) {
        ESP_LOGE(TAG, "beginOTAA fallo (%d)", st);
        return LORAWAN_JOIN_FAIL;
    }

    /* Restaurar nonces (y sesion) si existen, para continuar DevNonce/sesion. */
    bool have_saved_session = false;
    uint8_t nonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    if (nvs_load_blob(KEY_NONCES, nonces, sizeof(nonces))) {
        node->setBufferNonces(nonces);
#if !PICARO_FORCE_FRESH_JOIN
        uint8_t session[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
        if (nvs_load_blob(KEY_SESSION, session, sizeof(session))) {
            if (node->setBufferSession(session) == RADIOLIB_ERR_NONE) have_saved_session = true;
        }
#endif
        ESP_LOGI(TAG, "Nonces restaurados de NVS%s.", have_saved_session ? " (con sesion)" : "");
    } else {
        ESP_LOGI(TAG, "Sin nonces guardados: primer join.");
    }

    /* Intento de union OTAA con reintentos. Con sesion restaurada,
     * activateOTAA() devuelve SESSION_RESTORED sin tocar el aire. */
    for (int attempt = 1; attempt <= 8; attempt++) {
        ESP_LOGI(TAG, "activateOTAA intento #%d ...", attempt);
        st = node->activateOTAA();
        nvs_save_blob(KEY_NONCES, node->getBufferNonces(), RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

        if (st == RADIOLIB_LORAWAN_NEW_SESSION) {
            s_joined = true;
            nvs_save_blob(KEY_SESSION, node->getBufferSession(), RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
            ESP_LOGI(TAG, "JOIN NUEVO por aire (JoinAccept recibido). Enlace REAL.");
            return LORAWAN_JOIN_NEW;
        }
        if (st == RADIOLIB_LORAWAN_SESSION_RESTORED) {
            s_joined = true;
            ESP_LOGW(TAG, "Sesion RESTAURADA de NVS. OJO: NO confirma que haya gateway "
                          "en rango; el enlace se validara con un uplink confirmado.");
            return LORAWAN_JOIN_RESTORED;
        }
        /* Si teniamos sesion guardada y aun asi no restauro, no reintentes en
         * bucle: probablemente no hay red. Un join nuevo requiere gateway. */
        ESP_LOGW(TAG, "  activateOTAA sin exito (codigo %d). Reintento en 15 s...", st);
        vTaskDelay(pdMS_TO_TICKS(15000));
    }

    ESP_LOGE(TAG, "No se logro unir (sin gateway en rango?).");
    return LORAWAN_JOIN_FAIL;
}

int lorawan_send(const uint8_t *data, uint8_t len, uint8_t fport, bool confirmed)
{
    if (!s_joined) return -1;

    int16_t st = node->sendReceive(data, len, fport, confirmed);
    if (st >= RADIOLIB_ERR_NONE) {
        nvs_save_blob(KEY_SESSION, node->getBufferSession(), RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
        if (st > 0) ESP_LOGI(TAG, "Uplink%s OK + downlink/ACK en Rx%d (ENLACE CONFIRMADO)",
                             confirmed ? " confirmado" : "", st);
        else        ESP_LOGI(TAG, "Uplink enviado%s (sin downlink)%s",
                             confirmed ? " confirmado" : "",
                             confirmed ? " -> SIN ACK (no hay enlace)" : "");
        return st;
    }
    ESP_LOGE(TAG, "Uplink fallo (codigo %d)", st);
    return st;
}

bool lorawan_is_joined(void) { return s_joined; }
