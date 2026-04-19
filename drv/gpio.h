/**
 * @file gpio.h
 * @brief GPIO driver — named GPIO instances with interrupt-driven callbacks.
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

/** @addtogroup gpio_driver
 * @{
 */

#include "avrOS.h"

// Data Types -----------------------------------------------------------------
struct GPIO_TYPE;

typedef void (*gpioHandler_t)(struct GPIO_TYPE *gpio);

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

/*typedef struct
{
	
}gpio_t;*/

typedef struct GPIO_TYPE
{
#ifdef GPIO_STATS
	char	*name;
#endif
	PORT_t				*port;
	uint8_t				pin;
	gpioDirection_t		direction;
	gpioHandler_t		handler;
	volatile event_t	*event;
#ifdef GPIO_STATS
	gpioStats_t		*stats;
#endif
}gpio_t;

// Gpio Macros -----------------------------------------------------------------
/**
 * @brief Add a GPIO instance and register its initializer.
 *
 * Creates a static GPIO descriptor in the GPIO table and registers
 * `gpioInit` to initialize the instance at startup.
 *
 * @param gpioName Name of the GPIO instance symbol.
 * @param gpioPort Hardware port used by the GPIO.
 * @param gpioPin Pin mask assigned to the GPIO.
 * @param gpioDirection Initial direction (`GPIO_OUTPUT` or `GPIO_INPUT`).
 * @param ... Optional GPIO handler callback.
 */
#ifdef GPIO_STATS
#define ADD_GPIO(gpioName, gpioPort, gpioPin, gpioDirection, ...) \
		const static gpio_t SECTION(GPIO_TABLE) gpioName = {.name = #gpioName, .port = &gpioPort, .pin = gpioPin, .direction = gpioDirection, .handler = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)}; \
		ADD_INITIALIZER(gpioName ## _GPIO,gpioInit,(void *)&gpioName);

/**
 * @brief Add a GPIO instance with an associated event source.
 *
 * Creates an event object, stores it in the GPIO descriptor, and registers
 * `gpioInit` to initialize the event-driven GPIO instance at startup.
 *
 * @param gpioName Name of the GPIO instance symbol.
 * @param gpioPort Hardware port used by the GPIO.
 * @param gpioPin Pin mask assigned to the GPIO.
 * @param gpioDirection Initial direction (`GPIO_OUTPUT` or `GPIO_INPUT`).
 * @param gpioEventType Event trigger type associated with this GPIO.
 * @param ... Optional GPIO handler callback.
 */
#define ADD_GPIO_EVENT(gpioName, gpioPort, gpioPin, gpioDirection, gpioEventType, ...) \
		ADD_EVENT(gpioName ## _event); \
		const static gpio_t SECTION(GPIO_TABLE) gpioName = {.name = #gpioName, .port = &gpioPort, .pin = gpioPin, .direction = gpioDirection, .event = &CONCAT(gpioName,_event), eventType = gpioEventType, .handler = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)}; \
		ADD_INITIALIZER(gpioName ## _GPIO,gpioInit,(void *)&gpioName);
#else
#define ADD_GPIO(gpioName, gpioPort, gpioPin, gpioDirection, ...) \
		const static gpio_t SECTION(GPIO_TABLE) gpioName = {.port = &gpioPort, .pin = gpioPin, .direction = gpioDirection, .handler = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)}; \
		ADD_INITIALIZER(gpioName ## _GPIO,gpioInit,(void *)&gpioName);
#endif

// External Functions -----------------------------------------------------------
/**
 * @brief Set output pin(s) selected by a bit mask.
 *
 * Sets output pins corresponding to `value` using the OUTSET register. Pins not
 * included in this GPIO definition are ignored.
 *
 * @param gpio Pointer to the GPIO descriptor.
 * @param value Bit mask of pin(s) to set.
 */
void gpioSetOutput(const gpio_t *gpio, uint8_t value);

/**
 * @brief Clear output pin(s) selected by a bit mask.
 *
 * Clears output pins corresponding to `value` using the OUTCLR register. Pins
 * not included in this GPIO definition are ignored.
 *
 * @param gpio Pointer to the GPIO descriptor.
 * @param value Bit mask of pin(s) to clear.
 */
void gpioClearOutput(const gpio_t *gpio, uint8_t value);

/**
 * @brief Toggle output pin(s) selected by a bit mask.
 *
 * Toggles output pins corresponding to `value` using the OUTTGL register. Pins
 * not included in this GPIO definition are ignored.
 *
 * @param gpio Pointer to the GPIO descriptor.
 * @param value Bit mask of pin(s) to toggle.
 */
void gpioToggleOutput(const gpio_t *gpio, uint8_t value);

/**
 * @brief Write output state for GPIO-managed pin(s).
 *
 * Writes to the OUT register after masking `value` with the GPIO pin mask so
 * that unrelated pins on the same port are not affected.
 *
 * @param gpio Pointer to the GPIO descriptor.
 * @param value Desired output value bit mask.
 */
void gpioWriteOutput(const gpio_t *gpio, uint8_t value);

/**
 * @brief Read current input state for GPIO-managed pin(s).
 *
 * Returns the IN register value masked by the GPIO pin mask, so pins not part
 * of this GPIO always read as zero.
 *
 * @param gpio Pointer to the GPIO descriptor.
 * @return Masked input pin state.
 */
uint8_t gpioReadInput(const gpio_t *gpio);

/**
 * @brief Read current output latch state for GPIO-managed pin(s).
 *
 * Returns the OUT register value masked by the GPIO pin mask, so pins not part
 * of this GPIO always read as zero.
 *
 * @param gpio Pointer to the GPIO descriptor.
 * @return Masked output pin state.
 */
uint8_t gpioReadOutput(const gpio_t *gpio);

/** @} */ // end of gpio_driver

#endif /* GPIO_H_ */
