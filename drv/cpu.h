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
#define DISABLE_INTERRUPTS()	cli()
#define ENABLE_INTERRUPTS()		sei()

// Function Prototypes --------------------------------------------------------
// Enable or Disable the CPU/Peripheral clock output to external pin
void cpuClockOut(bool enable);
// Set the internal high frequency oscillator as the source clock and configure
void cpuSetOSCHF(CLKCTRL_FRQSEL_t frequency, bool prescalerEnable, CLKCTRL_PDIV_t prescaler);
// Calculate the CPU frequency using the Clock Controller settings. Note: If an external clock is being used, this function will return 0
uint16_t cpuGetFrequency();
void cpuReset();

#endif /* CPU_H_ */