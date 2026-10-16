/*!
 * @file      tft_ili9341.h
 *
 * @brief     Driver minimo de la TFT ILI9341 (shield de 2,8" del kit LR1110) sobre el SPI de la radio
 *
 * La TFT comparte SCK/MOSI/MISO con el LR1110 y tiene su propio chip select. Cada rafaga SPI hacia la
 * TFT se hace con las interrupciones enmascaradas para que el radio planner de LBM no meta una
 * transaccion del LR1110 a mitad de un dibujo.
 *
 * Orientacion apaisada: 320 x 240 px. Texto con fuente 5x7 escalada.
 */

#ifndef TFT_ILI9341_H
#define TFT_ILI9341_H

#include <stdint.h>
#include <stdbool.h>

#define TFT_WIDTH 320
#define TFT_HEIGHT 240

/**
 * @brief Colores RGB565
 */
#define TFT_NEGRO 0x0000
#define TFT_BLANCO 0xFFFF
#define TFT_GRIS 0x8410
#define TFT_ROJO 0xF800
#define TFT_VERDE 0x07E0
#define TFT_AMARILLO 0xFFE0
#define TFT_CIAN 0x07FF
#define TFT_NARANJA 0xFD20
#define TFT_AZUL_OSCURO 0x0010

/**
 * @brief Configura los pines e inicializa el controlador. El SPI ya lo inicializo hal_mcu_init().
 *
 * @return Identificador leido del controlador (0x9341 si la lectura funciona; la escritura no depende de ello)
 */
uint16_t tft_init( void );

/**
 * @brief Rellena un rectangulo de un color
 */
void tft_fill_rect( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color );

/**
 * @brief Escribe texto ASCII con la fuente 5x7 escalada (celda de 6x8 px por escala)
 *
 * @param [in] scale  Factor de escala (1 = 6x8 px por caracter, 2 = 12x16 px)
 */
void tft_draw_text( uint16_t x, uint16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale );

#endif  // TFT_ILI9341_H
