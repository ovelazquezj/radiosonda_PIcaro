/*!
 * @file      pantalla_campo.h
 *
 * @brief     Pantalla de la prueba de campo: resultado del ultimo LinkCheck y estadisticas por SF
 */

#ifndef PANTALLA_CAMPO_H
#define PANTALLA_CAMPO_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Numero de DR que se alternan (SF10 y SF7)
 */
#define CAMPO_NB_DR 2

typedef enum
{
    RESULTADO_NINGUNO,    // aun no hubo ningun envio
    RESULTADO_PENDIENTE,  // enviado, esperando la respuesta del LinkCheck
    RESULTADO_OK,
    RESULTADO_SIN_RESPUESTA,
} resultado_t;

/**
 * @brief Estadisticas de un DR desde el arranque
 */
typedef struct
{
    uint8_t  dr;
    uint16_t enviados;    // LinkCheck intentados con este DR
    uint16_t respondidos; // LinkCheck con respuesta de la red
    uint8_t  margen_db;   // del ultimo LinkCheck respondido
    uint8_t  gw;          // del ultimo LinkCheck respondido
    bool     dl_valido;
    int16_t  dl_rssi;     // del ultimo downlink recibido tras un envio con este DR
    int8_t   dl_snr;
} stats_dr_t;

/**
 * @brief Todo lo que muestra la pantalla. Los tiempos son segundos desde el arranque; 0 = nunca.
 */
typedef struct
{
    bool     unida;
    uint16_t intentos_join;
    uint32_t devaddr;

    resultado_t ultimo;      // resultado del ultimo envio
    uint8_t     ultimo_idx;  // indice (en dr[]) del ultimo envio
    uint32_t    t_ultimo_s;  // instante del ultimo resultado (OK o sin respuesta)

    uint32_t t_proximo_s;  // instante del proximo envio
    uint8_t  proximo_idx;  // indice (en dr[]) del proximo envio

    stats_dr_t dr[CAMPO_NB_DR];
} pantalla_campo_t;

/**
 * @brief Inicializa la TFT y dibuja la cabecera. Devuelve el id leido del controlador (diagnostico).
 */
uint16_t pantalla_campo_init( void );

/**
 * @brief Redibuja lo que cambio respecto al ultimo dibujo
 *
 * @param [in] ahora_s  Segundos desde el arranque (para cuentas atras y "hace N s")
 */
void pantalla_campo_actualizar( const pantalla_campo_t* e, uint32_t ahora_s );

#endif  // PANTALLA_CAMPO_H
