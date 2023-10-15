/*
 * gpio.h
 *
 * Types, constants, macros, and function prototypes for a standard 
 * general purpose input/output driver for the AVR-Dx
 *
 * Created: 11/10/2023 6:59:29 PM
 * Author: john anderson
 *
 * Copyright (C) 2021 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#ifndef GPIO_H_
#define GPIO_H_
#include "avrOS.h"

// Data Types -----------------------------------------------------------------
typedef void (*gpioHandler_t)(PORT_t *port);

typedef enum
{
    GPIO_PIN_0 = 0b00000001,
    GPIO_PIN_1 = 0b00000010,
    GPIO_PIN_2 = 0b00000100,
    GPIO_PIN_3 = 0b00001000,
    GPIO_PIN_4 = 0b00010000,
    GPIO_PIN_5 = 0b00100000,
    GPIO_PIN_6 = 0b01000000,
    GPIO_PIN_7 = 0b10000000        
} gpioPin_t;

typedef enum
{
    GPIO_OUTPUT,
    GPIO_INPUT
} gpioDirection_t;

typedef struct
{
    uint32_t	toggle;
}gpioStats_t;

typedef struct  
{
    char	*name;
    PORT_t  *port;
    uint8_t pin;
    gpioDirection_t direction;
    gpioHandler_t   handler;

#ifdef GPIO_STATS
    gpioStats_t		*stats;
#endif
}gpio_t;

// Gpio Macros -----------------------------------------------------------------
// Adds a new state machine to the list of state machines handled by the FSM manager
#ifdef GPIO_STATS
#define ADD_GPIO(gpioName, gpioPort, gpioPin, gpioDirection, ...) \
        const static gpio_t SECTION(GPIO_TABLE) name = {.name = #gpioName, .port = &gpioPort, .pin = gpioPin, .direction = gpioDirection, .handler = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};
#else
#define ADD_GPIO(gpioName, gpioPort, gpioPin, gpioDirection, ...) \
        const static gpio_t SECTION(GPIO_TABLE) name = {.name = #gpioName, .port = &gpioPort, .pin = gpioPin, .direction = gpioDirection, .handler = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};
#endif

// External Functions ---------------------------------------------------------

#endif /* UART_H_ */
