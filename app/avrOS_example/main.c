/*
 * main.c
 *
 * Application OS resources, state machine(s), CLI commands, and main entry 
 * point. The main entry point calls sysInit(), enables interrupts, and then 
 * loops forever calling the fsmDispatch() function that implements the state
 * machine scheduler.
 *
 * Created: 2/8/2021 12:39:38 PM
 * Author : john anderson
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

#include "avrOS.h"

int main(void)
{
	// Initialize the system -----------------------------------------------------
	// Setup the internal CPU clock source
	cpuSetOSCHF(CPU_SPEED,false,0);

#ifdef MEM_STATS	
	// Fill the stack area with pattern to detect max stack size
	memStackFill();
#endif	

#if LOG_FORMAT>0 && LOG_LEVEL>0
	logInit();
#endif

	// Initialize the system tick counter
	sysInitTick();
	
	// Initialize the CLI and the interface associated with it
#ifdef CLI_SERVICE
	// Initialize the CLI and hook stdout so printf and other stdout function
	// output to the CLI interface
	cliInit();
#endif
	
	// Enable global interrupts
	sei();
	
	INFO("Starting state machine dispatch loop");
	// Loop forever --------------------------------------------------------------
    while (1) 
    {
	    // Continuously call the main state machine dispatcher
		fsmDispatch();
    }
}

