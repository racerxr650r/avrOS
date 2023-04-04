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
	sysInit();
	
	INFO("Starting state machine dispatch loop");
	// Loop forever --------------------------------------------------------------
    while (1) 
    {
	    // Continuously call the main state machine dispatcher
		fsmDispatch();
    }
}

