/*
 * cpu.c
 *
 * Functions to set up the CPU main clock, sleep/run states, and memory regions. 
 *
 * Created: 2/19/2021 2:33:18 PM
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
#include <avr/cpufunc.h>
#include "avrOS.h"

// Constants ------------------------------------------------------------------
// Prescaler divisor table
static const uint32_t divisor[] = {2,4,5,16,32,64,1,1,6,10,12,24,48};
// Internal HF Oscillator frequency table
static const uint32_t oschfFrequency[] = {1000000,2000000,3000000,4000000,0,8000000,12000000,16000000,20000000,24000000};

// CLI Commands ---------------------------------------------------------------
#ifdef CPU_CLI
ADD_COMMAND("reset",cpuResetCmd);

int cpuResetCmd(int argC, char *argV[])
{
	// The UNUSED macro prevents the compiler from warning about unused variables
	// Include these only if the function does not use the argC and argV parameters
	UNUSED(argC);
	UNUSED(argV);

	cpuReset();
	return(0);
}
#endif // CPU_CLI

// CPU Clock control functions ------------------------------------------------
// Enable or Disable the CPU/Peripheral clock output to external pin
void cpuClockOut(bool enable)
{
	// Clock Output is enabled...
	if(enable)
		ccp_write_io((void *)&(CLKCTRL.MCLKCTRLA),CLKCTRL.MCLKCTRLA|CLKCTRL_CLKOUT_bm);
	// Clock Output is disabled...
	else
		ccp_write_io((void *)&(CLKCTRL.MCLKCTRLA),CLKCTRL.MCLKCTRLA&(~CLKCTRL_CLKOUT_bm));
}

// Set the internal high frequency oscillator as the source clock and configure
void cpuSetOSCHF(CLKCTRL_FRQSEL_t frequency, bool prescalerEnable, CLKCTRL_PDIV_t prescaler)
{
	uint8_t	value;
	
	// Set the internal HF oscillator frequency
	value = (uint8_t)frequency;
	ccp_write_io((void *)&CLKCTRL.OSCHFCTRLA, value);
	// Set the prescaler divisor
	ccp_write_io((void *)&CLKCTRL.MCLKCTRLB, prescaler|prescalerEnable);
	// Set the internal HF oscillator as the clock source
	ccp_write_io((void *)&CLKCTRL.MCLKCTRLA, CLKCTRL.MCLKCTRLA&(~CLKCTRL_CLKSEL_gm));
}

// Calculate the CPU frequency using the Clock Controller settings. Note: If an external clock is being used, this function will return 0
uint32_t cpuGetFrequency()
{
	uint8_t				clkSelect = CLKCTRL.MCLKCTRLA&CLKCTRL_CLKSEL_gm; 	// Get the clock select from the Clock Control Register A
	uint8_t				pdiv = (CLKCTRL.MCLKCTRLB&CLKCTRL_PDIV_gm)>>CLKCTRL_PDIV_gp;
	uint32_t			freq = 0;
	
	// If internal HF clock is enabled...
	if(clkSelect == CLKCTRL_CLKSEL_OSCHF_gc)
	{
		// Get the frequency select value from the HF oscillator control register
		uint8_t freqSelect = (CLKCTRL.OSCHFCTRLA&CLKCTRL_FRQSEL_gm)>>CLKCTRL_FRQSEL_gp;
		// If the frequency select value is valid...
		if(freqSelect<=sizeof(oschfFrequency))
			freq = oschfFrequency[freqSelect];
	}
	// Else if the internal or external 32KHz clock is enabled...
	else if(clkSelect == CLKCTRL_CLKSEL_OSC32K_gc || clkSelect == CLKCTRL_CLKSEL_XOSC32K_gc)
	{
		freq = 32000;
	}
	
	// Factor the prescaler divisor
	if((CLKCTRL.MCLKCTRLB&CLKCTRL_PEN_bp) && (pdiv<=sizeof(divisor)))
		freq = freq/divisor[pdiv];
	
	return(freq);
}

void cpuReset()
{
	// Configuration Change Protection write to the Software Reset Register
	ccp_write_io((void *)&RSTCTRL.SWRR,RSTCTRL_SWRST_bm);
}
