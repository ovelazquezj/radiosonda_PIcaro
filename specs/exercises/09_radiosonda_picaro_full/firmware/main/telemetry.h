/**
 * ============================================================================
 *  telemetry.h  -  Recoleccion de sensores, payload LoRaWAN y linea CSV
 * ============================================================================
 *  telemetry_collect()      lee los sensores ACTIVOS (segun sensores_config.h)
 *  telemetry_build_payload()arma el subconjunto que va por LoRaWAN (big-endian)
 *  telemetry_csv_*()        arma la telemetria COMPLETA para la microSD
 * ============================================================================
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t uptime_s;
    /* GPS */
    bool     gps_active;   /* modulo encendido y enviando NMEA (se apaga por boton) */
    bool     gps_fix; double lat, lon; float alt_m; uint8_t sats; float hdop, speed_kmh;
    /* Bateria / PMU */
    uint16_t batt_mv; uint8_t batt_pct; bool usb, charging;
    /* BME280 */
    float    temp_c, press_hpa, hum_pct;
    /* IMU (opcional) */
    bool     imu_ok; float ax, ay, az, gx, gy, gz;
    /* Magnetometro (opcional) */
    bool     mag_ok; float heading_deg;
} telemetry_t;

void   telemetry_collect(telemetry_t *t, uint32_t uptime_s);
size_t telemetry_build_payload(const telemetry_t *t, uint8_t *buf, size_t bufsz);
void   telemetry_csv_header(char *out, size_t n);
void   telemetry_csv_line(const telemetry_t *t, char *out, size_t n);

#ifdef __cplusplus
}
#endif
