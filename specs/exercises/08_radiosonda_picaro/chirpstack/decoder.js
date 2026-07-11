// ============================================================================
//  decoder.js  -  Codec de payload para ChirpStack (Radiosonda PICARO)
// ============================================================================
//
//  COMO SE USA (ChirpStack v4):
//    1. Ve a: Device profiles -> (tu perfil) -> pestania "Codec".
//    2. En "Payload codec" elige: "JavaScript functions".
//    3. Pega TODO este archivo en el editor y guarda.
//    4. A partir de ahi, cada mensaje del dispositivo aparecera ya "traducido"
//       en Applications -> (tu app) -> Device -> pestania "Events" -> "up".
//
//  Este codec entiende el payload de 15 bytes que arma radiosonda_picaro.ino.
//  Debe COINCIDIR con la tabla de bytes del sketch. Si cambias el payload en
//  el firmware, cambia tambien este archivo.
// ============================================================================

// --- decodeUplink: convierte los bytes recibidos en un objeto legible -------
function decodeUplink(input) {
    var b = input.bytes;
    var warnings = [];
    var errors = [];

    // Verificacion basica de tamanio.
    if (b.length < 15) {
        errors.push("Payload demasiado corto: se esperaban 15 bytes, llegaron " + b.length);
        return { data: {}, warnings: warnings, errors: errors };
    }

    // Helpers para leer enteros con signo en formato big-endian.
    function int32BE(i) {
        var v = (b[i] << 24) | (b[i + 1] << 16) | (b[i + 2] << 8) | b[i + 3];
        return v; // el operador << de JS ya devuelve valor con signo de 32 bits
    }
    function int16BE(i) {
        var v = (b[i] << 8) | b[i + 1];
        if (v & 0x8000) { v -= 0x10000; } // aplicar el signo
        return v;
    }
    function uint16BE(i) {
        return (b[i] << 8) | b[i + 1];
    }

    // Byte 0: estado (flags)
    var status = b[0];
    var gpsValid = (status & 0x01) !== 0;
    var charging = (status & 0x02) !== 0;
    var usb      = (status & 0x04) !== 0;

    // GPS
    var latitude  = int32BE(1) / 1000000.0;
    var longitude = int32BE(5) / 1000000.0;
    var altitude  = int16BE(9);           // metros
    var satellites = b[11];

    // Bateria
    var batteryMv  = uint16BE(12);
    var batteryPct = b[14];               // 255 = desconocido

    var data = {
        gps_fix: gpsValid,
        satellites: satellites,
        altitude_m: altitude,
        battery_v: batteryMv / 1000.0,
        charging: charging,
        usb_powered: usb
    };

    // Solo publicamos coordenadas cuando el GPS tiene fix (evita mandar 0,0).
    if (gpsValid) {
        data.latitude = latitude;
        data.longitude = longitude;
    } else {
        warnings.push("Sin fix de GPS: no se incluyen latitud/longitud");
    }

    if (batteryPct === 255) {
        data.battery_pct = null;
    } else {
        data.battery_pct = batteryPct;
    }

    return { data: data, warnings: warnings, errors: errors };
}

// --- encodeDownlink: no se usa en esta practica, pero ChirpStack lo pide -----
function encodeDownlink(input) {
    return { bytes: [], fPort: input.fPort || 1, warnings: [], errors: [] };
}
