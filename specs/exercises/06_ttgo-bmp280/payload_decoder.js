// radiosonda_PIcaro — codec educativo original (NO forma parte de Semtech SWL2001).
// Provisto bajo Clear BSD License. (c) 2026 radiosonda_PIcaro contributors.
// Codec ChirpStack para el ejercicio 06 (TTGO + BMP280).
// Payload de 5 bytes en fPort 2 (big-endian):
//   [0]    version = 0x01
//   [1..2] temperature = int16  (°C × 100)
//   [3..4] pressure    = uint16 (hPa)
// Ejemplo: 01 09 29 03 F5  ->  { version:1, temperature:23.45, pressure:1013 }

function decodeUplink(input) {
  var b = input.bytes;
  if (input.fPort !== 2 || b.length < 5) {
    return { data: {}, warnings: ["fPort/longitud inesperados"] };
  }
  var t = (b[1] << 8) | b[2];
  if (t & 0x8000) { t -= 0x10000; }         // int16 con signo
  var p = (b[3] << 8) | b[4];               // uint16
  return {
    data: {
      version: b[0],
      temperature: t / 100.0,   // °C
      pressure: p               // hPa
    }
  };
}

function encodeDownlink(input) {
  return { bytes: [], fPort: 2 };
}
