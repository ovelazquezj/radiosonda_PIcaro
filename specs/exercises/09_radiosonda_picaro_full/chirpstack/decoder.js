// ============================================================================
//  decoder.js  -  Codec de payload para ChirpStack (Radiosonda PICARO FULL)
// ============================================================================
//
//  COMO SE USA (ChirpStack v4):
//    Device profiles -> (tu perfil) -> pestania "Codec" -> "JavaScript functions"
//    Pega TODO este archivo y guarda. Los uplinks apareceran decodificados en
//    Applications -> (tu app) -> Device -> Events -> "up" (campo "object").
//
//  Este codec entiende el payload POR DEFECTO (19 bytes) que arma el firmware
//  del ejercicio 09 (ver telemetry.c). Si activas/desactivas campos en
//  sensores_config.h (TELEMETRY_*), el tamanio cambia: ACTUALIZA este archivo
//  para que coincida byte a byte.
//
//    [0]      status: bit0 gps_fix, bit1 charging, bit2 usb, bit3 imu, bit4 mag, bit5 gps_active
//    [1..4]   lat   int32  (grados * 1e6)
//    [5..8]   lon   int32  (grados * 1e6)
//    [9..10]  alt   int16  (metros)
//    [11]     sats  uint8
//    [12..13] batt  uint16 (mV)
//    [14]     batt  uint8  (%)
//    [15..16] temp  int16  (C * 100)
//    [17..18] pres  uint16 (hPa * 10)
// ============================================================================

function decodeUplink(input) {
    var b = input.bytes;
    var warnings = [];
    var errors = [];

    if (b.length < 19) {
        errors.push("Payload demasiado corto: se esperaban 19 bytes, llegaron " + b.length);
        return { data: {}, warnings: warnings, errors: errors };
    }

    function u16(i) { return (b[i] << 8) | b[i + 1]; }
    function i16(i) { var v = u16(i); return (v & 0x8000) ? v - 0x10000 : v; }
    function i32(i) { return (b[i] << 24) | (b[i + 1] << 16) | (b[i + 2] << 8) | b[i + 3]; }

    var status = b[0];
    var gpsFix   = (status & 0x01) !== 0;
    var charging = (status & 0x02) !== 0;
    var usb      = (status & 0x04) !== 0;
    var gpsActive = (status & 0x20) !== 0;

    var data = {
        gps_active: gpsActive,
        gps_fix: gpsFix,
        satellites: b[11],
        altitude_m: i16(9),
        battery_v: u16(12) / 1000.0,
        charging: charging,
        usb_powered: usb,
        temperature_c: i16(15) / 100.0,
        pressure_hpa: u16(17) / 10.0
    };

    var pct = b[14];
    data.battery_pct = (pct === 255) ? null : pct;

    if (gpsFix) {
        data.latitude = i32(1) / 1000000.0;
        data.longitude = i32(5) / 1000000.0;
    } else {
        warnings.push("Sin fix de GPS: no se incluyen latitud/longitud");
    }

    return { data: data, warnings: warnings, errors: errors };
}

// encodeDownlink: no se usa en esta practica, pero ChirpStack lo pide.
function encodeDownlink(input) {
    return { bytes: [], fPort: input.fPort || 1, warnings: [], errors: [] };
}
