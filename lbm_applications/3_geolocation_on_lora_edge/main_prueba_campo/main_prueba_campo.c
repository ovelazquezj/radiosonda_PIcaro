/*!
 * @file      main_prueba_campo.c
 *
 * @brief     Prueba de campo del alcance de los gateways con el LR1121-DevKit (Nucleo-L476RG, US915)
 *
 * Arranca sola al encender: join OTAA en la sub-banda 2 y, una vez unida, un envio cada
 * CAMPO_PERIODO_S segundos alternando SF10 (DR0) y SF7 (DR3). Cada envio es una trama corta con un
 * LinkCheckReq: la respuesta de la red trae el margen (dB sobre el umbral de demodulacion del mejor
 * gateway) y cuantos gateways oyeron la trama, y el propio kit mide RSSI/SNR del downlink que la trae.
 * Todo se ve en la TFT del kit (pantalla_campo.c) y en la serie; no hay app ni payload de aplicacion.
 *
 * El DR se fija antes de cada envio con un perfil ADR personalizado de un solo DR: la distribucion
 * personalizada de LBM sortea el DR al azar, y asi la alternancia es estricta. Con ese perfil la trama
 * sale con el bit ADR a 0, de modo que la red no cambia ni el DR ni la potencia durante la prueba.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type
#include <string.h>

#include "main.h"

#include "smtc_modem_api.h"
#include "smtc_modem_utilities.h"

#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"

#include "smtc_hal_mcu.h"
#include "smtc_hal_watchdog.h"

#include "lr11xx_system.h"

#include "lr1110_board.h"

#include "pantalla_campo.h"

#if defined( __has_include )
#if __has_include( "credenciales_campo.h" )
#include "credenciales_campo.h"
#else
#warning "Sin credenciales_campo.h: se usan las claves de ejemplo, que no se uniran a ninguna red"
#include "credenciales_campo.ejemplo.h"
#endif
#else
#include "credenciales_campo.h"
#endif

/*
 * Funcion interna de LBM (lorawan_api.h) que solo se usa para mostrar el DevAddr en la pantalla. Se
 * declara aqui para no arrastrar toda la cadena de includes internos de la pila.
 */
uint32_t lorawan_api_devaddr_get( uint8_t stack_id );

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

#ifndef MIN
#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

#define xstr( a ) str( a )
#define str( a ) #a

/*!
 * @brief Avisa por la serie si una llamada al modem no devuelve SMTC_MODEM_RC_OK
 */
#define CHECK_SMTC_MODEM_RC( rc_func )                                                                      \
    do                                                                                                      \
    {                                                                                                       \
        smtc_modem_return_code_t rc = rc_func;                                                              \
        if( rc != SMTC_MODEM_RC_OK )                                                                        \
        {                                                                                                   \
            SMTC_HAL_TRACE_ERROR( "%s (line %d): %s -> rc %d\n", __func__, __LINE__, xstr( rc_func ), rc ); \
        }                                                                                                   \
    } while( 0 )

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define STACK_ID 0

/**
 * @brief Periodo entre dos envios, en segundos
 */
#ifndef CAMPO_PERIODO_S
#define CAMPO_PERIODO_S 30
#endif

/**
 * @brief Espera entre el join y el primer envio, en segundos
 */
#define CAMPO_PRIMER_ENVIO_S 5

/**
 * @brief DR que se alternan: DR0 = SF10 y DR3 = SF7 (US915, 125 kHz)
 */
static const uint8_t campo_dr[CAMPO_NB_DR] = { 0, 3 };

/**
 * @brief Recarga del watchdog durante el sueño (menor que el periodo del watchdog, 32 s)
 */
#define WATCHDOG_RELOAD_PERIOD_MS 20000

/**
 * @brief La pantalla se refresca como mucho una vez por segundo (cuentas atras y "hace N s")
 */
#define PANTALLA_REFRESH_MS 1000

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static const uint8_t campo_dev_eui[8]  = CAMPO_DEVICE_EUI;
static const uint8_t campo_join_eui[8] = CAMPO_JOIN_EUI;
static const uint8_t campo_app_key[16] = CAMPO_APP_KEY;

static pantalla_campo_t estado = { 0 };

static uint8_t en_curso_idx = 0;  // indice (en campo_dr) del envio cuya respuesta se espera

static uint8_t                  rx_payload[SMTC_MODEM_MAX_LORAWAN_PAYLOAD_LENGTH] = { 0 };
static uint8_t                  rx_payload_size                                   = 0;
static smtc_modem_dl_metadata_t rx_metadata                                       = { 0 };
static uint8_t                  rx_remaining                                      = 0;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void modem_event_callback( void );

/**
 * @brief Fija el DR del proximo envio, pide el LinkCheck y programa el siguiente
 */
static void enviar_link_check( void );

/**
 * @brief Muestra por la serie el tipo y la version de firmware del LR11xx (diagnostico)
 */
static void print_lr11xx_version( void );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void main_prueba_campo( void )
{
    uint32_t sleep_time_ms     = 0;
    uint32_t ultimo_refresco_s = 0;

    hal_mcu_disable_irq( );
    hal_mcu_init( );
    smtc_modem_init( &modem_event_callback );
    hal_mcu_enable_irq( );

    SMTC_HAL_TRACE_INFO( "[CAMPO] Prueba de campo: LinkCheck cada %d s alternando SF10 y SF7\n", CAMPO_PERIODO_S );

    for( uint8_t i = 0; i < CAMPO_NB_DR; i++ )
    {
        estado.dr[i].dr = campo_dr[i];
    }

    // La TFT comparte el SPI con el LR1121: se inicializa despues del modem (que ya configuro el SPI)
    uint16_t tft_id = pantalla_campo_init( );
    SMTC_HAL_TRACE_INFO( "[CAMPO] TFT inicializada (id leido 0x%04X; 0x9341 = ILI9341)\n", tft_id );

    while( 1 )
    {
        sleep_time_ms = smtc_modem_run_engine( );

        const uint32_t ahora_s = smtc_modem_hal_get_time_in_s( );
        if( ahora_s != ultimo_refresco_s )
        {
            ultimo_refresco_s = ahora_s;
            pantalla_campo_actualizar( &estado, ahora_s );
        }

        hal_mcu_disable_irq( );
        if( smtc_modem_is_irq_flag_pending( ) == false )
        {
            hal_watchdog_reload( );
            hal_mcu_set_sleep_for_ms( MIN( MIN( sleep_time_ms, WATCHDOG_RELOAD_PERIOD_MS ), PANTALLA_REFRESH_MS ) );
        }
        hal_watchdog_reload( );
        hal_mcu_enable_irq( );
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void modem_event_callback( void )
{
    smtc_modem_event_t current_event;
    uint8_t            event_pending_count;
    uint8_t            stack_id = STACK_ID;
    const uint32_t     ahora_s  = smtc_modem_hal_get_time_in_s( );

    do
    {
        CHECK_SMTC_MODEM_RC( smtc_modem_get_event( &current_event, &event_pending_count ) );

        switch( current_event.event_type )
        {
        case SMTC_MODEM_EVENT_RESET:
            print_lr11xx_version( );
            SMTC_HAL_TRACE_INFO( "[CAMPO] Arranque: uniendo a la red (OTAA, US915 sub-banda 2)...\n" );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_deveui( stack_id, campo_dev_eui ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_joineui( stack_id, campo_join_eui ) );
            // LoRaWAN 1.0.x: la AppKey es la NwkKey
            CHECK_SMTC_MODEM_RC( smtc_modem_set_nwkkey( stack_id, campo_app_key ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_region( stack_id, SMTC_MODEM_REGION_US_915 ) );
            // Tambien los downlinks solo de MAC (LinkCheckAns): dan el RSSI/SNR del downlink
            CHECK_SMTC_MODEM_RC( smtc_modem_set_report_all_downlinks_to_user( stack_id, true ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_join_network( stack_id ) );
            estado.intentos_join = 1;
            smtc_board_led_set( smtc_board_get_led_tx_mask( ), true );
            break;

        case SMTC_MODEM_EVENT_JOINED:
            SMTC_HAL_TRACE_INFO( "[CAMPO] UNIDA a la red\n" );
            smtc_board_led_set( smtc_board_get_led_tx_mask( ), false );
            estado.unida       = true;
            estado.devaddr     = lorawan_api_devaddr_get( stack_id );
            estado.proximo_idx = 0;
            estado.t_proximo_s = ahora_s + CAMPO_PRIMER_ENVIO_S;
            CHECK_SMTC_MODEM_RC( smtc_modem_alarm_start_timer( CAMPO_PRIMER_ENVIO_S ) );
            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            estado.intentos_join++;
            SMTC_HAL_TRACE_WARNING( "[CAMPO] Join fallido, el modem reintenta solo\n" );
            break;

        case SMTC_MODEM_EVENT_ALARM:
            enviar_link_check( );
            break;

        case SMTC_MODEM_EVENT_LINK_CHECK:
        {
            stats_dr_t* s     = &estado.dr[en_curso_idx];
            estado.ultimo_idx = en_curso_idx;
            estado.t_ultimo_s = ahora_s;
            if( current_event.event_data.link_check.status == SMTC_MODEM_EVENT_MAC_REQUEST_ANSWERED )
            {
                CHECK_SMTC_MODEM_RC( smtc_modem_get_lorawan_link_check_data( stack_id, &s->margen_db, &s->gw ) );
                s->respondidos++;
                estado.ultimo = RESULTADO_OK;
                SMTC_HAL_TRACE_INFO( "[CAMPO] DR%u: margen %u dB, %u gateway(s)  [%u/%u]\n", s->dr, s->margen_db,
                                     s->gw, s->respondidos, s->enviados );
            }
            else
            {
                estado.ultimo = RESULTADO_SIN_RESPUESTA;
                SMTC_HAL_TRACE_WARNING( "[CAMPO] DR%u: LinkCheck SIN RESPUESTA  [%u/%u]\n", s->dr, s->respondidos,
                                        s->enviados );
            }
            break;
        }

        case SMTC_MODEM_EVENT_DOWNDATA:
        {
            CHECK_SMTC_MODEM_RC(
                smtc_modem_get_downlink_data( rx_payload, &rx_payload_size, &rx_metadata, &rx_remaining ) );
            stats_dr_t* s = &estado.dr[en_curso_idx];
            s->dl_valido  = true;
            s->dl_rssi    = rx_metadata.rssi - 64;
            s->dl_snr     = rx_metadata.snr >> 2;
            SMTC_HAL_TRACE_INFO( "[CAMPO] Downlink tras DR%u: RSSI %d dBm, SNR %d dB\n", s->dr, s->dl_rssi,
                                 s->dl_snr );
            break;
        }

        case SMTC_MODEM_EVENT_TXDONE:
            SMTC_HAL_TRACE_INFO( "[CAMPO] TXDONE estado %u\n", current_event.event_data.txdone.status );
            break;

        case SMTC_MODEM_EVENT_NO_DOWNLINK_THRESHOLD:
            // El DR lo fija la app en cada envio; no se re-une en plena prueba
            SMTC_HAL_TRACE_WARNING( "[CAMPO] Umbral sin downlinks alcanzado (se ignora)\n" );
            break;

        default:
            SMTC_HAL_TRACE_INFO( "[CAMPO] Evento %u\n", current_event.event_type );
            break;
        }
    } while( event_pending_count > 0 );
}

static void enviar_link_check( void )
{
    const uint32_t ahora_s = smtc_modem_hal_get_time_in_s( );
    const uint8_t  idx     = estado.proximo_idx;

    // El siguiente envio se programa pase lo que pase con este
    CHECK_SMTC_MODEM_RC( smtc_modem_alarm_start_timer( CAMPO_PERIODO_S ) );
    estado.proximo_idx = ( idx + 1 ) % CAMPO_NB_DR;
    estado.t_proximo_s = ahora_s + CAMPO_PERIODO_S;

    // Perfil personalizado con un solo DR: este envio sale si o si con campo_dr[idx]
    uint8_t distribucion[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH];
    memset( distribucion, campo_dr[idx], sizeof( distribucion ) );
    smtc_modem_return_code_t rc = smtc_modem_adr_set_profile( STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, distribucion );
    if( rc != SMTC_MODEM_RC_OK )
    {
        SMTC_HAL_TRACE_ERROR( "[CAMPO] No se pudo fijar DR%u (rc %d): envio omitido\n", campo_dr[idx], rc );
        return;
    }

    rc = smtc_modem_trig_lorawan_mac_request( STACK_ID, SMTC_MODEM_LORAWAN_MAC_REQ_LINK_CHECK );
    if( rc != SMTC_MODEM_RC_OK )
    {
        SMTC_HAL_TRACE_ERROR( "[CAMPO] LinkCheck DR%u rechazado por el modem (rc %d)\n", campo_dr[idx], rc );
        return;
    }

    en_curso_idx = idx;
    estado.dr[idx].enviados++;
    estado.ultimo = RESULTADO_PENDIENTE;
    smtc_board_led_pulse( smtc_board_get_led_tx_mask( ), true, 250 );
    SMTC_HAL_TRACE_INFO( "[CAMPO] Envio #%u con DR%u (LinkCheckReq)  t=%lu s\n", estado.dr[idx].enviados,
                         campo_dr[idx], ( unsigned long ) ahora_s );
}

static void print_lr11xx_version( void )
{
    lr11xx_system_version_t version;

    // Suspende el modem para acceder a la radio
    smtc_modem_suspend_radio_communications( true );
    lr11xx_status_t status = lr11xx_system_get_version( NULL, &version );
    smtc_modem_suspend_radio_communications( false );

    if( status != LR11XX_STATUS_OK )
    {
        SMTC_HAL_TRACE_ERROR( "[CAMPO] No se pudo leer la version del LR11xx\n" );
        return;
    }
    SMTC_HAL_TRACE_INFO( "[CAMPO] LR11xx tipo 0x%02X (0x03 = LR1121), hw 0x%02X, firmware 0x%04X\n", version.type,
                         version.hw, version.fw );
}

/* --- EOF ------------------------------------------------------------------ */
