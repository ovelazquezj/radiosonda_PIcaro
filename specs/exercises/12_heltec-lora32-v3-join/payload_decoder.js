// ============================================================================
//  payload_decoder.js  -  Codec de ChirpStack v4 para el ejercicio 12
// ----------------------------------------------------------------------------
//  Se pega en la consola de ChirpStack: Device profile -> pestana "Codec" ->
//  "JavaScript functions". ChirpStack lo ejecuta con cada uplink y deja el
//  resultado en el campo `object` del evento (lo que consume el dashboard).
//
//  Formato del payload (9 bytes, big-endian), fPort 1:
//    0     magic     uint8   0x48 ('H')
//    1..2  counter   uint16  numero de paquete
//    3..6  uptime_s  uint32  segundos desde el arranque
//    7..8  vbat_mv   uint16  voltaje de bateria en mV (0 = sin bateria / USB)
// ============================================================================

function decodeUplink(input) {
  var b = input.bytes;
  if (b.length < 9) {
    return { data: {}, errors: ["payload demasiado corto: " + b.length + " bytes (esperados 9)"] };
  }
  if (b[0] !== 0x48) {
    return { data: {}, errors: ["magic incorrecto: 0x" + b[0].toString(16) + " (esperado 0x48)"] };
  }

  var counter  = (b[1] << 8) | b[2];
  var uptime_s = ((b[3] << 24) >>> 0) + (b[4] << 16) + (b[5] << 8) + b[6];
  var vbat_mv  = (b[7] << 8) | b[8];

  var data = {
    counter: counter,
    uptime_s: uptime_s,
    vbat_mv: vbat_mv,
    vbat_v: Math.round(vbat_mv / 10) / 100,          // dos decimales
    on_battery: vbat_mv > 0,
    fport: input.fPort
  };

  // Estimacion lineal de carga entre 3.3 V (0 %) y 4.2 V (100 %).
  if (vbat_mv > 0) {
    var pct = Math.round((vbat_mv - 3300) / 9);
    data.battery_pct = Math.max(0, Math.min(100, pct));
  }

  return { data: data };
}

// Este ejercicio no envia downlinks de aplicacion, pero ChirpStack exige la
// funcion. Codifica un objeto {"led": 1} como un byte, por si quieres probar.
function encodeDownlink(input) {
  var led = input.data && input.data.led ? 1 : 0;
  return { bytes: [led] };
}
