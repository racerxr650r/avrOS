/*
 * cpu.h
 *
 * Functions to set up the CPU main clock, sleep/run states, and memory regions.
 *
 * Created: 2/28/2021 3:44:00 PM
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
 
#ifndef CPU_H_
#define CPU_H_

// Macros ---------------------------------------------------------------------
/**
 * @brief Disable global CPU interrupts.
 */
#define DISABLE_INTERRUPTS()	cli()

/**
 * @brief Enable global CPU interrupts.
 */
#define ENABLE_INTERRUPTS()		sei()

// Function Prototypes --------------------------------------------------------
/**
 * @brief Enable or disable the CPU/peripheral clock output to an external pin.
 *
 * @param enable Set to true to enable clock output, false to disable it.
 */
void cpuClockOut(bool enable);

/**
 * @brief Set and configure the internal high-frequency oscillator.
 *
 * Configures the oscillator frequency and optional clock prescaler settings.
 *
 * @param frequency Target oscillator frequency selector.
 * @param prescalerEnable Set to true to enable the CPU clock prescaler.
 * @param prescaler Prescaler division value to apply when enabled.
 */
void cpuSetOSCHF(CLKCTRL_FRQSEL_t frequency, bool prescalerEnable, CLKCTRL_PDIV_t prescaler);

/**
 * @brief Get the current CPU frequency derived from clock controller settings.
 *
 * Returns 0 if an external clock source is active and cannot be derived from
 * internal clock controller configuration.
 *
 * @return Current CPU frequency in Hz, or 0 when not derivable.
 */
uint16_t cpuGetFrequency();

/**
 * @brief Reset the CPU.
 */
void cpuReset();

#endif /* CPU_H_ */