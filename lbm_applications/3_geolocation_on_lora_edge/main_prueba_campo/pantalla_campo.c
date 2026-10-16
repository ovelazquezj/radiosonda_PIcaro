/*!
 * @file      pantalla_campo.c
 *
 * @brief     Pantalla de la prueba de campo: resultado del ultimo LinkCheck y estadisticas por SF
 *
 * Arriba, un recuadro de color con el ultimo resultado (verde/amarillo/rojo segun el margen) para leerlo
 * de un vistazo; debajo, la tabla por SF (enviados, respondidos, % de exito, ultimo margen y gateways) y
 * el ultimo downlink de cada SF. Solo se redibuja un texto si su contenido o su color cambian, para no
 * ocupar el SPI que la TFT comparte con el LR1121 mas de lo necesario.
 */

#include <stdio.h>
#include <string.h>

#include "pantalla_campo.h"
#include "tft_ili9341.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define FONDO TFT_NEGRO
#define MARGEN_X 4
#define COLUMNAS 26  // a escala 2 (12 px por caracter)

// Recuadro del ultimo resultado
#define CAJA_Y 38
#define CAJA_ALTO 62
#define CAJA_COLUMNAS_GRANDE 17  // a escala 3 (18 px por caracter)

#define TFT_AZUL 0x001F

/**
 * @brief Huecos de texto que se cachean para no redibujar lo que no cambio
 */
enum
{
    HUECO_CABECERA,
    HUECO_RED,
    HUECO_CAJA_GRANDE,
    HUECO_CAJA_DETALLE,
    HUECO_TABLA_CABECERA,
    HUECO_TABLA_DR0,
    HUECO_TABLA_DR1,
    HUECO_DL_DR0,
    HUECO_DL_DR1,
    HUECO_PROXIMO,
    HUECO_ENCENDIDA,
    NB_HUECOS
};

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static char     texto_dibujado[NB_HUECOS][COLUMNAS + 1];
static uint16_t color_dibujado[NB_HUECOS];
static uint16_t fondo_caja = 0xFFFF - 1;  // fuerza el primer dibujo del recuadro

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS -------------------------------------------------------
 */

/**
 * @brief Dibuja un texto en su hueco si cambio, rellenando con espacios para borrar el anterior
 */
static void poner( uint8_t hueco, uint16_t x, uint16_t y, uint8_t escala, uint8_t columnas, uint16_t color,
                   uint16_t fondo, const char* texto )
{
    char buf[COLUMNAS + 1];
    snprintf( buf, sizeof( buf ), "%-*s", columnas, texto );
    if( ( strcmp( buf, texto_dibujado[hueco] ) == 0 ) && ( color == color_dibujado[hueco] ) )
    {
        return;
    }
    tft_draw_text( x, y, buf, color, fondo, escala );
    memcpy( texto_dibujado[hueco], buf, sizeof( buf ) );
    color_dibujado[hueco] = color;
}

static void poner_linea( uint8_t hueco, uint16_t y, uint16_t color, const char* texto )
{
    poner( hueco, MARGEN_X, y, 2, COLUMNAS, color, FONDO, texto );
}

static uint8_t dr_a_sf( uint8_t dr )
{
    // US915: DR0..DR3 = SF10..SF7 a 125 kHz, DR4 = SF8 a 500 kHz
    return ( dr <= 3 ) ? 10 - dr : 8;
}

static uint16_t color_margen( uint8_t margen_db )
{
    return ( margen_db >= 10 ) ? TFT_VERDE : ( margen_db >= 3 ) ? TFT_AMARILLO : TFT_NARANJA;
}

/**
 * @brief Recuadro grande con el ultimo resultado. Si cambia el fondo se repinta entero.
 */
static void dibujar_caja( const pantalla_campo_t* e, uint32_t ahora_s )
{
    char           grande[24];
    char           detalle[32];
    uint16_t       fondo;
    uint16_t       tinta;
    const uint8_t  sf = dr_a_sf( e->dr[e->ultimo_idx].dr );
    const uint32_t d  = ( ahora_s > e->t_ultimo_s ) ? ahora_s - e->t_ultimo_s : 0;

    if( e->unida == false )
    {
        fondo = TFT_GRIS;
        tinta = TFT_NEGRO;
        snprintf( grande, sizeof( grande ), "SIN RED" );
        snprintf( detalle, sizeof( detalle ), "uniendo a la red..." );
    }
    else
    {
        switch( e->ultimo )
        {
        case RESULTADO_PENDIENTE:
            fondo = TFT_AZUL;
            tinta = TFT_BLANCO;
            snprintf( grande, sizeof( grande ), "SF%u ENVIANDO", dr_a_sf( e->dr[e->proximo_idx].dr ) );
            snprintf( detalle, sizeof( detalle ), "esperando LinkCheck" );
            break;
        case RESULTADO_OK:
            fondo = color_margen( e->dr[e->ultimo_idx].margen_db );
            tinta = TFT_NEGRO;
            snprintf( grande, sizeof( grande ), "SF%u  %u dB", sf, e->dr[e->ultimo_idx].margen_db );
            snprintf( detalle, sizeof( detalle ), "%u gateway%s  hace %lu s", e->dr[e->ultimo_idx].gw,
                      ( e->dr[e->ultimo_idx].gw == 1 ) ? "" : "s", ( unsigned long ) d );
            break;
        case RESULTADO_SIN_RESPUESTA:
            fondo = TFT_ROJO;
            tinta = TFT_BLANCO;
            snprintf( grande, sizeof( grande ), "SF%u SIN RESP.", sf );
            snprintf( detalle, sizeof( detalle ), "ningun gw  hace %lu s", ( unsigned long ) d );
            break;
        case RESULTADO_NINGUNO:
        default:
            fondo = TFT_GRIS;
            tinta = TFT_NEGRO;
            snprintf( grande, sizeof( grande ), "UNIDA" );
            snprintf( detalle, sizeof( detalle ), "primer envio en breve" );
            break;
        }
    }

    if( fondo != fondo_caja )
    {
        tft_fill_rect( 0, CAJA_Y, TFT_WIDTH, CAJA_ALTO, fondo );
        fondo_caja = fondo;
        texto_dibujado[HUECO_CAJA_GRANDE][0]  = '\0';
        texto_dibujado[HUECO_CAJA_DETALLE][0] = '\0';
    }
    poner( HUECO_CAJA_GRANDE, 8, CAJA_Y + 6, 3, CAJA_COLUMNAS_GRANDE, tinta, fondo, grande );
    poner( HUECO_CAJA_DETALLE, 8, CAJA_Y + 38, 2, COLUMNAS - 1, tinta, fondo, detalle );
}

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS --------------------------------------------------------
 */

uint16_t pantalla_campo_init( void )
{
    uint16_t id = tft_init( );
    memset( texto_dibujado, 0, sizeof( texto_dibujado ) );
    memset( color_dibujado, 0, sizeof( color_dibujado ) );
    poner_linea( HUECO_CABECERA, 0, TFT_CIAN, "PiCARO LR1121   CAMPO" );
    return id;
}

void pantalla_campo_actualizar( const pantalla_campo_t* e, uint32_t ahora_s )
{
    char txt[48];

    // Red
    if( e->unida )
    {
        snprintf( txt, sizeof( txt ), "Red UNIDA  Addr %08lX", ( unsigned long ) e->devaddr );
        poner_linea( HUECO_RED, 18, TFT_VERDE, txt );
    }
    else
    {
        snprintf( txt, sizeof( txt ), "Red: uniendo... (%u)", e->intentos_join );
        poner_linea( HUECO_RED, 18, TFT_AMARILLO, txt );
    }

    dibujar_caja( e, ahora_s );

    // Tabla por SF: enviados, respondidos, % de exito, ultimo margen y gateways
    snprintf( txt, sizeof( txt ), "%-4s%5s%5s%5s%4s%3s", "", "env", "resp", "%", "mrg", "gw" );
    poner_linea( HUECO_TABLA_CABECERA, 106, TFT_GRIS, txt );
    for( uint8_t i = 0; i < CAMPO_NB_DR; i++ )
    {
        const stats_dr_t* s = &e->dr[i];
        char              sf[8];
        char              mrg[8] = "--";
        char              gw[8]  = "-";
        uint16_t          color  = TFT_GRIS;
        uint16_t          pct    = 0;

        snprintf( sf, sizeof( sf ), "SF%u", dr_a_sf( s->dr ) );
        if( s->enviados > 0 )
        {
            pct   = ( uint16_t ) ( ( 100UL * s->respondidos ) / s->enviados );
            color = ( pct >= 90 ) ? TFT_VERDE : ( pct >= 50 ) ? TFT_AMARILLO : TFT_ROJO;
        }
        if( s->respondidos > 0 )
        {
            snprintf( mrg, sizeof( mrg ), "%u", s->margen_db );
            snprintf( gw, sizeof( gw ), "%u", s->gw );
        }
        snprintf( txt, sizeof( txt ), "%-4s%5u%5u%5u%4s%3s", sf, s->enviados, s->respondidos, pct, mrg, gw );
        poner_linea( HUECO_TABLA_DR0 + i, 124 + 18 * i, color, txt );

        // Ultimo downlink (la respuesta del LinkCheck) visto por el kit tras un envio con este SF
        if( s->dl_valido )
        {
            snprintf( txt, sizeof( txt ), "DL %-4s %4d dBm %3d dB", sf, s->dl_rssi, s->dl_snr );
            poner_linea( HUECO_DL_DR0 + i, 160 + 18 * i, TFT_BLANCO, txt );
        }
        else
        {
            snprintf( txt, sizeof( txt ), "DL %-4s sin downlinks", sf );
            poner_linea( HUECO_DL_DR0 + i, 160 + 18 * i, TFT_GRIS, txt );
        }
    }

    // Proximo envio
    if( e->unida == false )
    {
        poner_linea( HUECO_PROXIMO, 200, TFT_GRIS, "Envios tras el join" );
    }
    else if( e->ultimo == RESULTADO_PENDIENTE )
    {
        poner_linea( HUECO_PROXIMO, 200, TFT_BLANCO, "Enviando..." );
    }
    else
    {
        const uint32_t falta = ( e->t_proximo_s > ahora_s ) ? e->t_proximo_s - ahora_s : 0;
        snprintf( txt, sizeof( txt ), "Proximo: SF%u en %lu s", dr_a_sf( e->dr[e->proximo_idx].dr ),
                  ( unsigned long ) falta );
        poner_linea( HUECO_PROXIMO, 200, TFT_BLANCO, txt );
    }

    snprintf( txt, sizeof( txt ), "Encendida %02lu:%02lu:%02lu", ( unsigned long ) ( ahora_s / 3600 ),
              ( unsigned long ) ( ( ahora_s / 60 ) % 60 ), ( unsigned long ) ( ahora_s % 60 ) );
    poner_linea( HUECO_ENCENDIDA, 220, TFT_GRIS, txt );
}
