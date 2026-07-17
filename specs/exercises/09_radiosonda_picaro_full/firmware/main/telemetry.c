/**
 * ============================================================================
 *  telemetry.c  -  Recoleccion + payload (subset) + CSV (completo)
 * ============================================================================
 *  PAYLOAD LoRaWAN (fPort 10, big-endian). Los campos se incluyen segun los
 *  flags TELEMETRY_* de sensores_config.h. Layout POR DEFECTO (~19 bytes):
 *    [0]     status: bit0 gps_fix, bit1 charging, bit2 usb, bit3 imu, bit4 mag
 *    [1..4]  lat  int32 (grados * 1e6)      \
 *    [5..8]  lon  int32 (grados * 1e6)       | TELEMETRY_GPS
 *    [9..10] alt  int16 (metros)             |
 *    [11]    sats uint8                      /
 *    [12..13]batt uint16 (mV)   [14] batt %  -> TELEMETRY_BATTERY
 *    [15..16]temp int16 (C*100)              -> TELEMETRY_TEMP
 *    [17..18]pres uint16 (hPa*10)            -> TELEMETRY_PRESSURE
 *  Si cambias los flags, ACTUALIZA tambien chirpstack/decoder.js.
 * ============================================================================
 */
#include "telemetry.h"
#include "sensores_config.h"

#include "pmu_axp2101.h"
#if USE_BME280
#include "bme280.h"
#endif
#if USE_L76K_GPS
#include "gps_l76k.h"
#endif
#if USE_QMI8658
#include "qmi8658.h"
#endif
#if USE_QMC6310
#include "qmc6310.h"
#endif

#include <string.h>
#include <stdio.h>
#include <math.h>

void telemetry_collect(telemetry_t *t, uint32_t uptime_s)
{
    memset(t, 0, sizeof(*t));
    t->uptime_s = uptime_s;

    /* Bateria / PMU (siempre) */
    t->batt_mv   = pmu_battery_mv();
    t->batt_pct  = pmu_battery_percent();
    t->usb       = pmu_usb_present();
    t->charging  = pmu_charging();

#if USE_BME280
    bme280_read(&t->temp_c, &t->press_hpa, &t->hum_pct);
#endif
#if USE_L76K_GPS
    {
        gps_fix_t f;
        t->gps_fix = gps_get(&f);
        t->gps_active = gps_active();   /* modulo encendido y enviando NMEA */
        t->lat = f.lat; t->lon = f.lon; t->alt_m = f.alt_m;
        t->sats = f.sats; t->hdop = f.hdop; t->speed_kmh = f.speed_kmh;
    }
#else
    t->gps_active = false;
#endif
#if USE_QMI8658
    {
        imu_sample_t s;
        if (qmi8658_read(&s) == 0) {
            t->imu_ok = true;
            t->ax = s.ax; t->ay = s.ay; t->az = s.az;
            t->gx = s.gx; t->gy = s.gy; t->gz = s.gz;
        }
    }
#endif
#if USE_QMC6310
    if (qmc6310_read(&t->heading_deg, NULL, NULL, NULL) == 0) t->mag_ok = true;
#endif
}

/* Helpers big-endian. */
static size_t put_u8 (uint8_t *b, size_t i, uint8_t v)  { b[i] = v; return i + 1; }
static size_t put_u16(uint8_t *b, size_t i, uint16_t v) { b[i] = v >> 8; b[i+1] = v & 0xFF; return i + 2; }
static size_t put_i16(uint8_t *b, size_t i, int16_t v)  { return put_u16(b, i, (uint16_t)v); }
static size_t put_i32(uint8_t *b, size_t i, int32_t v)  {
    b[i]=(v>>24)&0xFF; b[i+1]=(v>>16)&0xFF; b[i+2]=(v>>8)&0xFF; b[i+3]=v&0xFF; return i + 4;
}

size_t telemetry_build_payload(const telemetry_t *t, uint8_t *buf, size_t bufsz)
{
    if (bufsz < 1) return 0;
    size_t i = 0;

    uint8_t status = 0;
    if (t->gps_fix)    status |= 0x01;
    if (t->charging)   status |= 0x02;
    if (t->usb)        status |= 0x04;
    if (t->imu_ok)     status |= 0x08;
    if (t->mag_ok)     status |= 0x10;
    if (t->gps_active) status |= 0x20;   /* GPS encendido y enviando NMEA */
    i = put_u8(buf, i, status);

#if TELEMETRY_GPS
    i = put_i32(buf, i, (int32_t)lround(t->lat * 1e6));
    i = put_i32(buf, i, (int32_t)lround(t->lon * 1e6));
    i = put_i16(buf, i, (int16_t)lroundf(t->alt_m));
    i = put_u8 (buf, i, t->sats);
#endif
#if TELEMETRY_BATTERY
    i = put_u16(buf, i, t->batt_mv);
    i = put_u8 (buf, i, t->batt_pct);
#endif
#if TELEMETRY_TEMP
    i = put_i16(buf, i, (int16_t)lroundf(t->temp_c * 100.0f));
#endif
#if TELEMETRY_PRESSURE
    i = put_u16(buf, i, (uint16_t)lroundf(t->press_hpa * 10.0f));
#endif
#if TELEMETRY_HUMIDITY
    i = put_u8 (buf, i, (uint8_t)lroundf(t->hum_pct));
#endif
#if TELEMETRY_HEADING
    i = put_u16(buf, i, (uint16_t)lroundf(t->heading_deg * 10.0f));
#endif
#if TELEMETRY_IMU
    i = put_i16(buf, i, (int16_t)lroundf(t->ax * 1000.0f));
    i = put_i16(buf, i, (int16_t)lroundf(t->ay * 1000.0f));
    i = put_i16(buf, i, (int16_t)lroundf(t->az * 1000.0f));
#endif
    (void)bufsz;
    return i;
}

void telemetry_csv_header(char *out, size_t n)
{
    snprintf(out, n,
        "uptime_s,gps_active,gps_fix,lat,lon,alt_m,sats,hdop,speed_kmh,"
        "batt_mv,batt_pct,usb,charging,temp_c,press_hpa,hum_pct,"
        "imu_ok,ax,ay,az,gx,gy,gz,mag_ok,heading_deg");
}

void telemetry_csv_line(const telemetry_t *t, char *out, size_t n)
{
    snprintf(out, n,
        "%lu,%d,%d,%.6f,%.6f,%.1f,%u,%.1f,%.1f,"
        "%u,%u,%d,%d,%.2f,%.1f,%.1f,"
        "%d,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%d,%.1f",
        (unsigned long)t->uptime_s, t->gps_active ? 1 : 0, t->gps_fix ? 1 : 0,
        t->lat, t->lon, t->alt_m,
        t->sats, t->hdop, t->speed_kmh,
        t->batt_mv, t->batt_pct, t->usb ? 1 : 0, t->charging ? 1 : 0,
        t->temp_c, t->press_hpa, t->hum_pct,
        t->imu_ok ? 1 : 0, t->ax, t->ay, t->az, t->gx, t->gy, t->gz,
        t->mag_ok ? 1 : 0, t->heading_deg);
}
