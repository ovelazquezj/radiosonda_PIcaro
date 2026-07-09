# SRS-002 — Driver del sensor BMP280

> **Sensor del proyecto: Bosch BMP280** (temperatura + presión, **sin humedad**), chip-id `0x58`.
> Se descartó el BME280 (que sí mide humedad, chip-id `0x60`); este SRS describe el BMP280 real
> ya implementado en `main_geolocation/bmp280.[ch]`.

## Objetivo

Implementar un driver del **BMP280** que use exclusivamente la API `smtc_hal_i2c` (SRS-001) y
exponga lecturas compensadas de **temperatura (°C) y presión (hPa)**. El BMP280 **no tiene canal
de humedad**.

## Archivos que debes leer primero

- `specs/bmp280-gnss-tracker/SRS-001-i2c-hal.md` — API I²C disponible.
- Datasheet BMP280 (registros clave): `0xD0` id (=`0x58`), `0xF3` status (bit `measuring`),
  `0xF4` ctrl_meas (oversampling + modo), `0xF5` config, `0xF7..0xFC` datos crudos (presión + temperatura),
  `0x88..0x9F` coeficientes de calibración (`dig_T1..T3`, `dig_P1..P9`).
  > El BMP280 **no** tiene registro `ctrl_hum` (`0xF2`) ni coeficientes de humedad: no aplican.

## Entregables

- `main_geolocation/bmp280.h`
- `main_geolocation/bmp280.c`
- (o una subcarpeta `sensors/` dentro de la app; mantener consistencia con el build).

## Requisitos funcionales

- **REQ-002-1** — API pública mínima con estas firmas exactas:
  ```c
  typedef struct {
      float temperature_c;   // grados Celsius
      float pressure_hpa;    // hectopascales
  } bmp280_data_t;

  bool bmp280_init( uint32_t i2c_id, uint8_t device_addr );        // true si chip-id == 0x58
  bool bmp280_read( bmp280_data_t* out );                          // lectura forzada + compensación
  ```
- **REQ-002-2** — `bmp280_init` debe leer el registro id (`0xD0`) y **verificar que vale `0x58`**;
  si no, retorna `false` (sensor ausente/incorrecto, o es un BME280 `0x60` no soportado). Debe leer
  y cachear los coeficientes de calibración (`dig_T1..T3`, `dig_P1..P9`).
- **REQ-002-3** — Configurar oversampling x1 en temperatura y presión y usar **modo forzado**
  (forced mode): disparar una conversión, esperar a que termine (polling del bit `measuring` del
  registro status `0xF3`, con timeout), leer datos.
- **REQ-002-4** — Aplicar las fórmulas de **compensación oficiales** de Bosch para obtener valores
  físicos reales (no valores crudos). La temperatura debe calcularse primero (produce `t_fine`,
  usado por la compensación de presión). Son **dos** funciones de compensación (temperatura y presión).
- **REQ-002-5** — `bmp280_read` retorna `false` si el I²C falla o si la conversión no termina antes del timeout.
- **REQ-002-6** — El driver **no** llama a la ST HAL directamente (RESTR-1); solo `smtc_hal_i2c_*`.
- **REQ-002-7** — Cabecera de licencia + guardas de include (RESTR-3).

## Direcciones I²C

- **REQ-002-8** — Direcciones 7-bit soportadas: `0x76` (`BMP280_I2C_ADDR_PRIMARY`, SDO a GND) y
  `0x77` (`BMP280_I2C_ADDR_SECONDARY`, SDO a VDDIO). El firmware usa `0x76` por defecto.

## Rangos y validación

- Temperatura esperada: −40..+85 °C. Presión: 300..1100 hPa.
- **REQ-002-9** — Valores fuera de rango físico tras compensación se consideran lectura inválida → retornar `false`.

## Fuera de alcance

- Modo normal continuo, filtro IIR, o interrupciones de data-ready.
- Calibración de campo / offsets de usuario.
- Cualquier medida de **humedad** (el BMP280 no la provee; usar un BME280 sería otro driver).

## Checklist de aceptación

- [ ] **A1** Existen `bmp280.h`/`bmp280.c` con las firmas de REQ-002-1 (`grep -n "bmp280_" bmp280.h`).
- [ ] **A2** `bmp280_init` valida chip-id `0x58` y lee calibración (revisar código, REQ-002-2).
- [ ] **A3** Se usa modo forzado con espera por polling y timeout (REQ-002-3, REQ-002-5).
- [ ] **A4** Están implementadas las 2 funciones de compensación Bosch (temperatura → `t_fine` → presión) (REQ-002-4).
- [ ] **A5** No hay ninguna llamada `HAL_I2C_*` ni `HAL_` de ST en el driver (`grep -c "HAL_" bmp280.c` → solo prefijos permitidos; ideal 0 de ST HAL).
- [ ] **A6** Validación de rango físico presente (REQ-002-9).
- [ ] **A7** Cabecera de licencia + guardas presentes.
- [ ] **A8** El driver compila enlazado con SRS-001 (build de humo llamando `bmp280_init` desde un stub o desde la app).
