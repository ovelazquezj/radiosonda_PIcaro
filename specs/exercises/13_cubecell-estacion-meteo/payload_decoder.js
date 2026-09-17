// ============================================================================
//  payload_decoder.js  -  Codec de ChirpStack v4 para el ejercicio 13
// ----------------------------------------------------------------------------
//  Se pega en la consola de ChirpStack: Device profile -> pestana "Codec" ->
//  Payload codec = "JavaScript functions". ChirpStack lo ejecuta con cada
//  uplink y deja el resultado en el campo `object` del evento.
//
//  Payload (11 bytes, big-endian), fPort 2. Los campos "no disponible" se
//  OMITEN del objeto, asi el dashboard sabe que ese sensor aun no esta.
//    0     status      bit0 DHT11 ok · bit1 BMP280 ok · bit2 BH1750 ok
//                      bit3 lluvia (DO) · bit4 MH-RD ok
//    1     vbat        uint8   voltaje / 20 mV  (0 = sin medida)
//    2     temp_dht    int8    grados C          (0x7F = n/d)
//    3     hum_dht     uint8   %                 (0xFF = n/d)
//    4..5  temp_bmp    int16   grados C x 10     (0x7FFF = n/d)
//    6..7  presion     uint16  hPa x 10          (0xFFFF = n/d)
//    8..9  lux         uint16  lux               (0xFFFF = n/d)
//    10    lluvia_pct  uint8   0..100            (0xFF = n/d)
// ============================================================================

function decodeUplink(input) {
  var b = input.bytes;
  if (b.length < 11) {
    return { data: {}, errors: ["payload demasiado corto: " + b.length + " bytes (esperados 11)"] };
  }

  var status = b[0];
  var data = {
    fport: input.fPort,
    sensores: {
      dht11:  (status & 0x01) !== 0,
      bmp280: (status & 0x02) !== 0,
      bh1750: (status & 0x04) !== 0,
      mhrd:   (status & 0x10) !== 0
    }
  };

  // Bateria
  if (b[1] > 0) {
    data.vbat_mv = b[1] * 20;
    data.vbat_v  = Math.round(b[1] * 2) / 100;
    var pct = Math.round((data.vbat_mv - 3300) / 9);   // 3.3 V = 0 %, 4.2 V = 100 %
    data.battery_pct = Math.max(0, Math.min(100, pct));
  }

  // DHT11
  if (status & 0x01) {
    var t = b[2]; if (t > 127) t -= 256;              // int8
    if (t !== 127 && b[3] !== 255) {
      data.temp_dht_c = t;
      data.humedad_pct = b[3];
    }
  }

  // BMP280
  if (status & 0x02) {
    var t10 = (b[4] << 8) | b[5]; if (t10 > 32767) t10 -= 65536;   // int16
    var p10 = (b[6] << 8) | b[7];
    if (t10 !== 32767) data.temp_bmp_c = t10 / 10;
    if (p10 !== 65535) data.presion_hpa = p10 / 10;
  }

  // BH1750
  if (status & 0x04) {
    var lux = (b[8] << 8) | b[9];
    if (lux !== 65535) data.lux = lux;
  }

  // MH-RD (lluvia)
  if (status & 0x10) {
    if (b[10] !== 255) data.lluvia_pct = b[10];
    data.lluvia = (status & 0x08) !== 0;
  }

  return { data: data };
}

// Sin downlinks de aplicacion en este ejercicio; ChirpStack exige la funcion.
function encodeDownlink(input) {
  return { bytes: [] };
}
