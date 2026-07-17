/**
 * ============================================================================
 *  wifi_demo.h  -  Demo del WiFi del ESP32-S3 (escaneo de redes)  [apagado]
 * ============================================================================
 *  Capacidad WiFi de la placa, como DEMO opcional. Apagado por defecto para no
 *  perturbar el timing de LoRaWAN. Actívalo con USE_WIFI_DEMO en
 *  sensores_config.h: hace UN escaneo al arrancar (antes del join) y lista las
 *  redes por el monitor serie.
 * ============================================================================
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/* Un escaneo bloqueante que imprime las redes encontradas. */
void wifi_demo_scan(void);
#ifdef __cplusplus
}
#endif
