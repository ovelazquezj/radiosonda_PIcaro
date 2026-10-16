/*!
 * @file      tft_ili9341.c
 *
 * @brief     Driver minimo de la TFT ILI9341 (shield de 2,8" del kit LR1110) sobre el SPI de la radio
 */

#include "tft_ili9341.h"
#include "font5x7.h"

#include "stm32l4xx.h"  // __get_PRIMASK / __set_PRIMASK

#include "smtc_hal_gpio.h"
#include "smtc_hal_mcu.h"
#include "smtc_hal_spi.h"
#include "modem_pinout.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

// Pines del shield (cabecera Arduino de la Nucleo)
#define TFT_CS PB_6        // D10
#define TFT_DC PC_7        // D9
#define TFT_TOUCH_CS PA_9  // D8: tactil resistivo (STMPE610), se mantiene sin seleccionar
// D4 (PB_5) es el CS de la microSD del shield y a la vez el LED SCAN de la EVK: sin tarjeta no molesta

#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID 0xD3
#define ILI9341_SLPOUT 0x11
#define ILI9341_DISPON 0x29
#define ILI9341_CASET 0x2A
#define ILI9341_PASET 0x2B
#define ILI9341_RAMWR 0x2C
#define ILI9341_MADCTL 0x36
#define ILI9341_PIXFMT 0x3A

#define MADCTL_MV 0x20
#define MADCTL_BGR 0x08

/**
 * @brief Pixeles por rafaga con las interrupciones enmascaradas (acota el retraso que ve LBM)
 */
#define TFT_PIXELS_PER_BURST 128

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS -------------------------------------------------------
 */

static uint32_t irq_save( void )
{
    uint32_t primask = __get_PRIMASK( );
    __disable_irq( );
    return primask;
}

static void irq_restore( uint32_t primask )
{
    __set_PRIMASK( primask );
}

static void spi_write( uint8_t data )
{
    hal_spi_in_out( RADIO_SPI_ID, data );
}

static void write_command( uint8_t cmd, const uint8_t* data, uint8_t len )
{
    uint32_t primask = irq_save( );
    hal_gpio_set_value( TFT_CS, 0 );
    hal_gpio_set_value( TFT_DC, 0 );
    spi_write( cmd );
    hal_gpio_set_value( TFT_DC, 1 );
    for( uint8_t i = 0; i < len; i++ )
    {
        spi_write( data[i] );
    }
    hal_gpio_set_value( TFT_CS, 1 );
    irq_restore( primask );
}

static void set_window( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1 )
{
    const uint8_t col[4] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    const uint8_t row[4] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    write_command( ILI9341_CASET, col, 4 );
    write_command( ILI9341_PASET, row, 4 );
    write_command( ILI9341_RAMWR, NULL, 0 );
}

/**
 * @brief Envia n pixeles de la ventana activa, en rafagas cortas con las interrupciones enmascaradas
 *
 * @param [in] pixel  Devuelve el color del pixel i (permite pintar texto sin buffer)
 */
static void write_pixels( uint32_t n, uint16_t ( *pixel )( uint32_t i, const void* ctx ), const void* ctx )
{
    uint32_t i = 0;
    while( i < n )
    {
        uint32_t end     = ( n - i > TFT_PIXELS_PER_BURST ) ? i + TFT_PIXELS_PER_BURST : n;
        uint32_t primask = irq_save( );
        hal_gpio_set_value( TFT_DC, 1 );
        hal_gpio_set_value( TFT_CS, 0 );
        for( ; i < end; i++ )
        {
            uint16_t c = pixel( i, ctx );
            spi_write( c >> 8 );
            spi_write( c & 0xFF );
        }
        hal_gpio_set_value( TFT_CS, 1 );
        irq_restore( primask );
    }
}

static uint16_t solid_pixel( uint32_t i, const void* ctx )
{
    ( void ) i;
    return *( const uint16_t* ) ctx;
}

typedef struct
{
    uint8_t  columns[6];  // 5 columnas de la fuente + 1 de separacion
    uint8_t  scale;
    uint16_t fg;
    uint16_t bg;
} glyph_ctx_t;

static uint16_t glyph_pixel( uint32_t i, const void* ctx )
{
    const glyph_ctx_t* g     = ( const glyph_ctx_t* ) ctx;
    const uint32_t     width = 6 * g->scale;
    const uint8_t      col   = ( i % width ) / g->scale;
    const uint8_t      row   = ( i / width ) / g->scale;
    return ( ( g->columns[col] >> row ) & 0x01 ) ? g->fg : g->bg;
}

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS --------------------------------------------------------
 */

uint16_t tft_init( void )
{
    hal_gpio_init_out( TFT_CS, 1 );
    hal_gpio_init_out( TFT_DC, 1 );
    hal_gpio_init_out( TFT_TOUCH_CS, 1 );

    write_command( ILI9341_SWRESET, NULL, 0 );
    hal_mcu_wait_us( 150000 );

    // Secuencia de arranque habitual del ILI9341 (la de los shields de 2,8")
    static const uint8_t init_seq[] = {
        // cmd, n, datos...
        0xEF, 3,  0x03, 0x80, 0x02,                    //
        0xCF, 3,  0x00, 0xC1, 0x30,                    // power control B
        0xED, 4,  0x64, 0x03, 0x12, 0x81,              // power on sequence
        0xE8, 3,  0x85, 0x00, 0x78,                    // driver timing A
        0xCB, 5,  0x39, 0x2C, 0x00, 0x34, 0x02,        // power control A
        0xF7, 1,  0x20,                                // pump ratio
        0xEA, 2,  0x00, 0x00,                          // driver timing B
        0xC0, 1,  0x23,                                // power control 1
        0xC1, 1,  0x10,                                // power control 2
        0xC5, 2,  0x3E, 0x28,                          // VCOM 1
        0xC7, 1,  0x86,                                // VCOM 2
        ILI9341_MADCTL, 1, MADCTL_MV | MADCTL_BGR,     // apaisada 320x240
        ILI9341_PIXFMT, 1, 0x55,                       // 16 bits por pixel
        0xB1, 2,  0x00, 0x18,                          // frame rate
        0xB6, 3,  0x08, 0x82, 0x27,                    // display function
        0xF2, 1,  0x00,                                // 3G off
        0x26, 1,  0x01,                                // gamma curve
        0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
        0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    };
    for( uint32_t i = 0; i < sizeof( init_seq ); )
    {
        const uint8_t cmd = init_seq[i];
        const uint8_t len = init_seq[i + 1];
        write_command( cmd, &init_seq[i + 2], len );
        i += 2 + len;
    }

    write_command( ILI9341_SLPOUT, NULL, 0 );
    hal_mcu_wait_us( 150000 );
    write_command( ILI9341_DISPON, NULL, 0 );
    hal_mcu_wait_us( 50000 );

    // Lectura del identificador (diagnostico: confirma que la TFT responde en este bus)
    uint32_t primask = irq_save( );
    hal_gpio_set_value( TFT_CS, 0 );
    hal_gpio_set_value( TFT_DC, 0 );
    spi_write( ILI9341_RDDID );
    hal_gpio_set_value( TFT_DC, 1 );
    ( void ) hal_spi_in_out( RADIO_SPI_ID, 0x00 );  // byte de relleno
    ( void ) hal_spi_in_out( RADIO_SPI_ID, 0x00 );
    uint16_t id = ( hal_spi_in_out( RADIO_SPI_ID, 0x00 ) & 0xFF ) << 8;
    id |= hal_spi_in_out( RADIO_SPI_ID, 0x00 ) & 0xFF;
    hal_gpio_set_value( TFT_CS, 1 );
    irq_restore( primask );

    tft_fill_rect( 0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_NEGRO );
    return id;
}

void tft_fill_rect( uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color )
{
    if( ( w == 0 ) || ( h == 0 ) || ( x >= TFT_WIDTH ) || ( y >= TFT_HEIGHT ) )
    {
        return;
    }
    if( x + w > TFT_WIDTH )
    {
        w = TFT_WIDTH - x;
    }
    if( y + h > TFT_HEIGHT )
    {
        h = TFT_HEIGHT - y;
    }
    set_window( x, y, x + w - 1, y + h - 1 );
    write_pixels( ( uint32_t ) w * h, solid_pixel, &color );
}

void tft_draw_text( uint16_t x, uint16_t y, const char* text, uint16_t fg, uint16_t bg, uint8_t scale )
{
    glyph_ctx_t g = { .scale = scale, .fg = fg, .bg = bg };
    const uint16_t cw = 6 * scale;
    const uint16_t ch = 8 * scale;

    for( ; *text != '\0'; text++ )
    {
        if( x + cw > TFT_WIDTH )
        {
            break;
        }
        char c = *text;
        if( ( c < FONT5X7_FIRST ) || ( c > FONT5X7_LAST ) )
        {
            c = '?';
        }
        for( uint8_t k = 0; k < 5; k++ )
        {
            g.columns[k] = font5x7[c - FONT5X7_FIRST][k];
        }
        g.columns[5] = 0x00;

        set_window( x, y, x + cw - 1, y + ch - 1 );
        write_pixels( ( uint32_t ) cw * ch, glyph_pixel, &g );
        x += cw;
    }
}
