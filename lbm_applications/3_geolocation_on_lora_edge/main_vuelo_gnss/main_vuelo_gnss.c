/*!
 * @file      main_vuelo_gnss.c
 *
 * @brief     Prueba de vuelo con el escaner GNSS del LR1110 (Nucleo-L476RG, US915)
 *
 * Arranca sola al encender: join OTAA en la sub-banda 2 y, una vez unida, un escaneo
 * GNSS cada VUELO_GNSS_PERIOD_S segundos. El resultado del escaneo (sin resolver: no hay
 * solver) sale en un uplink LoRa DR0 / SF10 de 11 bytes, igual que las corridas VUELO de
 * las otras sondas. ADR apagado en el dispositivo. No reporta a la app de campo: se
 * verifica en ChirpStack y en el monitor serie.
 *
 * Payload (fPort 20, 11 bytes, little-endian):
 *   [0..1]  secuencia (uint16), empieza en 0 tras cada arranque
 *   [2]     bits 0-5: satelites detectados (0-63)
 *           bit 6:    el LR1110 tiene hora GPS
 *           bit 7:    escaneo valido para un solver (suficientes satelites)
 *   [3..10] los 4 satelites con mejor C/N0: 4 x (id del satelite, C/N0 en dB)
 *           Hueco sin satelite: id 0xFF, C/N0 0
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
#include "smtc_modem_geolocation_api.h"
#include "smtc_modem_utilities.h"

#include "smtc_modem_hal.h"
#include "smtc_hal_dbg_trace.h"

#include "smtc_hal_mcu.h"
#include "smtc_hal_watchdog.h"

#include "lr11xx_system.h"

#include "lr1110_board.h"

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
 * @brief Periodo entre escaneos GNSS (y por tanto entre uplinks), en segundos
 */
#ifndef VUELO_GNSS_PERIOD_S
#define VUELO_GNSS_PERIOD_S ( 5 * 60 )
#endif

#define VUELO_DR 0  // US915 DR0 = LoRa SF10 BW125, 11 bytes maximo
#define VUELO_FPORT 20
#define VUELO_PAYLOAD_LEN 11
#define VUELO_NB_SV_IN_PAYLOAD 4

/**
 * @brief Firmware minimo del LR1110 que exige el servicio GNSS de LBM
 */
#define LR1110_FW_VERSION_MIN 0x0401

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

static uint16_t uplink_seq        = 0;      // Secuencia del proximo uplink
static uint32_t tx_sent_count     = 0;      // Uplinks que el modem confirma como enviados
static bool     gnss_started      = false;  // El ciclo de escaneos ya esta en marcha
static bool     almanac_started   = false;  // El servicio de demodulacion de almanaque ya se lanzo
static bool     gnss_fw_supported = false;  // El firmware del LR1110 admite el servicio GNSS

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
 * @brief Lee la version de firmware del LR1110, la muestra y dice si admite el servicio GNSS
 */
static bool check_lr11xx_fw_version( void );

/**
 * @brief Arma el payload de 11 bytes con el resultado del escaneo y lo pide al modem
 */
static void send_gnss_uplink( const smtc_modem_gnss_event_data_scan_done_t* scan_done );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void main_vuelo_gnss( void )
{
    uint32_t sleep_time_ms = 0;

    hal_mcu_disable_irq( );
    hal_mcu_init( );
    smtc_modem_init( &modem_event_callback );
    hal_mcu_enable_irq( );

    SMTC_HAL_TRACE_INFO( "[VUELO] Prueba de vuelo GNSS: escaneo cada %d s, uplink LoRa DR%d fPort %d\n",
                         VUELO_GNSS_PERIOD_S, VUELO_DR, VUELO_FPORT );

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
    smtc_modem_event_t                     current_event;
    uint8_t                                event_pending_count;
    uint8_t                                stack_id            = STACK_ID;
    smtc_modem_gnss_event_data_scan_done_t gnss_scan_done_data = { 0 };

    do
    {
        CHECK_SMTC_MODEM_RC( smtc_modem_get_event( &current_event, &event_pending_count ) );

        switch( current_event.event_type )
        {
        case SMTC_MODEM_EVENT_RESET:
            gnss_fw_supported = check_lr11xx_fw_version( );
            SMTC_HAL_TRACE_INFO( "[VUELO] Arranque: uniendo a la red (OTAA, US915 sub-banda 2)...\n" );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_deveui( stack_id, vuelo_dev_eui ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_joineui( stack_id, vuelo_join_eui ) );
            // LoRaWAN 1.0.x: la AppKey es la NwkKey
            CHECK_SMTC_MODEM_RC( smtc_modem_set_nwkkey( stack_id, vuelo_app_key ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_set_region( stack_id, SMTC_MODEM_REGION_US_915 ) );
            CHECK_SMTC_MODEM_RC( smtc_modem_join_network( stack_id ) );
            if( gnss_fw_supported == true )
            {
                // El resultado se manda en el payload propio: el servicio no envia el NAV
                CHECK_SMTC_MODEM_RC( smtc_modem_gnss_send_mode( stack_id, SMTC_MODEM_SEND_MODE_BYPASS ) );
                CHECK_SMTC_MODEM_RC(
                    smtc_modem_gnss_set_constellations( stack_id, SMTC_MODEM_GNSS_CONSTELLATION_GPS_BEIDOU ) );
                CHECK_SMTC_MODEM_RC( smtc_modem_almanac_demodulation_set_constellations(
                    stack_id, SMTC_MODEM_GNSS_CONSTELLATION_GPS_BEIDOU ) );
            }
            smtc_board_led_set( smtc_board_get_led_tx_mask( ), true );
            break;

        case SMTC_MODEM_EVENT_JOINED:
            SMTC_HAL_TRACE_INFO( "[VUELO] UNIDA a la red\n" );
            smtc_board_led_set( smtc_board_get_led_tx_mask( ), false );
            uplink_seq    = 0;
            tx_sent_count = 0;
            {
                // Perfil ADR propio con un solo DR: todos los uplinks salen en DR0 (una transmision)
                uint8_t dr_list[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH];
                memset( dr_list, VUELO_DR, sizeof( dr_list ) );
                CHECK_SMTC_MODEM_RC( smtc_modem_adr_set_profile( stack_id, SMTC_MODEM_ADR_PROFILE_CUSTOM, dr_list ) );
            }
            if( gnss_fw_supported == false )
            {
                SMTC_HAL_TRACE_ERROR( "[VUELO] Sin servicio GNSS: actualiza el firmware del LR1110\n" );
            }
            else if( gnss_started == false )
            {
                gnss_started = true;
                SMTC_HAL_TRACE_INFO( "[VUELO] Primer escaneo GNSS ahora\n" );
                CHECK_SMTC_MODEM_RC( smtc_modem_gnss_scan( stack_id, SMTC_MODEM_GNSS_MODE_MOBILE, 0 ) );
            }
            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            SMTC_HAL_TRACE_WARNING( "[VUELO] Join fallido, el modem reintenta solo\n" );
            break;

        case SMTC_MODEM_EVENT_GNSS_SCAN_DONE:
            CHECK_SMTC_MODEM_RC( smtc_modem_gnss_get_event_data_scan_done( stack_id, &gnss_scan_done_data ) );
            if( ( gnss_scan_done_data.time_available == true ) && ( almanac_started == false ) )
            {
                // Con hora GPS el LR1110 ya puede ir actualizando el almanaque desde el cielo
                almanac_started = true;
                CHECK_SMTC_MODEM_RC( smtc_modem_almanac_demodulation_start( stack_id ) );
            }
            send_gnss_uplink( &gnss_scan_done_data );
            break;

        case SMTC_MODEM_EVENT_GNSS_TERMINATED:
            CHECK_SMTC_MODEM_RC( smtc_modem_gnss_scan( stack_id, SMTC_MODEM_GNSS_MODE_MOBILE, VUELO_GNSS_PERIOD_S ) );
            SMTC_HAL_TRACE_INFO( "[VUELO] Proximo escaneo GNSS en %d s\n", VUELO_GNSS_PERIOD_S );
            break;

        case SMTC_MODEM_EVENT_GNSS_ALMANAC_DEMOD_UPDATE:
        {
            smtc_modem_almanac_demodulation_event_data_almanac_update_t almanac = { 0 };
            CHECK_SMTC_MODEM_RC(
                smtc_modem_almanac_demodulation_get_event_data_almanac_update( stack_id, &almanac ) );
            SMTC_HAL_TRACE_INFO( "[VUELO] Almanaque: GPS %u %%, BeiDou %u %%\n", almanac.update_progress_gps,
                                 almanac.update_progress_beidou );
            break;
        }

        case SMTC_MODEM_EVENT_TXDONE:
            if( current_event.event_data.txdone.status == SMTC_MODEM_EVENT_TXDONE_NOT_SENT )
            {
                SMTC_HAL_TRACE_WARNING( "[VUELO] TX NO enviado\n" );
            }
            else
            {
                tx_sent_count++;
                smtc_board_led_pulse( smtc_board_get_led_tx_mask( ), true, 250 );
                SMTC_HAL_TRACE_INFO( "[VUELO] TX OK  enviados=%lu\n", ( unsigned long ) tx_sent_count );
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

static void send_gnss_uplink( const smtc_modem_gnss_event_data_scan_done_t* scan_done )
{
    // Del grupo de escaneos se toma el que mas satelites vio
    const smtc_modem_gnss_event_data_scan_desc_t* best = NULL;
    for( uint8_t i = 0; i < scan_done->nb_scans_valid; i++ )
    {
        if( ( best == NULL ) || ( scan_done->scans[i].nb_svs > best->nb_svs ) )
        {
            best = &scan_done->scans[i];
        }
    }
    const uint8_t nb_svs = ( best != NULL ) ? best->nb_svs : 0;

    uint8_t buff[VUELO_PAYLOAD_LEN];
    buff[0] = uplink_seq & 0xFF;
    buff[1] = ( uplink_seq >> 8 ) & 0xFF;
    buff[2] = MIN( nb_svs, 0x3F ) | ( scan_done->time_available ? 0x40 : 0x00 ) | ( scan_done->is_valid ? 0x80 : 0x00 );

    // Los 4 satelites con mejor C/N0, de mayor a menor
    bool used[64] = { false };
    for( uint8_t k = 0; k < VUELO_NB_SV_IN_PAYLOAD; k++ )
    {
        int16_t best_idx = -1;
        for( uint8_t i = 0; i < MIN( nb_svs, 64 ); i++ )
        {
            if( ( used[i] == false ) && ( ( best_idx < 0 ) || ( best->info_svs[i].cnr > best->info_svs[best_idx].cnr ) ) )
            {
                best_idx = i;
            }
        }
        if( best_idx >= 0 )
        {
            used[best_idx]  = true;
            buff[3 + 2 * k] = best->info_svs[best_idx].satellite_id;
            buff[4 + 2 * k] = ( uint8_t ) best->info_svs[best_idx].cnr;
        }
        else
        {
            buff[3 + 2 * k] = 0xFF;
            buff[4 + 2 * k] = 0;
        }
    }

    SMTC_HAL_TRACE_INFO( "[VUELO] #%u  GNSS: %u satelites, hora GPS %s, valido %s  t=%lu s\n", uplink_seq, nb_svs,
                         scan_done->time_available ? "si" : "no", scan_done->is_valid ? "si" : "no",
                         ( unsigned long ) ( smtc_modem_hal_get_time_in_ms( ) / 1000 ) );
    for( uint8_t k = 0; k < VUELO_NB_SV_IN_PAYLOAD; k++ )
    {
        if( buff[3 + 2 * k] != 0xFF )
        {
            SMTC_HAL_TRACE_INFO( "[VUELO]     sat %u  C/N0 %d dB\n", buff[3 + 2 * k], ( int8_t ) buff[4 + 2 * k] );
        }
    }

    smtc_modem_status_mask_t status_mask = 0;
    smtc_modem_get_status( STACK_ID, &status_mask );
    if( ( status_mask & SMTC_MODEM_STATUS_JOINED ) != SMTC_MODEM_STATUS_JOINED )
    {
        SMTC_HAL_TRACE_WARNING( "[VUELO] Sin red: el escaneo no se envia\n" );
        return;
    }
    CHECK_SMTC_MODEM_RC( smtc_modem_request_uplink( STACK_ID, VUELO_FPORT, false, buff, VUELO_PAYLOAD_LEN ) );
    uplink_seq++;
}

static bool check_lr11xx_fw_version( void )
{
    lr11xx_system_version_t version;

    // Suspende el modem para acceder a la radio
    smtc_modem_suspend_radio_communications( true );
    lr11xx_status_t status = lr11xx_system_get_version( NULL, &version );
    smtc_modem_suspend_radio_communications( false );

    if( status != LR11XX_STATUS_OK )
    {
        SMTC_HAL_TRACE_ERROR( "[VUELO] No se pudo leer la version del LR1110\n" );
        return false;
    }
    SMTC_HAL_TRACE_INFO( "[VUELO] LR11xx tipo 0x%02X, hw 0x%02X, firmware 0x%04X\n", version.type, version.hw,
                         version.fw );
    if( ( version.type == LR11XX_SYSTEM_VERSION_TYPE_LR1110 ) && ( version.fw < LR1110_FW_VERSION_MIN ) )
    {
        SMTC_HAL_TRACE_ERROR( "[VUELO] Firmware 0x%04X < 0x%04X: el servicio GNSS no funcionara\n", version.fw,
                              LR1110_FW_VERSION_MIN );
        return false;
    }
    return true;
}

/* --- EOF ------------------------------------------------------------------ */
