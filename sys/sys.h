/**
 * @file sys.h
 * @brief System initialization and timer/tick functions.
 *
 * This header file provides functions and macros for system initialization, 
 * including setting up the system tick timer and managing sleep/idle states.
 *
 * @date 4/17/2021
 * @author John Anderson <racerxr650r@gmail.com>
 *
 * Copyright (C) 2021 by John Anderson
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

#ifndef SYS_H_
#define SYS_H_

/** @addtogroup sys_kernel
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

// Constants ------------------------------------------------------------------

/** 
 * @brief TCB timer selection for the system tick.
 * @details These constants define which Timer/Counter (TCB) peripheral
 * is used for generating the system tick interrupt.  Select one based
 * on your hardware configuration and availability.  Only one of these
 * should be uncommented (defined).
 */
#define SYS_TICK_TIMER  SYS_TIMER_TCB0  ///< Use TCB0 for the system tick.
//#define SYS_TICK_TIMER  SYS_TIMER_TCB1  ///< Use TCB1 for the system tick.
//#define SYS_TICK_TIMER  SYS_TIMER_TCB2  ///< Use TCB2 for the system tick.


/** @brief Base addresses of the TCB peripherals. */
#define SYS_TIMER_TCB0  0x0B00  ///< Base address of TCB0.
#define SYS_TIMER_TCB1  0x0B10  ///< Base address of TCB1.
#define SYS_TIMER_TCB2  0x0B20  ///< Base address of TCB2.


/** @brief Event type for system tick. */
#define EVENT_TYPE_TICK  1 ///< Event triggered on each system tick.


// External Functions ---------------------------------------------------------

/**
 * @brief Initialize the system.
 * 
 * This function performs essential system initialization, including 
 * setting up the system tick timer. It must be called once at 
 * the beginning of the program.
 *
 * @return True if initialization was successful, false otherwise.
 */
bool sysInit();

/**
 * @brief Set the system tick frequency.
 *
 * Sets the frequency of the system tick interrupt.
 *
 * @param sysTickFreq The desired system tick frequency in Hz.
 */
void sysSetTickFreq(uint16_t sysTickFreq);

/**
 * @brief Get the system tick frequency.
 *
 * Returns the current system tick frequency in kHz.
 *
 * @return The system tick frequency in kHz.
 */
uint16_t sysGetTickFreq();


/**
 * @brief Get the system tick count.
 *
 * Returns the current value of the system tick counter. This
 * value increments on each system tick interrupt. It can be used
 * for timing and scheduling purposes.
 *
 * @return The current system tick count.
 */
uint32_t sysGetTickCount();

/**
 * @brief Put the system into a low-power sleep mode.
 *
 * This function puts the microcontroller into a low-power sleep mode
 * until the next system tick interrupt occurs. This can be used to
 * conserve power when the system is idle.
 */
void sysSleep();


/** @} */ // end of sys_kernel

#endif /* SYS_H_ */
