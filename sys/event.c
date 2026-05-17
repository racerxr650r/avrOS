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
			printf(UNDERLINE BOLD FG_BLUE "%s\n\r" RESET,descr->status->state==EVENT_ARMED?"armed":"unarmed");
#ifdef EVNT_STATS
			printf("\tArmed: %8lu Triggered: %8lu  Disarmed: %8lu Error: %8lu\n\r",descr->status->stats.armed,descr->status->stats.triggered,descr->status->stats.disarmed,descr->status->stats.error);
#endif
			ret = 0;
		}
	}
	return(ret);
}

// Internal functions ----------------------------------------------------------
static volatile event_t* evntListRemoveHead(evntList_t *list)
{
	if(list == NULL || list->head == NULL)
		return NULL;

	volatile event_t *event = list->head;
	list->head = event->next;
	if(list->tail == event)
		list->tail = NULL;
	list->size--;
	return event;
}

static int evntListAdd(evntList_t *list, volatile event_t *event)
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

static int evntListRemove(evntList_t *list, volatile event_t *event)
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
	volatile event_t *prev = list->head;
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
// Get an event by name
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

// Arm an event for a state machine
evntState_t evntArm(volatile fsmStateMachine_t *stateMachine, volatile event_t *event)
{
	evntState_t ret = EVENT_ARMED;
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->state = EVENT_ARMED;
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

// Arm a system-wide event (no waiting state machine).
// Used for self-arming events such as the system tick, whose handler runs
// directly out of evntDispatch and re-arms the event itself.
evntState_t evntArmSystem(volatile event_t *event)
{
	evntState_t ret = EVENT_ARMED;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->stateMachine = NULL;

		// Remove the event from the disarmed list and add to armed list
		if(!evntListRemove(&evntListDisarmed, event))
		{
			event->state = EVENT_ARMED;
			evntListAdd(&evntListArmed, event);
		}
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = event->state = EVENT_ERROR;
		}
	}

#ifdef EVNT_STATS
	if(ret == EVENT_ARMED)
		++event->stats.armed;
#endif

	return(ret);
}

// Disarm an event
evntState_t evntDisarm(volatile event_t *event)
{
	evntState_t ret = EVENT_DISARMED;
	
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

// Trigger an event
evntState_t evntTrigger(volatile event_t *event, int subType)
{
	evntState_t   ret;

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Remove the event from the armed list
		if(!evntListRemove(&evntListArmed, event))
		{
			ret = event->state = EVENT_TRIGGERED;
			event->trigger = subType;
			// Add the event to the triggered list
			evntListAdd(&evntListTriggered, event);
#ifdef EVNT_STATS
			++event->stats.triggered;
#endif
		}
		// Else the event was not in the armed list
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = event->state = EVENT_ERROR;
		}
	} // End critical section of code

	return(ret);
}

// Wait for an event to be triggered
evntState_t evntWait(volatile fsmStateMachine_t *stateMachine, volatile event_t *event, int eventType)
{
	evntState_t ret = EVENT_IDLE;

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->evntType = eventType;
		if(evntArm(stateMachine, event) == EVENT_ARMED)
		{
			// Put the state machine in the wait queue
			fsmWait(stateMachine);
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

// Default event handler for events without a user defined handler
int evntHandler(volatile event_t *event)
{
	int	ret = -1;

	// If the event trigger type matches the current event type...
	if(event->evntType == event->trigger)
	{
		// Move the state machine associated with the event to the ready queue
		fsmReady(event->stateMachine);
		ret = 0;
	}

	return(ret);
}

// Initialize the system events
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

// Scan the trigger event list and call the appropriate event handlers
int evntDispatch(void)
{
	int     ret = 0;
	volatile event_t *event;

	// While there are events in the triggered list...
	while(evntListTriggered.size > 0)
	{
		event = evntListRemoveHead(&evntListTriggered);

		// Call the event handler for the event
		if(event->descr->handler)
			event->descr->handler(event);

		// Start critical section of code
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// System events (no associated state machine) manage their own
			// list membership inside the handler (e.g. self-arming tick).
			// Only auto-disarm FSM-bound events here.
			if(event->stateMachine != NULL)
			{
				// Add the event to the disarmed list
				evntListAdd(&evntListDisarmed, event);

				// Scan the armed list for events for the same state machine
				volatile event_t *armedEvent = evntListArmed.head;
				while(armedEvent != NULL)
				{
					volatile event_t *nextArmed = armedEvent->next;
					if(armedEvent->stateMachine == event->stateMachine)
					{
						// Remove the event from the armed list
						evntListRemove(&evntListArmed, armedEvent);
						// Add the event to the disarmed list
						evntListAdd(&evntListDisarmed, armedEvent);
					}
					armedEvent = nextArmed;
				}
			}
			// Increment the event count
			++ret;
		} // End critical section of code
	}
	return(ret);
}
