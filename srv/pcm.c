/*
 * pcm.c
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

// Internal Variables ---------------------------------------------------------
static uint8_t *pcmData, pcmRLEValue;
static uint16_t pcmLength = 0, pcmRLECount;
static uint32_t	pcmDelay = 0;

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
	// Initialize the DAC
	dacInit(VREF_REFSEL_VDD_gc,DAC_MID);
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
		dacOutput(pcmRLEValue);
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
			dacOutput(*(pcmData++));
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
			dacOutput(0);
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
