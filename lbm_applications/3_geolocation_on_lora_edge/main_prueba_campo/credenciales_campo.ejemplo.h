/*!
 * @file      credenciales_campo.ejemplo.h
 *
 * @brief     Plantilla de credenciales OTAA para PRUEBA_CAMPO
 *
 * Copia este fichero como credenciales_campo.h (no se versiona) y pon las claves del device dado de
 * alta en ChirpStack. Si no existe credenciales_campo.h se compila con estos valores de ejemplo, que
 * no se uniran a ninguna red.
 */

#ifndef CREDENCIALES_CAMPO_H
#define CREDENCIALES_CAMPO_H

#define CAMPO_DEVICE_EUI \
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define CAMPO_JOIN_EUI \
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define CAMPO_APP_KEY                                                                                  \
    {                                                                                                  \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
    }

#endif  // CREDENCIALES_CAMPO_H
