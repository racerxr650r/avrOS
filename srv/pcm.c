/**
 * @file pcm.c
 * @brief PCM audio service — streams run-length-encoded samples to the DAC.
 *
 * Implements PCM output (audio) using DAC
 *
 * Created: 6/6/2021 10:10:29 AM
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
// Includes -------------------------------------------------------------------
#include "../avrOS.h"

/** @addtogroup pcm_service
 * @{
 */

// Internal Variables ---------------------------------------------------------
static uint8_t *pcmData, pcmRLEValue;
static uint16_t pcmLength = 0, pcmRLECount;
static uint32_t	pcmDelay = 0;

// Internal Functions ---------------------------------------------------------
/**
 * @brief Convert a signed sample to a DAC code and output it.
 *
 * Offsets the signed sample to the DAC's unsigned mid-scale, clamps it to the
 * valid range, and writes it to the DAC via the driver.
 *
 * @param value Signed audio sample, relative to mid-scale.
 */
static inline void pcmDacOutput(int16_t value)
{
	value += DAC_MID;
	if(value > DAC_MAX)
		value = DAC_MAX;
	else if(value < DAC_MIN)
		value = DAC_MIN;

	// The DATA register is left-justified (10-bit code in bits [15:6]), so
	// shift the code into position before writing.
	dacSetData((uint16_t)value << DAC_DATA_gp);
}

// PCM State Machine ----------------------------------------------------------
#ifdef PCM_SERVICE
// Create instance of the pcm state machine
ADD_STATE_MACHINE(pcmStateMachine, pcmInit, false);

// State handler function prototypes
static int pcmIdle(fsmStateMachine_t *stateMachine);
static int pcmUpdate(fsmStateMachine_t *stateMachine);
static int pcmWait(fsmStateMachine_t *stateMachine);

int pcmInit(fsmStateMachine_t *stateMachine)
{
	// Initialize the DAC ----------------------------------------------------
	// Disable the digital input buffer on the DAC output pin (PD6)
	pioSetPinConfig(&PORTD, 6, PORT_ISC_INPUT_DISABLE_gc);
	// Select the voltage reference (VDD)
	dacSetReference(VREF_REFSEL_VDD_gc);
	// Enable the analog output buffer
	dacOutputBufferEnable(true);
	// Load the mid-scale (idle) output value
	dacSetData(DAC_MID);
	// Enable the DAC
	dacEnable();
	// Goto the idle state
	fsmSetNextState(stateMachine,pcmIdle);
	return(0);
}

static int pcmIdle(fsmStateMachine_t *stateMachine)
{
	// If there is a pcm stream queued...
	if(pcmLength)
		fsmSetNextState(stateMachine, pcmUpdate);
		
	return(0);
}

static int pcmUpdate(fsmStateMachine_t *stateMachine)
{
	// Write the next byte in the data stream to the DAC output
	if(pcmRLECount)
	{
		--pcmRLECount;
		pcmDacOutput(pcmRLEValue);
	}
	else
	{
		if((*pcmData) == 0)
		{
			pcmRLECount = *(++pcmData);
			pcmRLEValue = 0;
			++pcmData;
		}
		else if((*pcmData) == 0xff)
		{
			pcmRLECount = *(++pcmData);
			pcmRLEValue = *(++pcmData);
			++pcmData;
		}
		else
			pcmDacOutput(*(pcmData++));
	}
	// Decrement the data stream counter
	--pcmLength;
	// Goto the wait for next update state
	fsmSetNextState(stateMachine, pcmWait);
	return(0);
}

static int pcmWait(fsmStateMachine_t *stateMachine)
{
	// If the wait timer expired...
	if(tmrOnDelay(&pcmTimer,pcmDelay))
	{
		// If there is more data in the stream...
		if(pcmLength)
			fsmSetNextState(stateMachine, pcmUpdate);
		// Else there is no more data...
		else
		{
			// "zero" the DAC output
			pcmDacOutput(0);
			// Goto idle and wait for the next pcm audio stream
			fsmSetNextState(stateMachine, pcmIdle);
		}
	}
	return(0);
}
#endif // PCM_SERVICE

// External Functions ---------------------------------------------------------
bool pcmBusy()
{
	bool ret = false;
	
	if(pcmLength)
		ret = true;
		
	return(ret);
}

void pcmPlay(const uint8_t *sound, uint16_t length)
{
}

/** @} */ // end of pcm_service
