/*!
 * \file      smtc_hal_i2c.c
 *
 * \brief     I2C Hardware Abstraction Layer implementation
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 *
 * Pin assignment (Nucleo-L476RG, Arduino connector) -- SRS-001 REQ-001-3:
 *   I2C1 SCL -> PB_8  (Arduino header D15)
 *   I2C1 SDA -> PB_9  (Arduino header D14)
 *
 * These pins are the board default I2C1 mapping and DO NOT collide with:
 *   - the LR11xx radio SPI (PA_5/PA_6/PA_7 SCLK/MISO/MOSI, PA_8 NSS, PA_0 NRST,
 *     PB_3 BUSY, PB_4 DIOX)  -- see modem_pinout.h
 *   - the debug UART (PA_2 TX / PA_3 RX)                    -- see modem_pinout.h
 *   - the modem LEDs / HW-modem lines (PC_x, PB_0, PB_5)    -- see modem_pinout.h
 * PB_8 and PB_9 are otherwise unused by this application.
 */

#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type

#include "smtc_hal_i2c.h"
#include "smtc_hal_gpio_pin_names.h"
#include "stm32l4xx_hal.h"

#include "smtc_hal_mcu.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*!
 * \brief Extracts the STM32 GPIO port base address from a hal_gpio_pin_names_t
 *        value (upper nibble encodes the port, same scheme as smtc_hal_spi.c).
 */
#define I2C_GPIO_PORT( pin ) ( ( GPIO_TypeDef* ) ( AHB2PERIPH_BASE + ( ( ( pin ) &0xF0 ) << 6 ) ) )

/*!
 * \brief Extracts the STM32 GPIO pin mask from a hal_gpio_pin_names_t value.
 */
#define I2C_GPIO_PIN( pin ) ( ( uint32_t )( 1U << ( ( pin ) &0x0F ) ) )

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*!
 * \brief I2C TIMINGR value for Standard mode 100 kHz.
 *
 * Computed for an I2C kernel clock of 80 MHz (I2C1 defaults to PCLK1 = 80 MHz on
 * this board: HSI 16 MHz, PLLN=10, PLLR=2 -> SYSCLK 80 MHz, AHB/APB1 div 1).
 * Value is the ST CubeMX reference for 100 kHz Standard mode with analog filter
 * enabled. Satisfies SRS-001 REQ-001-2 (>= 100 kHz).
 */
#define I2C_TIMING_100KHZ_80MHZ ( 0x10909CECU )

/*!
 * \brief 7-bit slave address to 8-bit ST HAL address format conversion.
 *
 * The ST HAL expects the address left-aligned on 8 bits (R/W bit as LSB).
 */
#define I2C_ADDR_7BIT_TO_HAL( addr ) ( ( uint16_t )( ( uint16_t )( addr ) << 1 ) )

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

typedef struct i2c_s
{
    I2C_TypeDef*      interface;
    I2C_HandleTypeDef handle;
    struct
    {
        hal_gpio_pin_names_t sda;
        hal_gpio_pin_names_t scl;
    } pins;
} i2c_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static i2c_t i2c_periph[] = {
    [0] =
        {
            .interface = I2C1,
            .handle    = { 0 },
            .pins =
                {
                    .sda = NC,
                    .scl = NC,
                },
        },
};

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void smtc_hal_i2c_init( uint32_t id, uint32_t sda, uint32_t scl )
{
    // Guard against an invalid id without hanging the firmware (REQ-001-6)
    if( ( id == 0 ) || ( ( id - 1 ) >= ( sizeof( i2c_periph ) / sizeof( i2c_periph[0] ) ) ) )
    {
        return;
    }
    uint32_t local_id = id - 1;

    i2c_periph[local_id].pins.sda = ( hal_gpio_pin_names_t ) sda;
    i2c_periph[local_id].pins.scl = ( hal_gpio_pin_names_t ) scl;

    i2c_periph[local_id].handle.Instance              = i2c_periph[local_id].interface;
    i2c_periph[local_id].handle.Init.Timing           = I2C_TIMING_100KHZ_80MHZ;
    i2c_periph[local_id].handle.Init.OwnAddress1      = 0;
    i2c_periph[local_id].handle.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    i2c_periph[local_id].handle.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    i2c_periph[local_id].handle.Init.OwnAddress2      = 0;
    i2c_periph[local_id].handle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    i2c_periph[local_id].handle.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    i2c_periph[local_id].handle.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    if( HAL_I2C_Init( &i2c_periph[local_id].handle ) != HAL_OK )
    {
        mcu_panic( );
    }

    // Enable the Analog and Digital filters
    HAL_I2CEx_ConfigAnalogFilter( &i2c_periph[local_id].handle, I2C_ANALOGFILTER_ENABLE );
    HAL_I2CEx_ConfigDigitalFilter( &i2c_periph[local_id].handle, 0 );
}

void smtc_hal_i2c_deinit( uint32_t id )
{
    if( ( id == 0 ) || ( ( id - 1 ) >= ( sizeof( i2c_periph ) / sizeof( i2c_periph[0] ) ) ) )
    {
        return;
    }
    uint32_t local_id = id - 1;

    HAL_I2C_DeInit( &i2c_periph[local_id].handle );
}

uint8_t smtc_hal_i2c_write_buffer( uint32_t id, uint8_t device_addr, uint16_t mem_addr, const uint8_t* buffer,
                                   uint16_t size )
{
    if( ( id == 0 ) || ( ( id - 1 ) >= ( sizeof( i2c_periph ) / sizeof( i2c_periph[0] ) ) ) )
    {
        return 1;
    }
    uint32_t local_id = id - 1;

    // HAL_I2C_Mem_Write returns HAL_OK (0) on success, a non-zero HAL_StatusTypeDef
    // on error/timeout/NACK. A finite HAL_I2C_TIMEOUT_MS guarantees no infinite
    // blocking on bus errors (REQ-001-4 / REQ-001-6).
    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Write( &i2c_periph[local_id].handle, I2C_ADDR_7BIT_TO_HAL( device_addr ), mem_addr,
                           I2C_MEMADD_SIZE_8BIT, ( uint8_t* ) buffer, size, HAL_I2C_TIMEOUT_MS );

    return ( uint8_t ) status;
}

uint8_t smtc_hal_i2c_read_buffer( uint32_t id, uint8_t device_addr, uint16_t mem_addr, uint8_t* buffer, uint16_t size )
{
    if( ( id == 0 ) || ( ( id - 1 ) >= ( sizeof( i2c_periph ) / sizeof( i2c_periph[0] ) ) ) )
    {
        return 1;
    }
    uint32_t local_id = id - 1;

    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Read( &i2c_periph[local_id].handle, I2C_ADDR_7BIT_TO_HAL( device_addr ), mem_addr,
                          I2C_MEMADD_SIZE_8BIT, buffer, size, HAL_I2C_TIMEOUT_MS );

    return ( uint8_t ) status;
}

void HAL_I2C_MspInit( I2C_HandleTypeDef* i2cHandle )
{
    if( i2cHandle->Instance == i2c_periph[0].interface )
    {
        // Enable the GPIO port clock (SCL/SDA are on GPIOB by default) (REQ-001-2)
        __HAL_RCC_GPIOB_CLK_ENABLE( );

        // SDA and SCL share the same GPIO port (GPIOB) on the default mapping.
        GPIO_TypeDef*    gpio_port = I2C_GPIO_PORT( i2c_periph[0].pins.sda );
        GPIO_InitTypeDef gpio      = {
            .Mode      = GPIO_MODE_AF_OD,      // alternate-function, open-drain (REQ-001-2)
            .Pull      = GPIO_PULLUP,          // internal pull-up on SDA/SCL (REQ-001-2)
            .Speed     = GPIO_SPEED_FREQ_HIGH,
            .Alternate = GPIO_AF4_I2C1,
        };
        gpio.Pin = I2C_GPIO_PIN( i2c_periph[0].pins.sda ) | I2C_GPIO_PIN( i2c_periph[0].pins.scl );
        HAL_GPIO_Init( gpio_port, &gpio );

        __HAL_RCC_I2C1_CLK_ENABLE( );
    }
    else
    {
        mcu_panic( );
    }
}

void HAL_I2C_MspDeInit( I2C_HandleTypeDef* i2cHandle )
{
    if( i2cHandle->Instance == i2c_periph[0].interface )
    {
        __HAL_RCC_I2C1_CLK_DISABLE( );

        GPIO_TypeDef*    gpio_port = I2C_GPIO_PORT( i2c_periph[0].pins.sda );
        GPIO_InitTypeDef gpio      = {
            .Mode  = GPIO_MODE_ANALOG,
            .Pull  = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
        };
        gpio.Pin = I2C_GPIO_PIN( i2c_periph[0].pins.sda ) | I2C_GPIO_PIN( i2c_periph[0].pins.scl );
        HAL_GPIO_Init( gpio_port, &gpio );
    }
    else
    {
        mcu_panic( );
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/* --- EOF ------------------------------------------------------------------ */
