/*
 * dac.c
 *
 * Implement Digital to Analog output driver for AVR128DA
 *
 * Created: 7/11/2021 3:48:27 PM
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
#include "../avrOS.h"

void dacInit(VREF_REFSEL_t vRef, register16_t output)
{
	// Setup the voltage reference
	VREF.DAC0REF = vRef;
	// Disable the pin input buffer
	PORTD.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	// Enable the DAC output buffer
	DAC0.CTRLA |= DAC_OUTEN_bm;
	// Set output
	DAC0.DATA = output;
	// Enable the DAC
	DAC0.CTRLA |= DAC_ENABLE_bm;	
}

void dacOutput(int16_t value)
{
	value += DAC_MID;
	if(value > DAC_MAX)
		value = DAC_MAX;
	else if(value < DAC_MIN)
		value = DAC_MIN;
	
	DAC0.DATA = value;
}