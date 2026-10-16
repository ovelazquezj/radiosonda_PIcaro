/*!
 * @file      pantalla.h
 *
 * @brief     Pantalla de estado del enlace en la TFT: join, uplinks, LinkCheck por SF, downlinks, GNSS y energia
 */

#ifndef PANTALLA_H
#define PANTALLA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Indice de cada SF en las tablas: los ciclos alternan SF10 (DR0) y SF7 (DR3)
 */
#define PANTALLA_SF10 0
#define PANTALLA_SF7 1
#define PANTALLA_NB_SF 2

/**
 * @brief Resultado de los LinkCheck de un SF (margen en el gateway visto por la red)
 */
typedef struct
{
    uint16_t enviados;    // LinkCheck pedidos con este SF
    uint16_t respuestas;  // LinkCheck contestados
    bool     ultimo_ok;   // el ultimo tuvo respuesta
    uint8_t  margen_db;   // margen del ultimo contestado
    uint8_t  gw;          // gateways del ultimo contestado
    uint32_t t_s;         // instante del ultimo resultado (0 = nunca)
} pantalla_sf_t;

/**
 * @brief Todo lo que muestra la pantalla. Los tiempos son segundos desde el arranque; 0 = nunca.
 */
typedef struct
{
    bool     unida;
    uint16_t intentos_join;
    uint32_t devaddr;
    uint8_t  sf_actual;   // PANTALLA_SF10 o PANTALLA_SF7: el del ciclo en curso
    uint16_t ul_pedidos;  // uplinks GNSS pedidos al modem
    uint16_t ul_ok;       // TXDONE enviados
    uint16_t ul_no;       // TXDONE no enviados
    uint32_t t_ultimo_tx_s;

    pantalla_sf_t sf[PANTALLA_NB_SF];

    bool     dl_valido;
    int16_t  dl_rssi;
    int8_t   dl_snr;
    uint32_t t_dl_s;

    uint8_t  gnss_sv;
    bool     gnss_hora;
    bool     gnss_valido;
    uint8_t  almanaque_gps;
    uint32_t t_prox_escaneo_s;

    uint32_t energia_mah;
} pantalla_estado_t;

/**
 * @brief Inicializa la TFT y dibuja la cabecera. Devuelve el id leido del controlador (diagnostico).
 */
uint16_t pantalla_init( void );

/**
 * @brief Redibuja las lineas que cambiaron respecto al ultimo dibujo
 *
 * @param [in] ahora_s  Segundos desde el arranque (para "hace N s" y cuentas atras)
 */
void pantalla_actualizar( const pantalla_estado_t* e, uint32_t ahora_s );

#endif  // PANTALLA_H
