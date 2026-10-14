/*!
 * @file      main_vuelo_lrfhss.c
 *
 * @brief     Prueba de vuelo LR-FHSS (US915) para la LR1110 sobre Nucleo-L476RG
 *
 * Arranca sola al encender: join OTAA en LoRa y, una vez unida, un uplink cada
 * VUELO_UPLINK_PERIOD_S segundos. Los uplinks salen en LR-FHSS DR5 (CR 1/3) y uno
 * de cada VUELO_REF_EVERY_N sale en LoRa DR0 (SF10) como referencia: si en ChirpStack
 * solo aparecen los de referencia, la placa esta viva y el gateway no recibe LR-FHSS.
 *
 * El DR se fija en el dispositivo (perfil ADR propio), ADR del servidor apagado.
 * No reporta a la app de campo: se verifica en ChirpStack y en el monitor serie.
 *
 * Payload (6 bytes, big-endian):
 *   [0]    version (0x01)
 *   [1]    DR usado (5 = LR-FHSS, 0 = LoRa SF10)
 *   [2..5] numero de secuencia (uint32), empieza en 0 tras cada arranque
 *
 * fPort 18 = LR-FHSS DR5, fPort 19 = referencia LoRa DR0.
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
 * @brief Credenciales del device LR1110-DevKit (las mismas del Ejercicio 01 US915)
 */
#define VUELO_DEVICE_EUI \
    { 0xAA, 0xBB, 0xCC, 0xDD, 0x10, 0x91, 0x50, 0x01 }
#define VUELO_JOIN_EUI \
    { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 }
#define VUELO_APP_KEY                                                                                  \
    {                                                                                                  \
        0x10, 0x15, 0x10, 0x15, 0x10, 0x15, 0x10, 0x15, 0x10, 0x15, 0x10, 0x15, 0x10, 0x15, 0x10, 0x15 \
    }

/**
 * @brief Periodo entre uplinks, en segundos
 */
#ifndef VUELO_UPLINK_PERIOD_S
#define VUELO_UPLINK_PERIOD_S 15
#endif

/**
 * @brief Uno de cada N uplinks sale en LoRa como referencia (el resto, LR-FHSS)
 */
#ifndef VUELO_REF_EVERY_N
#define VUELO_REF_EVERY_N 4
#endif

#define VUELO_DR_LRFHSS 5  // US915 DR5 = LR-FHSS CR 1/3, OCW 1.523 MHz
#define VUELO_DR_REF 0     // US915 DR0 = LoRa SF10 BW125

#define VUELO_FPORT_LRFHSS 18
#define VUELO_FPORT_REF 19

#define VUELO_PAYLOAD_VERSION 0x01
#define VUELO_PAYLOAD_LEN 6

/**
 * @brief Recarga del watchdog durante el sueño (menor que el periodo del watchdog, 32 s)
 */
#define WATCHDOG_RELOAD_PERIOD_MS 20000

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static const uint8_t vuelo_dev_eui[8]  = VUELO_DEVICE_EUI;
static const uint8_t vuelo_join_eui[8] = VUELO_JOIN_EUI;
static const uint8_t vuelo_app_key[16] = VUELO_APP_KEY;

static uint32_t uplink_seq    = 0;  // Secuencia del proximo uplink
static uint32_t tx_sent_count = 0;  // Uplinks que el modem confirma como enviados
static uint8_t  last_dr       = 0;  // DR del ultimo uplink pedido

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
 * @brief Fija el DR del proximo uplink y lo pide al modem
 */
static void send_vuelo_uplink( void );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void main_vuelo_lrfhss( void )
{
    uint32_t sleep_time_ms = 0;

    hal_mcu_disable_irq( );
    hal_mcu_init( );
    smtc_modem_init( &modem_event_callback );
    hal_mcu_enable_irq( );

    SMTC_HAL_TRACE_INFO( "[VUELO] Prueba de vuelo LR-FHSS: DR%d cada %d s, referencia LoRa DR%d 1 de cada %d\n",
                         VUELO_DR_LRFHSS, VUELO_UPLINK_PERIOD_S, VUELO_DR_REF, VUELO_REF_EVERY_N );

    while( 1 )
    {
        sleep_time_ms = smtc_modem_run_engine( );

        hal_mcu_disable_irq( );
        if( smtc_modem_is_irq_flag_pending( ) == false )
        {
            hal_watchdog_reload( );
            hal_mcu_set_sleep_for_ms( MIN( sleep_time_ms, WATCHDOG_RELOAD_PERIOD_MS ) );
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

    do
    {
        CHECK_SMTC_MODEM_RC( smtc_modem_get_event( &current_event, &event_pending_count ) );

        switch( current_event.event_type )
        {
        case SMTC_MODEM_EVENT_RESET:
            SMTC_HAL_TRACE_INFO( "[VUELO] Arranque: uniendo a la red (OTAA, US915)...\n" );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_deveui( stack_id, vuelo_dev_eui ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_joineui( stack_id, vuelo_join_eui ) );
            // LoRaWAN 1.0.x: la AppKey es la NwkKey
            CHECK_SMTC_MODEM_RC( smtc_modem_set_appkey( stack_id, vuelo_app_key ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_nwkkey( stack_id, vuelo_app_key ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_region( stack_id, SMTC_MODEM_REGION_US_915 ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_join_network( stack_id ) );
            break;

        case SMTC_MODEM_EVENT_JOINED:
            SMTC_HAL_TRACE_INFO( "[VUELO] UNIDA a la red. Primer uplink en %d s\n", VUELO_UPLINK_PERIOD_S );
            uplink_seq    = 0;
            tx_sent_count = 0;
            // Una sola transmision por uplink: la fija el perfil ADR propio (BSP_USER_NUMBER_OF_RETRANSMISSION = 1)
            CHECK_SMTC_MODEM_RC( smtc_modem_alarm_start_timer( VUELO_UPLINK_PERIOD_S ) );
            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            SMTC_HAL_TRACE_WARNING( "[VUELO] Join fallido, el modem reintenta solo\n" );
            break;

        case SMTC_MODEM_EVENT_ALARM:
            send_vuelo_uplink( );
            CHECK_SMTC_MODEM_RC( smtc_modem_alarm_start_timer( VUELO_UPLINK_PERIOD_S ) );
            break;

        case SMTC_MODEM_EVENT_TXDONE:
            if( current_event.event_data.txdone.status == SMTC_MODEM_EVENT_TXDONE_NOT_SENT )
            {
                SMTC_HAL_TRACE_WARNING( "[VUELO] TX NO enviado (DR%d)\n", last_dr );
            }
            else
            {
                tx_sent_count++;
                SMTC_HAL_TRACE_INFO( "[VUELO] TX OK  DR%d  enviados=%lu\n", last_dr, ( unsigned long ) tx_sent_count );
            }
            break;

        case SMTC_MODEM_EVENT_DOWNDATA:
            CHECK_SMTC_MODEM_RC(
                smtc_modem_get_downlink_data( rx_payload, &rx_payload_size, &rx_metadata, &rx_remaining ) );
            SMTC_HAL_TRACE_INFO( "[VUELO] Downlink fPort %u, RSSI %d dBm, SNR %d dB\n", rx_metadata.fport,
                                 rx_metadata.rssi - 64, rx_metadata.snr >> 2 );
            break;

        case SMTC_MODEM_EVENT_NO_DOWNLINK_THRESHOLD:
            // Sin ADR del servidor no se esperan downlinks: solo se informa, no se re-une en pleno vuelo
            SMTC_HAL_TRACE_WARNING( "[VUELO] Umbral sin downlinks alcanzado (se ignora)\n" );
            break;

        default:
            SMTC_HAL_TRACE_INFO( "[VUELO] Evento %u\n", current_event.event_type );
            break;
        }
    } while( event_pending_count > 0 );
}

static void send_vuelo_uplink( void )
{
    const bool    is_ref = ( ( uplink_seq % VUELO_REF_EVERY_N ) == ( VUELO_REF_EVERY_N - 1 ) );
    const uint8_t dr     = is_ref ? VUELO_DR_REF : VUELO_DR_LRFHSS;
    const uint8_t fport  = is_ref ? VUELO_FPORT_REF : VUELO_FPORT_LRFHSS;

    // Perfil ADR propio con un solo DR: el modem usa exactamente ese DR en el proximo uplink
    uint8_t dr_list[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH];
    memset( dr_list, dr, sizeof( dr_list ) );
    smtc_modem_return_code_t rc = smtc_modem_adr_set_profile( STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM, dr_list );
    if( rc != SMTC_MODEM_RC_OK )
    {
        SMTC_HAL_TRACE_ERROR( "[VUELO] DR%d no permitido (rc %d): revisa que el canal 65 este habilitado\n", dr, rc );
        return;
    }

    uint8_t buff[VUELO_PAYLOAD_LEN];
    buff[0] = VUELO_PAYLOAD_VERSION;
    buff[1] = dr;
    buff[2] = ( uplink_seq >> 24 ) & 0xFF;
    buff[3] = ( uplink_seq >> 16 ) & 0xFF;
    buff[4] = ( uplink_seq >> 8 ) & 0xFF;
    buff[5] = uplink_seq & 0xFF;

    last_dr = dr;
    SMTC_HAL_TRACE_INFO( "[VUELO] #%lu  %s  DR%d  fPort %d  t=%lu s\n", ( unsigned long ) uplink_seq,
                         is_ref ? "LoRa SF10 (ref)" : "LR-FHSS", dr, fport,
                         ( unsigned long ) ( smtc_modem_hal_get_time_in_ms( ) / 1000 ) );
    CHECK_SMTC_MODEM_RC( smtc_modem_request_uplink( STACK_ID, fport, false, buff, VUELO_PAYLOAD_LEN ) );

    uplink_seq++;
}

/* --- EOF ------------------------------------------------------------------ */
