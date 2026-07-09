// Codec ChirpStack para el TTGO ESP32 LoRa v1 (sketch TTGO_LoRaWAN_v3.ino)
// Payload de 8 bytes en fPort 1:
//   [0]    magic     = 0x99  (identificador "paquete de prueba")
//   [1..2] counter   = uint16 little-endian (contador de paquetes de la app)
//   [3..6] timestamp = uint32 little-endian (uptime del dispositivo, según el sketch)
//   [7]    checksum  = (suma de bytes 0..6) & 0xFF
// Vector de prueba real capturado: 99 05 00 12 01 00 00 B1 -> counter=5, timestamp=274, checksum OK.

function decodeUplink(input) {
  var b = input.bytes;
  if (input.fPort !== 1 || b.length < 8) {
    return { data: {}, warnings: ["fPort/longitud inesperados"] };
  }
  var counter   = b[1] | (b[2] << 8);
  var timestamp = (b[3] | (b[4] << 8) | (b[5] << 16) | (b[6] << 24)) >>> 0;
  var calc = (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6]) & 0xFF;
  var ok = (b[7] === calc);
  return {
    data: {
      magic: b[0],
      counter: counter,
      timestamp: timestamp,
      checksum_ok: ok
    },
    warnings: ok ? [] : ["checksum no coincide"]
  };
}

// (opcional) sin uso en este device, pero ChirpStack lo admite:
function encodeDownlink(input) {
  return { bytes: [], fPort: 1 };
}
