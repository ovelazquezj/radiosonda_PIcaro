// Codec ChirpStack v4 para la app VUELO_GNSS (LR1110, fPort 20, 11 bytes little-endian).
//   [0..1]  secuencia (uint16)
//   [2]     bits 0-5: satelites detectados; bit 6: hora GPS; bit 7: escaneo valido
//   [3..10] 4 x (id del satelite, C/N0 en dB con signo); hueco vacio = id 0xFF
function decodeUplink(input) {
  var b = input.bytes;
  if (input.fPort !== 20) {
    return { data: {} };
  }
  if (b.length !== 11) {
    return { errors: ["VUELO_GNSS: se esperaban 11 bytes, llegaron " + b.length] };
  }
  var satelites = [];
  for (var k = 0; k < 4; k++) {
    var id = b[3 + 2 * k];
    if (id === 0xff) {
      continue;
    }
    var cnr = b[4 + 2 * k];
    satelites.push({ id: id, cn0_db: cnr > 127 ? cnr - 256 : cnr });
  }
  return {
    data: {
      secuencia: b[0] | (b[1] << 8),
      satelites_detectados: b[2] & 0x3f,
      hora_gps: (b[2] & 0x40) !== 0,
      escaneo_valido: (b[2] & 0x80) !== 0,
      mejores_satelites: satelites
    }
  };
}
