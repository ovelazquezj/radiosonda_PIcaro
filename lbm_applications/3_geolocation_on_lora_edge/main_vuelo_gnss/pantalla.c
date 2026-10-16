/*!
 * @file      pantalla.c
 *
 * @brief     Pantalla de estado del enlace en la TFT: join, uplinks, LinkCheck por SF, downlinks, GNSS y energia
 *
 * 13 lineas de 26 caracteres (fuente 5x7 a escala 2). Solo se redibuja una linea si su texto o su color
 * cambian, para no ocupar el SPI que comparte con el LR1110 mas de lo necesario.
 */

#include <stdio.h>
#include <string.h>

#include "pantalla.h"
#include "tft_ili9341.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define ESCALA 2
#define COLUMNAS 26
#define ALTO_LINEA 18
#define NB_LINEAS 13
#define MARGEN_X 4

#define FONDO TFT_NEGRO

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static char     linea_dibujada[NB_LINEAS][COLUMNAS + 1];
static uint16_t color_dibujado[NB_LINEAS];

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS -------------------------------------------------------
 */

/**
 * @brief Dibuja la linea n si cambio, rellenando con espacios para borrar el texto anterior
 */
static void poner_linea( uint8_t n, uint16_t color, const char* texto )
{
    char buf[COLUMNAS + 1];
    snprintf( buf, sizeof( buf ), "%-*s", COLUMNAS, texto );
    if( ( strcmp( buf, linea_dibujada[n] ) == 0 ) && ( color == color_dibujado[n] ) )
    {
        return;
    }
    tft_draw_text( MARGEN_X, n * ALTO_LINEA, buf, color, FONDO, ESCALA );
    memcpy( linea_dibujada[n], buf, sizeof( buf ) );
    color_dibujado[n] = color;
}

/**
 * @brief "hace N s" / "hace N min" desde un instante (0 = nunca)
 */
static void hace( char* out, size_t len, uint32_t t_s, uint32_t ahora_s )
{
    if( t_s == 0 )
    {
        snprintf( out, len, "nunca" );
        return;
    }
    uint32_t d = ( ahora_s > t_s ) ? ahora_s - t_s : 0;
    if( d < 120 )
    {
        snprintf( out, len, "hace %lu s", ( unsigned long ) d );
    }
    else
    {
        snprintf( out, len, "hace %lu min", ( unsigned long ) ( d / 60 ) );
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS --------------------------------------------------------
 */

uint16_t pantalla_init( void )
{
    uint16_t id = tft_init( );
    memset( linea_dibujada, 0, sizeof( linea_dibujada ) );
    memset( color_dibujado, 0, sizeof( color_dibujado ) );
    poner_linea( 0, TFT_CIAN, "PiCARO LR1110  VUELO_GNSS" );
    return id;
}

/**
 * @brief Segundos transcurridos en formato corto para la tabla ("14s", "3m", "--")
 */
static void hace_corto( char* out, size_t len, uint32_t t_s, uint32_t ahora_s )
{
    if( t_s == 0 )
    {
        snprintf( out, len, "--" );
        return;
    }
    uint32_t d = ( ahora_s > t_s ) ? ahora_s - t_s : 0;
    if( d < 100 )
    {
        snprintf( out, len, "%lus", ( unsigned long ) d );
    }
    else
    {
        snprintf( out, len, "%lum", ( unsigned long ) ( d / 60 ) );
    }
}

/**
 * @brief Fila de la tabla de un SF: margen y gateways del ultimo LinkCheck, respuestas/enviados y antiguedad
 */
static void poner_fila_sf( uint8_t n, const char* nombre, const pantalla_sf_t* sf, bool en_curso, uint32_t ahora_s )
{
    char txt[48];
    char t[8];
    uint16_t color;

    hace_corto( t, sizeof( t ), sf->t_s, ahora_s );
    if( sf->t_s == 0 )
    {
        snprintf( txt, sizeof( txt ), "%c%-4s --      0/%-3u --", en_curso ? '>' : ' ', nombre, sf->enviados );
        color = TFT_GRIS;
    }
    else if( sf->ultimo_ok == false )
    {
        snprintf( txt, sizeof( txt ), "%c%-4s SIN RESP %u/%-3u %s", en_curso ? '>' : ' ', nombre, sf->respuestas,
                  sf->enviados, t );
        color = TFT_ROJO;
    }
    else
    {
        snprintf( txt, sizeof( txt ), "%c%-4s %2udB %ugw %u/%-3u %s", en_curso ? '>' : ' ', nombre, sf->margen_db,
                  sf->gw, sf->respuestas, sf->enviados, t );
        color = ( sf->margen_db >= 10 ) ? TFT_VERDE : ( sf->margen_db >= 3 ) ? TFT_AMARILLO : TFT_ROJO;
    }
    poner_linea( n, color, txt );
}

void pantalla_actualizar( const pantalla_estado_t* e, uint32_t ahora_s )
{
    char txt[48];
    char t[20];

    // 1-2: red (ADR apagado: el SF lo alterna la placa en cada ciclo)
    if( e->unida )
    {
        snprintf( txt, sizeof( txt ), "Red: UNIDA  ADR off" );
        poner_linea( 1, TFT_VERDE, txt );
        snprintf( txt, sizeof( txt ), "Addr %08lX  ciclo %s", ( unsigned long ) e->devaddr,
                  ( e->sf_actual == PANTALLA_SF10 ) ? "SF10" : "SF7" );
        poner_linea( 2, TFT_BLANCO, txt );
    }
    else
    {
        snprintf( txt, sizeof( txt ), "Red: uniendo... (%u)", e->intentos_join );
        poner_linea( 1, TFT_AMARILLO, txt );
        poner_linea( 2, TFT_GRIS, "Addr --------" );
    }

    // 3-4: uplinks
    snprintf( txt, sizeof( txt ), "UL #%u ok %u no %u", e->ul_pedidos, e->ul_ok, e->ul_no );
    poner_linea( 3, ( e->ul_no == 0 ) ? TFT_BLANCO : TFT_NARANJA, txt );
    hace( t, sizeof( t ), e->t_ultimo_tx_s, ahora_s );
    snprintf( txt, sizeof( txt ), "Ult. TX %s", t );
    poner_linea( 4, TFT_BLANCO, txt );

    // 5-7: tabla de LinkCheck por SF (calidad del uplink vista desde el gateway)
    poner_linea( 5, TFT_GRIS, " SF   marg  gw resp  hace" );
    poner_fila_sf( 6, "SF10", &e->sf[PANTALLA_SF10], e->unida && ( e->sf_actual == PANTALLA_SF10 ), ahora_s );
    poner_fila_sf( 7, "SF7", &e->sf[PANTALLA_SF7], e->unida && ( e->sf_actual == PANTALLA_SF7 ), ahora_s );

    // 8: ultimo downlink recibido (respuestas a los LinkCheck)
    if( e->dl_valido )
    {
        hace( t, sizeof( t ), e->t_dl_s, ahora_s );
        snprintf( txt, sizeof( txt ), "DL %d/%ddB %s", e->dl_rssi, e->dl_snr, t );
        poner_linea( 8, TFT_BLANCO, txt );
    }
    else
    {
        poner_linea( 8, TFT_GRIS, "DL: sin downlinks" );
    }

    // 9-10: GNSS
    snprintf( txt, sizeof( txt ), "GNSS %u sat  hora %s", e->gnss_sv, e->gnss_hora ? "si" : "no" );
    poner_linea( 9, ( e->gnss_sv > 0 ) ? TFT_VERDE : TFT_GRIS, txt );
    if( e->t_prox_escaneo_s > ahora_s )
    {
        snprintf( txt, sizeof( txt ), "Alm %u%%  escaneo en %lus", e->almanaque_gps,
                  ( unsigned long ) ( e->t_prox_escaneo_s - ahora_s ) );
    }
    else
    {
        snprintf( txt, sizeof( txt ), "Alm %u%%  escaneando...", e->almanaque_gps );
    }
    poner_linea( 10, TFT_BLANCO, txt );

    // 11-12: energia y tiempo
    snprintf( txt, sizeof( txt ), "Energia %lu mAh", ( unsigned long ) e->energia_mah );
    poner_linea( 11, TFT_BLANCO, txt );
    snprintf( txt, sizeof( txt ), "Encendida %02lu:%02lu:%02lu", ( unsigned long ) ( ahora_s / 3600 ),
              ( unsigned long ) ( ( ahora_s / 60 ) % 60 ), ( unsigned long ) ( ahora_s % 60 ) );
    poner_linea( 12, TFT_GRIS, txt );
}
