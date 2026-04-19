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
evntList_t	evntListArmed = { .head = NULL, .tail = NULL, .size = 0 };
evntList_t	evntListDisarmed = { .head = NULL, .tail = NULL, .size = 0 };
evntList_t	evntListTriggered = { .head = NULL, .tail = NULL, .size = 0 };

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
			printf("\tArmed: %8lu Triggered: %8lu  Disarmed: %8lu Error: %8lu\n\r",descr->status->stats.armed,descr->status->stats.triggered,descr->status->stats.disarmed,descr->status->stats.error);
#endif
			ret = 0;
		}
	}
	return(ret);
}

// Internal functions ----------------------------------------------------------
static event_t* evntListRemoveHead(evntList_t *list)
{
	if(list == NULL || list->head == NULL)
		return NULL;

	event_t *event = list->head;
	list->head = event->next;
	if(list->tail == event)
		list->tail = NULL;
	list->size--;
	return event;
}

static int evntListAdd(evntList_t *list, event_t *event)
{
	if(list == NULL || event == NULL)
		return -1;

	event->next = NULL;

	if(list->tail == NULL)
	{
		list->head = event;
		list->tail = event;
	}
	else
	{
		list->tail->next = event;
		list->tail = event;
	}

	list->size++;
	return 0;
}

static int evntListRemove(evntList_t *list, event_t *event)
{
	// Check for null pointers
	if(list == NULL || event == NULL)
		return -1;

	// If the list is empty, nothing to remove
	if(list->head == NULL)
		return -1;

	// If the event to remove is the head of the list
	if(list->head == event)
	{
		list->head = event->next;
		if(list->tail == event)
			list->tail = NULL;
		list->size--;
		return 0;
	}

	// Search for the event in the list
	event_t *prev = list->head;
	while(prev->next != NULL && prev->next != event)
	{
		prev = prev->next;
	}

	// If the event was not found
	if(prev->next == NULL)
		return -1;

	// Remove the event from the list
	prev->next = event->next;
	if(list->tail == event)
		list->tail = prev;
	list->size--;
	return 0;
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
	return(event->state);
}

evntState_t evntArm(volatile event_t *event, evntHandler_t handler, volatile fsmStateMachine_t *stateMachine)
{
	evntState_t ret = EVENT_ARMED;
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->state = EVENT_ARMED;
		event->handler = handler;
		event->stateMachine = stateMachine;

		// Remove the event from the disarmed list
		if(!evntListRemove(&evntListDisarmed, event))
		{
			// Add the event to the armed list
			evntListAdd(&evntListArmed, event);
			// Put the state machine in the wait queue
			fsmWait(fsmGetCurrentStateMachine());
		}
		// Else the event was not in the disarmed list, return an error
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = event->state = EVENT_ERROR;
		}
	} // End of critical section

#ifdef EVNT_STATS
	++event->stats.armed;
#endif

	return(ret);
}

evntState_t evntDisarm(volatile event_t *event)
{
	evntState_t ret = EVENT_DISARMED;
	
	event->handler = NULL;
	event->stateMachine = NULL;
	event->state = EVENT_DISARMED;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Remove the event from the armed list
		if(!evntListRemove(&evntListArmed, event))
		{
			// Add the event to the disarmed list
			evntListAdd(&evntListDisarmed, event);
		}
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = EVENT_ERROR;
		}
	} // End of critical section

#ifdef EVNT_STATS
	++event->stats.disarmed;
#endif

	return(ret);
}

evntState_t evntTrigger(volatile event_t *event, int subType)
{
	evntState_t   ret=EVENT_TRIGGERED;

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Remove the event from the armed list
		if(!evntListRemove(&evntListArmed, event))
		{
			event->state = EVENT_TRIGGERED;
			// Add the event to the triggered list
			evntListAdd(&evntListTriggered, event);
#ifdef EVNT_STATS
			++event->stats.triggered;
#endif
			ret = 0;
		}
		// Else the event was not in the armed list
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = subType = EVENT_ERROR;
		}
	} // End critical section of code

	return(ret);
}

evntState_t evntWait(volatile event_t *event, evntHandler_t handler, volatile fsmStateMachine_t *stateMachine)
{
	evntState_t ret = EVENT_IDLE;

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(evntArm(event, handler, stateMachine) == EVENT_ARMED)
		{
			// Put the state machine in the wait queue
			fsmWait(fsmGetCurrentStateMachine());
			ret = event->state;
		}
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = EVENT_ERROR;
		}
	} // End of critical section

	return(ret);
}

int evntInit(void)
{
	evntDescriptor_t    *descr = (evntDescriptor_t *)&__start_EVNT_TABLE;
	int count = 0;

	// Walk the table of events
	for(; descr < (evntDescriptor_t *)&__stop_EVNT_TABLE; ++descr)
	{
		descr->status->state = EVENT_DISARMED;
		evntListAdd(&evntListDisarmed, descr->status);
		count++;
	}

	return(count);
}

int evntDispatch(void)
{
	int     ret = 0;
	event_t *event;

	// While there are events in the triggered list...
	while(evntListTriggered.size > 0)
	{
		event = evntListRemoveHead(&evntListTriggered);

		// Call the event handler for the event
		if(event->handler)
			event->handler(event->stateMachine);

		// Start critical section of code
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// Remove the event from the triggered list
			evntListRemove(&evntListTriggered, event);
			// Add the event to the disarmed list
			evntListAdd(&evntListDisarmed, event);

			// Scan the armed list for events for the same state machine
			event_t *armedEvent = evntListArmed.head;
			while(armedEvent != NULL)
			{
				if(armedEvent->stateMachine == event->stateMachine)
				{
					// Remove the event from the armed list
					evntListRemove(&evntListArmed, armedEvent);
					// Add the event to the disarmed list
					evntListAdd(&evntListDisarmed, armedEvent);
				}
				armedEvent = armedEvent->next;
			}
			// Increment the event count
			++ret;
		} // End critical section of code
	}
	return(ret);
}
