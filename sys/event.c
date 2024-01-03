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

// Locals ----------------------------------------------------------------------
ADD_QUEUE(evntQue,sizeof(event_t *),4);

// Command line interface ------------------------------------------------------
#ifdef EVNT_CLI
ADD_COMMAND("evnt",evntCmd,true);
#endif

static int evntCmd(int argc, char *argv[])
{
	evntDescriptor_t    *descr = (evntDescriptor_t *)&__start_EVNT_TABLE;
	int                 ret = -1;

	// Walk the table of events
	for(; descr < (evntDescriptor_t *)&__stop_EVNT_TABLE; ++descr)
	{
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
		{
#ifdef EVNT_STATS
			printf(UNDERLINE BOLD FG_BLUE "%-24s",descr->name);
#endif
			printf(UNDERLINE BOLD FG_BLUE "%s\n\r" RESET,descr->status->handler==NULL?"unarmed":"armed");
#ifdef EVNT_STATS
			printf("\tArmed: %8lu Triggered: %8lu  Handled: %8lu Unhandled: %8lu Error: %8lu\n\r",descr->status->stats.armed,descr->status->stats.handled+descr->status->stats.unhandled,descr->status->stats.handled,descr->status->stats.unhandled,descr->status->stats.error);
#endif
			ret = 0;
		}
	}
	return(ret);
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

evntState_t evntGetStatus(volatile event_t *event)
{
	evntState_t    ret = EVENT_IDLE;

	if(event->handler)
		ret = EVENT_ARMED;
	else
		ret = EVENT_DISARMED;

	return(ret);
}

evntState_t evntReset(volatile event_t *event)
{
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->stateMachine = NULL;
		event->handler = NULL;
	} // End critical section of code
	return(EVENT_DISARMED);
}

evntState_t evntEnable(volatile event_t *event, evntType_t filter, evntHandler_t handler, volatile fsmStateMachine_t *stateMachine)
{
	evntState_t ret;

	// 
	if(event != NULL && handler != NULL)
	{
		// Start critical section of code
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// Set up the event
			event->stateMachine = stateMachine;
			event->filter = filter;
			event->handler = handler;

		} // End critical section of code
#ifdef EVNT_STATS
		++event->stats.armed;
#endif
		ret = EVENT_ARMED;
	}
	else 
	{
#ifdef EVNT_STATS
		++event->stats.error;
#endif
		ret = EVENT_ERROR;
	}

	return(ret);
}

evntState_t evntWait(volatile event_t *event, evntType_t filter)
{
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Enable the event for the current state machine with the given filter
		evntEnable(event, filter, fsmReady, fsmGetCurrentStateMachine());
		// Put the state machine in the wait queue
		fsmWait(fsmGetCurrentStateMachine());
	} // End of critical section

	return(EVENT_ARMED);
}

evntState_t evntDisable(volatile event_t *event)
{
	event->handler = NULL;
	return(EVENT_DISARMED);
}

evntState_t evntTrigger(volatile event_t *event, evntType_t type)
{
	evntState_t    ret;

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If the event is armed and the type is legit...
		if(event->handler!=NULL && type != EVENT_TYPE_NONE)
		{
			// If the type is in the filter, or the filter is "all events"...
			if(type&event->filter || event->filter==EVENT_TYPE_ALL)
			{
#ifdef EVNT_STATS
				++(event->stats.handled);
#endif
				// Set the type and put the event in the queue
				event->type = type;
				quePutPtr(&evntQue,(void *)event);

				ret = EVENT_TRIGGERED;
			}
		}
		// Else if the trigger type is "no event"...
		else if(type==EVENT_TYPE_NONE)
		{
			ret = EVENT_ERROR;
#ifdef EVNT_STATS
			++event->stats.error;
#endif
		}
		// Else the type is not in the filter...
		else
		{
			ret = EVENT_IDLE;
#ifdef EVNT_STATS
			++event->stats.unhandled;
#endif
		}
	} // End critical section of code

	return(ret);
}

int evntDispatch(void)
{
	int     ret = 0;
	event_t *event;

	// While there are events in the queue...
	while(queGetPtr(&evntQue,(void **)&event) == true)
	{
		// Call the event handler for the event
		if(!event->handler(event->stateMachine))
		{
			// If this event has a state machine assigned...
			if(event->stateMachine)
			{
				// Disarm the event
				event->handler = NULL;
				event->stateMachine = NULL;
			}
		}

		// Increment the event count
		++ret;
	}
	return(ret);
}
