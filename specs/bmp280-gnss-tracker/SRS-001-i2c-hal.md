# SRS-001 — HAL I²C para STM32L4

## Objetivo

Crear la capa de abstracción I²C que hoy **no existe** en el repo, siguiendo el mismo patrón
que `smtc_hal_spi.[ch]`, para que el driver del sensor (SRS-002) sea independiente de la ST HAL.

## Archivos que debes leer primero

- `lbm_applications/3_geolocation_on_lora_edge/smtc_hal_l4/smtc_hal_spi.h` — patrón de API a imitar.
- `lbm_applications/3_geolocation_on_lora_edge/smtc_hal_l4/smtc_hal_spi.c` — patrón de implementación (clock enable, GPIO alt-func, init/deinit).
- `lbm_applications/3_geolocation_on_lora_edge/smtc_hal_l4/smtc_hal_gpio_pin_names.h` — enumeración de pines.
- `lbm_applications/3_geolocation_on_lora_edge/mcu_drivers/` — dónde vive `stm32l4xx_hal_i2c.*` (debe existir en la ST HAL incluida).

## Entregables

- `smtc_hal_l4/smtc_hal_i2c.h`
- `smtc_hal_l4/smtc_hal_i2c.c`
- Alta de `smtc_hal_i2c.c` en el sistema de build de la app (`CMakeLists.txt` y/o `Makefile` de la app y de `smtc_hal_l4`).

## Requisitos funcionales

- **REQ-001-1** — La API pública debe exponer, como mínimo, estas firmas exactas:
  ```c
  void    smtc_hal_i2c_init( uint32_t id, uint32_t sda, uint32_t scl );
  void    smtc_hal_i2c_deinit( uint32_t id );
  uint8_t smtc_hal_i2c_write_buffer( uint32_t id, uint8_t device_addr, uint16_t mem_addr,
                                     const uint8_t* buffer, uint16_t size );
  uint8_t smtc_hal_i2c_read_buffer(  uint32_t id, uint8_t device_addr, uint16_t mem_addr,
                                     uint8_t* buffer, uint16_t size );
  ```
  - `device_addr` es la dirección de 7 bits del esclavo (p.ej. `0x76`).
  - `mem_addr` es el registro interno del sensor (direccionamiento de 8 bits admitido; el ancho por defecto es 1 byte).
  - Retorno: `0` = OK, distinto de `0` = error (timeout/NACK).
- **REQ-001-2** — `smtc_hal_i2c_init` debe habilitar el reloj del periférico I²C y de los puertos GPIO,
  configurar SDA/SCL en modo alternate-function open-drain con pull-up, y dejar el I²C listo a **100 kHz** (Standard mode) como mínimo; 400 kHz opcional.
- **REQ-001-3** — Los pines SDA/SCL elegidos **no** deben colisionar con los pines del radio (SPI del LR1110)
  ni con UART de depuración. Documentar en un comentario de cabecera qué pines Nucleo se usan (Arduino connector).
- **REQ-001-4** — Todas las operaciones deben tener **timeout** finito (constante `HAL_I2C_TIMEOUT_MS`, valor ≤ 100 ms) y no bloquear indefinidamente (cumple RESTR-4).
- **REQ-001-5** — El archivo `.h` incluye guardas `#ifndef SMTC_HAL_I2C_H`, `extern "C"`, y cabecera de licencia Clear BSD (RESTR-3).
- **REQ-001-6** — Ante error de bus, la función retorna código de error; no debe entrar en bucle infinito ni hacer `assert` que cuelgue el firmware en producción.

## Interfaz de datos

- `id`: identificador lógico del periférico (p.ej. `1` → I²C1). Un `#define` mapea `id` al `I2C_TypeDef*`.
- La implementación puede envolver `HAL_I2C_Mem_Write`/`HAL_I2C_Mem_Read` de la ST HAL internamente
  (eso está permitido **dentro** de `smtc_hal_i2c.c`, RESTR-1 solo prohíbe hacerlo desde app/driver).

## Fuera de alcance

- DMA o I²C por interrupción (polling es suficiente).
- Multi-master / arbitraje.

## Checklist de aceptación

- [ ] **A1** Existen `smtc_hal_i2c.h` y `smtc_hal_i2c.c` con las 4 firmas de REQ-001-1 (verificar con `grep -n "smtc_hal_i2c_" smtc_hal_l4/smtc_hal_i2c.h`).
- [ ] **A2** `smtc_hal_i2c.c` aparece en el build (grep en `CMakeLists.txt`/`Makefile`).
- [ ] **A3** `smtc_hal_i2c_init` habilita reloj + GPIO AF-OD y configura ≥100 kHz (revisar código).
- [ ] **A4** Existe `HAL_I2C_TIMEOUT_MS` (≤100) usado en read/write (REQ-001-4).
- [ ] **A5** Un comentario documenta los pines SDA/SCL y confirma no-colisión con SPI/UART (REQ-001-3).
- [ ] **A6** Cabecera de licencia + guardas de include presentes (REQ-001-5).
- [ ] **A7** La app compila con el nuevo archivo enlazado (build de humo, sin usar todavía la API).
