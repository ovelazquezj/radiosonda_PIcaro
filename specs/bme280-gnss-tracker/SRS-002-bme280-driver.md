# SRS-002 — Driver del sensor BME280

## Objetivo

Implementar un driver del **BME280** que use exclusivamente la API `smtc_hal_i2c` (SRS-001) y
exponga lecturas compensadas de **temperatura (°C), presión (hPa) y humedad (%RH)**.

## Archivos que debes leer primero

- `specs/bme280-gnss-tracker/SRS-001-i2c-hal.md` — API I²C disponible.
- Datasheet BME280 (registros clave): `0xD0` id (=0x60), `0xF2` ctrl_hum, `0xF4` ctrl_meas,
  `0xF7..0xFE` datos crudos, `0x88..0xA1` y `0xE1..0xE7` coeficientes de calibración.

## Entregables

- `main_geolocation/bme280.h`
- `main_geolocation/bme280.c`
- (o una subcarpeta `sensors/` dentro de la app; mantener consistencia con el build).

## Requisitos funcionales

- **REQ-002-1** — API pública mínima con estas firmas exactas:
  ```c
  typedef struct {
      float temperature_c;   // grados Celsius
      float pressure_hpa;    // hectopascales
      float humidity_rh;     // %RH
  } bme280_data_t;

  bool bme280_init( uint32_t i2c_id, uint8_t device_addr );        // true si chip-id == 0x60
  bool bme280_read( bme280_data_t* out );                          // lectura forzada + compensación
  ```
- **REQ-002-2** — `bme280_init` debe leer el registro id (`0xD0`) y **verificar que vale `0x60`**;
  si no, retorna `false` (sensor ausente/incorrecto). Debe leer y cachear los coeficientes de calibración.
- **REQ-002-3** — Configurar oversampling x1 en temp/pres/hum y usar **modo forzado** (forced mode):
  disparar una conversión, esperar a que termine (polling del bit de measuring, con timeout), leer datos.
- **REQ-002-4** — Aplicar las fórmulas de **compensación oficiales** de Bosch para obtener valores
  físicos reales (no valores crudos). La temperatura debe calcularse primero (produce `t_fine` usado por presión y humedad).
- **REQ-002-5** — `bme280_read` retorna `false` si el I²C falla o si la conversión no termina antes del timeout.
- **REQ-002-6** — El driver **no** llama a la ST HAL directamente (RESTR-1); solo `smtc_hal_i2c_*`.
- **REQ-002-7** — Cabecera de licencia + guardas de include (RESTR-3).

## Compatibilidad BMP280 (opcional)

- **REQ-002-8 (opcional)** — Si el chip-id leído es `0x58` (BMP280, sin humedad), el driver puede
  operar dejando `humidity_rh = NaN` o `0` y marcándolo. No es obligatorio para el DoD.

## Rangos y validación

- Temperatura esperada: −40..+85 °C. Presión: 300..1100 hPa. Humedad: 0..100 %RH.
- **REQ-002-9** — Valores fuera de rango físico tras compensación se consideran lectura inválida → retornar `false`.

## Fuera de alcance

- Modo normal continuo, filtro IIR, o interrupciones de data-ready.
- Calibración de campo / offsets de usuario.

## Checklist de aceptación

- [ ] **A1** Existen `bme280.h`/`bme280.c` con las firmas de REQ-002-1 (`grep -n "bme280_" bme280.h`).
- [ ] **A2** `bme280_init` valida chip-id `0x60` y lee calibración (revisar código, REQ-002-2).
- [ ] **A3** Se usa modo forzado con espera por polling y timeout (REQ-002-3, REQ-002-5).
- [ ] **A4** Están implementadas las 3 funciones de compensación Bosch con `t_fine` (REQ-002-4).
- [ ] **A5** No hay ninguna llamada `HAL_I2C_*` ni `HAL_` de ST en el driver (`grep -c "HAL_" bme280.c` → solo prefijos permitidos; ideal 0 de ST HAL).
- [ ] **A6** Validación de rango físico presente (REQ-002-9).
- [ ] **A7** Cabecera de licencia + guardas presentes.
- [ ] **A8** El driver compila enlazado con SRS-001 (build de humo llamando `bme280_init` desde un stub o desde la app).
