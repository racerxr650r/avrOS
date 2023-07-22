/*
 * event.c
 *
 * Functions to implement system events.
 *
 * Created: 7/13/2023
 * Author : john anderson
 *
 * Copyright (C) 2023 by John Anderson <racerxr650r@gmail.com>
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

// Externs ---------------------------------------------------------------------
extern void *__start_EVNT_TABLE,*__stop_EVNT_TABLE;

// Command line interface ------------------------------------------------------
#ifdef EVNT_CLI
ADD_COMMAND("evnt",evntCmd,true);
#endif

static int evntCmd(int argc, char *argv[])
{
	// Walk the table of events
	evntDescriptor_t *descr = (evntDescriptor_t *)&__start_EVNT_TABLE;
	for(; descr < (evntDescriptor_t *)&__stop_EVNT_TABLE; ++descr)
	{
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
			printf("Event: %s - status %s\n\r",descr->name,descr->status->handler!=NULL?"armed":"unarmed");
	}
	return(0);
}


// External functions ----------------------------------------------------------
volatile event_t* evntGetEvent(char *name)
{
	// Walk the table of events
	evntDescriptor_t *descr = (evntDescriptor_t *)&__start_EVNT_TABLE;
	for(; descr < (evntDescriptor_t *)&__stop_EVNT_TABLE; ++descr)
	{
		if(!strcmp(name,descr->name))
			return(descr->status);
	}
	
	return(NULL);
}

int evntReset(volatile event_t *event)
{
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->stateMachine = NULL;
		event->handler = NULL;
	} // End critical section of code
	return(EVENT_DISARMED);
}

int evntEnable(volatile event_t *event, int8_t trigger, evntHandler_t handler, fsmStateMachine_t *stateMachine)
{
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Set up the event
		event->stateMachine = stateMachine;
		event->trigger = trigger;
		event->handler = handler;
	} // End critical section of code
	
    return(EVENT_ARMED);
}

int evntDisable(volatile event_t *event)
{
	event->handler = NULL;
	return(0);
}

int evntTrigger(volatile event_t *event, int8_t signal)
{
	int    ret;
	
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If the event is armed and the trigger matches...
		if(event->handler && (!event->trigger || event->trigger == signal))
		{
			// Set the signal and call the handler
			event->signal = signal;
			event->handler(event->stateMachine);
			
			// Disarm the event
			event->handler = NULL;
			event->stateMachine = NULL;
			ret = EVENT_TRIGGERED;
		}
		else
			ret = EVENT_NONE;
	} // End critical section of code
		
	return(ret);	
}
