/*
 * tmr.c
 *
 * System timers
 *
 * Created: 4/11/2021 11:52:48 AM
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

// Externs --------------------------------------------------------------------
extern void *__start_TMR_TABLE,*__stop_TMR_TABLE;

// Command line interface -----------------------------------------------------
#ifdef TMR_CLI
ADD_COMMAND("tmr",tmrCmd,true);

static int tmrCmd(int argc, char *argv[])
{
	// Walk the table of state machines
	timerDescr_t *descr = (timerDescr_t *)&__start_TMR_TABLE;
	for(; descr < (timerDescr_t *)&__stop_TMR_TABLE; ++descr)
	{
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
		{
			bool		running = false;
			uint32_t	scanCycle = fsmGetScanCycle();
			
			if(descr->timerState->lastScanCycle == scanCycle || descr->timerState->lastScanCycle == scanCycle-1)
				running = true;
				
			printf("Timer: %s\n\r",descr->name);
			printf("\tRunning: %s\tOutput: %s\n\r",running==true?"Yes":" No",descr->timerState->output==true?" TRUE":"FALSE");
			printf("\tPT:%8lu\tET:%8lu\tRT:%8lu\n\r",descr->timerState->PT,descr->timerState->ET,descr->timerState->RT);
		}
	}
	return(0);
}
#endif //TMR_CLI

// Internal Functions ---------------------------------------------------------
static void tmrSet(timer_t *timer, uint32_t ticks, bool direction)
{
  timer->PT = timer->RT = ticks;
  timer->output = direction;
  timer->ET = 0;
  timer->sysTick = sysGetTick();
}  

// External Functions ---------------------------------------------------------
void tmrReset(timer_t *timer, bool direction)
{
	timer->RT = timer->PT;
	timer->ET = 0;
	timer->output = direction;
	timer->sysTick = sysGetTick();
}

bool tmrFunc(timer_t *timer, uint32_t ticks, bool direction)
{
	uint32_t scanCycle = fsmGetScanCycle();
	
	// If the timer wasn't called last scan cycle...
	if((scanCycle-timer->lastScanCycle>1) && !(scanCycle == 0 && timer->lastScanCycle == 0xffffffff))
	{
		// Start the timer
		tmrSet(timer,ticks,direction);
	}
	else
	{
		uint32_t current_tick = sysGetTick();
    
		// Increment the timer and update the output (if needed)
		timer->ET=current_tick - timer->sysTick;
		if(timer->ET < timer->PT)      
			timer->RT = timer->PT - timer->ET;
		else
			timer->RT = 0;
    
		if(timer->RT == 0)
			timer->output = direction ^ true;
	}
	timer->lastScanCycle = scanCycle;
  
	return(timer->output);
}
